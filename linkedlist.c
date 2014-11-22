/*
 * Generic singly linked list implementation.
 *
 */
#include "linkedlist.h"
#include <stdlib.h>
#include <stdio.h>

//Items in the linkedList
struct linkedListNode { 
	void* data;	//Pointer to item
	int key;
	struct linkedListNode* next; //Next linkedListNode in linkedList
};

//linkedList
struct linkedList { 
	int size; //Size of linkedList
	struct linkedListNode *front; //First linkedList in queue
};

/*
 * Return an empty linkedList.
 * linkedList will be size 0.
 */
linkedList_t
linkedList_new() {
	linkedList_t linkedList = (linkedList_t) malloc(sizeof(struct linkedList));
    
    //Makes sure linkedList was made sucessfully
    if (linkedList == NULL)
    	return NULL;
    //else
    linkedList->size = 0;
    linkedList->front = NULL;

    return (linkedList_t) linkedList;
}

/*
 * Prepend a void* to a linkedList (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
linkedList_insert(linkedList_t linkedList, int key, void* item) {
	linkedListNode_t listNode = (linkedListNode_t) malloc(sizeof(struct linkedListNode));

	//Check that linkedList and item exists and that listNode was properly created
	if (linkedList != NULL && item != NULL && listNode != NULL) 
	{
		listNode->data = item;
		listNode->key = key;
		listNode->next = linkedList->front;

		linkedList->front = listNode;

		//Reflect that linkedList grew in size
		linkedList->size++;

		return 0;
	}
	//else 
	return -1;
}

/*
 * Dequeue and return the first void* from the linkedList or NULL if linkedList
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
linkedList_dequeue(linkedList_t linkedList, void** item) {
	linkedListNode_t frontNode;

	//Check that linkedList and item exists
	if (linkedList != NULL && item != NULL) 
	{
		frontNode = linkedList->front;
		if (frontNode != NULL)
		{
			*item = linkedList->front->data;

			//Reassign front
			linkedList->front = linkedList->front->next;
						
			free(frontNode);
			
			//Reflect that linkedList shrunk in size
			linkedList->size--;

			return 0;
		}
	}
	//else return null and -1 if the linkedList is empty or item doesnt exist
	*item = NULL;						
	return -1;
}

/*
 * Free the linkedList and return 0 (success) or -1 (failure).
 */
int
linkedList_free(linkedList_t linkedList) {
	linkedListNode_t frontNode;
	linkedListNode_t tempNode;

	//Make sure linkedList exists
	if (linkedList != NULL)
	{
		frontNode = linkedList->front;
		while (frontNode != NULL)
		{
			tempNode = frontNode;
			frontNode = frontNode->next;
			free(tempNode);							
		}
		free(linkedList);
		return 0;
	}
	//else 
	return -1;
}

/*
 * Check if the linkedList is empty. Return 1 (empty) 0 (not empty)
 */
int linkedList_isEmpty(linkedList_t linkedList)
{
	return (linkedList == NULL || linkedList->size == 0);
}

/*
 * Return the number of items in the linkedList.
 */
int
linkedList_length(linkedList_t linkedList) {
	//If queue is not empty
	if (linkedList != NULL)
		return linkedList->size;
	//else 
	return 0;
}

/*
 * Delete the specified item from the given linkedList.
 * Return 0 on success. Return -1 on error.
 */
int
linkedList_delete(linkedList_t linkedList, int id) {
	linkedListNode_t nodeToDelete;
	linkedListNode_t lastPtr = NULL;
	linkedListNode_t currentPtr;
	
	if (linkedList != NULL)
	{	
		currentPtr = linkedList->front;

		while (currentPtr != NULL)
		{	
			//Item Found
			if(currentPtr->key == id)
			{		
				if (lastPtr != NULL)
					lastPtr->next = currentPtr->next;
				
				if (linkedList->front == currentPtr)
					linkedList->front = currentPtr->next;

				nodeToDelete = currentPtr;
				currentPtr = currentPtr->next;

				linkedList->size--;

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
linkedList_get(linkedList_t list, int id, void **item)
{
	linkedListNode_t currentPtr;
	
	if (list != NULL)
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
		return -1;
	}
	return -1;

}
