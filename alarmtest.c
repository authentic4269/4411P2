#include<stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "alarm.h"

int x;

void func(void *arg)
{
	x++;
}

int
main(int argc, char *argv[])
{
	sortedlist_t list = new_sortedlist();
	listnode_t node;
	int i;
	int *arg;
	x = 0;
	for (i = 99; i >= 0; i--)
	{
		node = (listnode_t) malloc(sizeof(listnode));
		node->id = malloc(sizeof(short));
		node->time = i;
		arg = (int *) malloc(sizeof(int));	
		*arg = i;
		node->arg = arg;
		node->func = func;
		insert(list, node);
	}	
	callAlarms(list, 50);
	assert(x == 50);
	callAlarms(list, 50);
	assert(x == 50);
	callAlarms(list, 100);
	assert(x == 100);
	return 0;
}
