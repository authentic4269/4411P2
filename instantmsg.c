/* 
 * instant messenger application for multiple machines
 *
 * USAGE: ./minithread <souceport> <destport> <hostname>
 *
 * sourceport = udp port to listen on
 * destport   = udp port to send to
 */

#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"
#include "read.h"
#include "read_private.h"
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAX_COUNT 100

char* hostname;

int receive_first(int* arg) 
{
  int length;
  miniport_t port;
  miniport_t sourcePort;

  port = miniport_create_unbound(1);

  while(1)
  {
	  char buffer[BUFFER_SIZE];
	  char buffer2[BUFFER_SIZE];
    length = BUFFER_SIZE;

    printf("\t\t(Waiting for a Friend)\n");
    minimsg_receive(port, &sourcePort, buffer, &length);
    printf("Friend: %s", buffer);
    printf("You: ");
    fflush(stdout);
    length = miniterm_read(buffer2, BUFFER_SIZE);
    minimsg_send(port, sourcePort, buffer2, length+1);
    miniport_destroy(sourcePort);
  }

  return 0;
}

int transmit_first(int* arg) 
{
  int length = BUFFER_SIZE;
  network_address_t addr;
  miniport_t port;
  miniport_t destinationPort;
  miniport_t sourcePort;

  AbortOnCondition(network_translate_hostname(hostname, addr) < 0, 
    "Could not resolve hostname, exiting.");

  port = miniport_create_unbound(0);
  destinationPort = miniport_create_bound(addr, 1);

  while(1)
  {
	  char buffer[BUFFER_SIZE];
	  char buffer2[BUFFER_SIZE];
    printf("You: ");
    fflush(stdout);
    length = miniterm_read(buffer, BUFFER_SIZE);
    minimsg_send(port, destinationPort, buffer, length+1);
    length = BUFFER_SIZE;
    printf("\t\t(Waiting for a Friend)\n");
    minimsg_receive(port, &sourcePort, buffer2, &length);
    printf("Friend: %s", buffer2);
    miniport_destroy(sourcePort);
  }

  return 0;
}

#ifdef WINCE
void main(void)
{
  READCOMMANDLINE
#else /* WINNT code */
int main(int argc, char** argv) 
{
#endif
  short fromport, toport;
  fromport = atoi(argv[1]);
  toport = atoi(argv[2]);
  network_udp_ports(fromport,toport); 

  if (argc > 3) 
  {
    hostname = argv[3];
		minithread_system_initialize(transmit_first, NULL);
  }
  else 
		minithread_system_initialize(receive_first, NULL);

  return 0;
}
