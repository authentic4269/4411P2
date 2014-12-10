/*
 * minithread.c:
 *      This file provides a few function headers for the procedures that
 *      you are required to implement for the minithread assignment.
 *
 *      EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *      NAMING AND TYPING OF THESE PROCEDURES.
 *
 */
#include "minifile.h"
#include <stdlib.h>
#include <stdio.h>
#include "interrupts.h"
#include "network.h"
#include "minimsg.h"
#include "miniroute.h"
#include "miniheader.h"
#include "minithread.h"
#include "minisocket.h"
#include "multilevel_queue.h"
#include "queue.h"
#include "synch.h"
#include <assert.h>
#include "alarm.h"
#include "read.h"
#define USER_DEBUG 1

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueueed and dequeued, and any other state
 * that you feel they must have.
 */

sortedlist_t alarms;

int QUANTA = 10 * MILLISECOND;
int currentTime;
int quantaRemaining;
int quantaAssignments[4] = {1, 2, 4, 8};

//Keeps track of the RUNNING Thread
minithread_t runningThread;

//Used to set Thread ID
int threadId = 0;

//Used to set id of outgoing ROUTING_DISCOVERY/REPLY packets

//Queue of all READY Threads
static multilevel_queue_t multiqueue;

//Queue of all FINISHED Threads
static queue_t queue_finished_threads;

//Cleanup Semaphore
static semaphore_t cleanupSemaphore;


/*
//HELPER FUNCTIONS
int minithread_cleanup(arg_t arg)
{
	minithread_t newMinithread;
	interrupt_level_t previousLevel;

	while (1)
	{
		//Editing the cleanup queue, so disable interrupts
		previousLevel = set_interrupt_level(DISABLED);

		//Wait to be woken up
		semaphore_P(cleanupSemaphore);

		//Only cleanup when there are threads to cleanup
		while (queue_length(queue_finished_threads) > 0)
		{
			//Get the next newMinithread for the thread to free up
			queue_dequeue(queue_finished_threads, (void**) &newMinithread);

			//Free the stack
			minithread_free_stack(*(newMinithread->stack_base));

			//Free the newMinithread
			free(newMinithread);
		}

		//Restore interrupt level
		set_interrupt_level(previousLevel);
	}

	return 0;
}*/

int minithread_cleanup(arg_t arg)
{
	queue_t oldThreads = (queue_t) arg;
	minithread_t oldThread;
	while (queue_length(oldThreads) > 0)
	{
		queue_dequeue(oldThreads, (void **) &oldThread);
		minithread_free_stack(*(oldThread->stack_base));
		free(oldThread);
	}
	queue_free(oldThreads);
	return 0;
}


void
minithread_exit(minithread_t thread)
{
	minithread_t to_run;
	minithread_t old_thread;
	queue_t oldThreads;
	int start = get_priority_of_thread();
	queue_append(queue_finished_threads, thread);
	if (queue_length(queue_finished_threads) > 10)
	{
		oldThreads = queue_finished_threads;
		queue_finished_threads = queue_new();
		minithread_fork(minithread_cleanup, (int *) oldThreads);
	}
	multilevel_queue_dequeue(multiqueue, start, (void **) &to_run);
	if (to_run == NULL)
	{
		while (multilevel_queue_fulllength(multiqueue) == 0);
		multilevel_queue_dequeue(multiqueue, 0, (void **) &to_run);
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
	multilevel_queue_enqueue(multiqueue, 0, newMinithread);
	//Return newMinithread
	return (minithread_t) newMinithread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
	minithread_t newMinithread = (minithread_t) malloc(sizeof(struct minithread));
	stack_pointer_t* stack_top = (stack_pointer_t*) malloc(sizeof(stack_pointer_t));
	stack_pointer_t* stack_base = (stack_pointer_t*) malloc(sizeof(stack_pointer_t));

	*stack_base = NULL;

	minithread_allocate_stack(stack_base, stack_top);

	if (*stack_base == NULL)
		return NULL;

	newMinithread->proc = proc;
	newMinithread->arg = arg;
	newMinithread->level = 0;
	//newMinithread->status = WAITING;
	newMinithread->id = threadId++;
	newMinithread->stack_base = stack_base;
	newMinithread->stack_top = stack_top;
	newMinithread->currentDirectoryInode = 0;

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
	int start = get_priority_of_thread();
	set_interrupt_level(DISABLED);
	

	multilevel_queue_dequeue(multiqueue, start, (void **) &to_run);

	if (to_run == NULL)
	{
		set_interrupt_level(ENABLED);
		while (multilevel_queue_fulllength(multiqueue) == 0);
		set_interrupt_level(DISABLED);
		multilevel_queue_dequeue(multiqueue, start, (void **) &to_run);
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
	multilevel_queue_enqueue(multiqueue, 0, t);
	set_interrupt_level(old_level);
}

void
minithread_yield() {
	//Variable Initization
	minithread_t to_run;
	minithread_t old_thread;
	int start = get_priority_of_thread();
	set_interrupt_level(DISABLED);
	//Retrieve to_run
	multilevel_queue_dequeue(multiqueue, start, (void **) &to_run);
	
	//Ensure to_run was retreived properly
	if (to_run == NULL)
	{
		return;
	}
	
	old_thread = runningThread;
	runningThread = to_run;

	multilevel_queue_enqueue(multiqueue, old_thread->level, (void *) old_thread);
	
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
	int start = get_priority_of_thread();
	set_interrupt_level(DISABLED);
	currentTime++;
	quantaRemaining--;
	callAlarms(alarms, currentTime);

	if (quantaRemaining <= 0)
	{
		multilevel_queue_dequeue(multiqueue, start, (void **) &to_run);
		if (to_run == NULL)
		{
			return;
		}

		old_thread = runningThread;
		runningThread = to_run;
		if (old_thread->level < 3)
			old_thread->level++;
		quantaRemaining = quantaAssignments[to_run->level];

		multilevel_queue_enqueue(multiqueue, old_thread->level, (void *) old_thread);

		minithread_switch(old_thread->stack_top, to_run->stack_top);
	}
	set_interrupt_level(ENABLED);
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
	semaphore_t sleepSemaphore = semaphore_create();

	interrupt_level_t previousLevel;

	semaphore_initialize(sleepSemaphore, 0);

	//Interrupts are disabled so that if we context switch away the alarm will
	//not be triggered before semaphore_P is called and will not hang forever
	previousLevel = set_interrupt_level(DISABLED);
	printf("delay: %d\n", delay);

	register_alarm(delay, &minithread_sleep_alarm_wakeup, sleepSemaphore); 

	semaphore_P(sleepSemaphore);

	semaphore_destroy(sleepSemaphore);

	set_interrupt_level(previousLevel);
}


void handle_data_packet(network_interrupt_arg_t *arg) {
	minisocket_t incomingSocket;
	mini_header_reliable_t reliable_header;
	miniport_t incomingPort;
	unsigned int ack;
	unsigned int seq;
	mini_header_t header = (mini_header_t) arg->buffer; 
	if (header->protocol == PROTOCOL_MINIDATAGRAM)
	{
		incomingPort = miniport_get_unbound(unpack_unsigned_short(header->destination_port));
		if (incomingPort == NULL)
		{
			// Nobody was waiting to receive the packet
			printf("packet dropped like it's hot\n");
			return;
		}
  		queue_append(incomingPort->port_data.unbound.data_queue, (void *) arg);
		semaphore_V(incomingPort->port_data.unbound.data_available);
	}
	else 
	{
		reliable_header = (mini_header_reliable_t) arg->buffer;
		printf("dst port: %d, type: %d\n", unpack_unsigned_short(reliable_header->destination_port), reliable_header->message_type);
		incomingSocket = minisocket_get(unpack_unsigned_short(reliable_header->destination_port));
		if (incomingSocket == NULL)
		{
			printf("reliable packed dropped like it's hot\n");
			return;
		}
		semaphore_P(incomingSocket->mutex);
		ack = unpack_unsigned_int(reliable_header->ack_number);
		seq = unpack_unsigned_int(reliable_header->seq_number);
		if ((incomingSocket->waiting == TCP_PORT_WAITING_ACK && reliable_header->message_type == MSG_ACK) || (
			incomingSocket->waiting == TCP_PORT_WAITING_SYNACK && reliable_header->message_type == MSG_SYNACK))
		{
			if (ack <= incomingSocket->seq_number) {
				incomingSocket->waiting = TCP_PORT_WAITING_NONE;
				if (reliable_header->message_type == MSG_ACK)
					printf("received ack packet, local seq=%d, packet ack=%d\n", incomingSocket->seq_number, ack);
				if (reliable_header->message_type == MSG_SYNACK)
					printf("received synack packet, local seq=%d, packet ack=%d\n", incomingSocket->seq_number, ack);
				semaphore_V(incomingSocket->mutex);
				semaphore_V(incomingSocket->wait_for_ack_semaphore);
			}
			else {
				printf("reliable ack packet dropped like it's hot: duplicate, local seq=%d, packet ack=%d\n", 
					incomingSocket->seq_number, ack);
				semaphore_V(incomingSocket->mutex);
				return;
			}
		}
		
		else 
		{
			if (seq != incomingSocket->ack_number)
			{
				printf("reliable data packet dropped like it's hot: duplicate, local ack=%d, packet seq=%d\n", 
					incomingSocket->ack_number, seq);
				semaphore_V(incomingSocket->mutex);
				return;
			}
			printf("received data packet, local ack=%d, packet seq=%d\n", incomingSocket->ack_number, seq);
			incomingSocket->ack_number++;
			queue_append(incomingSocket->waiting_packets, arg);
			semaphore_V(incomingSocket->packet_ready);
		}
		semaphore_V(incomingSocket->mutex);
	}

}

char* reverse_path(char (*path)[8], int len1) {
	int i, j;
	char *ret = (char *) malloc(sizeof(char) * len1 * 8);
	if (ret == NULL)
		return NULL;
	for (i = 0; i < len1; i++)
	{
		for (j = 0; j < 8; j++)
		{
			ret[(len1-i-1)*8 + j] = path[i][j];
		}
	}
	return ret;
}

void network_handler(network_interrupt_arg_t *arg) {
	routing_header_t header = (routing_header_t) arg->buffer;	
	network_address_t my_addr;
	network_address_t dst;
	network_address_t cur;
	network_address_t src;
	network_address_t *unpacked_path;
	char temporary_buffer[MAX_NETWORK_PKT_SIZE];
	char *reversed_path;
	int i;
	short success = 0;
	unsigned int pathLen;
	network_get_my_address(my_addr);
	pathLen = unpack_unsigned_int(header->path_len);
	unpack_address(header->destination, dst);
	unpack_address(header->path[0], src);
	if (header->routing_packet_type == ROUTING_DATA)
	{
		if (network_compare_network_addresses(my_addr, dst) != 0) 
		{
			// before passing to handle_data_packet, need to chop off the routing header
			arg->size = arg->size - sizeof(struct routing_header);
			memcpy((void *) temporary_buffer, (void *) arg->buffer + sizeof(struct routing_header), arg->size);
			memcpy((void *) arg->buffer, temporary_buffer, arg->size);
			handle_data_packet(arg);
		}
		else
		{
			unpack_address(header->path[0], cur);
			if (network_compare_network_addresses(my_addr, cur) != 0)
			{
				if (USER_DEBUG) {
					printf("error: got own data packet! dropping it\n");
				}	
				return;
			}
			for (i = 1; i < pathLen; i++)
			{
				unpack_address(header->path[i], cur);
				if (network_compare_network_addresses(my_addr, cur) != 0)
				{
					unpack_address(header->path[i+1], cur);
					success = 1;
					break;
				}
			} 			
			if (success) 
			{
				i = unpack_unsigned_int(header->ttl);
				if (i == 0)
				{
					printf("ttl 0, dropping packet");
					return;
				}
				pack_unsigned_int(header->ttl, --i);
				if (USER_DEBUG) 
					printf("forwarding data packet\n");
				network_send_pkt(cur, sizeof(struct routing_header), (char *) header, arg->size-sizeof(struct routing_header), arg->buffer+ sizeof(struct routing_header));
			}
			else
			{
				printf("failed to forward data packet\n");
			}
		}
	}
	else if (header->routing_packet_type == ROUTING_ROUTE_DISCOVERY) 
	{
		if (network_compare_network_addresses(my_addr, dst) != 0) 
		{
			memcpy(header->destination, header->path[0], 8);
			pack_address(header->path[pathLen++], my_addr);
			reversed_path = reverse_path(header->path, pathLen);
			// This is a little hacky, miniroute_cache unpacks the path anyway so its convenient to not write unpack_path as a separate method
			unpacked_path = miniroute_cache(reversed_path, MAX_ROUTE_LENGTH, src);
			for(i = 0; i < pathLen; i++)
				pack_address(header->path[i], unpacked_path[i]);
			pack_unsigned_int(header->path_len, (1 + pathLen)); 
			pack_unsigned_int(header->ttl, MAX_ROUTE_LENGTH);
			header->routing_packet_type = ROUTING_ROUTE_REPLY;
			if (USER_DEBUG) printf("Discovered by remote host!\n");
			network_send_pkt(arg->sender, sizeof(struct routing_header), (char *) header, arg->size - sizeof(struct routing_header), 
				(char *) arg->buffer + sizeof(struct routing_header)); 
		}
		else if (network_compare_network_addresses(my_addr, src) != 0)
		{
			printf("received own discovery packet, dropping it\n");
			return;
		} 
		else
		{
			if (pathLen++ >= MAX_ROUTE_LENGTH)
			{
				if (USER_DEBUG) 
					printf("path length at maximum, dropping discovery packet\n");	
				return;
			}
			pack_address(header->path[pathLen], my_addr);
			pack_unsigned_int(header->path_len, (1 + pathLen)); 
			i = unpack_unsigned_int(header->ttl) - 1;
			if (i == 0)
			{
				if (USER_DEBUG)
					printf("ttl 0, dropping packet\n");
				return;
			}
			
			if (USER_DEBUG) printf("rebroadcasting discovery packet\n");
			pack_unsigned_int(header->ttl, i);
			network_bcast_pkt(sizeof(struct routing_header), (char *) header, arg->size-sizeof(struct routing_header), arg->buffer+ sizeof(struct routing_header));
			
		}
	}
	else if (header->routing_packet_type == ROUTING_ROUTE_REPLY)
	{
		
		if (network_compare_network_addresses(my_addr, dst) != 0) 
		{
			miniroute_recieve_reply(src, arg);	
		}
		else
		{
			if (network_compare_network_addresses(my_addr, src) != 0)
			{
				printf("recieved own routing reply, this is an error unless you're sending packets to the local machine\n");
			} 
			i = unpack_unsigned_int(header->ttl);
			i--;
			if (i == 0)
			{
				if (USER_DEBUG)
					printf("ttl 0, dropping packet\n");
				return;
			}
			
			pack_unsigned_int(header->ttl, i);

			for (i = 1; i < pathLen; i++)
			{
				unpack_address(header->path[i], cur);
				if (network_compare_network_addresses(my_addr, cur) != 0)
				{
					unpack_address(header->path[i+1], cur);
					success = 1;
					break;
				}
			} 			
			if (success)
			{
				if (USER_DEBUG) printf("forwarding reply packet\n");
				network_send_pkt(cur, sizeof(struct routing_header), (char *) header, arg->size-sizeof(struct routing_header), arg->buffer+ sizeof(struct routing_header));
			}
			else
			{
				if (USER_DEBUG) printf("failed to forward reply packet\n");
				return;
			}
		
		}
			
	}
	else 
	{
		printf("Unrecognized packet type\n");
	}
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
	minithread_t mainThread = (minithread_t) malloc(sizeof(struct minithread));
	
	minithread_t idleThread;
	
	alarms = new_sortedlist();

	cleanupSemaphore = semaphore_create();
	semaphore_initialize(cleanupSemaphore, 0);
	
	currentTime = 0;

	quantaRemaining = 0;

	multiqueue = multilevel_queue_new(4);

	queue_finished_threads = queue_new();
	
	idleThread = minithread_create(placeholder, (arg_t) NULL);
	queue_append(queue_finished_threads, idleThread);
	mainThread = minithread_create(mainproc, mainarg);
	
	runningThread = mainThread;


	minithread_clock_init(QUANTA, clock_handler);
	minifile_initialize();
	miniterm_initialize();
	miniroute_initialize();
	minisocket_initialize();
	minimsg_initialize();
	miniroute_initialize();
	network_initialize(network_handler);
	minithread_switch(idleThread->stack_top, mainThread->stack_top);	
}
