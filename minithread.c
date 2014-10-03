/*
 * minithread.c:
 *      This file provides a few function headers for the procedures that
 *      you are required to implement for the minithread assignment.
 *
 *      EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *      NAMING AND TYPING OF THESE PROCEDURES.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "interrupts.h"
#include "minithread.h"
#include "multilevel_queue.h"
#include "queue.h"
#include "synch.h"
#include <assert.h>
#include "alarm.h"

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueueed and dequeued, and any other state
 * that you feel they must have.
 */

sortedlist_t alarms;

int QUANTA = 100 * MILLISECOND;
int currentTime;
int quantaRemaining;

//Keeps track of the RUNNING Thread
static minithread_t runningThread;

//Used to set Thread ID
int threadId = 0;

//Queue of all READY Threads
static queue_t queue;

//Queue of all FINISHED Threads
static queue_t queue_finished_threads;


/* minithread functions */
struct minithread {
	proc_t proc;
	arg_t arg;
	int status;
	int id;
	stack_pointer_t *stack_base;
	stack_pointer_t *stack_top;
};

//HELPER FUNCTIONS
int minithread_cleanup(int *arg)
{
	queue_t old_threads = (queue_t) arg;
	minithread_t oldThread;
	while (queue_length(old_threads) > 0)
	{
		queue_dequeue(old_threads, (void **) &oldThread);
		//minithread_free_stack(*(oldThread->stack_base));
		//free(oldThread);	
	}
	queue_free(old_threads);
	return 0;
}

void
minithread_exit(minithread_t thread)
{
	minithread_t to_run;
	minithread_t old_thread;
	queue_t old_threads;
	queue_append(queue_finished_threads, thread);
	if (queue_length(queue_finished_threads) > 10)
	{
		old_threads = queue_finished_threads;
		queue_finished_threads = queue_new();
		minithread_fork(minithread_cleanup, (int *) old_threads);
	}
	queue_dequeue(queue, (void **) &to_run);
	if (to_run == NULL)
	{
		while (queue_length(queue) == 0);
		queue_dequeue(queue, (void **) &to_run);
	}
	old_thread = runningThread;
	runningThread = to_run;
	minithread_switch(old_thread->stack_top, to_run->stack_top);
}

//REQUIRED FUNCTIONS
minithread_t
minithread_fork(proc_t proc, arg_t arg) {
	//Make newMinithread
	minithread_t newMinithread = (minithread_t) minithread_create(proc, arg);
	//Start newMinithread
	queue_append(queue, newMinithread);
	//Return newMinithread
	return (minithread_t) newMinithread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
	//Variable Initization
	minithread_t newMinithread = (minithread_t) malloc(sizeof(struct minithread));
	stack_pointer_t* stack_top = (stack_pointer_t*) malloc(sizeof(stack_pointer_t));
	stack_pointer_t* stack_base = (stack_pointer_t*) malloc(sizeof(stack_pointer_t));

	*stack_base = NULL;

	minithread_allocate_stack(stack_base, stack_top);

	//Make sure the stack allocation properly set stack_base
	if (*stack_base == NULL)
		return NULL;

	//Initialize the Minithread
	newMinithread->proc = proc;
	newMinithread->arg = arg;
	//newMinithread->status = WAITING;
	newMinithread->id = threadId++;
	newMinithread->stack_base = stack_base;
	newMinithread->stack_top = stack_top;

	minithread_initialize_stack(stack_top, proc, arg, (proc_t) minithread_exit, (arg_t) newMinithread);

	//Return minithread
    return (minithread_t) newMinithread;
}

minithread_t
minithread_self() {
    return (minithread_t) runningThread;
}

int
minithread_id() {
    return (int) runningThread->id;
}

void
minithread_stop() {
	//Get next thread to be run
	minithread_t oldThread = runningThread;
	minithread_t to_run;
	set_interrupt_level(DISABLED);

	queue_dequeue(queue, (void **) &to_run);

	if (to_run == NULL)
	{
		set_interrupt_level(ENABLED);
		while (queue_length(queue) == 0);
		set_interrupt_level(DISABLED);
		queue_dequeue(queue, (void **) &to_run);
	}
	//Switch to the next thread to be "run"
	//Set Status' of Threads
	//oldThread->status = RUNABLE;
	runningThread = to_run;

	//runningThread->status = RUNNING;

	//Set running thread as currently running thread
	minithread_switch(oldThread->stack_top, runningThread->stack_top);
}

void
minithread_start(minithread_t t) {
	interrupt_level_t old_level = set_interrupt_level(DISABLED);
	queue_append(queue, t);
	set_interrupt_level(old_level);
}

void
minithread_yield() {
	//Variable Initization
	minithread_t to_run;
	minithread_t old_thread;
	set_interrupt_level(DISABLED);
	//Retrieve to_run
	queue_dequeue(queue, (void **) &to_run);
	
	//Ensure to_run was retreived properly
	if (to_run == NULL)
	{
		set_interrupt_level(ENABLED);
		while (queue_length(queue) == 0);
		set_interrupt_level(DISABLED);
		queue_dequeue(queue, (void **) &to_run);
	}
	
	//Store last running thread
	old_thread = runningThread;
	//Store to_run thread as runningThread
	runningThread = to_run;

	//Add old thread to the end
	queue_append(queue, (void *) old_thread);
	
	//Switch to next thread (to_run) from old thread (runningThread)
	minithread_switch(old_thread->stack_top, to_run->stack_top);
}

int placeholder(int *arg) {
	return 0;	
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void 
clock_handler(void* arg)
{
	minithread_t to_run;
	minithread_t old_thread;
	set_interrupt_level(DISABLED);
	currentTime++;
	quantaRemaining--;
	callAlarms(alarms, currentTime);

	if (quantaRemaining <= 0)
	{
		quantaRemaining = 1;

		//Variable Initization
		set_interrupt_level(DISABLED);
		//Retrieve to_run
		queue_dequeue(queue, (void **) &to_run);

		//Ensure to_run was retreived properly
		if (to_run == NULL)
		{
			return;
		}

		//Store last running thread
		old_thread = runningThread;
		//Store to_run thread as runningThread
		runningThread = to_run;

		//Add old thread to the end
		queue_append(queue, (void *) old_thread);

		//Switch to next thread (to_run) from old thread (runningThread)
		minithread_switch(old_thread->stack_top, to_run->stack_top);
	}
}

void
minithread_sleep_alarm_wakeup(void *arg)
{
	semaphore_t sleepSem = (semaphore_t) arg;
	semaphore_V(sleepSem);
}

/*
* sleep with timeout in milliseconds
*/
void 
minithread_sleep_with_timeout(int delay)
{
	//Sleep semaphore
	semaphore_t sleepSemaphore = semaphore_create();

	//Holder for last interrupt level
	interrupt_level_t previousLevel;

	//Initialize sleep semaphore to a value of 0
	semaphore_initialize(sleepSemaphore, 0);

	//Disable interrupts
	//Interrupts are disabled so that if we context switch away the alarm will
	//not be triggered before semaphore_P is called and will not hang forever
	previousLevel = set_interrupt_level(DISABLED);

	//Register the alarm
	register_alarm(delay, &minithread_sleep_alarm_wakeup, sleepSemaphore); 

	//Wait on sleep semaphore. 
 	//semaphore_P calls minithread_yield, which re-enables interrupts
	semaphore_P(sleepSemaphore);

	//Free the sleep semaphore
	semaphore_destroy(sleepSemaphore);

	//Restore the previous interrupt level 
	set_interrupt_level(previousLevel);
}

/*
 * Initialization.
 *
 *      minithread_system_initialize:
 *       This procedure should be called from your C main procedure
 *       to turn a single threaded UNIX process into a multithreaded
 *       program.
 *
 *       Initialize any private data structures.
 *       Create the idle thread.
 *       Fork the thread which should call mainproc(mainarg)
 *       Start scheduling.
 *
 */
void
minithread_system_initialize(proc_t mainproc, arg_t mainarg) {
	minithread_t main_thread;
	minithread_t idleThread;
	alarms = new_sortedlist();	
	currentTime = 0;
	quantaRemaining = 0;
	queue = queue_new();
	queue_finished_threads = queue_new();
	idleThread = minithread_create(placeholder, (arg_t) NULL);
	queue_append(queue_finished_threads, idleThread);
	main_thread = minithread_create(mainproc, mainarg);
	runningThread = main_thread;
	minithread_clock_init(MILLISECOND, clock_handler);
	minithread_switch(idleThread->stack_top, main_thread->stack_top);	
}
