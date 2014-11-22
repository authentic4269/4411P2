#include "minimsg.h"
#include "miniroute.h"
#include "miniheader.h"
#include "interrupts.h"
#include "queue.h"
#include "synch.h"
#include <stdlib.h>
#include <stdio.h>


/*
 *  Implementation of minimsgs and miniports.
 */

//Common Numbers
#define MINIMUM_UNBOUND 0
#define MAXIMUM_UNBOUND 32767
#define MINIMUM_BOUND 32768
#define MAXIMUM_BOUND 65535

#define MAXIMUM_MSG_SIZE (4096)

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
	nextBoundPort = 0;

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

/*
 * Gets a miniport by port_number. Its here so that we don't rely on an array
 * implementation of miniports, and for convenience. The caller needs to perform
 * a null check on the returned value. 
 */
miniport_t
miniport_get_unbound(int port_number) {
	if (port_number > MAXIMUM_UNBOUND || port_number < MINIMUM_UNBOUND)
		return NULL;
	return miniports[port_number];
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
	int totalBoundPorts = MAXIMUM_BOUND - MINIMUM_BOUND + 1;
	int convertedPortNumber = (((nextBoundPort - MINIMUM_BOUND)) % totalBoundPorts) + totalBoundPorts;
	int i = 1;

	semaphore_P(bound_semaphore);

	while (i < totalBoundPorts && (miniports[convertedPortNumber] != NULL))
	{
		convertedPortNumber = (((nextBoundPort - MINIMUM_BOUND)+i) % totalBoundPorts) + totalBoundPorts;
		i++;
	}

	if (miniports[convertedPortNumber] == NULL)
	{
		newPort = (miniport_t) malloc(sizeof(struct miniport));

		if (newPort != NULL)
		{
			newPort->port_number = convertedPortNumber;
			newPort->type = 1;
			newPort->port_data.bound.remote_port_number = remote_unbound_port_number;
			network_address_copy(addr, newPort->port_data.bound.remote_addr);
			miniports[convertedPortNumber] = newPort;
			nextBoundPort = convertedPortNumber + 1;
			semaphore_V(bound_semaphore);
			return newPort;
		}
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
	int ret;

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

	if (msg == NULL)
		return 0;

	if (len < 0 || len > MAXIMUM_MSG_SIZE)
		return 0;

	header = (mini_header_t) malloc(sizeof(struct mini_header));

	if (header == NULL)
		return 0;

	header->protocol = PROTOCOL_MINIDATAGRAM;

	pack_address(header->source_address, myaddr);
	pack_unsigned_short(header->source_port, local_unbound_port->port_number);
	pack_address(header->destination_address, local_bound_port->port_data.bound.remote_addr);
	pack_unsigned_short(header->destination_port, local_bound_port->port_data.bound.remote_port_number);

	ret= miniroute_send_pkt(local_bound_port->port_data.bound.remote_addr, sizeof(struct mini_header), (char *) header, len, (char *) msg);  

	free(header);

	return ret;
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
	//Store header
	mini_header_t header;

	//Holds addresses
	network_address_t source_addr;
	network_address_t destination_addr;
	network_address_t my_addr;

	//Holds port numbers
	int source_port_number;
	int destination_port_number;

	//Stores packet contents
	int data_size;
	char* data;

	network_interrupt_arg_t* incoming_data;

	if (local_unbound_port == NULL)
		return 0;

	if (msg == NULL)
		return 0;

	if (*len <= 0 || *len >= MAXIMUM_MSG_SIZE)
		return 0;

	semaphore_P(local_unbound_port->port_data.unbound.data_available);
	queue_dequeue(local_unbound_port->port_data.unbound.data_queue, (void**) &incoming_data);

	header = (mini_header_t) ((char*) incoming_data->buffer + sizeof(struct mini_header));

	source_port_number = (int) unpack_unsigned_short(header->source_port);
	destination_port_number = (int) unpack_unsigned_short(header->destination_port);
	unpack_address(header->source_address, source_addr);
	unpack_address(header->destination_address, destination_addr);

	network_get_my_address(my_addr);

	//Ensure packet is going to a valid location and port
	if (network_compare_network_addresses(my_addr, destination_addr) != 0 ||
			(destination_port_number < MINIMUM_UNBOUND || destination_port_number > MAXIMUM_UNBOUND))
	{
		free(incoming_data);
		return 0;
	}

	//Set data pointer to ((start of the packet) + (size of the header))
	data = (char*) (incoming_data->buffer + sizeof(struct mini_header));

	//Set data_size to ((size of the packet) - (size of the header))
	data_size = incoming_data->size - sizeof(struct mini_header);

	//Set return value
	if (*len > data_size)
	{
		*len = data_size;
	}

	//Set minimsg
	memcpy(msg, data, *len);

	//Create and set bound port targeting sender's address and listening port
	*new_local_bound_port = miniport_create_bound(source_addr, source_port_number);

	free(incoming_data);

	return *len;
}

