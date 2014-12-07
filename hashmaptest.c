#include "hashmap.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char **argv) {
	hashmap_t hashmap; 
	int i;
	int *val;
	hashmap = hashmap_new();
	for (i = 0; i < 100; i++)
	{
		val = malloc(sizeof(int)); 
		*val = i * 10; 
		hashmap_insert(hashmap, i, val);
	}
	if (hashmap_length(hashmap) != 100)
		printf("length failure\n");
	for (i = 0; i < 100; i++)
	{
		hashmap_get(hashmap, i, (void **) &val);
		if (*val != i * 10)
			printf("get failure\n");
	}
	for (i = 0; i < 100; i++)
	{
		hashmap_delete(hashmap, i);
		hashmap_get(hashmap, i, (void **) &val);
		if (val != NULL)
			printf("remove failure\n");
	}
	if (hashmap_length(hashmap) != 0)
		printf("length failure\n");	
	return 0;
	
}
