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
main(int argc, char *argv[]) {
	void *hi = NULL;
	void **ptr = &hi;
	linkedList_t testlist = linkedList_new();
	int **iptr;
	int a = 0, b = 1, c = 2;

	//See that list is empty
	if (linkedList_isEmpty(testlist) != 1)
		printf("Empty test 1 failed\n");
	
	//Add a = 0 to list
	linkedList_insert(testlist, &a);
	//Check that list length = 1
	if (linkedList_length(testlist) != 1)
		printf("Length test 1 failed\n");
	//Check list not empty
	if (linkedList_isEmpty(testlist) != 0)
		printf("Empty test 2 failed\n");

	//Add b = 1 and c = 2 to list
	linkedList_insert(testlist, &b);
	linkedList_insert(testlist, &c);
	
	//Check length = 3
	if (linkedList_length(testlist) != 3)
		printf("Length test 2 failed\n");

	//Dequeue first element from list
	linkedList_dequeue(testlist, ptr);
	iptr = (int **) ptr;
	if (**iptr != 0)
		printf("Dequeue test failed. Expected 0, got %d\n", **iptr);			

	//Delete b = 1 from list
	if (linkedList_delete(testlist, &b) != 0)
		printf("Delete test failed\n");

	//Dequeue c = 2
	linkedList_dequeue(testlist, ptr);
	iptr = (int **) ptr;
	if (**iptr != 2)
		printf("Delete test failed. Expected to dequeue 1, dequeued %d\n", **iptr);

	//Check that length = 0
	if (linkedList_length(testlist) != 0)
		printf("Length test 3 failed\n");
	
	return 0;
}

