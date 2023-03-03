#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "xstatus.h"
#include "xparameters.h"
#include "platform.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xil_types.h"
#include "xil_io.h"

/* Definitions for hard peripherals attached to the Cortex A9 (e.g., interrupts) */
#include "xparameters_ps.h"

/* Canonical device ID definitions from xparameters.h */
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define GIC_DEVICE_ID		XPAR_SCUGIC_0_DEVICE_ID

/* Triple timer and watchdog timers for each processor are also available */
#define TIMER_INTR_ID       XPS_SCU_TMR_INT_ID

/* SLCR definitions */
#define SLCR_BASE_ADDR      0xF8000000
#define CLK_621_TRUE        0x000001C4

#define CLK_400_NS          2.500
#define CLK_300_NS          3.333

#define ASCII_ESC	        27

#define MAX_TIMER_COUNT     10

/* Function declarations */
void MemCleanup(void *ptra, void *ptrb);
int SetupTimerSystem(XScuTimer *Timer, XScuTimer_Config *TimerConfig);
int SetupIntrSystem(XScuGic *Gic, XScuGic_Config *GicConfig);
static void TimerIntrHandler(void *CallBackRef);

/* Reimplementing a few of the XScuTimer driver functions that do not work in the 7.0 SDK driver */
uint32_t Timer_GetLoadReg(XScuTimer *Timer) {return Xil_In32((Timer->Config.BaseAddr) + (XSCUTIMER_LOAD_OFFSET));}
uint32_t Timer_GetCounterReg(XScuTimer *Timer){return Xil_In32((Timer->Config.BaseAddr) + (XSCUTIMER_COUNTER_OFFSET));}

/*
 * Per Cortex-A9 TRM r4p0 we get the following bit assignments
 *
 * [31:16]      UNK / SBZP
 * [15:8]       Prescaler
 * [7:3]        UNK / SBZP
 * [2]          IRQ enable (if set, ID 29 is set as pending)
 * [1]          Auto-reload mode
 * [0]          Timer enable
 */
uint32_t Timer_GetControlReg(XScuTimer *Timer){return Xil_In32((Timer->Config.BaseAddr) + (XSCUTIMER_CONTROL_OFFSET));}

static int TimerCount = 0;

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

	Xil_ExceptionInit();
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
	} else {
		return XST_FAILURE;
	}
}

static void TimerIntrHandler(void *CallBackRef)
{
	XScuTimer *Timer = (XScuTimer *) CallBackRef;
	XScuTimer_ClearInterruptStatus(Timer);
	printf("Timer expired, count = %d\n", ++TimerCount);
	fflush(stdout);

	return;
}

void SetTimerDuration(XScuTimer *Timer, uint32_t sec, int one_shot)
{
	/*
	 * All private timers and watchdog timers are always clocked at 1/2 of
	 * the CPU frequency (CPU_3x2x). Per chapter 25 of UG585:
	 *
	 *  - 6:2:1 CPU_3x2x = 400MHz
	 *  - 4:2:1 CPU_3x2x = 300MHz
	 *
	 * Determine the 32-bit private timer period based on the clocking ratio, then
	 * calculate the value to load into the timer. Ignore the prescaler for now.
	 *
	 * Some of the language is a bit confusing - the phrase "half the CPU
	 * frequency (CPU_3x2x)" really means that the timer is clocked at
	 * whatever frequency the CPU_3x2c clock is run at, which is half of the
	 * CPU_6x4x clock.  Table 25-1 in the TRM and section 25.2 on "CPU
	 * Clock" make these relationships explicit and are well worth a bit of
	 * study if it is unclear which portions of the device are clocked at
	 * which rates.
	 */

	uint32_t clk_ratio_mode = 0;
	uint32_t tick_period_ns = 0;
	uint32_t ticks = 0;

	/* Shut everything up before touching the timer */
	XScuTimer_Stop(Timer);
	XScuTimer_DisableInterrupt(Timer);
	XScuTimer_ClearInterruptStatus(Timer);
	printf("Load reg\t\t\t0x%08"PRIx32"\n", Timer_GetLoadReg(Timer));
	printf("Counter reg\t\t\t0x%08"PRIx32"\n", Timer_GetCounterReg(Timer));
	printf("Control reg\t\t\t0x%08"PRIx32"\n", Timer_GetControlReg(Timer));

	/* Determine the CPU clock ratio mode and tick duration */
	clk_ratio_mode = Xil_In32((SLCR_BASE_ADDR) + (CLK_621_TRUE)) & 0x00000001;
	if (clk_ratio_mode) {
		printf("CPU clock ratio mode\t\t6:2:1\n");
		tick_period_ns = CLK_400_NS;
	} else {
		printf("CPU clock ratio mode\t\t4:2:1\n");
		tick_period_ns = CLK_300_NS;
	}
	ticks = (uint32_t) (sec * (1e9) / tick_period_ns);

	if (one_shot) {
		printf("Auto reload disabled\n");
		XScuTimer_DisableAutoReload(Timer);
	} else {
		printf("Enabling auto reload\n");
		XScuTimer_EnableAutoReload(Timer);
	}
	XScuTimer_LoadTimer(Timer, ticks);
	printf("Load reg\t\t\t0x%08"PRIx32"\n", Timer_GetLoadReg(Timer));
	printf("Counter reg\t\t\t0x%08"PRIx32"\n", Timer_GetCounterReg(Timer));
	printf("Control reg\t\t\t0x%08"PRIx32"\n", Timer_GetControlReg(Timer));

	return;
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
	 *  - Connect the timer and interrupt subsystems together
	 *  - Configure the timer in one-shot mode for N seconds
	 *  - Enable interrupts in the interrupt subsystem
	 *  - Enable timer interrupts
	 *  - Start the timer
	 *  - ....
	 */

	int Status;

	XScuTimer_Config *TimerConfig = NULL;
	XScuTimer *Timer = NULL;

	XScuGic_Config *GicConfig = NULL;
	XScuGic *Gic = NULL;

	/* Enable PS7 cache and UART */
	init_platform();

	printf("%c[2J", ASCII_ESC);
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
	fprintf(stdout, "Initialize private timer subsystem\t\t");
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

	/*
	 * Register the GIC interrupt handler with the exception handling logic
	 * within the ARM Cortex-A9 processor
	 *
	 * This is basically going to register a specific type of exception (an
	 * interrupt) with the exception handler of the processor (this probably
	 * gets a bit more complicated when you have more than one core, but for
	 * now we'll stick with it).  To do this, we need to know three things
	 *  - The exception ID for the kinds of interrupts we wish to register
	 *    with the processor (there could be different kinds)
	 *  - The handler (in this case, an interrupt handler) for this kind of
	 *    exception
	 *  - What data to pass to the handler when it gets called
	 *
	 * The first is relatively easy - in `xil_exception.h` there are a number
	 * of exception types defined for various architectures. For regular IRQ
	 * instead of fast IRQ (which are unique to ARM) we see that the desired
	 * definition is XIL_EXCEPTION_ID_IRQ_INT, which is just the integer value
	 * that the exception handler in the processor has assigned to general
	 * interrupts.  Later, we'll probably want to use fast IRQ for things like
	 * receiving data from a network interface.  FIQ take precedence over general
	 * IRQ and only one FIQ source is supported at a time.
	 *
	 * The interrupt handler that we wish to register with the processor's
	 * exception handler (i.e., the function that we want to stick in the
	 * processor's exception vector table) is going to be defined by the GIC
	 * driver API.
	 *
	 * Lastly, the data that is going to be passed to the handler is also
	 * described in the GIC driver API, namely, the driver instance itself.
 	 */

	Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			Gic);

	/* Disable private timer interrupts until it has been configured */
	XScuGic_Disable(Gic, TIMER_INTR_ID);

	/* Connect the timer interrupt handler with the GIC */
	XScuGic_Connect(
			Gic,
			TIMER_INTR_ID,
			(Xil_ExceptionHandler) TimerIntrHandler,
			(void *) Timer);

	/* Configure the timer in auto-reload mode for 1 second */
	SetTimerDuration(Timer, 1, 0);

	/* Enable interrupts from the timer subsystem */
	XScuTimer_EnableInterrupt(Timer);
	/* Enable timer interrupts within the GIC */
	XScuGic_Enable(Gic, TIMER_INTR_ID);
	/* Enable interrupts at the Cortex-A9 exception handler */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	printf("Starting timer\n");
	printf("Int = 0x%"PRIx32"\n", XScuTimer_GetInterruptStatus(Timer));
	XScuTimer_Start(Timer);
	while (TimerCount < MAX_TIMER_COUNT) {
		/* Waiting for MAX_TIMER_COUNT interrupts */
	}
	printf("Counted %d interrupts\n", TimerCount);

	MemCleanup(Gic, Timer);
	cleanup_platform();

	return 0;
}

