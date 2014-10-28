/*
 *  Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "miniheader.h"
#include "queue.h"
#include "synch.h"
#include <stdlib.h>
#include <stdio.h>

//Common Numbers
#define MINIMUM_UNBOUND 0
#define MAXIMUM_UNBOUND 32767
#define MINIMUM_BOUND 32768
#define MAXIMUM_BOUND 65535

struct miniport
{
	unsigned short port_number;
	// type == 0 if unbound, == 1 if bound
	short type;

	union 
	{
		struct 
		{
			queue_t data_queue;
			semaphore_t data_available;
		} unbound;

		struct 
		{
			network_address_t remote_addr;
			unsigned int remote_port_number;
		} bound;
	} port_data;
};

miniport_t* miniports;

int nextBoundPort;

// Mutexes for creation of bound/unbound miniports, and destruction of any miniport_t
semaphore_t bound_semaphore; 
semaphore_t unbound_semaphore; 
semaphore_t destroy_semaphore;

/* performs any required initialization of the minimsg layer.
*/
	void
minimsg_initialize()
{
	int currentPort = MINIMUM_UNBOUND;
	int totalPorts = (MAXIMUM_BOUND - MINIMUM_UNBOUND + 1);
	nextBoundPort = MINIMUM_BOUND;

	miniports = (miniport_t*) malloc(sizeof(miniport_t) * totalPorts);

	if (miniports == NULL)
		return;

	while (currentPort < totalPorts)
	{
		miniports[currentPort] = NULL;
		currentPort++;
	}

	bound_semaphore = semaphore_create();
	semaphore_initialize(bound_semaphore, 1);

	unbound_semaphore = semaphore_create();
	semaphore_initialize(unbound_semaphore, 1);

	destroy_semaphore = semaphore_create();
	semaphore_initialize(destroy_semaphore, 1);
}

/* Creates an unound port for listening. Multiple requests to create the same
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

	if (port_number > MAXIMUM_UNBOUND || port_number < MINIMUM_UNBOUND)
		return NULL;

	semaphore_P(unbound_semaphore);

	if (miniports[port_number] != NULL)
	{
		semaphore_V(unbound_semaphore);
		return miniports[port_number];
	}

	newUnboundPort = (miniport_t) malloc(sizeof(struct miniport));

	if (newUnboundPort != NULL)
	{
		newUnboundPort -> port_number = port_number;
		newUnboundPort -> port_data.unbound.data_queue = queue_new();
		newUnboundPort -> port_data.unbound.data_available = semaphore_create();
		newUnboundPort -> type = 0;

		semaphore_initialize(newUnboundPort->port_data.unbound.data_available, 0);

		miniports[port_number] = newUnboundPort;

		semaphore_V(unbound_semaphore);

		return newUnboundPort;
	}
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
	miniport_t newPort;
	semaphore_P(bound_semaphore);
	if (nextBoundPort >= MAXIMUM_BOUND)
	{
		nextBoundPort = MINIMUM_BOUND;
	}

	newPort = (miniport_t) malloc(sizeof(struct miniport));

	if (newPort != NULL)
	{
		newPort->port_data.bound.remote_port_number = remote_unbound_port_number;
		// this is a hack, because array pointers are constant	
		newPort->port_data.bound.remote_addr[0] = addr[0];
		newPort->port_data.bound.remote_addr[1] = addr[1];
		newPort->type = 1;


		miniports[nextBoundPort++] = newPort;

		semaphore_V(bound_semaphore);

		return newPort;
	}
	semaphore_V(bound_semaphore);
	return NULL;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
	void
miniport_destroy(miniport_t miniport)
{
	//Destroy miniport only if it exists
	if (miniport != NULL)
	{
		semaphore_P(destroy_semaphore);

		//NULL array at port_number
		miniports[miniport -> port_number] = NULL;

		//If miniport is an unbound port, free the data it contains that we malloc
		if (miniport->type == 0)
		{
			semaphore_destroy(miniport->port_data.unbound.data_available);
			queue_free(miniport->port_data.unbound.data_queue);
		}
		free(miniport);

		semaphore_V(destroy_semaphore);
	}
	return;
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
	mini_header_t header;
	network_address_t myaddr;
	network_get_my_address(myaddr);
	if (local_bound_port == NULL || local_bound_port->type != 1)
	{
		printf("err in minimsg_send\n");
		return -1;
	}
	if (local_unbound_port == NULL || local_unbound_port->type != 0)
	{
		printf("err in minimsg_send\n");
		return -1;
	}
	header = (mini_header_t) malloc(sizeof(struct mini_header));
	header->protocol = PROTOCOL_MINIDATAGRAM;
	pack_address(header->source_address, myaddr);
	pack_unsigned_short(header->source_port, local_unbound_port->port_number);
	pack_address(header->destination_address, local_bound_port->port_data.bound.remote_addr);
	pack_unsigned_short(header->destination_port, local_bound_port->port_data.bound.remote_port_number);
	
	return network_send_pkt(local_bound_port->port_data.bound.remote_addr, sizeof(struct mini_header), (char *) header, len, (char *) msg);  
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

