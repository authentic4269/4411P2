/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

//Items in the queue
struct queueNode { 
	void* data;	//Pointer to item
	struct queueNode* next; //Next queue node in queue
	struct queueNode* prev; //Last queue node in queue
};

//Queue
struct queue { 
	int size; //Size of queue
	struct queueNode *front; //First queueNode in queue
	struct queueNode *rear; //Last queueNode in queue
};

/*
 * Return an empty queue.
 * Queue will be size 0.
 */
queue_t
queue_new() {
	queue_t newQueue = (queue_t) malloc(sizeof(struct queue));
    
    //Makes sure newQueue was made sucessfully
    if (newQueue == NULL)
    	return NULL;
    //else
    newQueue->size = 0;
    newQueue->front = NULL;
    newQueue->rear = NULL;

    return (queue_t) newQueue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  
 * Return 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t queue, void* item) {
	queueNode_t newFront = (queueNode_t) malloc(sizeof(struct queueNode));

	//Check that queue and item exists and that newFront was properly created
	if (queue != NULL && item != NULL && newFront != NULL) 
	{
		//Populate newFront
		newFront->data = item;
		newFront->next = queue->front;
		newFront->prev = NULL;

		if (!queue_length(queue))
			queue->rear = newFront;
		else
			queue->front->prev = newFront;

		//Assign newFront to be the front of the queue
		queue->front = newFront;

		//Reflect that queue grew in size
		queue->size++;

		return 0;
	}
	//else 
	return -1;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
queue_append(queue_t queue, void* item) {
	queueNode_t newRear = (queueNode_t) malloc(sizeof(struct queueNode));

	//Check that queue and item exists and that newRear was properly created
	if (queue != NULL && item != NULL && newRear != NULL) 
	{
		newRear->data = item;
		newRear->next = NULL;
		newRear->prev = queue->rear;

		if (queue_length(queue) == 0)
 			queue->front = newRear;
		else
			queue->rear->next = newRear;

		//Assign newRear to be the rear of the queue
		queue->rear = newRear;

		//Reflect that queue grew in size
		queue->size++;

		return 0;
	}
	//else 
	return -1;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
queue_dequeue(queue_t queue, void** item) {
	queueNode_t frontNode;

	//Check that queue and item exists
	if (queue != NULL && item != NULL) 
	{
		frontNode = queue->front;
		if (frontNode != NULL)
		{
			*item = queue->front->data;

			//Reassign front
			queue->front = queue->front->next;
			
			if (queue->front != NULL)
				queue->front->prev = NULL;
			if (queue->size == 1)
				queue->rear = NULL;
			
			free(frontNode);
			
			//Reflect that queue shrunk in size
			queue->size--;

			return 0;
		}
	}
	//else return null and -1 if the queue is empty or item doesnt exist
	*item = NULL;						
	return -1;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure).
 */
int
queue_iterate(queue_t queue, func_t f, void* item) {
	queueNode_t nextPtr;
	queueNode_t currentPtr;

	if (queue != NULL && f != NULL)
	{
		//Iterate from front to back
		currentPtr = queue->front;

		if (currentPtr != NULL)
		{
			while (currentPtr != NULL)
			{
				nextPtr = currentPtr->next;

				f(item, currentPtr->data);

				currentPtr = nextPtr;
			}
			return 0;
		}
		//else
		return -1;
	}
	return -1;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int
queue_free (queue_t queue) {
	queueNode_t frontNode;
	queueNode_t tempNode;

	//Make sure queue exists
	if (queue != NULL)
	{
		frontNode = queue->front;
		while (frontNode != NULL)
		{
			tempNode = frontNode;
			frontNode = frontNode->next;
			free(tempNode);							
		}
		free(queue);
		return 0;
	}
	//else 
	return -1;
}

/*
 * Check if the queue is empty. Return 1 (empty) 0 (not empty)
 */
int queue_isEmpty(queue_t queue)
{
	return (queue == NULL);
}

/*
 * Return the number of items in the queue.
 */
int
queue_length(queue_t queue) {
	//If queue is not empty
	if (!queue_isEmpty(queue))
		return queue->size;
	//else 
	return 0;
}

/*
 * Delete the specified item from the given queue.
 * Return 0 on success. Return -1 on error.
 */
int
queue_delete(queue_t queue, void* item) {
	queueNode_t nodeToDelete;
	queueNode_t currentPtr;
	
	if (queue != NULL && item != NULL)
	{	
		currentPtr = queue->front;

		while (currentPtr != NULL)
		{	
			//Item Found
			if(currentPtr->data == item)
			{		
				if (currentPtr->prev != NULL)
					currentPtr->prev->next = currentPtr->next;
				
				if (currentPtr->next != NULL)
					currentPtr->next->prev = currentPtr->prev;

				if (queue->front == currentPtr)
					queue->front = currentPtr->next;
				if (queue->rear == currentPtr)
					queue->rear = currentPtr->prev;

				nodeToDelete = currentPtr;
				currentPtr = currentPtr->next;

				queue->size--;

				free(nodeToDelete);
			}
			//Item not found
			else 
				currentPtr = currentPtr->next;
		}
		return 0;
	}
	//else
	return -1;
}
