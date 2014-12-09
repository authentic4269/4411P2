#include "minifile.h"
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
char *free_block_bitmap;

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
	free(buf);
}

void minifile_initialize()
{
	char *buf;
	buf = malloc(DISK_BLOCK_SIZE);
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


int minifile_read(minifile_t file, char *data, int maxlen)
{
	return -1;
}

int minifile_write(minifile_t file, char *data, int len){
	return -1;
}

int minifile_close(minifile_t file){
	return -1;
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
