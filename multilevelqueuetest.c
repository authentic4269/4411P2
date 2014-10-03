#include<stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "multilevel_queue.h"

int z;

int
main(int argc, char *argv[])
{
	//Multilevel_queue with 4 (four) levels
	multilevel_queue_t queue = multilevel_queue_new(4);
	int i;
	int j;
	int *x;
	int *p;
	void** item = (void **) &p;

	x = 0;

	//Add 12 items to Queue
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 3; j++)
		{
			x = (int *) malloc(sizeof(int));
			*x = (j*i);	
			multilevel_queue_enqueue(queue, i, x);
		}
	}	

	z = multilevel_queue_length(queue, 0);
	assert(z == 3);

	z = multilevel_queue_dequeue(queue, 1, item); //Items Remaining = 11
	assert(z == 1);	
	z = multilevel_queue_dequeue(queue, 1, item); //Items Remaining = 10
	assert(z == 1);	
	z = multilevel_queue_dequeue(queue, 1, item); //Items Remaining = 9
	assert(z == 1);	
	z = multilevel_queue_dequeue(queue, 1, item); //Items Remaining = 8
	assert(z == 2);	
	
	z = multilevel_queue_length(queue, 1);
	assert(z == 0);	

	z = multilevel_queue_length(queue, 2);
	assert(z == 2);

	//Free remaining items
	z = 1;
	while (z == 1)
		z = multilevel_queue_dequeue(queue, 1, item);

	//Make sure free works
	z = multilevel_queue_free(queue);
	assert(z == 0);

	return 0;
}
