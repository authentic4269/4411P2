/*
 * Generic queue manipulation functions
 */
#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

/*
 * queue_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how queues are
 * represented.  They see and manipulate only queue_t's.
 */
typedef struct linkedList* linkedList_t;
typedef struct linkedListNode* linkedListNode_t;

/*
 * Return an empty queue.  Returns NULL on error.
 */
extern linkedList_t linkedList_new();

/*
 * Appends a void* to a linkedlist (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
extern int linkedList_insert(linkedList_t, void*);

/*
 * Dequeue and return the first void* from the queue.
 * Return 0 (success) and first item if queue is nonempty, or -1 (failure) and
 * NULL if queue is empty.
 */
extern int linkedList_dequeue(linkedList_t, void**);

/*
 * Free the linkedlist and return 0 (success) or -1 (failure).
 */
extern int linkedList_free (linkedList_t linkedlist);

/*
 * Check if the linkedlist is empty. Return 1 (empty) 0 (not empty)
 */
extern int linkedList_isEmpty(linkedList_t linkedlist);

/*
 * Return the number of items in the linkedlist, or -1 if an error occured
 */
extern int linkedList_length(linkedList_t linkedlist);

/*
 * Delete the first instance of the specified item from the given linkedlist.
 * Returns 0 if an element was deleted, or -1 otherwise.
 */
extern int linkedList_delete(linkedList_t linkedlist, void* item);

#endif /*__LINKEDLIST_H__*/
