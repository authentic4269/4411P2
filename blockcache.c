/*
 * Generic singly linked list implementation.
 *
 */
#include "blockcache.h"
#include <stdlib.h>
#include <stdio.h>

#define LIMIT 30

//Items in the blockcache
struct blockcacheNode { 
	void* data;	//Pointer to item
	int key;
	struct blockcacheNode* next; //Next blockcacheNode in blockcache
};

//blockcache
struct blockcache { 
	int size; //Size of blockcache
	struct blockcacheNode *front; //First blockcache in queue
};

/*
 * Return an empty blockcache.
 * blockcache will be size 0.
 */
blockcache_t
blockcache_new() {
	blockcache_t blockcache = (blockcache_t) malloc(sizeof(struct blockcache));
    
    //Makes sure blockcache was made sucessfully
    if (blockcache == NULL)
    	return NULL;
    //else
    blockcache->size = 0;
    blockcache->front = NULL;

    return (blockcache_t) blockcache;
}

/*
 * Prepend a void* to a blockcache (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
blockcache_insert(blockcache_t blockcache, int key, void* item) {
	blockcacheNode_t listNode = (blockcacheNode_t) malloc(sizeof(struct blockcacheNode));

	//Check that blockcache and item exists and that listNode was properly created
	if (blockcache != NULL && item != NULL && listNode != NULL) 
	{
		listNode->data = item;
		listNode->key = key;
		listNode->next = blockcache->front;

		blockcache->front = listNode;

		//Reflect that blockcache grew in size
		blockcache->size++;

		if (blockcache->size > LIMIT)
		{
			blockcache_delete_last(blockcache);
		}
		return 0;
	}
	//else 
	return -1;
}

/*
 * Dequeue and return the first void* from the blockcache or NULL if blockcache
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
blockcache_dequeue(blockcache_t blockcache, void** item) {
	blockcacheNode_t frontNode;

	//Check that blockcache and item exists
	if (blockcache != NULL && item != NULL) 
	{
		frontNode = blockcache->front;
		if (frontNode != NULL)
		{
			*item = blockcache->front->data;

			//Reassign front
			blockcache->front = blockcache->front->next;
						
			free(frontNode);
			
			//Reflect that blockcache shrunk in size
			blockcache->size--;

			return 0;
		}
	}
	//else return null and -1 if the blockcache is empty or item doesnt exist
	*item = NULL;						
	return -1;
}

/*
 * Free the blockcache and return 0 (success) or -1 (failure).
 */
int
blockcache_free(blockcache_t blockcache) {
	blockcacheNode_t frontNode;
	blockcacheNode_t tempNode;

	//Make sure blockcache exists
	if (blockcache != NULL)
	{
		frontNode = blockcache->front;
		while (frontNode != NULL)
		{
			tempNode = frontNode;
			frontNode = frontNode->next;
			free(tempNode);							
		}
		free(blockcache);
		return 0;
	}
	//else 
	return -1;
}

/*
 * Check if the blockcache is empty. Return 1 (empty) 0 (not empty)
 */
int blockcache_isEmpty(blockcache_t blockcache)
{
	return (blockcache == NULL || blockcache->size == 0);
}

/*
 * Return the number of items in the blockcache.
 */
int
blockcache_length(blockcache_t blockcache) {
	//If queue is not empty
	if (blockcache != NULL)
		return blockcache->size;
	//else 
	return 0;
}

void
blockcache_delete_last(blockcache_t blockcache) {
	blockcacheNode_t lastPtr = NULL;
	blockcacheNode_t currentPtr;
	blockcacheNode_t secondLastPtr = NULL;
	
	if (blockcache != NULL && blockcache->size > 2)
	{	
		currentPtr = blockcache->front;

		while (currentPtr != NULL)
		{	
			secondLastPtr = lastPtr;
			lastPtr = currentPtr;
			currentPtr = currentPtr->next;
		}
		secondLastPtr->next = NULL;
		free(lastPtr->data);
		free(lastPtr);
		blockcache->size--;
	}

}

/*
 * Delete the specified item from the given blockcache.
 * Return 0 on success. Return -1 on error.
 */
int
blockcache_delete(blockcache_t blockcache, int id) {
	blockcacheNode_t nodeToDelete;
	blockcacheNode_t lastPtr = NULL;
	blockcacheNode_t currentPtr;
	
	if (blockcache != NULL)
	{	
		currentPtr = blockcache->front;

		while (currentPtr != NULL)
		{	
			//Item Found
			if(currentPtr->key == id)
			{		
				if (lastPtr != NULL)
					lastPtr->next = currentPtr->next;
				
				if (blockcache->front == currentPtr)
					blockcache->front = currentPtr->next;

				nodeToDelete = currentPtr;
				currentPtr = currentPtr->next;

				blockcache->size--;

				free(nodeToDelete);
				return 0;
			}
			//Item not found
			else 
			{
				lastPtr = currentPtr;
				currentPtr = currentPtr->next;
			}
		}
	}
	return -1;
}

int
blockcache_get(blockcache_t list, int id, void **item)
{
	blockcacheNode_t currentPtr;
	
	if (list != NULL && list->size > 0)
	{	
		currentPtr = list->front;

		while (currentPtr != NULL)
		{	
			//Item Found
			if(currentPtr->key == id)
			{		
				*item = currentPtr->data;
				return 1;
			}
			//Item not found
			else 
			{
				currentPtr = currentPtr->next;
			}
		}
	}
	*item = NULL;
	return -1;
}
