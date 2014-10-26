/*
 *  Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "queue.h"
#include "synch.h"

//Common Numbers
#define MINIMUM_UNBOUND 0
#define MAXIMUM_UNBOUND 32767
#define MINIMUM_BOUND 32768
#define MAXIMUM_BOUND 65535

struct miniport
{
   int port_number;

   union 
   {
   		struct 
   		{
   			queue_t data_queue;
   			semaphore_t data_available;
   		} unbound;

   		struct 
   		{
   			 network_address_t remote_address;
   			 int remote_unbound_port_number;
   		} bound;
   } port_data;
};

//Array of miniports
miniport_t* miniports;

//Needed Semaphores
semaphore_t bound_semaphore; //Controls access to bound miniports
semaphore_t unbound_semaphore; //Controls access to unbound miniports
semaphore_t destroy_semaphore; //Only allows one thread to destroy a port at a time

/* performs any required initialization of the minimsg layer.
 */
void
minimsg_initialize()
{
	int currentPort = MINIMUM_UNBOUND;
	int totalPorts = (MAXIMUM_BOUND - MINIMUM_UNBOUND + 1);

	miniports = (miniport_t*) malloc(sizeof(miniport_t) * totalPorts);

	if (miniports == NULL)
		return;

	while (currentPort < totalPorts)
	{
		miniports[currentPort] = NULL;
		currentPort++;
	}

	//Initialize bound semaphore which controls access to bound miniports
	bound_semaphore = semaphore_create();
	semaphore_initialize(bound_semaphore, 1);

	//Initialize unbound semaphore which controls access to unbound miniports
	unbound_semaphore = semaphore_create();
	semaphore_initialize(unbound_semaphore, 1);

	//Initialize destroy semaphore which controls access to destroy miniports
	destroy_semaphore = semaphore_create();
	semaphore_initialize(destroy_semaphore, 1);
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t
miniport_create_unbound(int port_number)
{
    miniport_t newUnboundPort;

    if (port_number > MAXIMUM_BOUND || port_number < MINIMUM_UNBOUND)
    	return NULL;

    semaphore_P(unbound_semaphore);

    //Return reference to any already existing ports
    if (miniports[port_number] != NULL)
    {
    	semaphore_V(unbound_semaphore);
    	return miniports[port_number];
    }

    //Create a new port if not already existing
    newUnboundPort = (miniport_t) malloc(sizeof(miniport));

    //Ensure port recreated properly
    if (newUnboundPort != NULL)
    {
    	//Store port number
    	newUnboundPort -> port_number = port_number;

    	//Store unbound fields
    	newUnboundPort -> port_data.unbound.data_queue = queue_new();
    	newUnboundPort -> port_data.unbound.data_available = semaphore_create();

    	//Initialize data_available semaphore
		semaphore_initialize(newUnboundPort->port_data.unbound.data_available, 0);

    	//Store newPort into miniports array
    	miniports[port_number] = newUnboundPort;

    	semaphore_V(unbound_semaphore);

    	//Return miniport
    	return miniports[port_number];
    }
    //else
    semaphore_V(unbound_semaphore);
    return NULL;
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t
miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
{
    return 0;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void
miniport_destroy(miniport_t miniport)
{
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int
minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len)
{
    return 0;
}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len)
{
    return 0;
}

