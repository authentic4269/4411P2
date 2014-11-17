/* 
 * A linear probing hashmap.
 *
 * This hashmap maintains all inserts as a linked list and stores the clock time
 * that each entry was inserted. This hashmap supports resizing and a static cache size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

#define LOAD_FACTOR 0.5

//Create hashmap with space for int size items
hashmap_t hashmap_new(int size, int resizable)
{
	hashmap_t newHashmap = (hashmap_t) malloc(sizeof(struct hashmap));
	newHashmap->size = size;
	newHashmap->items = 0;
	newHashmap->resizable = resizable;
	newHashmap->data calloc(size, sizeof(hashmap_item_t));
	newHashmap->first = NULL;
	newHashmap->last = NULL;

	for (int i = 0; i < size; i++)
		newHashmap->data[i] = NULL;

	return newHashmap;
}

//Destroy hashmap
void hashmap_destroy(hashmap_t hashmap)
{
	//Free items
	for (int i = 0; i < hashmap->size; i++)
		if (hashmap->data[i] != NULL)
			free(hashmap->data[i]);

	//Free data struct
	free(hashmap->data);
	free(hashmap);
}

//Determine the next index to insert the item
int find_insert_index(hashmap_t hashmap, unsigned int key)
{
	int hash = key % hashmap->size;

	for (int i = 0; i < hashmap->size; i++)
		if (hashmap->data[(hash+i)%hashmap->size] == NULL || hashmap->data[(hash+i)%hashmap->size]->key == key)
			return (hash+i)%(hashmap->size);

	//unable to find
	return -1;
}

//Get item associated with a key in the hashmap
void* hashmap_get_item(hashmap_t hashmap, unsigned int key)
{
	int index = find_insert_index(hashmap, key);

	if (index != -1 && hashmap->data[index] != NULL)
		return hashmap->data[index]->value;

	//Item not found
	return NULL;
}

//Get key of hashmap item
unsigned int item_get_key(hashmap_item_t item)
{
	return item->key;
}

//Get value of hashmap item
void* item_get_value(hashmap_item_t item)
{
	return item->value;
}

//Insert key, value pair into hashmap
int hashmap_insert(hashmap_t hashmap, unsigned int key, void* value)
{
	int index;
	int oldSize;
	int i;
	hashmap_item_t item;
	hashmap_item_t* oldItems;

	if ((hashmap->items+1)/hashmap->size > LOAD_FACTOR)
	{
		if (hashmap->resizable == 0)
		{
			if (hashmap->items == hashmap->size)
				return -1;
		}
		else
		{
			oldSize = hashmap->size;
			oldItems = hashmap->data;

			hashmap->size *= 2;
			hashmap->data = (hashmap_item_t*) calloc(hashmap->size, sizeof(struct hashmap));
			hashmap->first = NULL;
			hashmap->last = NULL;

			for (i = 0; i < oldSize; i++)
			{
				if (oldItems[i] != NULL)
				{
					index = find_insert_index(hashmap, oldItems[i]-> key);
					hashmap->data[index] = oldItems[i];
				}
			}

			free(oldItems);
		}
	}

	index = find_insert_index(hashmap, key);

	if (index == -1)
		return -1;

	if (hashmap->data[index] == NULL)
		hashmap->items++;
	else
	{
		free(hashmap->data[index]->value);
		hashmap->data[index]->value = value;
		return 0;
	}

	//Make new hashmap
	item = (hashmap_item_t) malloc(sizeof(struct hashmap_item));
	item->prev = hashmap->last;
	item->next = NULL;
	item->key = key;

	if (hashmap->items == 1)
		hashmap->first = item;

	hashmap->last = item;

	return 0;
}

//Delete key from hashmap
int hashmap_delete(hashmap_t hashmap, unsigned int key)
{
	int index = find_insert_index(hashmap, key);
	int foundItem = -1;
	int indexSearch;

	if (index == -1 || hashmap->data[index] == NULL)
		return -1;

	if (hashmap->data[index] == hashmap->first)
		hashmap->first = hashmap->first->next;

	if (hashmap->data[index] == hashmap->last)
		hashmap->last = hashmap->last->prev;

	if (hashmap->data[index]->next != NULL)
		hashmap->data[index]->next->prev = hashmap->data[index]->prev;

	if (hashmap->data[index]->prev != NULL)
		hashmap->data[index]->prev->next = hashmap->data[index]->prev;

	free(hashmap->data[index]);
	hashmap->data[index] = NULL;

	while (foundItem != -1)
	{
		foundItem = -1;
		indexSearch = (index + 1) % hashmap->size;

		while (indexSearch != index && hashmap->data[indexSearch] != NULL)
		{
			if (hashmap->data[indexSearch]->key % hashmap->size <= index)
				foundItem = indexSearch;
			indexSearch = (indexSearch + 1) % hashmap->size;
		}

		if (foundItem != -1)
		{
			hashmap->data[index] = hashmap->data[foundItem];
			hashmap->data[foundItem] == NULL;
			//index = foundItem;
		}

		index = foundItem; //I think this goes here could go above
	}

	hashmap->items--;

	return 0;
}
