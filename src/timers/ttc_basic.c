/*
 * Example to demonstrate the use of the PS TTC as a PWM driver for LED
 *
 * Requirements are to configure a TTC output waveform with a programmable
 * frequency and duty cycle.  The PWM frequency should be between 10Hz and
 * 1kHz, with a duty cycle between 0 and 1 (i.e., hard on or off). The TTC
 * waveform output for whichever timer is being used (e.g., TTC0_WAVE0_OUT)
 * should be tied to an LED, probably via the EMIO interface. For this example
 * the Microzed is installed on an Avnet FMC carrier card with four user
 * LEDs that are accessible from the programmable logic.  One could also use
 * the Microzed PMOD connector and an LED bank such as the Digilent PMOD 8LD.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/* Required libraries for interrupt handling */
#include "xscugic.h"
#include "xil_exception.h"

/* Triple timer counter libraries */
#include "xttcps.h"

#define GIC_DEVICE_ID				XPAR_SCUGIC_SINGLE_DEVICE_ID
/*
 * Since we will likely use more than one of the three TTC available, we will
 * identify them as their intended use case
 */
#define TTC_PWM_DEVICE_ID			XPAR_XTTCPS_0_DEVICE_ID
/*
 * A word of caution here - the six TTC that the Zynq provides are numbered 0-2
 * in some contexts, for core 0 and core 1.  In other contexts, notably interrupt
 * identification, they are numbered 0-5 - so, beware.
 */
#define TTC_PWM_IRQ_ID				XPAR_XTTCPS_0_INTR

#define ASCII_ESC					27

void clear_console()
{
	fprintf(stdout, "%c[2J", ASCII_ESC);
	fflush(stdout);

}
void print_operation(char *str)
{
	fprintf(stdout, "%-50s", str);
	fflush(stdout);
}
void print_result(char *str)
{
	fprintf(stdout, "%10s\n", str);
	fflush(stdout);
}

/* Creates and registers a GIC instance with the Cortex-A9 exception handler */
void setup_generic_intr_ctrl(XScuGic *gic, XScuGic_Config *gic_config)
{
	int status = 0;

	Xil_ExceptionHandler check_intr_handler = NULL;
	void *check_data = NULL;

	gic_config = XScuGic_LookupConfig(GIC_DEVICE_ID);
	if ( gic_config == NULL ) {
		fprintf(stderr, "Could not find configuration for GIC device ID %d\n", GIC_DEVICE_ID);
	} else {
		if ( gic == NULL ) {
			fprintf(stderr, "Cannot dereference NULL pointer\n");
		} else {
			status = XScuGic_CfgInitialize(gic, gic_config, gic_config->CpuBaseAddress);
			if ( ( status != XST_SUCCESS ) || ( gic == NULL ) ) {
				fprintf(stderr, "Could not initialize GIC for GIC device ID %d\n", GIC_DEVICE_ID);
			} else {
				/* Register and confirm the exception handler with the processor */
				Xil_ExceptionRegisterHandler(
						XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, gic);
				Xil_GetExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, &check_intr_handler, &check_data);
				if ( ( check_intr_handler != (Xil_ExceptionHandler) XScuGic_InterruptHandler) || ( check_data != gic ) ) {
					fprintf(stderr, "Could not register GIC instance with CPU exception handler\n");
				} else {
					Xil_ExceptionDisable();
				}
			}
		}
	}
	return;
}

/* Creates a TTC instance for the requested TTC device ID */
void setup_triple_timer(XTtcPs *ttc, XTtcPs_Config *ttc_config, int device_id)
{
	int status = 0;

	ttc_config = XTtcPs_LookupConfig(device_id);
	if ( ttc_config == NULL ) {
		fprintf(stderr, "Could not find configuration for TTC device ID %d\n", device_id);
	} else {
		if ( ttc == NULL ) {
			fprintf(stderr, "Cannot dereference NULL pointer\n");
		} else {
			status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
			if ( ( status != XST_SUCCESS ) || ( ttc == NULL ) ) {
				fprintf(stderr, "Could not initialize TTC for TTC device ID %d\n", device_id);
			} else if ( status == XST_DEVICE_IS_STARTED ) {
				fprintf(stderr, "TTC device ID %d was already started\n", device_id);
			}
		}
	}
	return;
}

/* BROKEN: Configures a TTC as a PWM with initial duty cycle */
void config_timer_pwm_freq(XTtcPs *ttc, uint32_t freq)
// This is just a temp function using frequency - we will later include duty cycle as well
{
	/* The interrupt interval is platform dependent, hence the use of typedefs for this */
	XInterval *interval = NULL;
	XInterval check_interval = 0;

	u8 *prescaler = NULL;
	u8 check_prescaler = 0;

	char str[50];

	/* Dangerous string handling */
	snprintf(str, 50, "Configuring TTC with frequency %"PRId32" Hz", freq);
	print_operation(str);
	if ( ttc == NULL ) {
		fprintf(stderr, "Cannot dereference NULL pointer\n");
	} else {
		// Set the match values for this timer
		// Configure match interrupts for this timer
		XTtcPs_CalcIntervalFromFreq(ttc, freq, interval, prescaler);
		/* One of these is platform dependent, the other is hard coded in the driver source */
		if ( ( *interval == XTTCPS_MAX_INTERVAL_COUNT ) && ( *prescaler == 0xFF ) ) {
			fprintf(stderr, "Unsuccessful TTC interval calculation\n");
		} else {
			XTtcPs_SetInterval(ttc, *interval);
			XTtcPs_SetPrescaler(ttc, *prescaler);
			check_interval = XTtcPs_GetInterval(ttc);
			check_prescaler = XTtcPs_GetPrescaler(ttc);
			if ( ( check_interval != *interval ) || ( check_prescaler != *prescaler ) ) {
				print_result("FAIL");
				fprintf(stderr, "Could not configure TTC interval or prescaler\n");
				fprintf(stderr, "Interval:\t\tExpected: 0x%08"PRIx32"\tReceived: 0x%08"PRIx32"\n",
						(uint32_t) *interval, (uint32_t) check_interval);
				fprintf(stderr, "Prescaler:\t\tExpected: 0x%02"PRIx8"\tReceived: 0x%02"PRIx8"\n",
						*prescaler, check_prescaler);
			} else {
				print_result("OK");
			}
		}
	}
	return;
}

/* Interrupt handler for TTC */
static void ttc_intr_handler(void *callback_ref)
{
	uint32_t ttc_intr_status = 0;
	XTtcPs *ttc_pwm = callback_ref;

	static uint32_t calls = 0;

	if ( ttc_pwm == NULL ) {
		fprintf(stderr, "Cannot service interrupt with NULL callback reference\n");
	} else {
		ttc_intr_status = XTtcPs_GetInterruptStatus(ttc_pwm) & XTTCPS_IXR_CNT_OVR_MASK;
		if ( ttc_intr_status != 0 ) {
			fprintf(stdout, "Received TTC overflow interrupt %"PRId32"\n", ++calls);
			XTtcPs_ClearInterruptStatus(ttc_pwm, XTTCPS_IXR_CNT_OVR_MASK);
		} else {
			fprintf(stdout, "Received some other TTC interrupt\n");
		}
	}
	fflush(stdout);
	return;
}

int main(int args, char *argv[])
{

	uint32_t ttc_options = 0;

	XScuGic *gic = NULL;
	XScuGic_Config *gic_config = NULL;

	XTtcPs *ttc_pwm = NULL;
	XTtcPs_Config *ttc_pwm_config = NULL;

	gic = malloc(sizeof(XScuGic));
	ttc_pwm = malloc(sizeof(XTtcPs));

	clear_console();

	print_operation("Configuring generic interrupt controller");
	setup_generic_intr_ctrl(gic, gic_config);
	/*
	 * GIC self test code is safe since it's just reading and comparing an
	 * ID from the distributor register (per the SDK source code). Interestingly,
	 * the ARM documentation does not seem to contain references to the offset
	 * that is being addressed and the `xscugic_selftest.c` source code indicates
	 * that some of the offsets may indeed be reserved by the vendor
	 */
	if ( XScuGic_SelfTest(gic) == XST_SUCCESS ) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}

	print_operation("Configuring PWM triple timer counter");
	setup_triple_timer(ttc_pwm, ttc_pwm_config, TTC_PWM_DEVICE_ID);
	/*
	 * Triple timer counter self test code is safe, since it just tests to see if the control
	 * register values are their post-reset values
	 */
	if ( XTtcPs_SelfTest(ttc_pwm) == XST_SUCCESS ) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}

	/* Connect an interrupt handler for the IRQ ID of the chosen timer */
	XScuGic_Connect(gic, TTC_PWM_IRQ_ID, (Xil_InterruptHandler) ttc_intr_handler, (void *) ttc_pwm);
	/* Enable interrupt exceptions in the processor */
	Xil_ExceptionEnable();
	/* Enable interrupts at the GIC */
	XScuGic_Enable(gic, TTC_PWM_IRQ_ID);
	/* Enable overflow interrupts for the TTC we will be using */
	XTtcPs_EnableInterrupts(ttc_pwm, XTTCPS_IXR_CNT_OVR_MASK);
	/*
	 * Enable the wave out for the chosen timer
	 * Enable the counter through the control register
	 */
	ttc_options = XTtcPs_GetOptions(ttc_pwm) | XTTCPS_CNT_CNTRL_EN_WAVE_MASK;
	XTtcPs_SetOptions(ttc_pwm, ttc_options);

	XTtcPs_Start(ttc_pwm);

	for (;;) {};

	free(gic);
	free(ttc_pwm);

	return 0;
}
