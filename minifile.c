#include "minifile.h"
#include "disk.h"
#include "queue.h"

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

 #define TABLE_SIZE 15

char* current_directory;
superblock_t sBlock;

typedef enum {FILE,DIRECTORY,ND} inodetype;

struct block
{
	int id;
	union 
	{
		struct 
		{
			char directory_entries[TABLE_SIZE][256];
			inode_t inode_pointers[TABLE_SIZE];
		} directory;

		struct 
		{
			block_t direct_pointers[(DISK_BLOCK_SIZE/4) -1];
			block_t indirect_pointer;
		} indirect_block;

		char file_data[DISK_BLOCK_SIZE - sizeof(int)];
	};
};

struct inode
{
	int id;
	union
	{
		struct
		{
			int size;
			int open;
			
			int blockNumber;
			int cursor;

			inodetype type;

			block_t direct_pointers[TABLE_SIZE];
			block_t indirect_pointer;
		} data;

		char padding[DISK_BLOCK_SIZE - sizeof(int)];
	}
};

struct superblock 
{
	union 
	{
		struct
		{
			int number;
			int diskSize;
			inode_t root_inode;

			queue_t inodeQueueFree;
			queue_t	inodeQueueUsed;
			queue_t	blockQueueFree;
			queue_t blockQueueUsed;

		} data;

		char padding[DISK_BLOCK_SIZE];
	}
}

struct minifile {
  inode_t inode;
};

minifile_t minifile_creat(char *filename){

}

minifile_t minifile_open(char *filename, char *mode){

}

int minifile_read(minifile_t file, char *data, int maxlen){

}

int minifile_write(minifile_t file, char *data, int len){
minifile_t minifile_open(char *filename, char *mode){
	return NULL;
}

}

int minifile_close(minifile_t file){
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
