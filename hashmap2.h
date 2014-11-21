#ifndef __HASHMAP_H__
#define __HASHMAP_H__

typedef struct hashmap_item* hashmap_item_t;
typedef struct hashmap* hashmap_t;

typedef struct hashmap_item{
	int key;
	int in_use;
	hashmap_item_t data;
};

typedef struct hashmap{
	  int table_size;
	  int size;
	  hashmap_element *data;
};

// Return an empty hashmap. Returns NULL if empty.
hashmap_t hashmap_new();

//Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
int hashmap_put(hashmap_t in, int key, hashmap_item_t value);

//Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
int hashmap_get(hashmap_t in, int key, hashmap_item_t *arg);

//Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
int hashmap_remove(hashmap_t in, int key);

//Get any element. Return MAP_OK or MAP_MISSING. remove - should the element be removed from the hashmap
int hashmap_get_one(hashmap_t in, hashmap_item_t *arg, int remove);

//Free hashmap
void hashmap_destroy(hashmap_t in);

//Get current size of a hashmap
int hashmap_length(hashmap_t in);

#endif /*__HASHMAP_H_*/
