#ifndef __HASHMAP_H__
#define __HASHMAP_H__
#include "linkedlist.h"
typedef struct hashmap* hashmap_t;


struct hashmap {
	  int table_size;
	  int size;
	  linkedList_t *data;
};

// Return an empty hashmap. Returns NULL if empty.
hashmap_t hashmap_new();

//Add an element to the hashmap. Return 0 or -1.
int hashmap_insert(hashmap_t hashmap, int key, void *data);

//Get an element from the hashmap. Return 0 or -1.
int hashmap_get(hashmap_t hashmap, int key, void **data);

//Remove an element from the hashmap. Return 0 or -1.
int hashmap_delete(hashmap_t hashmap, int key);

//Get any element. Return 0 or -1. remove - should the element be removed from the hashmap
//int hashmap_get_one(hashmap_t hashmap, hashmap_item_t *arg, int remove);


//Free hashmap
void hashmap_destroy(hashmap_t hashmap);

//Get current size of a hashmap
int hashmap_length(hashmap_t hashmap);

#endif /*__HASHMAP_H_*/
