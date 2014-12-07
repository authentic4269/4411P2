#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "interrupts.h"
#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 *      You must implement the procedures and types defined in this interface.
 */

/*
 * Semaphores.
 */
struct semaphore {
	int count; //maximum number of clients
	queue_t waitQueue; //clients waiting
};


/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t newSemaphore = (semaphore_t) malloc(sizeof(struct semaphore));

	if (newSemaphore == NULL)
		return NULL;

	newSemaphore->waitQueue = queue_new();

    return (semaphore_t) newSemaphore;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	queue_free(sem->waitQueue); //Free waitQueue
	free(sem); //Free semaphore itself
}

/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	sem->count = cnt;
}

/*
 * semaphore_P(semaphore_t sem)
 *      P on the sempahore.
 */
void semaphore_P(semaphore_t sem) {
	//Holder for last interrupt level
	interrupt_level_t previousLevel;

	//Disable interrupts
	//Interrupts are disabled so that if we context switch away the alarm will
	//not be triggered before semaphore_P is called and will not hang forever
	previousLevel = set_interrupt_level(DISABLED);

	if(--(sem->count) < 0)
	{
		queue_append(sem->waitQueue, (void*) minithread_self());
		minithread_stop();
	}

	//Restore the previous interrupt level 
	set_interrupt_level(previousLevel);
}

/*
 * semaphore_V(semaphore_t sem)
 *      V on the sempahore.
 */
void semaphore_V(semaphore_t sem) {
	//Holder for last interrupt level
	interrupt_level_t previousLevel;

	//Disable interrupts
	//Interrupts are disabled so that if we context switch away the alarm will
	//not be triggered before semaphore_P is called and will not hang forever
	previousLevel = set_interrupt_level(DISABLED);

	if(++(sem->count) <= 0)
	{
		minithread_t minithread; //Minithread obtained in next line from dequeue

		queue_dequeue(sem->waitQueue, (void**) &minithread);

		minithread_start(minithread);
	}

	//Restore the previous interrupt level 
	set_interrupt_level(previousLevel);
}
