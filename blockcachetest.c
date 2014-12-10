/* blockcachetest.c

	Test blockcache implementation
*/

#include "blockcache.h"

#include <stdio.h>
#include <stdlib.h>

#define LIMIT 30
int x = 0;

void iter(void *cur, void *ptr) {
	x += 1;
}	

int
main(int argc, char *argv[]) {
	void *hi = NULL;
	void **ptr = &hi;
	blockcache_t testcache = blockcache_new();
	int **iptr;
	int a = 0, b = 1, c = 2, d = 4, e = 5, f = 6;
	int *loc;

	//See that list is empty
	if (blockcache_isEmpty(testcache) != 1)
		printf("Empty test 1 failed\n");
	
	//Add a = 0 to list
	blockcache_insert(testcache, d, &a);
	//Check that list length = 1
	if (blockcache_length(testcache) != 1)
		printf("Length test 1 failed\n");
	//Check list not empty
	if (blockcache_isEmpty(testcache) != 0)
		printf("Empty test 2 failed\n");

	//Add b = 1 and c = 2 to list
	blockcache_insert(testcache, e, &b);
	blockcache_insert(testcache, f, &c);
	
	//Check length = 3
	if (blockcache_length(testcache) != 3)
		printf("Length test 2 failed\n");

	//Dequeue first element from list
	blockcache_get(testcache, d, ptr);
	iptr = (int **) ptr;
	if (**iptr != 0)
		printf("Get test failed. Expected 0, got %d\n", **iptr);			

	//Delete b = 1 from list
	if (blockcache_delete(testcache, e) != 0)
		printf("Delete test failed\n");

	if (blockcache_get(testcache, e, ptr) > 0)
		printf("Get test failed\n");

	//Dequeue c = 2
	blockcache_get(testcache, f, ptr);
	iptr = (int **) ptr;
	if (**iptr != 2)
		printf("Delete test failed. Expected to dequeue 2, dequeued %d\n", **iptr);

	//Check that length = 0
	if (blockcache_length(testcache) != 2)
		printf("Length test 3 failed\n");
	

	// End standard linked list tests, begin block cache tests
	testcache = blockcache_new();
	for (a = 0; a < 100; a++) {
		loc = (int *) malloc(sizeof(int));	
		*loc = a;
		blockcache_insert(testcache, a, (void *) loc);
	}
	if (blockcache_length(testcache) != LIMIT)
	{
		printf("Blockcache test 1 failed\n");
	}
	for (a = 99; a >= 90; a--) 
	{
		blockcache_get(testcache, a, ptr);
		iptr = (int **) ptr;	 	
		if (**iptr != a)
		{
			printf("Blockcache test 2 failed key %d not present\n", a);
		}
	}
	return 0;
}

