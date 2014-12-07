#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "synch.h"
#include "queue.h"
#include "minithread.h"

//store.c

//Queue of all available phones Threads
static queue_t availablePhones;

/*Phone Serial Number 
 *each phone has a unique serial number, 
 *and the phones are unpacked in increasing serial order.
*/
int serialNumber = 0;

//Initialize other variables
int n = 0, m = 0;
int customersWaiting;
semaphore_t sem;

int customer(int *arg)
{
	//Initialize
	int *MAHPHONE;
	semaphore_P(sem);

	//Get first available phone 
	queue_dequeue(availablePhones, (void **) &MAHPHONE);
	
	//Activate Phone
	printf("I got a new PortPhone 5! Its serial number is %d\n", *MAHPHONE); 
	
	//Get out of line
	customersWaiting--;
	return 0;
}

int employee(int *arg)
{
	//Initialize
	int *newSerial;

	//Only unpack phones while customers are waiting and there arent enough
	//unpacked phones for remaining customers
	while (customersWaiting > queue_length(availablePhones))
	{
		//Make new phone
		newSerial = malloc(sizeof(int));
		*newSerial = serialNumber++;
		
		printf("Serial number is %d\n", *newSerial); 

		//Add phone to list of available phones
		queue_append(availablePhones, newSerial);

		semaphore_V(sem);
		minithread_yield();
	}
	return 0;
}

int storeSimulation(int *arg)
{
	//Initialize Variables
	int i;
	availablePhones = queue_new();
	sem = semaphore_create();

	//Number of customers waiting
	customersWaiting = m;

	//Interleave customers and employees as much as possible
	for (i = 0; (i < n && i < m); i++)
	{
		minithread_fork(employee, NULL);
		minithread_fork(customer, NULL);
	}

	//There will either be remaining employees or customers or neither
	//Add remaining employees if any
	if (i < n)
	{
		while (i < n) {
			minithread_fork(employee, NULL);
			i++;
		}
	}
	//Add remaining customers if any
	else
	{
		while (i < m) {
			minithread_fork(customer, NULL);
			i++;
		}
	}
	return 0;
}

int main (int argc, char **argv) {
	//int N - Employee Threads, unpack new phones at all time
	n = strtol(argv[1], NULL, 0);

	//int M - Customer Threads
	m = strtol(argv[2], NULL, 0);

	//Make sure program takes in proper number of arguements
	if (argc != 3)
	{
		printf("Usage: store [numEmployees] [numCustomers]\n");
		return -1;
	}
	
	minithread_system_initialize(storeSimulation, NULL); 
	return 0;
}
