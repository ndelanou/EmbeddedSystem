#ifndef CLIENT_QUEUE_H
#define CLIENT_QUEUE_H

#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#endif

#define QUEUE_SIZE	(500)

/* CLIENT STRUCT */
typedef struct
{
	int time_entering_queue;
	int time_begin_teller;
	int time_end_teller;
	int index_in_queue;
}
client_t;

/* CLIENT QUEUE STRUCT */
typedef struct
{
	struct client_t* queue[QUEUE_SIZE];

	int index_next_client;
	pthread_mutex_t mutex_index_next_client;

	int max_client_waiting;
	int nb_client_generated;
	sem_t sem_client_queue;

	pthread_t thread_client_generator;
}
client_queue_t;


struct client_t* newClient(client_queue_t* q, int timing);
int Welcome_client(int nb_teller, client_queue_t* q, int timing);
void Release_client(int nb_teller, client_queue_t* q, int timing, int clt_index);
