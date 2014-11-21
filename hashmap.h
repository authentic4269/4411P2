#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stdio.h>
#include <string.h>

typedef struct hashmap_item* hashmap_item_t;
typedef struct hashmap* hashmap_t;

struct hashmap_item {
	unsigned int key;
	void* value;
	hashmap_item_t prev;
	hashmap_item_t next;
};

struct hashmap {
	int size;
	int items;
	int resizable;
	hashmap_item_t* data;
	hashmap_item_t first;
	hashmap_item_t last;
};

//Create hashmap with space for int size items
hashmap_t hashmap_new(int size, int resizable);

//Destroy hashmap
void hashmap_destroy(hashmap_t map);

//Get key of hashmap item
unsigned int item_get_key(hashmap_item_t item);

//Get value of hashmap item
void* item_get_value(hashmap_item_t item);

//Insert key, value pair into hashmap
int hashmap_insert(hashmap_t hashmap, unsigned int key, void* value);

void* hashmap_get(hashmap_t hashmap, unsigned int key);

//Delete key from hashmap. Return -1 on error
int hashmap_delete(hashmap_t hashmap, unsigned int key);

#endif /*__HASHMAP_H_*/
