#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include "client_queue.h"

#endif


#define NB_TELLERS	(3)
#define MS_PER_MINUTE (100)

void Thread_Init();
void Thread_Exit();
void* fn_bankManagement(void *arg);
void* fn_tellerManagement(void *arg);
void* fn_clientGenerator(void *arg);
void* fn_timeManagement(void *arg);
int rand_a_b(int a, int b);
void printfMetrics();
void clean_memory();
float Average_Time_Waiting();
float Average_Time_With_Teller();
int Max_Time_Waiting_Client();
int Max_Time_Waiting_Teller();
int Max_Time_Working_Teller();
int Max_Time_In_Break_Teller();
float Average_Time_Waiting_Teller();

/* TELLER STRUCT */
typedef struct
{
	int time_working;
	int time_waiting;
	int time_in_break;
	int nb_client_handled;

	pthread_t thread_teller;
}
teller_t;

/* BANK STRUCT */
typedef struct
{
	int isOpen;
	pthread_mutex_t mutex_isOpen;

	teller_t tellers[NB_TELLERS];

	pthread_t thread_bank;
}
bank_t;

