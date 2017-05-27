#include "client_queue.h"

/*
	Create a new client and put it in the queue.
	A semaphore is used to notify the tellers that a new client is available
	return a pointer on the client created
*/
struct client_t* newClient(client_queue_t* q, int timing) {

	client_t* clt = (struct client_t*)malloc(sizeof(client_t));
	if (clt == NULL)
		return NULL;

	clt->time_entering_queue = timing;
	clt->time_begin_teller = -1;
	clt->time_end_teller = -1;
	clt->index_in_queue = q->nb_client_generated;
	q->queue[q->nb_client_generated] = clt;
	q->nb_client_generated++;

	printf("\tClient Generated !\n");
	sem_post(&q->sem_client_queue);
	return clt;
}

/*
	Beginning of transaction between the teller and the client. Use a mutex to protect the client queue
	return the index of the client in the queue
*/
int Welcome_client(int nb_teller, client_queue_t* q, int timing) {
	pthread_mutex_lock(&q->mutex_index_next_client);
	client_t* clt = NULL;
	clt = q->queue[q->index_next_client];
	clt->time_begin_teller = timing ;

	q->max_client_waiting = max(q->max_client_waiting, q->nb_client_generated - q->index_next_client );

	q->index_next_client++;
	pthread_mutex_unlock(&q->mutex_index_next_client);

	return clt->index_in_queue;
}

/*
	End of transaction between the teller and the client
*/
void Release_client(int nb_teller, client_queue_t* q, int timing, int clt_index) {
	client_t* clt = NULL;
	clt = q->queue[clt_index];
	clt->time_end_teller = timing;

	printf("Teller number %d done!\n", nb_teller);
}
