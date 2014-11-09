/*
 *	Implementation of minisockets.
 */
#include "minisocket.h"
#include "minithread.h"

#define TCP_PORT_TYPE_SERVER 0
#define TCP_PORT_TYPE_CLIENT 1

#define TCP_MINIMUM_SERVER 0
#define TCP_MAXIMUM_SERVER 32767
#define TCP_MINIMUM_CLIENT 32768
#define TCP_MAXIMUM_CLIENT 65535

minisocket_t* minisockets;

semaphore_t server_semaphore;
semaphore_t client_semaphore;

queue_t sockets_to_delete;

semaphore_t delete_semaphore;
semaphore_t destroy_semaphore;

int currentClientPort;

struct minisocket
{
	char port_type;
	int port_number;

	char status;
	char waiting;

	//Semaphore to wait for an ACK
	semaphore_t wait_for_ack_semaphore;

	int seq_number;
	int ack_number;

	//Stores the data
	char* data_buffer;
	int data_length;

	//Synchronizes access to parts of the socket
	semaphore_t mutex;

	int num_waiting_on_mutex;

	//Destination host's information
	network_address_t destination_addr;
	int destination_port;

	//Alerts the thread of waiting packets
	queue_t waiting_packets;
	semaphore_t packet_ready;

	int timeout;
};

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	int i = TCP_MINIMUM_SERVER;
	//int currentClientPort = 0;

	minisockets = (minisocket_t*) malloc(sizeof(minisocket_t) * (TCP_MAXIMUM_CLIENT - TCP_MINIMUM_SERVER + 1));
	
	if (minisockets == NULL)
		return;

	while (i <= TCP_MAXIMUM_CLIENT)
	{
		minisockets[i] = NULL;
		i++;
	}

	//Mutex that controls access to the minithreads array for server ports
	server_semaphore = semaphore_create();
	semaphore_initialize(server_semaphore, 1);

	//Mutex that controls access to the minithreads array for client ports
	client_semaphore = semaphore_create();
	semaphore_initialize(client_semaphore, 1);

	//Queue of sockets that will be deleted
	sockets_to_delete = queue_new();

	//Semaphore that signals the thread to delete sockets performs
	delete_semaphore = semaphore_create();
	semaphore_initialize(delete_semaphore, 0);

	//Synchronize access to semaphore_destroy()
	destroy_semaphore = semaphore_create();
	semaphore_initialize(destroy_semaphore, 1);

	//Fork the thread that deletes sockets on command
	//NOTE DELETE SOCKETS NOT COMPLETED
	//minithread_fork((proc_t) &delete_sockets, (void*) NULL);
}

void delete_sockets(void *arg) {

}

void minisocket_destory(minisocket_t minisocket, int FIN)
{
	int portNumber;

	if (minisocket == NULL)
		return;

	portNumber = minisocket->port_number;

	semaphore_P(destroy_semaphore);

	minisocket->waiting = TCP_PORT_WAITING_TO_CLOSE;

	if (minisockets[portNumber] == NULL)
		return;

	semaphore_V(minisocket->packet_ready);

	semaphore_P(minisocket->mutex);

	if (minisockets[portNumber] == NULL)
		return;

	if (FIN == 1)
		transmit_packet(minisocket, minisocket->destination_addr, minisocket->destination_port,
			1, MSG_FIN, 0, NULL, &error);

	minisocket->status = TCP_PORT_CLOSING;

	threads = minisocket->num_waiting_on_mutex;

	while (i < threads)
	{
		semaphore_V(minisocket->mutex);
		i++;
	}

	minisockets[portNumber] = NULL;

	//MITCH CHECK IF INTERRUPT SETTING REQUIRED
	//level = set_interrupt_level(DISABLED);
	semaphore_destroy(minisocket->wait_for_ack_semaphore);
	semaphore_destroy(minisocket->mutex);
	semaphore_destroy(minisocket->packet_ready);

	if (minisocket->data_length != 0)
		free(minisocket->data_buffer);

	queue_free(minisocket->waiting_packets);
	//set_interrupt_level(level);

	free(minisocket);

	semaphore_V(destroy_semaphore);
}

/*
 * Creates a socket 
 * The argument "port" is the port number of the created socket
 */
minisocket_t minisocket_create_socket(int port)
{
	minisocket_t newMinisocket;
	//semaphore_t wait_for_ack_sem;
	//Synchronizes access to parts of the socket
	//semaphore_t mutex;

	//Destination host's information
	//network_address_t destination_addr;

	//Threads waiting on the mutex
	//int num_waiting_on_mutex;

	//Alerts the thread of waiting packets
	//semaphore_t packet_ready;
	//int dst_port;
	newMinisocket = (minisocket_t) malloc(sizeof(struct minisocket));

	if (newMinisocket == NULL)
		return NULL;
	newMinisocket->port_number = port;
	newMinisocket->status = TCP_PORT_LISTENING;
	newMinisocket->seq_number = 0;
	newMinisocket->ack_number = 0;
	newMinisocket->data_buffer = NULL;
	newMinisocket->data_length = 0;
	newMinisocket->num_waiting_on_mutex = 0;
	newMinisocket->timeout = 100;

	newMinisocket->wait_for_ack_semaphore = semaphore_create();
	if (newMinisocket->wait_for_ack_semaphore == NULL)
	{
		free(newMinisocket);
		return NULL;
	}
	semaphore_initialize(newMinisocket->wait_for_ack_semaphore, 0);

	newMinisocket->mutex = semaphore_create();
	if (newMinisocket->mutex == NULL)
	{
		free(newMinisocket->wait_for_ack_semaphore);
		free(newMinisocket);
		return NULL;
	}
	semaphore_initialize(newMinisocket->mutex, 1);

	newMinisocket->packet_ready = semaphore_create();
	if (newMinisocket->packet_ready == NULL)
	{
		free(newMinisocket->mutex);
		free(newMinisocket->wait_for_ack_semaphore);
		free(newMinisocket);
		return NULL;
	}
	semaphore_initialize(newMinisocket->packet_ready, 0);

	newMinisocket->waiting_packets = queue_new();
	if (newMinisocket->waiting_packets == NULL)
	{
		free(newMinisocket->packet_ready);
		free(newMinisocket->mutex);
		free(newMinisocket->wait_for_ack_semaphore);
		free(newMinisocket);
		return NULL;
	}

	return newMinisocket;
}



/* 
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error)
{
	minisocket_t newMinisocket;
	int ack_check;
	int connected = 1;

	if (error == NULL)
		return NULL;

	if (port < TCP_MINIMUM_SERVER || port > TCP_MAXIMUM_SERVER)
	{
		*error = SOCKET_INVALIDPARAMS;
		return NULL;
	}

	semaphore_P(server_semaphore);

	//Checks if port already exists
	if (minisockets[port] != NULL)
	{
		*error = SOCKET_PORTINUSE;
		semaphore_V(server_semaphore);
		return NULL;
	}

	newMinisocket = minisocket_create_socket(port);
	if (newMinisocket == NULL)
	{
		*error = SOCKET_OUTOFMEMORY;
		semaphore_V(server_semaphore);
		return NULL;
	}

	newMinisocket->port_type = TCP_PORT_TYPE_SERVER;
	minisockets[port] = newMinisocket;

	semaphore_V(server_semaphore);

	while (connected == 1)
	{
		semaphore_P(newMinisocket->wait_for_ack_semaphore);

		newMinisocket->status = TCP_PORT_CONNECTING;

		newMinisocket->ack_number++;
		ack_check = transmit_packet(newMinisocket, newMinisocket->destination_addr, 
			newMinisocket->destination_port, 1, MSG_SYNACK, 0, NULL, error);

		if (ack_check == -1)
		{
			newMinisocket->status = TCP_PORT_LISTENING;
			newMinisocket->ack_number--;
			newMinisocket->seq_number--;
			network_address_blankify(newMinisocket->destination_addr);
			newMinisocket->destination_port = 0;
		}	
		else
		{
			newMinisocket->status = TCP_PORT_CONNECTED;
			connected = 0;
		}
	}

	*error = SOCKET_NOERROR;
	return newMinisocket;
}

/*
 * Get the minisocket associated with a given port number
 */
minisocket_t minisocket_get(int num)
{
	return minisockets[num];
}

/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine. 
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error)
{
	minisocket_t newMinisocket;
	int totalClientPorts = TCP_MAXIMUM_CLIENT - TCP_MINIMUM_CLIENT + 1;
	int convertedPortNumber = (((currentClientPort - TCP_MINIMUM_CLIENT)) % totalClientPorts) + totalClientPorts;
	int i = 1;
	int transmitCheck;

	if (error == NULL)
		return NULL;

	semaphore_P(client_semaphore);

	while (i < totalClientPorts && (minisockets[convertedPortNumber] != NULL))
	{
		convertedPortNumber = (((currentClientPort - TCP_MINIMUM_CLIENT)+i) % totalClientPorts) + totalClientPorts;
		i++;
	}

	convertedPortNumber+=TCP_MINIMUM_CLIENT;

	if (minisockets[convertedPortNumber] != NULL)
	{
		*error = SOCKET_NOMOREPORTS;
		semaphore_V(client_semaphore);
		return NULL;		
	}

	newMinisocket = minisocket_create_socket(convertedPortNumber);
	if (newMinisocket == NULL)
	{
		*error = SOCKET_OUTOFMEMORY;
		semaphore_V(server_semaphore);
		return NULL;
	}

	newMinisocket->port_type = TCP_PORT_TYPE_CLIENT;
	network_address_copy(addr, newMinisocket->destination_addr);
	minisocket->destination_port = port;

	minisockets[port] = newMinisocket;
	minisocket->status = TCP_PORT_CONNECTING;

	semaphore_V(client_semaphore);

	transmitCheck = transmit_packet(minisocket, addr, port, 1, MSG_SYN, 0, NULL, error);
	if (transmitCheck == -1)
	{
		//*error set by transmit_packet()
		minisockets[port] = NULL;
		free(minisocket);
		return NULL;
	}

	minisocket->status = TCP_PORT_CONNECTED;

	*error = SOCKET_NOERROR;
	return newMinisocket;
}


/* 
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error)
{
	int portNumber;
	int sentLength;
	int sentData;

	if (socket == NULL || msg == NULL || len <= 0)
	{
		*error = SOCKET_INVALIDPARAMS;
		return -1;
	}

	if (error == NULL)
		return -1;

	portNumber = socket->port_number;

	if (socket->status == TCP_PORT_CLOSING || socket->waiting == TCP_PORT_WAITING_TO_CLOSE
		|| minisockets[portNumber] == NULL)
	{
		*error = SOCKET_SENDERROR;
		return -1;
	}

	socket->num_waiting_on_mutex++;

	semaphore_P(socket->mutex);

	socket->num_waiting_on_mutex--;

	if (socket->status == TCP_PORT_CLOSING || socket->waiting == TCP_PORT_WAITING_TO_CLOSE
		|| minisockets[portNumber] == NULL)
	{
		*error = SOCKET_SENDERROR;
		semaphore_V(socket->mutex);
		return -1;
	}

	//MITCH HELP HERE! Concept is right but implementation might not be!!!
	maxDataSize = MAXIMUM_NETWORK_PKT_SIZE - sizeof(struct miniheader);
	while (len > 0)
	{
		sentLength = (maxDataSize > len ? len : maxDataSize);
		check = transmit_packet(socket, socket->destination_addr, socket->destination_port,
				, MSG_ACK, sentLength, (msg+sentData), error);

		if (check == -1)
		{
			semaphore_V(socket->mutex);
			minisocket_destory(socket, 0);
			return (sentData == 0 ? -1 : sentData);
		}

		len -= maxDataSize;
		sentData += sentLength;
	}

	semaphore_V(socket->mutex);
	*error = SOCKET_NOERROR;

	return 0;
}

/*
 * Receive a message from the other end of the socket. Blocks until
 * some data is received (which can be smaller than max_len bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error)
{
	int portNumber;
	int packetSize;
	char* packetContents;
	network_interrupt_arg_t* arg;
	int dataLength;
	int returnLength;
	char* oldDataBuffer;
	int newLength;
	//Maybe missing variable?

	if (socket == NULL || msg == NULL || max_len <= 0)
	{
		*error = SOCKET_INVALIDPARAMS;
		return -1;
	}	

	portNumber = socket->port_number;

	if (socket->status == TCP_PORT_CLOSING || socket -> waiting == TCP_PORT_WAITING_TO_CLOSE
		|| minisockets[portNumber] == NULL)
	{
		*error = SOCKET_RECEIVEERROR;
		return -1;
	}

	socket->num_waiting_on_mutex++;
	semaphore_P(socket->mutex);
	socket->num_waiting_on_mutex--;

	if (socket->status == TCP_PORT_CLOSING || socket -> waiting == TCP_PORT_WAITING_TO_CLOSE
		|| minisockets[portNumber] == NULL)
	{
		*error = SOCKET_RECEIVEERROR;
		semaphore_V(socket->mutex);
		return -1;
	}

	if (socket->data_length == 0 && queue_length(socket->waiting_packets) == 0)
	{
		semaphore_initialize(socket->packet_ready, 0);
		semaphore_P(socket->packet_ready);

		if (socket->status == TCP_PORT_CLOSING || socket->waiting == TCP_PORT_WAITING_TO_CLOSE
			|| minisockets[portNumber] == NULL)
		{
			*error = SOCKET_RECEIVEERROR;
			semaphore_V(socket->mutex);
			return -1;
		}
	}

	if (queue_length(socket->waiting_packets) > 0)
	{	
		//MITCH CHECK THIS!
		//prev_level = set_interrupt_level(DISABLED);
		queue_dequeue(socket->waiting_packets, (void**) &arg);
		//set_interrupt_level(prev_level);

		packetSize = arg->size;

		//check this line
		packetContents = (char*) (arg->buffer + sizeof(struct mini_header));

		dataLength = packetSize - sizeof(struct mini_header));

		if (socket->data_length > 0)
		{
			newDataBuffer = (char*) malloc(socket->data_length + dataLength);

			memcpy(newDataBuffer, socket->data_buffer, socket->data_length);

			memcpy(newDataBuffer+socket->data_length, packetContents+sizeof(struct miniheader), data_length);

			free(socket->data_buffer);

			socket->data_buffer = newDataBuffer;
			socket->data_length += dataLength;
		}
		else
		{
			socket->data_buffer = (char*) malloc(dataLength);
			memcpy(socket->data_buffer, packetContents+sizeof(struct miniheader), dataLength);
			socket->data_length = dataLength;
			semaphore_V(socket->packet_ready);
		}

		free(arg);
	}

	returnLength = max_len;
	if (socket->data_length < max_len)
		returnLength = socket->data_length;

	memcpy(msg, socket->data_buffer, returnLength);

	newLength = socket->data_length - returnLength;

	if (newLength == 0)
	{
		free(socket->data_buffer);
		socket->data_buffer = NULL;
	}
	else
	{
		oldDataBuffer = socket->data_buffer;
		socket->data_buffer = (char*) malloc(newLength);
		memcpy(socket->data_buffer, +returnLength, newLength);
		free(oldDataBuffer);
	}

	socket->data_length = newLength;

	semaphore_V(socket->mutex);
	*error = SOCKET_NOERROR;
	return returnLength;
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{
	minisocket_destory(socket, 1);
}
