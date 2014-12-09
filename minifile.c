#include "minifile.h"
#include "disk.h"
#include "queue.h"

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
disk_name = "minidisk.disk";
disk_t disk;

void setup_superblock(void *diskarg)
{

}

void minifile_initialize()
{
	char buf[DISK_BLOCK_SIZE];
	use_existing_disk = 1;	
	
	if (disk_initialize(&disk) < 0) {
		printf("Error initializing disk. Maybe you forgot to run mkfs first?\n");
		exit(0);
	}
	install_disk_handler(&setup_superblock);
	
	
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
