

struct listnode
{
	void *id;	
	int time;	
	void (*func)(void *);
	void *arg;
	struct listnode *next;
	struct listnode *prev;
};

struct sortedlist
{
	struct listnode *head;
	int length;
};

typedef struct listnode listnode;
typedef struct sortedlist sortedlist;

typedef listnode *listnode_t;
typedef sortedlist *sortedlist_t;

extern sortedlist_t new_sortedlist();

// returns 0 if not found, 1 if found
extern int remove_node(sortedlist_t list, void *removeid); 

extern void insert(sortedlist_t list, listnode_t newnode);

extern void callAlarms(sortedlist_t list, int curtime); 
