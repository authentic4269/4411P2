#include "hashmap2.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char *argv) {
		printf("insert");
	hashmap_t hashmap = hashmap_new(); 
	int i;
	int *val;
	for (i = 0; i < 100; i++)
	{
		val = malloc(sizeof(int)); 
		*val = i * 10; 
		hashmap_put(hashmap, i, val);
	}
	if (hashmap_length(hashmap) != 100)
		printf("length failure\n");
	for (i = 0; i < 100; i++)
	{
		hashmap_get(hashmap, i, val);
		if (*val != i * 10)
			printf("get failure\n");
	}
	for (i = 0; i < 100; i++)
	{
		hashmap_remove(hashmap, i);
		hashmap_get(hashmap, i, val);
		if (val != NULL)
			printf("remove failure\n");
	}
	if (hashmap_length(hashmap) != 0)
		printf("length failure\n");	
	
}
