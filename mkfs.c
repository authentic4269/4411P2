#include "minifile.h"
#include "minithread.h"
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/stat.h>
#include "disk.h"
#include "synch.h"
#include "unistd.h"


semaphore_t serial_mutex;
int file_fd;

void disk_handler(void *arg)
{
	disk_interrupt_arg_t *intr = (disk_interrupt_arg_t *) arg;	
	printf("hello from disk handler\n");
	if (intr->reply != DISK_REPLY_OK) 
	{
		printf("Disk error in mkfs, exiting\n");
		exit(0);
	}
	semaphore_V(serial_mutex);
}

void mkfs(int *arg) 
{
	int i, j;
	int blockposition = 0;
	superblock_t super;
	disk_t newdisk;
	inode_t newinode;
	char *buf;
	int bitmapsz;
	close(file_fd);
	disk_size = *arg;
	use_existing_disk = 0;
	disk_name = "MINIFILESYSTEM";
	disk_flags = DISK_READWRITE;
	serial_mutex = semaphore_create();
	semaphore_initialize(serial_mutex, 0);
	disk_initialize(&newdisk);
	install_disk_handler(disk_handler);
	buf = calloc(DISK_BLOCK_SIZE, 1);
	
	// initialize the root node separately, allocating its data blocks
	newinode = (inode_t) buf;
	newinode->id = 0;
	newinode->references = 1;
	newinode->free = 0;	
	newinode->type = DIRECTORY;
	newinode->size = TABLE_SIZE;
	strcpy(newinode->name, "/\0");
	
	for (i = 0; i < TABLE_SIZE; i++)
		newinode->directblocks[i] = i;
	
	disk_write_block(&newdisk, 1, buf);
	semaphore_P(serial_mutex);	
	free(buf);
	buf = calloc(DISK_BLOCK_SIZE, 1);
	newinode = (inode_t) buf;
	newinode->free = 1;
	
	for (i = 2; i < (disk_size / 100) + 1; i++)
	{
		newinode->id = (i - 1);
		disk_write_block(&newdisk, i, buf);
		semaphore_P(serial_mutex);	
	}
	
	printf("initializing fs named %s with %d inodes...\n", disk_name, (disk_size / 100));
	bitmapsz = ((disk_size - 1 - (disk_size / 100)) / DISK_BLOCK_SIZE) + 1; // the plus 1 accounts for floating part truncation
	j = i;
	buf = malloc(DISK_BLOCK_SIZE);

	for (i = 0; i < TABLE_SIZE; i++) 
	{
		// need a 0 in the first TABLE_SIZE blocks, because these are allocated to the root node
		buf[i / 8] = 0xFF;
	}
	
	for (i = 0; i < TABLE_SIZE; i++)
	{
		// this logic might be a little confusing. it puts a 0 in the first TABLE_SIZE bits of buf. 
		if (i && i % 8 == 0) 
		{
			blockposition = 8 / i;
		}
		// 0xfe << (i % 8) has a zero in the i %8'th position
		buf[blockposition] &= (0xfe << (i % 8));
	}

	for (i = blockposition + 1; i < DISK_BLOCK_SIZE; i++)
	{
		// need a 1 for every other block, because initially they're all free
		buf[i] = 0xFF;
	}

	disk_write_block(&newdisk, j, buf);
	semaphore_P(serial_mutex);

	for (i = 0; i < DISK_BLOCK_SIZE; i++)
	{
		// need a 1 for every other block, because initially they're all free
		buf[i] = 0xFF;
	}

	for (i = 1; i < bitmapsz; i++)
	{
		disk_write_block(&newdisk, i + j, buf);	
		semaphore_P(serial_mutex);
	}
	printf("initializng free block bitmap, in %d blocks\n", bitmapsz);
	buf = calloc(DISK_BLOCK_SIZE, 1);
	super = (superblock_t) buf;	
	super->magicNumber = MAGIC_NUMBER;
	super->num_inodes = disk_size / 100;
	super->num_free_blocks = bitmapsz;
	super->free_blocks = j;
	super->data_block_start = i + j;
	super->num_data_blocks = disk_size - bitmapsz - 1 - super->num_inodes; // 1 for superblock, then the inodes and bitmap
	super->fs_size = disk_size;
	printf("initializing superblock\n");
	disk_write_block(&newdisk, 0, (char *) buf);
	semaphore_P(serial_mutex);
	printf("shutting down disk\n");
	disk_shutdown(&newdisk);	
	exit(0);
}

int main(int argc, char *argv[]) 
{
	int sz;
	char *endptr;
	if (argc < 2) 
	{
		printf("Usage: mkfs <disksize>");
		return 0;
	}
	if (access("MINIFILESYSTEM", W_OK) > -1) 
	{
		remove("MINIFILESYSTEM");
	}
	sz = strtol(argv[1], &endptr, 10);
	minithread_system_initialize(mkfs, &sz);	
	return -1;	
}
