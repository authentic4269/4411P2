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
  miniport_t srcPort;
  char buf[BUFFER_SIZE];
  char buf2[BUFFER_SIZE];

  port = miniport_create_unbound(0);

  while(1)
  {
    length = BUFFER_SIZE;

    printf("\t\t(Waiting for a Friend)\n");
    minimsg_receive(port, &srcPort, buf, &length);
    printf("Friend: %s", buf);
    printf("You: ");
    fflush(stdout);
    length = miniterm_read(buf2, BUFFER_SIZE);
    minimsg_send(port, srcPort, buf2, length+1);
    miniport_destroy(srcPort);
  }

  return 0;
}

int transmit_first(int* arg) 
{
  int length = BUFFER_SIZE;
  network_address_t addr;
  miniport_t port;
  miniport_t dstPort;
  miniport_t srcPort;
  char buf[BUFFER_SIZE];
  char buf2[BUFFER_SIZE];
  

  AbortOnCondition(network_translate_hostname(hostname, addr) < 0, 
    "Could not resolve hostname, exiting.");

  port = miniport_create_unbound(0);
  dstPort = miniport_create_bound(addr, 0);

  while(1)
  {
    printf("Enter your message: \n");
    fflush(stdout);
    length = miniterm_read(buf, BUFFER_SIZE);
    if (minimsg_send(port, dstPort, buf, length+1) < 0)
	printf("Error in minimsg_send\n");
    length = BUFFER_SIZE;
    printf("Waiting for remote host...\n");
    if (minimsg_receive(port, &srcPort, buf2, &length) < 0)
	printf("Error in minimsg_receive\n");
    printf("Received message: %s", buf2);
    miniport_destroy(srcPort);
  }

  return 0;
}

int 
main(int argc, char** argv) 
{
  short from, to;
  from = atoi(argv[1]);
  to = atoi(argv[2]);
  network_udp_ports(from,to); 

  if (argc > 3) 
  {
    hostname = argv[3];
		minithread_system_initialize(transmit_first, NULL);
  }
  else if (argc == 3)
		minithread_system_initialize(receive_first, NULL);
  else 
	printf("Syntax: instantmsg fromport toport [remoteaddr]\n");
  return 0;
}
