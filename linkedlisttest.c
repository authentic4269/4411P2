/* linkedlisttest.c

	Test linkedList implementation
*/

#include "linkedlist.h"

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
	linkedList_t testlist = linkedList_new();
	int **iptr;
	int a = 0, b = 1, c = 2;
	linkedList_insert(testlist, &a);
	linkedList_insert(testlist, &b);
	linkedList_insert(testlist, &c);
	if (linkedList_length(testlist) != 3)
		printf("Length test failed\n");
	linkedList_dequeue(testlist, ptr);
	iptr = (int **) ptr;
	if (**iptr != 0)
		printf("Dequeue test failed. Expected 0, got %d\n", **iptr);			
	if (linkedList_delete(testlist, &b) != 0)
		printf("Delete test failed\n");
	queue_dequeue(testlist, ptr);
	iptr = (int **) ptr;
	if (**iptr != 1)
		printf("Delete test failed. Expected to dequeue 1, dequeued %d\n", **iptr);
	if (linkedList_length(testlist) != 0)
		printf("Length test failed\n");
	return 0;
}

