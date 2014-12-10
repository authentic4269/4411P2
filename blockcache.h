
/*
 * queue_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how queues are
 * represented.  They see and manipulate only queue_t's.
 */
typedef struct blockcache* blockcache_t;
typedef struct blockcacheNode* blockcacheNode_t;

/*
 * Return an empty queue.  Returns NULL on error.
 */
extern blockcache_t blockcache_new();

/*
 * Appends a void* to a blockcache (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
extern int blockcache_insert(blockcache_t list, int key, void* data);

extern int blockcache_get(blockcache_t list, int key, void **item);

/*
 * Dequeue and return the first void* from the queue.
 * Return 0 (success) and first item if queue is nonempty, or -1 (failure) and
 * NULL if queue is empty.
 */
extern int blockcache_dequeue(blockcache_t list, void** item);

/*
 * Free the blockcache and return 0 (success) or -1 (failure).
 */
extern int blockcache_free (blockcache_t blockcache);

/*
 * Check if the blockcache is empty. Return 1 (empty) 0 (not empty)
 */
extern int blockcache_isEmpty(blockcache_t blockcache);

/*
 * Return the number of items in the blockcache, or -1 if an error occured
 */
extern int blockcache_length(blockcache_t blockcache);

/*
 * Removes the item at the end of the list. Used to maintain size invariant. 
 */
extern void blockcache_delete_last(blockcache_t blockcache);

/*
 * Delete the first instance of the specified item from the given blockcache.
 * Returns 0 if an element was deleted, or -1 otherwise.
 */
extern int blockcache_delete(blockcache_t blockcache, int key);

