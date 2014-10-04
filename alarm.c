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
     
    printf("currentTime: %d, node->time: %d, delay: %f, MILLISECOND: %d, QUANTA: %d\n", currentTime, node->time, d, MILLISECOND, QUANTA);
    node->func = alarm;
    node->arg = arg;
    node->id = (void *) node;
    insert(alarms, node);

	//Restore the previous interrupt level 
	set_interrupt_level(previousLevel);

    return (alarm_id) node;
}
	
/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
    return !(remove_node(alarms, alarm));
}

/*
** vim: ts=4 sw=4 et cindent
*/
