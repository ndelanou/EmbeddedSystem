#include "main.h"

/* STRUCTURES INITIALIZATION */
static bank_t bank =
{
	.isOpen = 0,
	.mutex_isOpen = PTHREAD_MUTEX_INITIALIZER,
};

static client_queue_t client_queue =
{
	.index_next_client = 0,
	.nb_client_generated = 0,
	.max_client_waiting = 0,
	.mutex_index_next_client = PTHREAD_MUTEX_INITIALIZER,
};

/* CREATION OF THE GLOBAL TIME VARIABLE */
pthread_t thread_timeManagement;
int minutes_timer = 0;

/* MAIN FUNCTION */
int main(int argc, char *argv[]) {

	Thread_Init();		// Creating all the threads

	Thread_Exit();		// Waiting for all the threads to exit

	printfMetrics();	// Print the asked metrics
	clean_memory();		// Clean the memory allocated for the clients

	system("pause");
	return EXIT_SUCCESS;
}

/*
	Teller thread's function
	In charge of handeling the clients and the pause cycle of the tellers
*/
void* fn_tellerManagement(void* arg) {
	int current_client_index;
	int time_mem;
	int time_last_pause;
	int time_before_next_pause;
	int time_in_break = 0;


	while (!bank.isOpen);

	time_mem = minutes_timer;
	time_last_pause = minutes_timer;
	time_before_next_pause = rand_a_b(300, 600);

	sem_wait(&client_queue.sem_client_queue);

	while (bank.isOpen || (client_queue.nb_client_generated != client_queue.index_next_client)) {
		bank.tellers[(int)arg-1].time_waiting += minutes_timer - time_mem - (int)(time_in_break/10);
		time_mem = minutes_timer;
		current_client_index = Welcome_client((int)arg, &client_queue.queue, minutes_timer);
		usleep(1000 *  rand_a_b(50,600));

		bank.tellers[(int)arg-1].time_working += minutes_timer - time_mem;
		time_mem = minutes_timer;
		Release_client((int)arg, &client_queue.queue, minutes_timer, current_client_index);
		bank.tellers[(int)arg-1].nb_client_handled++;

		/* PAUSE BLOCK*/ 
		/*
		if (minutes_timer >= time_last_pause + time_before_next_pause) {
			int break_time = rand_a_b(10, 40);
			usleep(1000 * break_time);
			bank.tellers[(int)arg - 1].time_in_break += break_time;
			time_last_pause = minutes_timer;
			time_before_next_pause = rand_a_b(300, 600);
			time_in_break += break_time;
		}*/

		sem_wait(&client_queue.sem_client_queue);

	}
	bank.tellers[(int)arg - 1].time_waiting += minutes_timer - time_mem - (int)(time_in_break/10) ;
	time_mem = minutes_timer;

	pthread_exit(NULL);
}

/*
	Client Generation thread's function
	In charge of creating new clients randomly between 1 and 4 minutes
*/
void* fn_clientGenerator(void* arg) {
	while (!bank.isOpen);

	while (bank.isOpen) {
		newClient(&client_queue.queue, minutes_timer);
		usleep(1000 * rand_a_b(100, 400));	//default values: 100 - 400
	}

	pthread_exit(NULL);
}

/*
	Bank thread's function
	In charge of the opening and the closing of the bank
*/
void* fn_bankManagement(void *arg) {
	while (minutes_timer < 540);
	printf("BANK OPEN !\n");
	bank.isOpen = 1;

	while (minutes_timer < 960);
	printf("BANK CLOSED !\n");
	bank.isOpen = 0;

	pthread_exit(NULL);
}

/*
	Time management thread's function
	In charge the timer count (in minutes)
*/
void* fn_timeManagement(void *arg) {
	minutes_timer = 540;
	while (minutes_timer <= 960) {
		minutes_timer++;
		if (minutes_timer % 60 == 0) { printf("It's %.2d:%.2d \n", minutes_timer/60, 0); }
		usleep( 1000 * MS_PER_MINUTE);
	}

	pthread_exit(NULL);
}

/*
	Generate a random number between a and b
	return the value
*/
int rand_a_b(int a, int b) {
	return rand() % (b - a + 1) + a;
}

/*
	Print the metrics of the program
*/
void printfMetrics() {
	printf("\n\n\n\t############################################\n\t");
	printf("Customers served today : \t%d\n\t", client_queue.nb_client_generated);
	printf("Average time in queue : \t%.2f minutes\n\t", Average_Time_Waiting());
	printf("Average time with the teller : \t%.2f minutes\n\t", Average_Time_With_Teller());
	printf("Average time waiting customer: \t%.2f minutes\n\t", Average_Time_Waiting_Teller());
	printf("Max wait time in queue : \t%d minutes\n\t", Max_Time_Waiting_Client());
	printf("Max wait time for a teller : \t%d minutes\n\t", Max_Time_Waiting_Teller());
	printf("Max time working for tellers : \t%d minutes\n\t", Max_Time_Working_Teller());
	printf("Max time in break for tellers :\t%d minutes\n\t", Max_Time_In_Break_Teller());
	printf("Max depth of the queue : \t%d\n\t", client_queue.max_client_waiting);
	printf("############################################\n\n\n");
}

/*
	Clear the memory allocated to every clients
*/
void clean_memory() {
	int i;
	for (i = 0; i < client_queue.nb_client_generated; i++) {
		free(client_queue.queue[i]);
	}
}

/*
	Function in charge of the initialisation of all the threads
*/
void Thread_Init() {
	int i, ret = 0;
	srand(time(NULL));

	/* Bank Creation */
	ret = pthread_create(&bank.thread_bank, NULL, fn_bankManagement, NULL);
	if (ret) { printf("AH! SOMETHING WENT WRONG DURING THE BANK CREATION!\n"); }

	/* Tellers Creation */
	for (i = 0; i < NB_TELLERS; i++)
	{
		ret = pthread_create(&bank.tellers[i].thread_teller, NULL, fn_tellerManagement, (void *)(i + 1));
		bank.tellers[i].time_working = 0;
		bank.tellers[i].time_waiting = 0;
		bank.tellers[i].time_in_break = 0;
		bank.tellers[i].nb_client_handled = 0;
		if (ret) { printf("AH! SOMETHING WENT WRONG DURING THE TELLERS CREATION!\n"); }
	}

	/* Creation of the Clients */
	ret = pthread_create(&client_queue.thread_client_generator, NULL, fn_clientGenerator, NULL);
	if (ret) { printf("AH! SOMETHING WENT WRONG DURING THE BANK CREATION!\n"); }
	sem_init(&(client_queue.sem_client_queue), 0, 0);

	/* Creation of the Time Manager */
	ret = pthread_create(&thread_timeManagement, NULL, fn_timeManagement, NULL);
	if (ret) { printf("AH! SOMETHING WENT WRONG DURING THE TIME MANAGER CREATION!\n"); }
}

/*
	Function waiting for all the threads to exit before finishing
*/
void Thread_Exit() {
	int i;
	/* Waiting for the end of the threads */
	pthread_join(bank.thread_bank, NULL);
	pthread_join(client_queue.thread_client_generator, NULL);
	pthread_join(thread_timeManagement, NULL);
	for (i = 0; i < NB_TELLERS; i++)
	{
		sem_post(&client_queue.sem_client_queue);	// sem generated to exit the teller's loops
	}
	for (i = 0; i < NB_TELLERS; i++)
	{
		pthread_join(bank.tellers[i].thread_teller, NULL);
	}
}

/*
	Return the average time waiting for a client
*/
float Average_Time_Waiting() {
	int i;
	long cnt = 0;
	client_t* clt = NULL;

	for(i=0; i<client_queue.nb_client_generated; i++) {
		clt = client_queue.queue[i];
		cnt += clt->time_begin_teller - clt->time_entering_queue;
	}

	return ((float)cnt / client_queue.nb_client_generated);
}

/*
	Return the average spend by a client with a teller
*/
float Average_Time_With_Teller() {
	int i;
	long cnt = 0;
	client_t* clt = NULL;

	for(i=0; i<client_queue.nb_client_generated; i++) {
		clt = client_queue.queue[i];
		cnt += clt->time_end_teller - clt->time_begin_teller;
	}

	return ((float)cnt / client_queue.nb_client_generated);
}

/*
	Return the maximum time waiting by a client
*/
int Max_Time_Waiting_Client() {
	int i;
	int max_time = 0;
	client_t* clt = NULL;

	for(i=0; i<client_queue.nb_client_generated; i++) {
		clt = client_queue.queue[i];
		max_time = max(max_time, clt->time_begin_teller - clt->time_entering_queue);
	}

	return max_time;
}

/*
	Return the maximum time waited by a teller for clients
*/
int Max_Time_Waiting_Teller() {
	int i;
	int max_time = 0;

	for(i=0; i<NB_TELLERS; i++) {
		max_time = max(max_time, bank.tellers[i].time_waiting);
	}

	return max_time;
}

/*
	Return the maximum transaction time for a client
*/
int Max_Time_Working_Teller() {
	int i;
	int max_time = 0;

	for (i = 0; i<NB_TELLERS; i++) {
		max_time = max(max_time, bank.tellers[i].time_working);
	}

	return max_time;
}

/*
	Return maximum time in break for a teller
*/
int Max_Time_In_Break_Teller() {
	int i;
	int max_time = 0;

	for (i = 0; i<NB_TELLERS; i++) {
		max_time = max(max_time, bank.tellers[i].time_in_break);
	}

	return max_time;
}

/*
	Return the average time waiting by a teller
*/
float Average_Time_Waiting_Teller() {
	int i;
	float cnt = 0;

	for(i=0; i<NB_TELLERS; i++) {
		cnt += bank.tellers[i].time_waiting / bank.tellers[i].nb_client_handled;
	}

	return ((float)cnt / NB_TELLERS);
}
