#include "minifile.h"
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/stat.h>
#include "synch.h"

semaphore_t done_semaphore;

void disk_handler(disk_t *disk, disk_request_t request, disk_reply_t reply)
{
	semaphore_V(done_semaphore);
}

int main(int argc, char *argv[]) {
	int fd;
	char *endptr;
	int i;
	superblock_t super;
	semaphore_t done_semaphore;
	disk_t newdisk;
	inode_t newinode;
	void *buf;
	done_semaphore = semaphore_create();
	semaphore_initialize(done_semaphore, 0);
	disk_size = strtol(argv[0], &endptr, 2);
	buf = calloc(DISK_BLOCK_SIZE, 1); 
	if (argc < 1) {
		printf("Usage: mkfs <disksize>");
		return 0;
	}
	use_existing_disk = 0;
	disk_name = "minidisk.disk";
	disk_flags = DISK_READWRITE;
	super = (superblock_t) buf;	
	super->magicNumber = MAGIC_NUMBER;
	super->num_inodes = DISK_BLOCK_SIZE / 100;
	super->fs_size = disk_size;
	disk_initialize(&newdisk);
	install_disk_handler(&disk_handler);
	disk_write_block(&newdisk, 0, (char *) buf);
	semaphore_P(done_semaphore);
	free(buf);
	buf = calloc(DISK_BLOCK_SIZE, 1);
	for (i = 0; i < (DISK_BLOCK_SIZE / 100); i++)
	{
		newinode = (inode_t) buf;
		newinode->id = i++;
		newinode->size = 0;
		newinode->references = 0;
	}
	
}
