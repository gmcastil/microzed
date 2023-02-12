#include <stdio.h>
#include <stdlib.h>

#include "xstatus.h"
#include "xparameters.h"
#include "platform.h"

#include "xscutimer.h"
#include "xscugic.h"

/* Canonical device ID definitions from xparameters.h */
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define GIC_DEVICE_ID		XPAR_SCUGIC_0_DEVICE_ID

#define ASCII_ESC		27

void MemCleanup(void *ptra, void *ptrb)
{
	if (ptra != NULL) {
		free(ptra);
	}
	if (ptrb != NULL) {
		free(ptrb);
	}
	return;
}

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
		fprintf(stderr, "Timer device ID %d already started\n",
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

/* Initialize the GIC interrupt subsystem and perform a GIC self-test */
int SetupIntrSystem(XScuGic *Gic, XScuGic_Config *GicConfig)
{
	int Status = 0;

	GicConfig = XScuGic_LookupConfig(GIC_DEVICE_ID);
	if (GicConfig == NULL) {
		fprintf(stderr, "Failed to lookup configuration for GIC ID %d\n",
				GIC_DEVICE_ID);
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(Gic, GicConfig, GicConfig->CpuBaseAddress);
	/* 
	 * GIC driver documentation is unclear as to how failure to initialize is
	 * handled, so check for XST_SUCCESS rather than XST_FAILURE
	 */
	if (Status == XST_SUCCESS) {
		Status = XScuGic_SelfTest(Gic);
		if (Status == XST_FAILURE) {
			fprintf(stderr, "Self test failed for GIC ID %d\n",
					GIC_DEVICE_ID);
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
	XScuTimer *Timer = NULL;

	XScuGic_Config *GicConfig;
	XScuGic *Gic = NULL;

	/* Enable PS7 cache and UART */
	init_platform();

	printf("%s[2J", ASCII_ESC);
	printf("Private Timer Examples\n");
	printf("----------------------\n");

	/* Create a private timer instance */
	Timer = malloc(sizeof(XScuTimer));
	if (Timer == NULL) {
		fprintf(stderr, "Could not allocate memory for timer subsystem\n");
		MemCleanup(Gic, Timer);
		return XST_FAILURE;
	}
	Status = SetupTimerSystem(Timer, TimerConfig);
	fprintf(stdout, "Initialize private timer subsystem\t\t\t");
	if (Status == XST_FAILURE) {
		fprintf(stdout, "FAILED\n");
		MemCleanup(Gic, Timer);
		return XST_FAILURE;
	} else {
		fprintf(stdout, "COMPLETE\n");
	}

	/* Create a generic interrupt controller instance */
	Gic = malloc(sizeof(XScuGic));
	if (Gic == NULL) {
		fprintf(stderr, "Could not allocate memory for GIC subsystem\n");
		MemCleanup(Gic, Timer);
		return XST_FAILURE;
	}
	Status = SetupIntrSystem(Gic, GicConfig);
	fprintf(stdout, "Initialize GIC subsystem\t\t\t");
	if (Status == XST_FAILURE) {
		fprintf(stdout, "FAILED\n");
		MemCleanup(Gic, Timer);
		return XST_FAILURE;
	} else {
		fprintf(stdout, "COMPLETE\n");
	}

	return 0;
}

