/* queuetest.c

	Test queue implementation
*/

#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int x = 0;

void iter(void *cur, void *ptr) {
	x += 1;
}	

int
main(void) {
	void *hi = NULL;
	void **ptr = &hi;
	queue_t testqueue = queue_new();
	int **iptr;
	int a = 0, b = 1, c = 2;
	int f;
	queue_append(testqueue, &a);
	queue_append(testqueue, &b);
	queue_append(testqueue, &c);
	if (queue_length(testqueue) != 3)
		printf("Length test failed\n");
	queue_dequeue(testqueue, ptr);
	iptr = (int **) ptr;
	if (**iptr != 0)
		printf("Dequeue test failed. Expected 0, got %d\n", **iptr);			
	queue_iterate(testqueue, iter, NULL);  
	if (x != 2)
		printf("Iterate test failed. Expected 2, got %d\n", x);	
	if (queue_delete(testqueue, &a) != 0)
		printf("Delete test failed\n");
	queue_dequeue(testqueue, ptr);
	iptr = (int **) ptr;
	if (**iptr != 1)
		printf("Delete test failed. Expected to dequeue 1, dequeued %d\n", **iptr);
	f = 5;
	queue_prepend(testqueue, &f);
	queue_dequeue(testqueue, ptr);
	iptr = (int **) ptr;
	if (**iptr != 5)
		printf("Prepend test failed. Expected 5, got %d\n", **iptr);
	return 0;
}

