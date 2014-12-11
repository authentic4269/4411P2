#include "minifile.h"
#include "blockcache.h"
#include "disk.h"
#include "queue.h"
#include "synch.h"
#include "minithread.h"
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
semaphore_t allocate_inode_mutex;
semaphore_t allocate_block_mutex;
semaphore_t done_mutex;
blockcache_t blockcache;

semaphore_t *block_mutexes;

void handle_disk_response(void *arg); // forward declaration
int allocate_block();
int inode_read(inode_t inode, char *buf, int position, int len);
void free_inode(inode_t inode);
void get_data_block(int blockid, char **ret);
int inode_write(inode_t inode, char *data, int position, int len);

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
	
	free_block_bitmap = (unsigned char *) malloc(DISK_BLOCK_SIZE * sBlock->num_free_blocks);
	if (free_block_bitmap == NULL) {
		printf("Failed to allocate memory for free block bitmap, exiting\n");
		exit(0);
	}
	install_disk_handler(setup_bitmap);
	outstanding_requests = sBlock->num_free_blocks;
	for (i = 0; i < sBlock->num_free_blocks; i++)
	{
		disk_read_block(&disk, 1 + sBlock->num_inodes + i, (char *) (free_block_bitmap + (DISK_BLOCK_SIZE * i)));
	}
	semaphore_P(done_mutex);
	install_disk_handler(handle_disk_response);
	free(buf);
}

inode_t allocate_inode() {
	int i;
	semaphore_P(allocate_inode_mutex);
	for (i = 1; i < sBlock->num_inodes; i++)
	{
		if (inodes[i].free == 1)
		{
			inodes[i].directblocks[0] = allocate_block();
			inodes[i].references = 1;
			inodes[i].size = 1;
			inodes[i].bytesWritten = 0;
			inodes[i].free = 0;
			semaphore_V(allocate_inode_mutex);
			return &(inodes[i]);
		}
	}
	semaphore_V(allocate_inode_mutex);
	return NULL;
}

// find a free block, and mark it as not free in the bitmap. 1 is free.
int allocate_block() {
	int i, j;
	unsigned char andbyte;
	semaphore_P(allocate_block_mutex);
	for (i = 0; i < (DISK_BLOCK_SIZE * sBlock->num_free_blocks); i++)
	{
		j = 0;
		andbyte = 0x01;
		while (j < 8) {
			if (andbyte & free_block_bitmap[i])
			{
				free_block_bitmap[i] &= ~(andbyte);
				semaphore_V(allocate_block_mutex);
				return i + j;
			}
			andbyte  = andbyte << 1;
		}
	}
	semaphore_V(allocate_block_mutex);
	printf("Unable to allocate block, system out of memory\n");
	return -1;
}

// mark data block blockid as free. 1 is free.
void free_block(int blockid) {
	int bitoffset = blockid % 8;
	unsigned char orbyte = (0x01 << bitoffset);
	free_block_bitmap[blockid / 8] |= orbyte;
}

void handle_disk_response(void *arg) {
	disk_interrupt_arg_t diskarg = *((disk_interrupt_arg_t *) arg);
	if (diskarg.request.type == DISK_READ)
	{
		semaphore_V(block_mutexes[diskarg.request.blocknum - sBlock->data_block_start]);
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
	allocate_inode_mutex = semaphore_create();
	semaphore_initialize(allocate_inode_mutex, 1);
	allocate_block_mutex = semaphore_create();
	semaphore_initialize(allocate_block_mutex, 1);
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
	inode_t curdir = &(inodes[runningThread->currentDirectoryInode]);
	int numentries;
	int i;
	char *validname = strchr(filename, '/');
	char *entrybuf = (char *) malloc(sizeof(struct directory_entry));
	minifile_t ret = (minifile_t) malloc(sizeof(struct minifile));
	inode_t newinode;
	directory_entry_t entry;
	if (validname != NULL)
	{
		printf("Invalid file name. Valid file names may not contain the '/' character\n");
		return NULL;
	}
	if (curdir->type != DIRECTORY)
	{
		printf("Warning: current directory type is not set to DIRECTORY\n");
	}		
	numentries = curdir->bytesWritten / sizeof(struct directory_entry);	
	// test if such a file already exists in the current directory
	for (i = 0; i < numentries; i++)
	{
		inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		if (strcmp(entry->name, filename) == 0)
		{
			printf("Error: file already exists in current directory\n");
			return NULL;
		}
	} 
	newinode = allocate_inode();	
	newinode->type = REGULARFILE;
	entry = (directory_entry_t) entrybuf;
	strcpy(entry->name, filename);
	entry->inode_num = newinode->id;
	inode_write(curdir, entrybuf, curdir->bytesWritten, sizeof(struct directory_entry)); 
	ret->inode = newinode->id;
	ret->position = 0;
	ret->type = WRITE;
	return ret;
}

minifile_t minifile_open(char *filename, char *mode){

	inode_t curdir = &(inodes[runningThread->currentDirectoryInode]);
	int numentries;
	int i;
	int foundinode = 0;
	char *validname = strchr(filename, '/');
	char *entrybuf;
	minifile_t ret;
	inode_t newinode;
	directory_entry_t entry;
	if (validname != NULL)
	{
		printf("Invalid file name. Valid file names may not contain the '/' character\n");
		return NULL;
	}
	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') 
	{
		printf("This system supports four modes: r, w, and a. r is for reading only, r+, or update mode, for reading and writing (starting at the beginning), w for truncating and then writing, and a for appending. The request to minifile_open should contain a single character, which must be one of these three\n");
		return NULL;
	}
	if (curdir->type != DIRECTORY)
	{
		printf("Warning: current directory type is not set to DIRECTORY\n");
	}		
	entrybuf = (char *) malloc(sizeof(struct directory_entry));
	ret = (minifile_t) malloc(sizeof(struct minifile));
	
	numentries = curdir->bytesWritten / sizeof(struct directory_entry);	
	// test if such a file already exists in the current directory
	for (i = 0; i < numentries; i++)
	{
		inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		if (strcmp(entry->name, filename) == 0)
		{
			foundinode = entry->inode_num;
		}
	} 

	if (mode[0] == 'r')
	{
		if (foundinode == 0) {
			printf("File does not exist\n");
			return NULL;
		}
		if (mode[1] == '+')
		{
			ret->type = WRITE;
			ret->inode = foundinode;
			ret->position = 0;
			return ret;
		}
		else
		{
			ret->type = READ;
			ret->inode = foundinode;
			ret->position = 0;
			return ret;
		}
	}
	else if (mode[0] == 'w') 	
	{
		if (foundinode != 0) 
			free_inode(&(inodes[foundinode]));
		newinode = allocate_inode();	
		entry = (directory_entry_t) entrybuf;
		strcpy(entry->name, filename);
		entry->inode_num = newinode->id;
		inode_write(curdir, entrybuf, curdir->bytesWritten, sizeof(struct directory_entry)); 
		ret->inode = newinode->id;
		ret->position = 0;
		ret->type = WRITE;
		return ret;
	}
	else if (mode[0] == 'a')
	{
		if (foundinode == 0)
		{
			ret = minifile_creat(filename);
			ret->type = APPEND;
			ret->position = 0;
			return ret;
		}
		else
		{
			// not actually a newinode, heh
			newinode = &(inodes[foundinode]);
		}
		ret->inode = newinode->id;
		ret->type = APPEND;
		ret->position = newinode->bytesWritten;
		return ret;
	}
	else 
	{
		printf("Unknown condition in minifile_open, fatal error\n");
		return NULL;
	}
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
		if (position >= inode->bytesWritten)
		{
			// in this case, we hit the end of the file
			amountToRead = amountToRead - amountLeft;
			break;
		}	
		get_data_block(inode->directblocks[curblock], &blockptr);		
		curblock++;
		amountLeft -= (DISK_BLOCK_SIZE - blockoffset);
		position += (DISK_BLOCK_SIZE - blockoffset);
		if (amountLeft > DISK_BLOCK_SIZE - blockoffset) {
			memcpy(data, blockptr + blockoffset, DISK_BLOCK_SIZE - blockoffset);
			amountLeft -= (DISK_BLOCK_SIZE - blockoffset);
		}
		else {
			memcpy(data, blockptr + blockoffset, amountLeft);
			break;
		}
		blockoffset = 0;
	}
	return amountToRead;
}

int minifile_read(minifile_t file, char *data, int maxlen)
{
	int ret;
	inode_t inode = &(inodes[file->inode]);	
	if (file->position >= inode->bytesWritten)
		return 0;
	ret = inode_read(inode, data, file->position, maxlen);
	file->position += ret;
	return ret;
}

void get_data_block(int blockid, char **ret) 
{
	char *mem;
	if (blockcache_get(blockcache, blockid, (void **) ret) >= 0)
	{
		return;
	}	
	else
	{
		mem = (char *) malloc(DISK_BLOCK_SIZE);
		disk_read_block(&disk, blockid + sBlock->data_block_start, mem);
		*ret = mem;
		semaphore_P(block_mutexes[blockid]);
	}
}

int inode_write(inode_t inode, char *data, int position, int len)
{
	int currentblock = position / DISK_BLOCK_SIZE;
	int offset = position % DISK_BLOCK_SIZE;
	int amountRemaining = len;
	char *buf;
	while (amountRemaining > 0) {
		// TODO add support for indirect blocks here
		if (inode->directblocks[currentblock] == 0) {
			inode->directblocks[currentblock] = allocate_block();
			buf = (char *) malloc(DISK_BLOCK_SIZE);
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
			disk_write_block(&disk, inode->directblocks[currentblock] + sBlock->data_block_start, buf);
		}
		else
		{
			memcpy(buf + offset, data, DISK_BLOCK_SIZE - offset);
			disk_write_block(&disk, inode->directblocks[currentblock] + sBlock->data_block_start, buf);
			data = data + (DISK_BLOCK_SIZE - offset);
			offset = 0;	
			currentblock++;
			amountRemaining -= (DISK_BLOCK_SIZE - offset);
		}
	}
	if (position + len > inode->bytesWritten)
		inode->bytesWritten = position + len;	
	return len;
}


int minifile_write(minifile_t file, char *data, int len){
	inode_t inode = &(inodes[file->inode]);
	if (file->type == READ) {
		printf("Error: the file descriptor provided to minifile_write is read only\n");
		return -1;
	}
	return inode_write(inode, data, file->position, len); 
}

void free_inode(inode_t inode)
{
	int i;
	for (i = 0; i < inode->size; i++)
	{
		//TODO: add support for indirect blocks when inode->size > TABLE_SIZE
		free_block(inode->directblocks[i]);	
		inode->directblocks[i] = 0;
		strcpy(inode->name, "\0");
	}	
	inode->indirectblock = 0;
	inode->size = 0;
	inode->bytesWritten = 0;
	inode->references = 0;
	inode->free = 1;
}

int minifile_close(minifile_t file){
	inode_t inode = &(inodes[file->inode]);
	if (inode->free == 1)
	{
		return -1;
	}
	inode->references--;
	if (inode->references == 0) {
		free_inode(inode);
	}
	return 0;	
}

int minifile_unlink(char *filename){
	char *entrybuf = (char *) malloc(sizeof(struct directory_entry));
	char *overwritebuf = (char *) malloc(sizeof(struct directory_entry));
	int found = 0;
	int i;
	int numentries;
	inode_t curdir = &(inodes[runningThread->currentDirectoryInode]);	
	directory_entry_t entry;
	numentries = curdir->bytesWritten / sizeof(struct directory_entry);	
	// test if such a file already exists in the current directory
	for (i = 0; i < numentries; i++)
	{
		inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		if (strcmp(entry->name, filename) == 0)
		{
			found = 1;
			break;
		}
	} 
	if (found) {
		if (inodes[entry->inode_num].type != REGULARFILE)
		{
			printf("Error: unlink target is not a regular file\n");
			return -1;
		}
		for (i += 1; i < numentries; i++)
		{
			inode_read(curdir, overwritebuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
			inode_write(curdir, overwritebuf, (i-1) * sizeof(struct directory_entry), sizeof(struct directory_entry));
		}	
		curdir->bytesWritten -= sizeof(struct directory_entry);
		return 0;
	}
	printf("No such file exists\n");
	return -1;
}

int minifile_mkdir(char *dirname){
	inode_t curdir = &(inodes[runningThread->currentDirectoryInode]);
	int numentries;
	int i;
	char *validname = strchr(dirname, '/');
	char *entrybuf = (char *) malloc(sizeof(struct directory_entry));
	inode_t newinode;
	directory_entry_t entry;
	if (validname != NULL)
	{
		printf("Invalid file name. Valid file names may not contain the '/' character\n");
		return -1;
	}
	if (curdir->type != DIRECTORY)
	{
		printf("Warning: current directory type is not set to DIRECTORY\n");
	}		
	numentries = curdir->bytesWritten / sizeof(struct directory_entry);	
	// test if such a file already exists in the current directory
	for (i = 0; i < numentries; i++)
	{
		inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		if (strcmp(entry->name, dirname) == 0)
		{
			printf("Error: file or directory already exists with that name in current directory\n");
			return -1;
		}
	} 
	newinode = allocate_inode();	
	newinode->type = DIRECTORY;
	newinode->parent = curdir->id;
	entry = (directory_entry_t) entrybuf;
	strcpy(entry->name, dirname);
	entry->inode_num = newinode->id;
	inode_write(curdir, entrybuf, curdir->bytesWritten, sizeof(struct directory_entry)); 
	return 0;
}

int minifile_rmdir(char *dirname){
	inode_t curdir = &(inodes[runningThread->currentDirectoryInode]);
	int numentriesCurdir;
	int numentriesTarget;
	int found = 0;
	int i;
	char *validname = strchr(dirname, '/');
	char *entrybuf = (char *) malloc(sizeof(struct directory_entry));
	char *overwritebuf = (char *) malloc(sizeof(struct directory_entry));
	inode_t rmdir;
	directory_entry_t entry;
	if (validname != NULL)
	{
		printf("Invalid file name. Valid file names may not contain the '/' character\n");
		return -1;
	}
	if (curdir->type != DIRECTORY)
	{
		printf("Warning: current directory type is not set to DIRECTORY\n");
	}		
	numentriesCurdir = curdir->bytesWritten / sizeof(struct directory_entry);	
	
	for (i = 0; i < numentriesCurdir; i++)
	{
		inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		if (strcmp(entry->name, dirname) == 0)
		{
			found = 1; 
			break;
		}
	} 
	if (found) 
	{	
		rmdir = &(inodes[entry->inode_num]);
		if (rmdir->type != DIRECTORY)
		{
			printf("rmdir target is not a directory\n");
			return -1;
		}
		numentriesTarget = rmdir->bytesWritten / sizeof(struct directory_entry);	
		if (numentriesTarget > 0)
		{
			printf("Target directory is nonempty\n");
		}
		for (i += 1; i < numentriesCurdir; i++)
		{
			inode_read(curdir, overwritebuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
			inode_write(curdir, overwritebuf, (i-1) * sizeof(struct directory_entry), sizeof(struct directory_entry));
		}	
		curdir->bytesWritten -= sizeof(struct directory_entry);
		return 0;	
	}
	else {
		printf("rmdir target not found\n");
		return -1;
	}
}

inode_t find(char *path, inode_t curdir) {
	char *next;
	char *entrybuf;
	int numentries;
	directory_entry_t entry;
	int i;
	int found;
			
	entrybuf = (char *) malloc(sizeof(struct directory_entry));
	while ((next = strtok(path, "/")) != NULL)
	{
		numentries = curdir->bytesWritten / sizeof(struct directory_entry);	
		// test if such a file already exists in the current directory
		found = 0;
		for (i = 0; i < numentries; i++)
		{
			if (curdir->type != DIRECTORY)
			{
				printf("hit an unexpected regular file in find\n");
				return NULL;
			}
			inode_read(curdir, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
			entry = (directory_entry_t) entrybuf;
			if (strcmp(entry->name, next) == 0)
			{
				curdir = &(inodes[entry->inode_num]);
				found = 1;
				break;
			}
		}
	} 
	if (found) // curdir may be a regular file
		return curdir;
	return NULL;
}

int minifile_stat(char *path){
	inode_t target; 
	char newpath[FILENAMELEN * 30];
	if (path[0] == '/')
	{
		strncpy(newpath, (path + 1), strlen(path)-1);
		target = find(newpath, &(inodes[0]));	
	}
	else if ((path[0] == path[1]) && (path[0] == '.')) 
	{
		if (runningThread->currentDirectoryInode == 0)
		{
			printf("Can't specify a path relative to parent directory from the root node\n");
			return -1;
		}
		strncpy(newpath, (path + 3), strlen(path)-3);
		if (strlen(newpath) == 0)
			target = &(inodes[inodes[runningThread->currentDirectoryInode].parent]);
		else
			target = find(newpath, &(inodes[inodes[runningThread->currentDirectoryInode].parent]));
	
	}
	else
	{
		target = find(path, &(inodes[runningThread->currentDirectoryInode]));
	}
	if (target == NULL)
	{
		printf("File not found\n");
		return -1;
	}
	else
	{
		printf("Stats for file %s: \n", target->name);
		printf("Type: %d, bytes written: %d, blocks allocated: %d\n", target->type, target->bytesWritten, target->size);
		return 0;
	}
} 

int minifile_cd(char *path){
	inode_t target; 
	char newpath[FILENAMELEN * MAX_FS_DEPTH];
	if (path[0] == '/')
	{
		strncpy(newpath, (path + 1), strlen(path)-1);
		target = find(newpath, &(inodes[0]));	
	}
	else if ((path[0] == path[1]) && (path[0] == '.')) 
	{
		if (runningThread->currentDirectoryInode == 0)
		{
			printf("Can't specify a path relative to parent directory from the root node\n");
			return -1;
		}
		strncpy(newpath, (path + 3), strlen(path)-3);
		if (strlen(newpath) == 0)
			target = &(inodes[inodes[runningThread->currentDirectoryInode].parent]);
		else
			target = find(newpath, &(inodes[inodes[runningThread->currentDirectoryInode].parent]));
	}
	else
	{
		target = find(path, &(inodes[runningThread->currentDirectoryInode]));
	}
	if (target == NULL)
	{
		printf("Error: cd target not found\n");
		return -1;
	}
	if (target->type != DIRECTORY) 
	{
		printf("Error: cd target is not a directory\n");
		return -1;
	}	
	runningThread->currentDirectoryInode = target->id;
	return 0;
}

char **minifile_ls(char *path){
	inode_t target; 
	int numentries;
	int i;
	char *entrybuf;
	directory_entry_t entry;
	char **ret;
	char newpath[FILENAMELEN * MAX_FS_DEPTH];
	if (path[0] == '/')
	{
		strncpy(newpath, (path + 1), strlen(path)-1);
		target = find(newpath, &(inodes[0]));	
	}
	else if (path[0] == path[1] && path[0] == '.') 
	{
		if (runningThread->currentDirectoryInode == 0)
		{
			printf("Can't specify a path relative to parent directory from the root node\n");
			return NULL;
		}
		strncpy(newpath, (path + 3), strlen(path)-3);
		if (strlen(newpath) == 0)
			target = &(inodes[inodes[runningThread->currentDirectoryInode].parent]);
		else
			target = find(newpath, &(inodes[inodes[runningThread->currentDirectoryInode].parent]));
	}
	else
	{
		target = find(path, &(inodes[runningThread->currentDirectoryInode]));
	}
	if (target == NULL)
	{
		printf("Error: cd target not found\n");
		return NULL;
	}
	if (target->type != DIRECTORY) 
	{
		printf("Error: cd target is not a directory\n");
		return NULL;
	}
	numentries = target->bytesWritten / sizeof(struct directory_entry);	
	entrybuf = (char *) malloc(sizeof(struct directory_entry));
	ret = (char **) malloc(sizeof(char *) * numentries);
	// test if such a file already exists in the current directory
	for (i = 0; i < numentries; i++)
	{
		inode_read(target, entrybuf, i * sizeof(struct directory_entry), sizeof(struct directory_entry));
		entry = (directory_entry_t) entrybuf;
		ret[i] = (char *) malloc(FILENAMELEN);
		memcpy(ret[i], entry->name, FILENAMELEN);
	} 
	return ret;
}

char* minifile_pwd(void){
	char *ret = (char *) malloc(FILENAMELEN * 100);
	char **entries = (char **) malloc(sizeof(char *) * MAX_FS_DEPTH);
	int cur = runningThread->currentDirectoryInode;
	int i = 0;
	int j;
	while (cur != 0)
	{
		entries[i] = inodes[cur].name;
		i++;
		cur = inodes[cur].parent;
	}
	for (j = 0; j < i; j++)
	{
		strcat(ret, "/");
		strcat(ret, entries[i-j]);
	}
	return ret;
}
