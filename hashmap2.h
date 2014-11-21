typedef void *any_t;

/*
 *  * map_t is a pointer to an internally maintained data structure.
 *   * Clients of this package do not need to know how hashmaps are
 *    * represented.  They see and manipulate only map_t's. 
 *     */
typedef any_t map_t;

/*
 *  * Return an empty hashmap. Returns NULL if empty.
 *  */
extern map_t hashmap_new();


/*
 *  * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 *   */
extern int hashmap_put(map_t in, int key, any_t value);

/*
 *  * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 *   */
extern int hashmap_get(map_t in, int key, any_t *arg);

/*
 *  * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 *   */
extern int hashmap_remove(map_t in, int key);

/*
 *  * Get any element. Return MAP_OK or MAP_MISSING.
 *   * remove - should the element be removed from the hashmap
 *    */
extern int hashmap_get_one(map_t in, any_t *arg, int remove);

/*
 *  * Free the hashmap
 *   */
extern void hashmap_free(map_t in);

/*
 *  * Get the current size of a hashmap
 *   */
extern int hashmap_length(map_t in);
