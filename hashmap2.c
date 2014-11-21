#include <stdlib.h>
#include <stdio.h>
#include "hashmap.h"

#define INITIAL_SIZE 1024

//Return an empty hashmap, or NULL on failure.
hashmap_t hashmap_new() 
{
	hashmap_map* hashmap = (hashmap_map*) malloc(sizeof(hashmap_map));
	if(hashmap == NULL) 
		return NULL;

	hashmap->data = (hashmap_element*) calloc(INITIAL_SIZE, sizeof(hashmap_element));
	if(hashmap->data == NULL)
		return NULL;

	hashmap->table_size = INITIAL_SIZE;
	hashmap->size = 0;

	return hashmap;
}

//Return the integer of the location in data to store the point to the item, or -1.
int hashmap_hash(hashmap_t hashmap, int key)
{
	int curr;
	int i;

	if(hashmap->size == hashmap->table_size) 
		return -1;

	curr = hashmap_hash_int(hashmap, key);

	for(i = 0; i< hashmap->table_size; i++)
	{
		if(hashmap->data[curr].in_use == 0)
			return curr;

		if(hashmap->data[curr].key == key && hashmap->data[curr].in_use == 1)
			return curr;

		curr = (curr + 1) % hashmap->table_size;
	}

	return -1;
}

//Doubles the size of the hashmap, and rehashes all the elements
int hashmap_rehash(hashmap_t hashmap)
{
	int i;
	int old_size;
	hashmap_element* curr;

	/* Setup the new elements */
	hashmap_map *newHashmap = (hashmap_map *) hashmap;	
	hashmap_element* temp = (hashmap_element *) calloc(2 * newHashmap->table_size, sizeof(hashmap_element));
	if(!temp) 
		return -1;

	/* Update the array */
	curr = newHashmap->data;
	newHashmap->data = temp;

	/* Update the size */
	old_size = m->table_size;
	newHashmap->table_size = 2 * newHashmap->table_size;
	newHashmap->size = 0;

	/* Rehash the elements */
	for(i = 0; i < old_size; i++)
	{
		int status = hashmap_put(newHashmap, curr[i].key, curr[i].data);
		if (status != MAP_OK)	
			return status;
	}

	free(curr);

	return 0;
}

//Add a pointer to the hashmap with some key
int hashmap_put(hashmap_t hashmap, int key, hashmap_item_t value)
{
	int index = hashmap_hash(hashmap, key);

	while(index == -1)
	{
		if (hashmap_rehash(hashmap) == -1) 
			return -1;
		index = hashmap_hash(hashmap, key);
	}

	hashmap->data[index].data = value;
	hashmap->data[index].key = key;
	hashmap->data[index].in_use = 1;
	hashmap->size++; 

	return 0;
}

//Get your pointer out of the hashmap with a key
int hashmap_get(hashmap_t hashmap, int key, hashmap_item_t *arg)
{
	int curr;
	int i;

	curr = hashmap_hash_int(hashmap, key);

	for (i = 0; i< hashmap->table_size; i++)
	{
		if (hashmap->data[curr].key == key && hashmap->data[curr].in_use == 1)
		{
			*arg = (int *) (hashmap->data[curr].data);
			return 0;
		}

		curr = (curr + 1) % hashmap->table_size;
	}

	*arg = NULL;

	//Not found
	return -1;
}

/*
 * Get a random element from the hashmap
 */
int hashmap_get_one(hashmap_t hashmap, hashmap_item_t *arg, int remove)
{
	int i;

	if (hashmap_length(hashmap) <= 0) 	
		return -1;

	for (i = 0; i< hashmap->table_size; i++)
		if (hashmap->data[i].in_use != 0)
		{
			*arg = (any_t) (hashmap->data[i].data);
			if (remove) 
			{
				hashmap->data[i].in_use = 0;
				hashmap->size--;
			}
			return 0;
		}

	return 0;
}

//Remove an element with that key from the map, 0 on success, -1 on fail
int hashmap_remove(hashmap_t hashmap, int key)
{
	int i;
	int curr;

	curr = hashmap_hash_int(hashmap, key);

	for (i = 0; i< hashmap->table_size; i++)
	{
		if(hashmap->data[curr].key == key && hashmap->data[curr].in_use == 1)
		{
			hashmap->data[curr].in_use = 0;
			hashmap->data[curr].data = NULL;
			hashmap->data[curr].key = 0;

			hashmap->size--;
			return 0;
		}
		curr = (curr + 1) % hashmap->table_size;
	}

	return -1;
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
