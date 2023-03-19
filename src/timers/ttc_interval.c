#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/*
 * - Rewrite interrupt handler to use a switch statement for the different kinds of interrupts
 * - TTC setup for interval mode, decrement, generating interrupts, interval / prescaler
 */

/* Required libraries for interrupt handling */
#include "xscugic.h"
#include "xil_exception.h"

/* Triple timer counter libraries */
#include "xttcps.h"

#include "platform.h"
#include "sleep.h"

/* Functions for summarizing TTC output registers to stdout */
#include "ttc_dbg.h"
#include "ps7_dbg.h"

/* Necessary for creating driver instances */
#define GIC_DEVICE_ID				XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TTC_DEVICE_ID				XPAR_XTTCPS_0_DEVICE_ID

/*
 *  This defines the interrupt that the GIC will see, but it could be for several different
 * kinds of interrupts on that particular counter (e.g., match, interval, overflow)
 */
#define TTC0_IRQ_ID					XPS_TTC0_0_INT_ID

#define ASCII_ESC					27
#define EXPIRE_SECONDS				10

static int expired = 0;

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
	int pass = 0;

	Xil_ExceptionHandler check_intr_handler = NULL;
	void *check_data = NULL;

	print_operation("Configuring generic interrupt controller");
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
					pass = 1;
				}
			}
		}
	}
	if ( pass == 1) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}
	return;
}

/* Creates a TTC instance for the requested TTC device ID */
void setup_triple_timer(XTtcPs *ttc, XTtcPs_Config *ttc_config, int device_id)
{
	int status = 0;
	int pass = 0;

	char msg[50];

	snprintf(msg, 50, "Configuring triple timer device ID %d", TTC_DEVICE_ID);
	print_operation(msg);
	ttc_config = XTtcPs_LookupConfig(TTC_DEVICE_ID);
	if ( ttc_config == NULL ) {
		fprintf(stderr, "Could not find configuration for TTC device ID %d\n", TTC_DEVICE_ID);
	} else {
		status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
		if ( status == XST_DEVICE_IS_STARTED ) {
				XTtcPs_Stop(ttc);
				if ( XTtcPs_GetInterruptStatus(ttc) ) {
					fprintf(stderr, "Clearing pending interrupt\n");
					XTtcPs_ClearInterruptStatus(ttc, XTTCPS_IXR_ALL_MASK);
				}
				if ( XTtcPs_IsStarted(ttc) ) {
					fprintf(stderr, "Could not stop TTC device ID %d\n", TTC_DEVICE_ID);
				} else {
					fprintf(stdout, "Reinitializing TTC device ID %d\n", TTC_DEVICE_ID);
					status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
					if ( status == XST_DEVICE_IS_STARTED ) {
						fprintf(stdout, "Could not reinitialize TTC device ID %d\n", TTC_DEVICE_ID);
					}
				}
		}
		if ( status == XST_SUCCESS ) {
			XTtcPs_DisableInterrupts(ttc, XTTCPS_IXR_ALL_MASK);
			pass = 1;
		}
	}
	if ( pass == 1 ) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}
}

void config_ttc_interval(XTtcPs *ttc, uint32_t freq)
{
        /* The interrupt interval is platform dependent, hence the use of typedefs for this */
        XInterval *interval = NULL;
        XInterval check_interval = 0;

        XInterval hack_interval = 48828;
        u8 hack_prescaler = 10;

        u8 *prescaler = NULL;
        u8 check_prescaler = 0;

        uint32_t options = 0;
        uint32_t check_options = 0;

        char str[50];

        if ( ttc == NULL ) {
                fprintf(stderr, "Cannot dereference NULL pointer\n");
        } else {
                /* Calculate and configure the interval settings */
                snprintf(str, 50, "Configuring TTC with interrupt frequency %"PRId32" Hz", freq);
                print_operation(str);
                XTtcPs_CalcIntervalFromFreq(ttc, freq, interval, prescaler);
                interval = &hack_interval;
                prescaler = &hack_prescaler;
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
                /*
                 * Set options appropriately for this configuration - NOTE THAT WE ARE USING AN EXTERNAL CLOCK!
                 * So if your hardware is designed differently, this is the place to make those changes.  Why use
                 * an external clock for this example?  Well, dear reader, its because eventually there will be
                 * an ILA core hanging off the waveform output.  That ILA needs a clock source and the CPU_1x
                 * clock which drives the TTC by default is not available as a fabric clock.
                 */
                options = XTtcPs_GetOptions(ttc);
                options |= XTTCPS_OPTION_DECREMENT
                			| XTTCPS_OPTION_EXTERNAL_CLK
							| XTTCPS_OPTION_INTERVAL_MODE
							| XTTCPS_OPTION_WAVE_DISABLE;
                XTtcPs_SetOptions(ttc, options);
                check_options = XTtcPs_GetOptions(ttc);
                if ( options != check_options ) {
                	fprintf(stderr, "Could not set TTC options\n");
                	fprintf(stderr, "Interval:\t\tExpected: 0x%08"PRIx32"\tReceived: 0x%08"PRIx32"\n",
                            options, check_options);
                }
        }
        return;
}

/* Interrupt handler for TTC */
static void ttc_intr_handler(void *callback_ref)
{
	uint32_t ttc_intr_status = 0;
	XTtcPs *ttc = callback_ref;

	static uint32_t calls = 0;

	if ( ttc == NULL ) {
		fprintf(stderr, "Cannot service interrupt with NULL callback reference\n");
	} else {
		ttc_intr_status = XTtcPs_GetInterruptStatus(ttc) & XTTCPS_IXR_INTERVAL_MASK;
		if ( ttc_intr_status != 0 ) {
			fprintf(stdout, "Received TTC interval interrupt %"PRId32"\n", ++calls);
			XTtcPs_ClearInterruptStatus(ttc, XTTCPS_IXR_INTERVAL_MASK);
			if ( calls == EXPIRE_SECONDS ) {
				expired = 1;
			}
		} else {
			fprintf(stdout, "Received some other TTC interrupt\n");
		}
	}
	fflush(stdout);
	return;
}

int main(int args, char *argv[])
{
	XScuGic *gic = NULL;
	XScuGic_Config *gic_config = NULL;

	XTtcPs *ttc = NULL;
	XTtcPs_Config *ttc_config = NULL;

	gic = malloc(sizeof(XScuGic));
	ttc = malloc(sizeof(XTtcPs));

	init_platform();

	clear_console();

	setup_generic_intr_ctrl(gic, gic_config);
	print_operation("Running GIC self test");
	if ( XScuGic_SelfTest(gic) == XST_SUCCESS ) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}

	setup_triple_timer(ttc, ttc_config, TTC_DEVICE_ID);
	print_operation("Running TTC self test");
	if ( XTtcPs_SelfTest(ttc) == XST_SUCCESS ) {
		print_result("OK");
	} else {
		print_result("FAIL");
	}

	config_ttc_interval(ttc, 1);

	/* Connect an interrupt handler for the IRQ ID of the chosen timer */
	XScuGic_Connect(gic, TTC0_IRQ_ID, (Xil_InterruptHandler) ttc_intr_handler, (void *) ttc);

	/* Enable interrupt exceptions in the processor */
	Xil_ExceptionEnable();
	/* Enable interrupts at the GIC */
	XScuGic_Enable(gic, TTC0_IRQ_ID);
	/* Enable overflow interrupts for the TTC we will be using */
	XTtcPs_EnableInterrupts(ttc, XTTCPS_IXR_INTERVAL_MASK);

	fprintf(stdout, "Starting...\n");
	fprintf(stdout, "Silicon Revision 0x%"PRIx32"\n", (uint32_t) ps7_dbg_get_ps_version());
	ttc_dbg_print_input_freq(ttc);
	ttc_dbg_print_options(ttc);
	ttc_dbg_print_interval(ttc);
	fflush(stdout);

	XTtcPs_Start(ttc);

	/*
	for (i=0; i<EXPIRE_SECONDS; i++) {
		sleep(1);
		printf("Sleeping...%d\n", i+1);
	}
	*/

	for (;;) {
		if (expired) {
			XTtcPs_Stop(ttc);
			break;
		}
	}

	fprintf(stdout, "Finished.\n");
	fflush(stdout);

	free(gic);
	free(ttc);

	cleanup_platform();

	return 0;
}
