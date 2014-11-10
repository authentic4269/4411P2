#include <stdio.h>
#include <stdlib.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm, void *arg)
{
    double d;
	//Holder for last interrupt level
	interrupt_level_t previousLevel;
    listnode_t node;

	//Disable interrupts
	previousLevel = set_interrupt_level(DISABLED);

	//Check to make sure alarm != NULL
	if (alarm == NULL)
		return NULL;

	//If alarm supposed to go off NOW	
	if(delay == 0) {
		alarm(arg);
		return NULL;
	}
    node = (listnode_t) malloc(sizeof(listnode));        
    node->time = currentTime + ((delay * MILLISECOND) / QUANTA);
    d = (double) ((long) delay * (long) MILLISECOND);
    node->time = (int) (d / (double) QUANTA);
     
    node->func = alarm;
    node->arg = arg;
    node->id = (void *) node;
    insert(alarms, node);

	//Restore the previous interrupt level 
	set_interrupt_level(previousLevel);

    return (alarm_id) node;
}

void callAlarms(sortedlist_t list, int current) 
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
}
	
/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
    int ret;
    interrupt_level_t previousLevel;
    previousLevel = set_interrupt_level(DISABLED);
    ret = !(remove_node(alarms, alarm));
    set_interrupt_level(previousLevel);
    return ret;
}

/*
** vim: ts=4 sw=4 et cindent
*/
