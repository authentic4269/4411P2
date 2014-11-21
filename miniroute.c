#include "miniroute.h"
#include "hashmap.h"
#include "interrupts.h"
#include "alarm.h"
#include "miniheader.h"
#include "minithread.h"
#define DEBUG 1
int route_request_id;
int route_requests_index;

semaphore_t route_cache_semaphore; //Controls acces to route cache
semaphore_t request_id_semaphore; //Controls access to route_request_id
semaphore_t cached_route_semaphore;

// forward declaration
void purge_route_cache(void *arg);
void delete_route_data(route_data_t routeData);
route_request_t new_route_request();
void alarm_wakeup_semaphore(void *arg);
char* merge_headers(routing_header_t route_header, char* header, int header_len);
network_address_t* miniroute_reverse_raw_path(routing_header_t header, int path_len);
route_data_t create_route_data(network_address_t* route, int route_len, int time_found);
void delete_route_request(route_request_t route_request);

routing_header_t new_miniroute_header(char packet_type, network_address_t dest_address, 
	unsigned int id, unsigned int ttl, unsigned int path_len, network_address_t* path);


/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	int i;
	route_request_id = 0;
	route_requests_index = 0;

	route_cache = hashmap_new(SIZE_OF_ROUTE_CACHE, 0);

	discovery_packets_seen = hashmap_new(10, 1);

	current_discovery_requests = hashmap_new(10, 1);

	route_requests_seen = (route_requests_seen_t*) malloc(sizeof(route_requests_seen_t) * ROUTE_REQUESTS_TO_STORE);

	for (i = 0; i < ROUTE_REQUESTS_TO_STORE; i++)
	{
		route_requests_seen[i] = (route_requests_seen_t) (malloc(sizeof(struct route_requests_seen)));
		route_requests_seen[i]->active = 0;
	}

	route_cache_semaphore = semaphore_create();
	semaphore_initialize(route_cache_semaphore, 1);

	request_id_semaphore = semaphore_create();
	semaphore_initialize(request_id_semaphore, 1);

	register_alarm(3000, &purge_route_cache, NULL);
}

//Removes entries older than 3 seconds route cache
void purge_route_cache(void* arg)
{
	//Next item in linked list of hashmap entries
	hashmap_item_t nextItem;

	//Store pointer to route data struct
	route_data_t routeData;

	int ticks;

	hashmap_item_t item = route_cache->first;

	semaphore_P(route_cache_semaphore);


	//Find all entries older than 3 seconds and delete them
	while (item != NULL)
	{
		nextItem = item->next;

		//Get the value of the hashmap
		routeData = (route_data_t) item_get_value(item);

		ticks = (currentTime - routeData->time_found) * QUANTA / 3;
		if (ticks > SECOND)
		{
			//Delete route data struct
			delete_route_data(routeData);

			//Delete hashmap_item
			hashmap_delete(route_cache, item_get_key(item));

			item = nextItem;
		}
		else
			item = NULL;
	}
	semaphore_V(route_cache_semaphore);

	//Restart in 3 seconds
	register_alarm(3000, &purge_route_cache, NULL);
}

//Free route data struct
void delete_route_data(route_data_t routeData)
{
	free(routeData->route);
	free(routeData);
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	route_data_t routeData;
	int ticks;
	int routeLength;
	network_address_t* route;
	int routeValid = 0;
	route_request_t routeRequest;
	int i;
	int currentRequestId;
	network_address_t myAddr;
	routing_header_t routingHeader = NULL;
	char* fullHeader = NULL;
	routing_header_t tempRoutingHeader;
	int j;
	int success = 0;
	network_address_t destAddr2;

	if (hdr_len == 0 || hdr == NULL || data_len == 0 || data == NULL)
		return -1;

	semaphore_P(route_cache_semaphore);

	routeData = (route_data_t) hashmap_get(route_cache, hash_address(dest_address));


	if (routeData != NULL)
		ticks = (currentTime - routeData->time_found) * QUANTA / 3;
	if (routeData != NULL && ticks > SECOND)
	{
		routeLength = routeData->length;
		route = (network_address_t*) malloc(sizeof(network_address_t) * routeLength);

		if (route == NULL)
		{
			semaphore_V(route_cache_semaphore);
			return -1;
		}
		memcpy(route, routeData->route, sizeof(network_address_t) * routeLength);
		routeValid = 1;
	}

	semaphore_V(route_cache_semaphore);

	if (routeValid == 0)
	{
		semaphore_P(route_cache_semaphore);
		routeRequest = (route_request_t) hashmap_get(current_discovery_requests, hash_address(dest_address));
		semaphore_V(route_cache_semaphore);

		if (routeRequest != NULL)
		{
			// Bit of a hacky way to enfore mutual exclusion on routeRequest->threads_waiting
			semaphore_P(route_cache_semaphore);
			routeRequest->threads_waiting++;
			semaphore_V(route_cache_semaphore);

			semaphore_P(routeRequest->waiting_semaphore);
			semaphore_P(route_cache_semaphore);
			routeData = (route_data_t) hashmap_get(route_cache, hash_address(dest_address));

			if (routeData == NULL)
			{
				semaphore_V(route_cache_semaphore);
				if (DEBUG)
					printf("Expected route to be found, none present\n");
				return -1;
			}
			else
			{
				routeLength = routeData->length;

				ticks = (currentTime - routeData->time_found) * QUANTA / 3;
				if (ticks > SECOND)
				{
					semaphore_V(route_cache_semaphore);
					return -1;
				}

				route = (network_address_t*) malloc(sizeof(network_address_t) * routeLength);

				if (route == NULL)
				{
					semaphore_V(route_cache_semaphore);
					return -1;
				}

				memcpy(route, routeData->route, sizeof(network_address_t) * routeLength);
				semaphore_V(route_cache_semaphore);
			}
		}
		else
		{
			routeRequest = new_route_request();

			if (routeRequest == NULL)
				return -1;

			semaphore_P(route_cache_semaphore);
			hashmap_insert(current_discovery_requests, hash_address(dest_address), routeRequest);
			currentRequestId = route_request_id++;
			semaphore_V(route_cache_semaphore);

			for (i = 0; i < 3; i++)
			{
				semaphore_P(request_id_semaphore);
				currentRequestId = route_request_id++;
				semaphore_V(request_id_semaphore);
				register_alarm(12000, &alarm_wakeup_semaphore, (void*) routeRequest->initiator_semaphore);

				network_get_my_address(myAddr);

				routingHeader = new_miniroute_header(ROUTING_ROUTE_DISCOVERY, dest_address, 
					currentRequestId, MAX_ROUTE_LENGTH, 1, &myAddr);

				if (routingHeader == NULL)
					if (DEBUG)
						printf("new_miniroute_header failed\n");
					return -1;

				fullHeader = merge_headers(routingHeader, hdr, hdr_len);

				if (fullHeader == NULL)
				{
					free(routingHeader);
					if (DEBUG)
						printf("merge_headers failed\n");
					return -1;
				}

				network_bcast_pkt(sizeof(struct routing_header)+hdr_len, (char*) fullHeader, data_len, data);

				semaphore_P(routeRequest->initiator_semaphore);

				if (routeRequest->interrupt_arg != NULL)
				{
					semaphore_P(routeRequest->initiator_semaphore);

					tempRoutingHeader = (routing_header_t) routeRequest->interrupt_arg->buffer;
					routeLength = unpack_unsigned_int(tempRoutingHeader->path_len);
					route = miniroute_reverse_raw_path(tempRoutingHeader, routeLength);

					if (route == NULL)
					{
						free(routingHeader);
						free(fullHeader);
						return -1;
					}

					routeData = create_route_data(route, routeLength, currentTime);

					if (routeData == NULL)
					{
						free(routingHeader);
						free(fullHeader);
						return -1;
					}

					semaphore_P(route_cache_semaphore);
					hashmap_insert(route_cache, hash_address(dest_address), routeData);
					semaphore_V(route_cache_semaphore);

					for (j = 0; j < routeRequest->threads_waiting; j++)
						semaphore_V(routeRequest->waiting_semaphore);

					delete_route_request(routeRequest);
					hashmap_delete(current_discovery_requests, hash_address(dest_address));

					free(routingHeader);
					free(fullHeader);

					success = 1;
					break;
				}
			}

			if (success == 0)
			{
				for (j = 0; j < routeRequest->threads_waiting; j++)
					semaphore_V(routeRequest->waiting_semaphore);

				delete_route_request(routeRequest);
				hashmap_delete(current_discovery_requests, hash_address(dest_address));
				if (routingHeader != NULL)
					free(routingHeader);
				if (fullHeader != NULL)
					free(fullHeader);

				return -1;
			}
		}
	}

	network_address_copy(route[routeLength-1], destAddr2);

	pack_address(((mini_header_t) hdr)->destination_address, destAddr2);

	routingHeader = new_miniroute_header(ROUTING_DATA, destAddr2, 0, MAX_ROUTE_LENGTH, routeLength, route);

	if (routingHeader == NULL)
		return -1;

	fullHeader = merge_headers(routingHeader, hdr, hdr_len);

	if (fullHeader == NULL)
		free(routingHeader);

	network_address_copy(route[1], destAddr2);

	network_send_pkt(destAddr2, sizeof(struct routing_header) + hdr_len, fullHeader, data_len, data);

	free(route);
	free(routingHeader);
	free(fullHeader);

	return (hdr_len + data_len);
}

//Create a route request
route_request_t new_route_request()
{
	route_request_t routeRequest = (route_request_t) malloc(sizeof(struct route_request));

	if (routeRequest == NULL)
		return NULL;

	routeRequest->threads_waiting = 0;
	routeRequest->initiator_semaphore = semaphore_create();
	semaphore_initialize(routeRequest->initiator_semaphore, 0);
	routeRequest->waiting_semaphore = semaphore_create();
	semaphore_initialize(routeRequest->waiting_semaphore, 0);
	routeRequest->interrupt_arg = NULL;

	return routeRequest;
}

//Wakes up the sending thread after route discovery timeout
void alarm_wakeup_semaphore(void* arg)
{
	semaphore_t semaphore = (semaphore_t) arg;
	semaphore_V(semaphore);
}

//Create a miniroute header
routing_header_t new_miniroute_header(char packet_type, network_address_t dest_address, 
	unsigned int id, unsigned int ttl, unsigned int path_len, network_address_t* path)
{
	routing_header_t routingHeader = (routing_header_t) malloc(sizeof(struct routing_header));
	int i = 0;

	if (routingHeader == NULL)
		return NULL;

	routingHeader->routing_packet_type = packet_type;
	pack_address(routingHeader->destination, dest_address);
	pack_unsigned_int(routingHeader->id, id);
	pack_unsigned_int(routingHeader->ttl, ttl);
	pack_unsigned_int(routingHeader->path_len, path_len);

	for (i = 0; i < path_len; i++)
		pack_address(routingHeader->path[i], path[i]);

	return routingHeader;
}

char* merge_headers(routing_header_t route_header, char* header, int header_len)
{
	//Must malloc mergedHeader later
	char* mergedHeader = (char*) malloc(sizeof(struct routing_header) + header_len);

	if (mergedHeader == NULL)
		return NULL;

	memcpy(mergedHeader, route_header, sizeof(struct routing_header));
	memcpy(mergedHeader + sizeof(struct routing_header), header, header_len);

	return mergedHeader;
}

//Reverse a route
network_address_t* miniroute_reverse_raw_path(routing_header_t header, int path_len)
{
	network_address_t unpackedAddr;
	int i = 0;

	network_address_t* reversedPath = (network_address_t*) malloc(path_len * sizeof(network_address_t));

	if (reversedPath == NULL)
		return NULL;

	for (i = 0; i < path_len; i++)
	{
		unpack_address(header->path[i], unpackedAddr);
		network_address_copy(unpackedAddr, reversedPath[path_len-1-i]);
	}

	return reversedPath;
}

route_data_t create_route_data(network_address_t* route, int route_len, int time_found)
{
	route_data_t routeData = (route_data_t) malloc(sizeof(struct route_data));
	if (routeData == NULL)
		return NULL;

	routeData->route = route;
	routeData->length = route_len;
	routeData->time_found = time_found;

	return routeData;
}

//Delete route request struct
void delete_route_request(route_request_t route_request)
{
	if (route_request->interrupt_arg != NULL)
		free(route_request->interrupt_arg);

	semaphore_destroy(route_request->initiator_semaphore);
	semaphore_destroy(route_request->waiting_semaphore);
	free(route_request);
}

/* hashes a network_address_t into a 16 bit unsigned int */
unsigned int hash_address(network_address_t address)
{
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*)address)[counter];

	return result % 65521;
}
