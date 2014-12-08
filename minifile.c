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
int NUMSEGMENTS;
int DISKSIZE;

struct minifile {
  inode_t inode;
};

//Inode Functions

inode_t create_inode(int id)
{
	int i;
	inode_t newInode = (inode_t) malloc(DISK_BLOCK_SIZE);

	newInode->id = id;
	newInode->data.blockNumber = 0;
	newInode->data.cursor = 0;
	newInode->data.open = 0;
	newInode->data.type = ND;
	newInode->data.size = 0;

	for(i = 0; i < TABLE_SIZE; i++)
		strcpy((char*)newInode->data.direct_pointers[i],"0");

	strcpy((char*)newInode->data.indirect_pointer,"0");

	return newInode;
}

inode_t activate_free_inode(superblock_t sblock)
{
	inode_t extracted_inode = (inode_t) malloc(DISK_BLOCK_SIZE);
	int dequeue_result;

	if (queue_length(sblock->data.inode_queue_FREE) <= 0)
	{
		printf("ERROR in activate_free_inode(): No more free inodes");
		return ((inode_t) 0);
	}

	dequeue_result = queue_dequeue(sblock->data.inode_queue_FREE, (any_t*) extracted_inode);

	if (dequeue_result != 0)
	{
		printf("ERROR in activate_free_inode(): Dequeue Error");
		return ((inode_t) 0);
	}

	return extracted_inode;
}

//End Inode Functions

//Block Functions

block_t create_block(int id)
{
	block_t newBlock = (block_t) malloc(DISK_BLOCK_SIZE);
	newBlock->id = id;

	return newBlock;
}

//End Block Functions

//Call after setting disk.h's 'disk_size'
superblock_t minifile_initialize(superblock_t sBlock)
{
	int i;
	inode_t root_inode;
	inode_t first_inode;
	block_t first_block;

	//Create superblock
	sBlock = (superblock_t) malloc(sizeof(struct superblock));
	sBlock->data.disk_size = disk_size;
	sBlock->data.number = 19540119;
	
	//Initialize inodes & blocks
	sBlock->data.inodeQueueFree = queue_new();
	sBlock->data.inodeQueueUsed = queue_new();
	sBlock->data.blockQueueFree = queue_new();
	sBlock->data.blockQueueUsed = queue_new();

	root_inode = create_inode(0);
	first_inode = create_inode(1);
	queue_append(sBlock->data.inodeQueueUsed,(any_t)root_inode);
	queue_append(sBlock->data.inodeQueueFree,(any_t)first_inode);

	for(i = 2; i < (0.1 * disk_size); i++)
	{
		queue_append(sBlock->data.inodeQueueFree, (any_t) create_inode(i));
	}

	first_block = (block_t) malloc(sizeof(struct block));
	queue_append(sBlock->data.blockQueueFree, (any_t) first_block);

	for(i = 1; i < (0.9 * disk_size - 1); i++)
	{
		queue_append(sBlock->data.blockQueueFree, (any_t) create_block(i));
	}

	//Finish superblock
	sBlock->data.root_inode = root_inode;

	return sBlock;
}

minifile_t minifile_creat(char *filename)
{
	minifile_t newMinifile = (minifile_t) malloc(sizeof(struct minifile));
	inode newInode = activate_free_inode(sBlock);

	if (strcmp((char*) ((block_t) 0), (char*) newInode) == 0)
	{
		printf("Unable to create minifile. No free inodes left.\n");
		return ((minifile_t) 0);
	}

	newInode->data.type = FILE;
	newMinifile->inode = newInode;

	return newMinifile;
}

minifile_t minifile_open(char *filename, char *mode){
	return NULL;
}


int minifile_read(minifile_t file, char *data, int maxlen)
{
	for (int i = 0; i < TABLE_SIZE; i++)
		strcpy(data+(i*DISK_BLOCK_SIZE), file->inode->data.direct_pointers[i]->file_data);
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
