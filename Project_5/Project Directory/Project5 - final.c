#include <stdio.h>
#include <unistd.h>       	/* for sleep() */
#include <stdint.h>       	/* for uintptr_t */
#include <hw/inout.h>     	/* for in*() and out*() functions */
#include <sys/neutrino.h> 	/* for ThreadCtl() */
#include <sys/mman.h>     	/* for mmap_device_io() */
#include <sys/syspage.h>
#include <pthread.h>
#include <math.h>			/* for min() and max() */
#include <termios.h>


/* The Neutrino IO port used here corresponds to a single register, which is one byte long */
#define PORT_LENGTH 1

/* The base register address is 0x280 */
#define BASE_ADDRESS 	(0x280)
#define CONFIG_ADDRESS 	(BASE_ADDRESS+11)
#define PORTA_ADDRESS	(BASE_ADDRESS+8)
#define PORTB_ADDRESS	(BASE_ADDRESS+9)

/* Functions prototypes */
void InitPorts();
void InitThreads();
void ExitThreads();
void* send_pulse(void* data);
void* receive_echo(void* data);

/* Globale variables */
uintptr_t ctrl_handle;
uintptr_t data_handle_A;
uintptr_t data_handle_B;
pthread_t thread_send_pulse, thread_receive_echo;
int sensor_activated = 0;
int time_start_measurement = 0;
int min_distance = 1000, max_distance = 0;
uint64_t cps;
uint64_t max_accuracy, min_accuracy;


/*
	Main function of the program launching the different threads and receiving the user's entry
*/
int main( )
{
	/* Give this thread root permissions to access the hardware */
	int privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return -1;
	}

	/* Time base */
	cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	min_accuracy = cps / 4000;
	max_accuracy = cps / 100;

	/* Initialize port A and B */
	InitPorts();

	/* User interaction */
    printf("\n\nPress 's' to start the sensor (then 'e' to exit): ");
	char c;
    while((c = getchar()) != 'q')
    {
		if ( c == 's' && !sensor_activated){
			InitThreads();
		}
		else if(c == 'e' && sensor_activated)
		{
			ExitThreads();
			printf("\n\nPress 's' to start the sensor (then 'e' to exit): ");
		}
		else if(c == '\n') {}
		else{
			printf("\nEnter a correct character");
			printf("\n\nPress 's' to start the sensor (then 'e' to exit): ");
		}
    }

	if(sensor_activated) {
		ExitThreads();
	}

	printf("\nEnd of the program");

	return 0;
}

/*
	Function in charge of initialazing the ports.
	PORT A used as an output 	(Trigger)
	PORT B used as an input 	(Echo)
*/
void InitPorts() {
	/* Get a handle to the parallel port's Control register */
	ctrl_handle = mmap_device_io( PORT_LENGTH, CONFIG_ADDRESS );
	/* Initialize the parallel port with port A as an output (Trigger) and port B as an input (Echo) */
	int DIO_conf = in8(ctrl_handle);
	out8( ctrl_handle, (DIO_conf & ~0x10) | 0x02 );

	/* Get a handle to the parallel port's DIO registers */
	data_handle_A = mmap_device_io( PORT_LENGTH, PORTA_ADDRESS );
	data_handle_B = mmap_device_io( PORT_LENGTH, PORTB_ADDRESS );

	/* Initialization of the ports */
	out8(data_handle_A, 0x00);
}

/*
	Funnction in charge of creating the different threads.
*/
void InitThreads() {
	sensor_activated = 1;
	int ret;
	ret = pthread_create( &thread_send_pulse, NULL, send_pulse, NULL);
	ret += pthread_create( &thread_receive_echo, NULL, receive_echo, NULL);
	if(ret) {
		printf("\nProblem with the thread creation.");
		sensor_activated = 0;
	}
}

/*
	Function waiting for every thread to exit before printing the result.
	Then it reset the max and min distances.
*/
void ExitThreads() {
	sensor_activated = 0;
	pthread_join(thread_send_pulse, NULL);
	pthread_join(thread_receive_echo, NULL);

	printf("\n\n\tMin distance : %d inches\n\tMax Distance : %d inches\n", min_distance, max_distance);
	min_distance = 1000;
	max_distance = 0;
}

/*
	Thread function in charge of sending a 10µs pulse on the port A every 100ms
*/
void* send_pulse(void* data) {
	/* Give this thread root permissions to access the hardware */
	int privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	}
	else {
		/* Send a pulse on the trigger pin */
		while(sensor_activated) {
			time_start_measurement = ClockCycles( );
			out8(data_handle_A, 0x01);
			usleep(10);
			out8(data_handle_A, 0x00);
			while((ClockCycles( ) - time_start_measurement) < cps / 10 );	// every 100ms
		}
	}
}

/*
	Thread function in charge of measuring the width of the echo on port B 
	and printing the right result
*/
void* receive_echo(void* data) {
	/* Give this thread root permissions to access the hardware */
	int privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	}
	else {
		uint64_t time_echo_up, time_echo_down;

		while(sensor_activated) {
			/* Wait for the echo */
			while(!(in8(data_handle_B)&0x01));	// Use pin 1 of port B
			time_echo_up = ClockCycles( );
			while((in8(data_handle_B)&0x01) );
			time_echo_down = ClockCycles( );

			uint64_t res = (time_echo_down - time_echo_up);

			if(res < min_accuracy  || res > max_accuracy ) {
				printf("\n\tdistance: **********" );
			}
			else {
				int val = (res*6750/cps);
				printf("\n\tdistance: %d inches", val);
				min_distance = min(min_distance, val);
				max_distance = max(max_distance, val);
			}
		}
	}
}
