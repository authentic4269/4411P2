#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"


void print_item(hashmap_item_t item);
void test_hashmap_resize();
void print_map(hashmap_t hashmap);
void print_item(hashmap_t hashmap);

void test_hashmap()
{
	hashmap_t hashmap = hashmap_new(10, 0);

	int* val1 = malloc(sizeof(int));
	int* val2 = malloc(sizeof(int));
	int* val3 = malloc(sizeof(int));
	int* val4 = malloc(sizeof(int));
	int* val5 = malloc(sizeof(int));
	int* val6 = malloc(sizeof(int));

	*val1 = 5;
	*val2 = 5;
	*val3 = 5;
	*val4 = 5;
	*val5 = 5;
	*val6 = 5;

	hashmap_insert(hashmap, 1, val1);
	hashmap_insert(hashmap, 6, val2);
	hashmap_insert(hashmap, 3, val3);
	hashmap_insert(hashmap, 5, val4);
	hashmap_insert(hashmap, 4, val5);
	hashmap_insert(hashmap, 2, val6);

	print_list(hashmap);
	print_map(hashmap);

	hashmap_delete(hashmap, 3);
	printf("-> Deleted 3 \n");
	print_list(hashmap);
	print_map(hashmap);

	hashmap_delete(hashmap, 2);
	printf("-> Deleted 2 \n");
	print_list(hashmap);
	print_map(hashmap);

	hashmap_delete(hashmap, 1);
	printf("-> Deleted 1 \n");
	print_list(hashmap);
	print_map(hashmap);

	hashmap_delete(hashmap, 4);
	printf("-> Deleted 4 \n");
	print_list(hashmap);
	print_map(hashmap);

	hashmap_delete(hashmap, 5);
	printf("-> Deleted 5 \n");
	print_list(hashmap);
	print_map(hashmap);

	hashmap_destroy(hashmap);
}

void test_hashmap_resize()
{
	hashmap_t hashmap = hashmap_create(5, 1);

	int* val1 = malloc(sizeof(int));
	int* val2 = malloc(sizeof(int));
	int* val3 = malloc(sizeof(int));
	int* val4 = malloc(sizeof(int));
	int* val5 = malloc(sizeof(int));
	int* val6 = malloc(sizeof(int));

	*val1 = 5;
	*val2 = 5;
	*val3 = 5;
	*val4 = 5;
	*val5 = 5;
	*val6 = 5;

	hashmap_insert(hashmap, 1, val1);
	hashmap_insert(hashmap, 2, val2);
	hashmap_insert(hashmap, 3, val3);
	hashmap_insert(hashmap, 7, val4);
	hashmap_insert(hashmap, 8, val5);

	print_map(hashmap);

	hashmap_insert(hashmap, 10, val6);

	print_map(hashmap);
}


void print_item(hashmap_item_t item)
{
	printf("\n");
	printf("Item pointer\t%p\n", item);
	printf("Item Key\t%d\n", item->key);
	printf("Item Value\t%p\n", item->value);
	printf("Time Added\t%d\n", item->time_added);
	printf("Previous Item\t%p\n", item->prev);
	printf("Next Item\t%p\n", item->next);
}

void print_map(hashmap_t hashmap)
{
	int i;

	hashmap_item_t first = hashmap->first;
	hashmap_item_t last = hashmap->last;
	int size = hashmap->size;
	int items = hashmap->items;

	printf("\n");
	printf("Hashmap address\t\t%p\n", hashmap);
	printf("Hashmap first item\t%p\n", first);
	printf("Hashmap last item\t%p\n", last);
	printf("Hashmap Size\t\t%d\n", size);
	printf("Number of Items\t\t%d\n", items);
	printf("\n");

	printf("Item Listing\n");
	for (i = 0; i < hashmap->size; i++)
	{
		if (hashmap->data[i] == NULL)
		{
			printf("  [%d]\t%p\n", i, hashmap->data[i]);
		}
		else
		{
			printf("  [%d]\t%d (%d)\n", i, hashmap->data[i]->key, hashmap->data[i]->key % hashmap->size);
		}
	}
}

void print_list(hashmap_t hashmap)
{
	hashmap_item_t item = hashmap_get_first(hashmap);
	if (item == NULL)
		return;

	printf("%d", item_get_key(item));
	item = hashmap_get_next(item);

	while (item != NULL)
	{
		printf(" -> %d", item_get_key(item));
		item = hashmap_delete(hashmap, hashmap_get_first(hashmap));
	}

	printf("\n");
}

int main()
{
	setbuf(stdout, NULL);
	test_hashmap();
	test_hashmap_resize();
	return 0;
}
