/* test2.c

   Spawn a three threads.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int x;


int test_thread(int* arg) {
	//technically we can't rely on a serial order because
	//it might be more than 1000 milliseconds between
	//interrupts, but this is very unlikely. the test
	//assumes a serial order.
	minithread_sleep_with_timeout((1 + *arg) * 1000);
	printf("Thread %d, x = %d\n", *arg, x);
	assert(x == *arg);
	x++;
	return 0;
}

int thread1(int* nothing) {
	int i;
	int *arg;
	for (i = 0; i < 10; i++)
	{
		arg = (int *) malloc(sizeof(int));
		*arg = i;
		minithread_fork(test_thread, arg);
	}	

  return 0;
}

int get_priority_of_thread(int priority)
{
	unsigned int x = genintrand(100);
	if (x > 90)
		return 3;
	if (x > 75)
		return 2;
	if (x > 50)
		return 1;
	return 0;
}


int
main(int argc, char * argv[]) {
  x = 0;
  minithread_system_initialize(thread1, NULL);
  return 0;
}
