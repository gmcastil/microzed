#include <stdio.h>
#include <stdlib.h>

#include "xstatus.h"
#include "xparameters.h"
#include "platform.h"

#include "xscutimer.h"
#include "xscugic.h"

#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID

#define ASCII_ESC		27

/* Initialize the timer subsystem and perform a timer self-test */
int SetupTimerSystem(XScuTimer *Timer, XScuTimer_Config *TimerConfig)
{
	int Status = 0;

	TimerConfig = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	if (TimerConfig == NULL) {
		fprintf(stderr, "Failed to lookup configuration for timer device ID %d\n",
				TIMER_DEVICE_ID);
		return XST_FAILURE;
	}

	Status = XScuTimer_CfgInitialize(Timer, TimerConfig, TimerConfig->BaseAddr);
	if (Status == XST_DEVICE_IS_STARTED) {
		fprintf(stdout, "Timer device ID %d already started\n",
				TIMER_DEVICE_ID);
		return XST_FAILURE;
	} else {
		Status = XScuTimer_SelfTest(Timer);
		if (Status == XST_FAILURE) {
			fprintf(stderr, "Self test failed for timer device ID %d\n",
					TIMER_DEVICE_ID);
			return XST_FAILURE;
		} else {
			return XST_SUCCESS;
		}
	}
}

int main(int args, char *argv[])
{
	/* 
	 * Example using interrupt subsystem and the Cortex A9 private timer
	 *
	 * Start a timer for N seconds and display an interrupt-based message when
	 * finished (one-shot mode).  Then start a timer in auto-reload mode
	 * which prints out a status message each time an interrupt is
	 * generated.  After N seconds, disable the timer and stop counting.
	 *
	 * Procedure:
	 *  - Initialize the interrupt subsystem and perform a self test.
	 *  - Disable interrupts until everything is set up
	 *  - Initialize the timer subsystem and perform a self test
	 *  - Configure the timer in one-shot mode for N seconds
	 *  - Enable interrupts in the interrupt subsystem
	 *  - Enable timer interrupts
	 *  - Start the timer
	 *  - ....
	 */

	int Status;

	XScuTimer_Config *TimerConfig;
	XScuTimer *Timer;

	XScuGic_Config *GicConfig;
	XScuGic *Gic;

	/* Enable PS7 cache and UART */
	init_platform();

	printf("%s[2J", ASCII_ESC);
	printf("Private Timer Examples\n");
	printf("----------------------\n");

	/* Create a private timer instance */
	Timer = malloc(sizeof(XScuTimer));
	if (Timer == NULL) {
		fprintf(stderr, "Could not allocate memory for timer subsystem\n");
		return XST_FAILURE;
	}
	Status = SetupTimerSystem(Timer, TimerConfig);
	fprintf(stdout, "Initialize private timer subsystem\t\t\t");
	if ( Status == XST_FAILURE ) {
		fprintf(stdout, "FAILED\n");
		free(Timer);
	} else {
		fprintf(stdout, "SUCCESS\n");
	}


























	/* Create a GIC driver instance */
	Gic = malloc(sizeof(XScuGic));
	if (Gic == NULL) {
		fprintf(stderr, "Could not allocate memory for GIC instance\n");
		return XST_FAILURE;
	}
	GicConfig = XScuGic_LookupConfig(






	return 0;
}

