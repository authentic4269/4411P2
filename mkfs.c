#include "minifile.h"
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/stat.h>
#include "disk.h"
#include "synch.h"

semaphore_t done_semaphore;

void disk_handler(void *arg)
{
	disk_interrupt_arg_t *intr = (disk_interrupt_arg_t *) arg;	
	if (intr->reply != DISK_REPLY_OK) {
		printf("Disk error in mkfs, exiting\n");
		exit(0);
	}
}

int main(int argc, char *argv[]) {
	char *endptr;
	int i, j;
	superblock_t super;
	disk_t newdisk;
	inode_t newinode;
	char *buf;
	int bitmapsz;
	done_semaphore = semaphore_create();
	semaphore_initialize(done_semaphore, 0);
	if (argc < 2) {
		printf("Usage: mkfs <disksize>");
		return 0;
	}
	disk_size = strtol(argv[1], &endptr, 10);
	use_existing_disk = 0;
	disk_name = "minidisk.disk";
	disk_flags = DISK_READWRITE;
	disk_initialize(&newdisk);
	install_disk_handler(&disk_handler);
	buf = calloc(DISK_BLOCK_SIZE, 1);
	for (i = 1; i < (disk_size / 100) + 1; i++)
	{
		newinode = (inode_t) buf;
		newinode->id = i;
		disk_write_block(&newdisk, i, buf);
	}
	printf("initializing fs named %s with %d inodes...\n", disk_name, (disk_size / 100));
	bitmapsz = ((disk_size - 1 - (disk_size / 100)) / DISK_BLOCK_SIZE) + 1; // the plus 1 accounts for floating part truncation
	j = i;
	buf = malloc(DISK_BLOCK_SIZE);
	for (i = 0; i < DISK_BLOCK_SIZE; i++)
	{
		// need a 1 for every block, because initially every block is free
		buf[i] = 0xFF;
	}
	for (i = 0; i < bitmapsz; i++)
	{
		disk_write_block(&newdisk, i + j, buf);	
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
	
	return 0;	
}
