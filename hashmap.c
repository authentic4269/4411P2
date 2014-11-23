#include <stdlib.h>
#include <stdio.h>
#include "hashmap.h"
#include "linkedlist.h"
#include "miniroute.h"


//Return an empty hashmap, or NULL on failure.
hashmap_t hashmap_new() 
{
	hashmap_t hashmap = (hashmap_t) malloc(sizeof(hashmap));
	int i;
	if(hashmap == NULL) 
		return NULL;

	hashmap->data = (linkedList_t *) malloc(SIZE_OF_ROUTE_CACHE * sizeof(linkedList_t));
	if(hashmap->data == NULL)
		return NULL;
	for (i = 0; i < SIZE_OF_ROUTE_CACHE; i++)
		hashmap->data[i] = linkedList_new();

	hashmap->table_size = SIZE_OF_ROUTE_CACHE;
	hashmap->size = 0;

	return hashmap;
}

int hash_int(hashmap_t hashmap, int arg) {
	int hash = (arg * (arg + 3)) % hashmap->table_size;
	if (hash < 0)
		hash *= -1;
	// it would be better if we worked with unsigned int hashes, but time is at a premium right now.
	// so i won't change it, and will do this hack instead ;)
	return hash;
}


//Add a pointer to the hashmap with some key
int hashmap_insert(hashmap_t hashmap, int key, void *value)
{
	int index = hash_int(hashmap, key);
	linkedList_insert(hashmap->data[index], key, value);
	hashmap->size++; 
	return 0;
}

//Get your pointer out of the hashmap with a key
int hashmap_get(hashmap_t hashmap, int key, void **arg)
{
	int index = hash_int(hashmap, key);
	return linkedList_get(hashmap->data[index], key, arg);
}

//Remove an element with that key from the map, 0 on success, -1 on fail
int hashmap_delete(hashmap_t hashmap, int key)
{
	int index = hash_int(hashmap, key);
	if (linkedList_delete(hashmap->data[index], key) == 0)
	{
		hashmap->size--;
		return 0;
	}
	else return -1;
}

//Deallocate the hashmap
void hashmap_destroy(hashmap_t hashmap)
{
	free(hashmap->data);
	free(hashmap);
}

//Return the length of the hashmap
int hashmap_length(hashmap_t hashmap)
{
	if (hashmap != NULL) 
		return hashmap->size;
	else 
		return 0;
}
