#include "hashmap2.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char *argv) {
		printf("insert");
	hashmap_t hashmap = hashmap_new(20, 1); 
	int i;
	int *val;
	for (i = 0; i < 10; i++)
	{
		val = malloc(sizeof(int)); 
		*val = i * 10; 
		hashmap_put(hashmap, i, val);
	}
	for (i = 0; i < 10; i++)
	{
		val = hashmap_get(hashmap, i);
		if (*val != i * 10)
			printf("failure\n");
	}
	
}
