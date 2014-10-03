/* test2.c

   Spawn a three threads.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int x;


int testthread(int* arg) {
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
		minithread_fork(testthread, arg);
	}	

  return 0;
}


int
main(int argc, char * argv[]) {
  x = 0;
  minithread_system_initialize(thread1, NULL);
  return 0;
}
