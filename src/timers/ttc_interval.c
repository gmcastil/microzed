#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/* Required libraries for interrupt handling */
#include "xscugic.h"
#include "xil_exception.h"

/* Triple timer counter libraries */
#include "xttcps.h"

#include "platform.h"
#include "sleep.h"

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
#define EXPIRE_SECONDS				100

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

void setup_triple_timer(XTtcPs *ttc, XTtcPs_Config *ttc_config, int device_id)
{
       int status = 0;
       int pass = 0;

       char msg[50];

       snprintf(msg, 50, "Configuring triple timer device ID %d", device_id);
       print_operation(msg);
       ttc_config = XTtcPs_LookupConfig(device_id);
       if ( ttc_config == NULL ) {
               fprintf(stderr, "Could not find configuration for TTC device ID %d\n", device_id);
       } else {
               status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
               /*
                * In cases where the application was relaunched, the timer will still be running
                * and the configuration will indicate this, so we stop the timer, clear any pending
                * interrupts, and then make sure that the TTC is in the factory state before we
                * conclude the setup routine.
                */
               if ( status == XST_DEVICE_IS_STARTED ) {
            	   XTtcPs_Stop(ttc);
            	   if ( XTtcPs_GetInterruptStatus(ttc) ) {
            		   fprintf(stderr, "Cleared pending interrupt\n");
            		   XTtcPs_ClearInterruptStatus(ttc, XTTCPS_IXR_ALL_MASK);
            	   }
            	   /* Tried to stop the timer but it is still running */
            	   if ( XTtcPs_IsStarted(ttc) ) {
            		   fprintf(stderr, "Could not stop TTC device ID %d\n", device_id);
            	   } else {
            		   fprintf(stdout, "Reinitializing TTC device ID %d\n", device_id);
            		   status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
            		   if ( status == XST_DEVICE_IS_STARTED ) {
            			   fprintf(stdout, "Could not reinitialize TTC device ID %d\n", device_id);
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
       return;
}

void config_ttc_interval(XTtcPs *ttc, int duration)
{
	/* We hard code the 1 second we are trying to get for now */
	uint16_t interval = 0xBEBC;
	uint8_t prescaler = 8;

	XTtcPs_SetPrescaler(ttc, prescaler);
	if ( XTtcPs_GetPrescaler(ttc) != prescaler ) {
		fprintf(stderr, "Could not set prescaler value\n");
	}
	XTtcPs_SetInterval(ttc, interval);
	if ( XTtcPs_GetInterval(ttc) != interval ) {
		fprintf(stderr, "Could not set interval value\n");
	}
	/*
	 * In a world where the standalone drivers could be trusted, this is probably
	 * a good place to turn on interval mode, but alas, it is not.
	 */
	return;
}

/* Handler for TTC interrupts */
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
	/* Storage for reading and writing values to control registers */
	uint32_t val32 = 0;

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

	/* Connect an interrupt handler for the IRQ ID of the chosen timer */
	XScuGic_Connect(gic, TTC0_IRQ_ID, (Xil_InterruptHandler) ttc_intr_handler, (void *) ttc);
	/* Enable interrupt exceptions in the processor */
	Xil_ExceptionEnable();
	/* Enable interrupts at the GIC */
	XScuGic_Enable(gic, TTC0_IRQ_ID);
	/* Enable overflow interrupts for the TTC we will be using */
	XTtcPs_EnableInterrupts(ttc, XTTCPS_IXR_INTERVAL_MASK);

	fprintf(stdout, "Starting...\n");
	fprintf(stdout, "%-20s0x%01"PRIx32"\n", "Silicon Rev", ps7_dbg_get_ps_version());
	fprintf(stdout, "%-20s0x%02"PRIx32"\n", "Manufacturer ID", ps7_dbg_get_mfr_id());
	fprintf(stdout, "%-20s0x%02"PRIx32"\n", "Device Code", ps7_dbg_get_device_code());

	ttc_dbg_print_input_freq(ttc);
	ttc_dbg_print_interval(ttc);
	ttc_dbg_print_options(ttc);

	fprintf(stdout, "%-20s0x%08"PRIx32"\n", "CLK 0 CTRL", ttc_dbg_clk_ctrl(0, 0));
	fprintf(stdout, "%-20s0x%08"PRIx32"\n", "CNT 0 CTRL", ttc_dbg_cnt_ctrl(0, 0));
	ttc_dbg_print_summary(0, 0);

	fprintf(stdout, "Configuring TTC%d for 1 second in interval mode\n", TTC_DEVICE_ID);
	config_ttc_interval(ttc, 1);
	/*
	 * Now we need to manually set the clock and count control registers because the
	 * standalone driver does not
	 * - Set an external clock and keep the prescale and prescale enable bits untouched
	 * - Enable interval mode in the count controller and leave everything else alone
	 */

	val32 = ttc_dbg_clk_ctrl(0, 0);
	/* Clock source is determined by bit 5 in the clock control register */
	val32 |= (0x0020);
	ttc_dbg_set_clk_ctrl(0, 0, val32);


	val32 = ttc_dbg_cnt_ctrl(0, 0);
	/* Interval mode is determined by bit 1 in the counter control register */
	val32 |= (0x0002);
	ttc_dbg_set_cnt_ctrl(0, 0, val32);
	printf("Starting timer...\n");
	XTtcPs_Start(ttc);

	ttc_dbg_print_summary(0, 0);
	for (;;) {
		if ( expired == 1 ) {
			break;
		}
	}
	printf("Done\n");

	free(gic);
	free(ttc);

	cleanup_platform();

	return 0;
}
