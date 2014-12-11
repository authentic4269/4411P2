#include "minifile.h"
#include "blockcache.h"
#include "disk.h"
#include "queue.h"
#include "synch.h"
#include <unistd.h>

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

 #define NUM_DIRECT_BLOCKS 11
 #define SEGMENTSIZE = 128 
 #define NUMSEGMENTS = DISKSIZE / (DISK_BLOCK_SIZE * SEGMENTSIZE)

char* current_directory;
superblock_t sBlock;
disk_t disk;
inode_t inodes;
unsigned char *free_block_bitmap;

// outstanding_requests and setup_mutexes are used to synchronize loads of inodes. they enforce the property that
// every inode is loaded before the requests for free blocks are sent to disk
// 
// done_mutex is used to ensure that the vital filesystem data structures are initialized before the filesystem initialization 
// function returns. 
int outstanding_requests;
semaphore_t setup_superblock_mutex;
semaphore_t setup_inode_mutex;
semaphore_t setup_bitmap_mutex;
semaphore_t done_mutex;
blockcache_t blockcache;

semaphore_t *block_mutexes;

void handle_disk_response(void *arg); // forward declaration

void setup_inode(void *diskarg)
{
	disk_interrupt_arg_t arg = *((disk_interrupt_arg_t *) diskarg);
	if (arg.reply != DISK_REPLY_OK)
	{
		printf("Error in disk request, exiting\n");
		exit(0);
	}
	semaphore_P(setup_inode_mutex);
	outstanding_requests -= 1;
	if (outstanding_requests == 0)
	{
		semaphore_V(setup_superblock_mutex);
	}
	else {
		semaphore_V(setup_inode_mutex);
	}
}

void setup_bitmap(void *diskarg)
{
	disk_interrupt_arg_t arg = *((disk_interrupt_arg_t *) diskarg);
	if (arg.reply != DISK_REPLY_OK)
	{
		printf("Error in disk request, exiting\n");
		exit(0);
	}
	semaphore_P(setup_bitmap_mutex);
	outstanding_requests--;
	if (outstanding_requests == 0)
	{
		semaphore_V(done_mutex);
	}
	else
	{
		semaphore_V(setup_bitmap_mutex);
	}
}

void setup_superblock(void *diskarg)
{
	disk_interrupt_arg_t arg = *((disk_interrupt_arg_t *) diskarg);
	char *buf;
	int i;
	printf("Read block 0, initializing superblock\n");
	if (arg.reply != DISK_REPLY_OK) {
		printf("Error in disk request, exiting\n");
		exit(0);
	}
	sBlock = (superblock_t) arg.request.buffer;
	if (sBlock->magicNumber != MAGIC_NUMBER)
	{
		printf("%d, %d", sBlock->magicNumber, MAGIC_NUMBER);
		printf("Filesystem corrupted, magic numbers don't match, exiting\n");
		exit(0);
	}
	setup_superblock_mutex = semaphore_create();
	semaphore_initialize(setup_superblock_mutex, 0);
	setup_inode_mutex = semaphore_create();
	semaphore_initialize(setup_inode_mutex, 1);
	setup_bitmap_mutex = semaphore_create();
	semaphore_initialize(setup_bitmap_mutex, 1);
	inodes = (inode_t) malloc(sizeof(struct inode) * sBlock->num_inodes);
	buf = (char *) malloc(DISK_BLOCK_SIZE * sBlock->num_inodes);
	if (inodes == NULL) {
		printf("Failed to allocate memory for inode table, exiting\n");
		exit(0);
	}

	block_mutexes = (semaphore_t *) malloc(sizeof(semaphore_t) * sBlock->num_data_blocks);
	for (i = 0; i < sBlock->num_data_blocks; i++)
	{
		block_mutexes[i] = semaphore_create();
		semaphore_initialize(block_mutexes[i], 0); 
	}
	printf("initializing inodes\n");
	install_disk_handler(setup_inode);
	outstanding_requests = sBlock->num_inodes;
	for (i = 0; i < sBlock->num_inodes; i++)
	{
		disk_read_block(&disk, 1 + i, buf + (i * DISK_BLOCK_SIZE));	
	}
	semaphore_P(setup_superblock_mutex);	
	for (i = 0; i < sBlock->num_inodes; i++)
	{
		memcpy(&(inodes[i]), buf + (i * DISK_BLOCK_SIZE), sizeof(struct inode));
	}
	printf("Initialized inodes, going on to free block bitmap\n");
	
	free_block_bitmap = (char *) malloc(DISK_BLOCK_SIZE * sBlock->num_free_blocks);
	if (free_block_bitmap == NULL) {
		printf("Failed to allocate memory for free block bitmap, exiting\n");
		exit(0);
	}
	install_disk_handler(setup_bitmap);
	outstanding_requests = sBlock->num_free_blocks;
	for (i = 0; i < sBlock->num_free_blocks; i++)
	{
		disk_read_block(&disk, 1 + sBlock->num_inodes + i, free_block_bitmap + (DISK_BLOCK_SIZE * i));
	}
	semaphore_P(done_mutex);
	install_disk_handler(handle_disk_response);
	free(buf);
}

// find a free block, and mark it as not free in the bitmap. 1 is free.
int allocate_block() {
	int i, j;
	unsigned char orbyte;
	for (i = 0; i < (DISK_BLOCK_SIZE * sBlock->num_free_blocks); i++)
	{
		j = 0;
		orbyte = 0x01;
		while (j < 8) {
			if (cmp & free_block_bitmap[i])
			{
				free_block_bitmap[i] |= orbyte;
				return i + j;
			}
			cmp  = cmp << 1;
		}
	}
}

// mark data block blockid as free. 1 is free.
void free_block(int blockid) {
	int bitoffset = blockid % 8;
	int i;
	unsigned char andbyte = ~(0x01 << bitoffset);
	free_block_bitmap[blockid / 8] &= andbyte;
}

void handle_disk_response(void *arg) {
	disk_interrupt_arg_t arg = *((disk_interrupt_arg_t *) diskarg);
	if (arg->type == DISK_READ)
	{
		semaphore_V(block_mutexes[arg->request->blocknum - sBlock->data_block_start]);
	}
}

void minifile_initialize()
{
	char *buf;
	buf = malloc(DISK_BLOCK_SIZE);
	blockcache = blockcache_new();
	if (access("MINIFILESYSTEM", W_OK) < 0)
	{
		printf("No disk file found. If you're running mkfs, ignore this message. If you're not, please run mkfs first to create a disk before you run this program.\n");
		return;
	}
	disk_name = "MINIFILESYSTEM";
	use_existing_disk = 1;	
	done_mutex = semaphore_create();	
	semaphore_initialize(done_mutex, 0);
	printf("Reading block 0 from disk...\n");
	if (disk_initialize(&disk) < 0) {
		printf("Error initializing disk. Maybe you forgot to run mkfs first?\n");
		exit(0);
	}
	install_disk_handler(setup_superblock);
	disk_read_block(&disk, 0, buf);	
}

minifile_t minifile_creat(char *filename)
{
	return NULL;
}

minifile_t minifile_open(char *filename, char *mode){
	return NULL;
}


int inode_read(inode_t inode, char *data, int position, int maxlen)
{
	int curblock;
	int blockoffset;
	int amountToRead;
	char *blockptr;
	int amountLeft;
	curblock = position / DISK_BLOCK_SIZE;
	blockoffset = position % DISK_BLOCK_SIZE;
	if (maxlen > inode->bytesWritten - position) 
	{
		amountToRead = inode->bytesWritten - position;
	}
	else
	{
		amountToRead = maxlen;
	}
	amountLeft = amountToRead;
	while (amountLeft > 0) {
		//TODO add support for indirect blocks here
		get_data_block(inode->directblocks[curblock], &blockptr);		
		curblock++;
		if (amountLeft > DISK_BLOCK_SIZE) {
			memcpy(data, blockptr, DISK_BLOCK_SIZE);
			amountLeft -= DISK_BLOCK_SIZE;
		}
		else {
			memcpy(data, blockptr, amountLeft);
			break;
		}
	}
	return amountToRead;
}

int minifile_read(minifile_t file, char *data, int maxlen)
{
	int ret;
	inode_t inode = inodes[file->inode];	
	ret = inode_read(inode, data, position, maxlen);
	file->position += ret;
	return ret;
}

void get_data_block(int blockid, char **ret) 
{
	if (blockcache_get(blockcache, blockid, (void **) ret) >= 0)
	{
		return;
	}	
	else
	{
		ret = &((char *) malloc(DISK_BLOCK_SIZE));
		disk_read_block(&disk, blockid + sBlock->data_block_start, *ret);
		semaphore_P(block_mutexes[blockid]);
	}
}

int inode_write(inode_t inode, char *data, int position, int len)
{
	int currentblock = position / DATA_BLOCK_SIZE;
	int offset = position % DATA_BLOCK_SIZE;
	int amountToWrite = len;
	int amountRemaining = len;
	char *buf;
	while (amountRemaining > 0) {
		// TODO add support for indirect blocks here
		if (inode->directblocks[currentblock] == 0) {
			inode->directblocks[currentblock] = allocate_block();
			buf = (char *) malloc(DATA_BLOCK_SIZE);
			inode->size++;
		}
		else {
			get_data_block(inode->directblocks[currentblock], &buf);	
		}
		if (amountRemaining < DISK_BLOCK_SIZE - offset)
		{
			// can fit everything into the current block
			memcpy(buf + offset, data, amountRemaining);
			amountRemaining = 0; 
			disk_write_block(&disk, inode->directblocks[currentblock] + sBlock->data_block_start);
		}
		else
		{
			memcpy(buf + offset, data, DISK_BLOCK_SIZE - offset);
			data = data + (DISK_BLOCK_SIZE - offset);
			offset = 0;	
			currentblock++;
			amountRemaining -= (DISK_BLOCK_SIZE - offset);
		}
	}
	inode->bytesWritten += amountToWrite;	
	return amountToWrite;
}


int minifile_write(minifile_t file, char *data, int len){
	inode_t inode = inodes[file->inode];
	return inode_write(inode, data, file->position, len); 
}

void inode_remove(inode_t inode)
{
	int i;
	char *buf = (char *) calloc(DISK_BLOCK_SIZE, 1);
	inode_t overwrite = (inode_t) buf;
	for (i = 0; i < inode->size; i++)
	{
		//TODO: add support for indirect blocks when inode->size > TABLE_SIZE
		free_block(inode->directblocks[i]);	
	}	
}

int minifile_close(minifile_t file){
	inode_t inode = inodes[file->inode];
	inode->references--;
	if (inode->references == 0) {
		inode_remove(inode);
	}
}

int minifile_unlink(char *filename){
	return -1;
}

int minifile_mkdir(char *dirname){
	return -1;
}

int minifile_rmdir(char *dirname){
	return -1;
}

int minifile_stat(char *path){
	return -1;
} 

int minifile_cd(char *path){
	return -1;
}

char **minifile_ls(char *path){
	return NULL;
}

char* minifile_pwd(void){
	return NULL;
}
