/* test2.c

   Spawn a three threads.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

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
	int i, j, y = 0;
	int *arg;
	for (i = 0; i < 20; i++)
	{
		arg = (int *) malloc(sizeof(int));
		*arg = i;
		minithread_fork(test_thread, arg);
		// Do nothing, so that we get interrupted and
		// we register alarms with a different value of
		// the tick count (alarm.h, currentTime)
		for (j = 0; j < 100000000; j++)
		{
			y++;
		}
	}	

  return y;
}


int
main(int argc, char * argv[]) {
  x = 0;
  minithread_system_initialize(thread1, NULL);
  return 0;
}
