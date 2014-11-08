/*
 *	Implementation of minisockets.
 */
#include "minisocket.h"

#define TCP_PORT_TYPE_SERVER 0
#define TCP_PORT_TYPE_CLIENT 1

#define TCP_MINIMUM_SERVER 0
#define TCP_MAXIMUM_SERVER 32767
#define TCP_MINIMUM_CLIENT 32768
#define TCP_MAXIMUM_CLIENT 65535

minisocket_t* minisockets;

semaphore_t server_semaphore;
semaphore_t client_semaphore;

queue_t sockets_to_be_deleted;

semaphore_t destroy_semaphore;

struct minisocket
{
	char port_type;
	int port_number;

	char status;
	char waiting;

	//Semaphore to wait for an ACK
	semaphore_t wait_for_ack_sem;

	int seq_num;
	int ack_num;

	//Stores the data
	char* data_buffer;
	int data_len;

	//Synchronizes access to parts of the socket
	semaphore_t mutex;

	//Threads waiting on the mutex
	int num_waiting_on_mutex;

	//Destination host's information
	network_address_t destination_addr;
	int dst_port;

	//Alerts the thread of waiting packets
	queue_t waiting_packets;
	semaphore_t packet_ready;

	int timeout;
};

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	int i = TCP_MINIMUM_SERVER;

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
	sockets_to_be_deleted = semaphore_create();
	semaphore_initialize(sockets_to_be_deleted, 0);

	//Synchronize access to semaphore_destroy()
	destroy_semaphore = semaphore_create();
	semaphore_initialize(destroy_semaphore, 1);

	//Fork the thread that deletes sockets on command
	//NOTE DELETE SOCKETS NOT COMPLETED
	minithread_fork((proc_t) &delete_sockets, (void*) NULL);
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

}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{

}
