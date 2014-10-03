/*
 * Multilevel queue manipulation functions  
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

/*
Implement multilevel queues and use them to change your FCFS scheduler into a multilevel
feedback scheduler with four levels. The quanta size should double at each level, and
levels with shorter quanta should receive higher priority than levels with longer quanta.

To prevent starvation of lower priority threads, your scheduler should not always search
for threads starting at level 0. Instead, the starting point should be varied in the
proportions 50%, 25%, 15% and 10% for levels 0 to 3 respectively.
*/

struct multilevel_queue {
	queue_t* queues;
	int levels;
};


/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels)
{
	queue_t* queues; //Pointer for the queues
	int i; //Used in for loop

	//Malloc newMultilevelQueue
	multilevel_queue_t newMultilevelQueue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));

	//Ensures that newMultilevelQueue is properly initialized
	if (newMultilevelQueue == NULL)
		return NULL;

	//Initialize pointer to queues
	queues = (queue_t*) malloc(sizeof(queue_t) * number_of_levels);

	//Ensures that queues (array of secondary queue pointers) is properly initialized
	if (queues == NULL)
		return NULL;

	for (i = 0; i < number_of_levels; i++)
	{
		queues[i] = queue_new();
	}

	//Initialize newMultilevelQueue
	newMultilevelQueue->levels = number_of_levels;
	newMultilevelQueue->queues = queues;

	return (multilevel_queue_t) newMultilevelQueue;
}

/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item)
{
	//Ensure all arguments are valid - multilevel_queue & item are not null, levels exist in the queue
	//and not attempting to add to a level that doesnt exist in the queue
	if (queue == NULL || queue->levels == 0 || level >= queue->levels || item == NULL)
		return -1;

	//Append item to the proper level queue in multilevel queue queue_append 
	//will return sucess or failure based on ability to add item to queue
	return queue_append(queue->queues[level], item);
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item)
{
	int found = 1;
	int searchLevel = 0;

	//Ensure all arguments are valid - multilevel_queue & item are not null, levels exist in the queue
	//and not attempting to add to a level that doesnt exist in the queue
	if (queue == NULL || queue->levels == 0 || level < 0 || level >= queue->levels || item == NULL)
		return -1;

	//If found != 0 - void* not found yet, if searchLevel >= queue->levels - all levels searched for a void*
	while(found != 0 && searchLevel < queue->levels)
	{
		//((searchLevel+level)%queue->levels) will start at level and loop back to level-1
		//item will be set to the first void* in queue at the current level being searched
		found = queue_dequeue(queue->queues[((searchLevel+level)%queue->levels)], item);
		searchLevel++;
	}

	//If found != 0, 
	if (found != 0)
	{
		item = NULL;
		return -1;
	}

	//Return the level the first void* was on, must use searchLevel-1 as while loop will searchLevel++;
	return (((searchLevel-1)+level)%queue->levels);
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue)
{
	int i; //Used in for loop

	//Ensures that multilevel_queue is not null
	if (queue == NULL)
		return -1;

	for (i = 0; i < queue->levels; i++)
	{
		//Just frees the queues not the items within
		free(queue->queues[i]);
	}

	//Free malloced pointer
	free(queue->queues);
	//Free multilevel_queue
	free(queue);

	return 0;
}

int multilevel_queue_length(multilevel_queue_t queue, int level)
{
	//queue_length will check if queue is null
	return queue_length(queue->queues[level]);
}
