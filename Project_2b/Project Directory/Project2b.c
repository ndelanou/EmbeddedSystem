#include "recipe.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>       	/* for uintptr_t */
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>	/* for ClockCycles( )*/
#include <sys/mman.h>     	/* for mmap_device_io() */
#include <hw/inout.h>     	/* for in*() and out*() functions */
#include <sched.h>

#define MAX_CMD_VALUE 2
#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

#define PORT_LENGTH 	1
#define BASE_ADDRESS 	(0x280)
#define CONFIG_ADDRESS 	(BASE_ADDRESS+11)
#define PORTA_ADDRESS	(BASE_ADDRESS+8)
#define PORTB_ADDRESS	(BASE_ADDRESS+9)

void Ports_Init();
void Threads_Init(void);
void User_Interaction(void);
void Cmd_Management(unsigned char cmd[]);
void Sync_Management(void);
void* Update_Recipe(void* data);
void* Generate_PWM(void* data);
void* Set_Priority(void* data);

typedef union {
		struct _pulse   pulse;
} my_message_t;

struct Engine eng1, eng2;
pthread_t thread_engine_1, thread_engine_2, thread_recipe;
pthread_mutex_t mutex_writingPWM = PTHREAD_MUTEX_INITIALIZER;
uintptr_t ctrl_handle;
uintptr_t data_handle_A;
uintptr_t data_handle_B;


/*
	Main fonction creating initializing the port and creating the threads
	Handle the user interactions once program started.
*/
int main(void){

	/* Give this thread root permissions to access the hardware */
	int privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return -1;
	}

	/* ENGINE CONFIGURATION */
	Engine_Init(&eng1, CHANNEL_1_id);
	Fill_Test_Recipe1(&eng1);
	Engine_Init(&eng2, CHANNEL_2_id);
	Fill_Test_Recipe2(&eng2);

	/* INITIALIZATION */
	Ports_Init();
	Threads_Init();

	printf("\n\n\rNew Test...\r\n>");

	/* IDLE FUNCTION */
	while(1) {
		User_Interaction();
	}
}

/*
	Initialization of Port A and B on the QNX in order to generate the PWM
	Association of data handlers with the engines
*/
void Ports_Init() {
	/* Get a handle to the parallel port's Control register */
	ctrl_handle = mmap_device_io( PORT_LENGTH, CONFIG_ADDRESS );
	/* Initialize the parallel port with port A as an output (Trigger) and port B as an input (Echo) */
	int DIO_conf = in8(ctrl_handle);
	out8( ctrl_handle, DIO_conf & ~0x12);

	/* Get a handle to the parallel port's DIO registers */
	eng1.data_handler = mmap_device_io( PORT_LENGTH, PORTA_ADDRESS );
	eng2.data_handler = mmap_device_io( PORT_LENGTH, PORTB_ADDRESS );

	/* Initialization of the ports */
	out8(eng1.data_handler, 0x00);
	out8(eng2.data_handler, 0x00);
}


/*
	Creation of 3 threads
	- 1 for the recipe update every 100ms
	- 2 for the independant generation of the PWM of each ports with a period of 20ms
	Initial position : POSITION_0
*/
void Threads_Init(void) {
	int ret = 0;

	ret += pthread_create( &thread_engine_1, NULL, Generate_PWM, &eng1);
	ret += pthread_create( &thread_engine_2, NULL, Generate_PWM, &eng2);
	ret += pthread_create( &thread_recipe, NULL, Update_Recipe, NULL);

	if(ret) {
		printf("\nProblem with the thread creation.");
	}
}


/*
	Function in charge of the interaction with the user by the Serial Terminal
*/
void User_Interaction(void) {
	unsigned char cmd[MAX_CMD_VALUE];

	scanf("%s", cmd);

	Cmd_Management(cmd);
}

/*
	Function in charge of calling the right function after the user pressed enter with a new command
*/
void Cmd_Management(unsigned char cmd[]) {
	if(cmd[0]>90) {cmd[0] -= 32;}
	if(cmd[1]>90) {cmd[1] -= 32;}

	switch (cmd[0]) {
		case 'C' : eng1.in_Pause = 0;
			break;

		case 'P' : eng1.in_Pause = 1;
			break;

		case 'L' : goLeft(&eng1);
			break;

		case 'R' : goRight(&eng1);
			break;

		case 'B' : Restart_Recipe(&eng1);
			break;

		case 'N' :
			break;

		default:
			break;
	}

	switch (cmd[1]) {
		case 'C' : eng2.in_Pause = 0;
			break;

		case 'P' : eng2.in_Pause = 1;
			break;

		case 'L' : goLeft(&eng2);
			break;

		case 'R' : goRight(&eng2);
			break;

		case 'B' : Restart_Recipe(&eng2);
			break;

		case 'N' :
			break;

		default:
			break;
	}
}

/*
	Function checking if the two engine are on Sync mode
*/
void Sync_Management(void) {
	if(eng1.in_Sync && eng2.in_Sync) {
		eng1.in_Sync = eng2.in_Sync = 0;
	}
}


/*
	Interruption function called every 100ms
	Update the recipes
*/
void* Update_Recipe(void* data) {

   struct sigevent         event;
   struct itimerspec       itime;
   timer_t                 timer_id;
   int                     chid;
   int                     rcvid;
   my_message_t            msg;

   chid = ChannelCreate(0);

   event.sigev_notify = SIGEV_PULSE;
   event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
									chid,
									_NTO_SIDE_CHANNEL, 0);
   event.sigev_priority = getprio(0);
   event.sigev_code = MY_PULSE_CODE;
   timer_create(CLOCK_REALTIME, &event, &timer_id);

   // Timer Init with 100ms period
   itime.it_value.tv_sec = 0;
   itime.it_value.tv_nsec = 100000000;
   itime.it_interval.tv_sec = 0;
   itime.it_interval.tv_nsec = 100000000;
   timer_settime(timer_id, 0, &itime, NULL);

   while(1) {
	   rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
	   if (rcvid == 0) {
			if (msg.pulse.code == MY_PULSE_CODE) {

				Engine_Management(&eng1);
				Engine_Management(&eng2);

				Sync_Management();
			}
	   }
   }
}

/*
	Thread function that creates a signal with a 20ms period and a width depending of the servo's position.
	Input: take an Engine* struct as parameter in order to know on which port generate the PWM
*/
void* Generate_PWM(void* data) {

	/* Give this thread root permissions to access the hardware */
	int privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		pthread_exit((void*)pthread_self());
	}

	Engine* e = (Engine*) data;
	struct timespec* upTime = &(e->upTime);
	uintptr_t data_handler = e->data_handler;

	struct sigevent         event;
	struct itimerspec       itime;
	timer_t                 timer_id;
	int                     chid, rcvid;
	my_message_t            msg;

	chid = ChannelCreate(0);

	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
										chid,
										_NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = getprio(0);
	event.sigev_code = MY_PULSE_CODE;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

	// Timer init with 20ms period
	itime.it_value.tv_sec = 0;
	itime.it_value.tv_nsec = 20000000;
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_nsec = 20000000;
	timer_settime(timer_id, 0, &itime, NULL);

	while(1) {
		 rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		 if (rcvid == 0) {
			  if (msg.pulse.code == MY_PULSE_CODE) {
				  pthread_mutex_lock(&mutex_writingPWM);
				  out8(data_handler, 0xFF);
				  nanospin(upTime);
				  out8(data_handler, 0);
				  pthread_mutex_unlock(&mutex_writingPWM);
			  }
		 }
	}

    pthread_exit(NULL);
}
