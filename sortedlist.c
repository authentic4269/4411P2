#include "sortedlist.h"
#include <stdlib.h>
#include <stdio.h>

sortedlist_t new_sortedlist() 
{
	sortedlist_t newlist = (sortedlist_t) malloc(sizeof(sortedlist));
	newlist->length = 0;
	newlist->head = NULL;
	return newlist;
}

int remove_node(sortedlist_t list, void *removeid)
{
	listnode_t cur = list->head;
	while (cur != NULL)
	{
		if (cur->id == removeid)
		{
			if (cur->prev != NULL)
				cur->prev->next = cur->next;
			else //removing the root
				list->head = cur->next;
			if (cur->next != NULL)
				cur->next->prev = cur->prev;
			list->length--;
			free(cur);	
			return 1;

		} 
		cur = cur->next;
	}
	printf("%d\n", list->length);
	return 0;
}

void insert(sortedlist_t list, listnode_t newnode)
{
	listnode_t cur;
	if (list->length == 0)
	{
		list->head = newnode;
		list->length = 1;
		newnode->next = NULL;
		newnode->prev = NULL;
	}
	else
	{
		cur = list->head;
		while (cur->time < newnode->time)
		{
			if (cur->next == NULL)
			{
				cur->next = newnode;
				newnode->prev = cur;
				break;
			}
			else
			{
				cur = cur->next;
			}
		}
		if (newnode->time < cur->time)
		{
			newnode->next = cur;
			newnode->prev = cur->prev;
			//inserting at root if cur->prev == null
			if (newnode->prev != NULL)
				newnode->prev->next = newnode;
			else
				list->head = newnode;
			cur->prev = newnode;
		}
		list->length++;	
	}	
}

/*void callAlarms(sortedlist_t list, int current) 
{
	listnode_t cur = list->head;
	listnode_t tmp;
	while (cur != NULL && cur->time < current)
	{
		cur->func(cur->arg);
		tmp = cur;
		cur = cur->next;
		remove_node(list, tmp->id);
	}
}*/
