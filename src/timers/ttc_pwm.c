/*
 *	ttc_pwm.c - Zynq-7000 PWM generator based on Xilinx triple timer counter (TTC)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/* Libraries required for interrupt handling */
#include "xscugic.h"
#include "xil_exception.h"

/* Xilinx triple timer counter libraries */
#include "xttcps.h"
#include "platform.h"

/* Debug and workaround codes */
#include "ttc_dbg.h"
#include "ps7_dbg.h"

/* Necessary for creating driver instances */
#define GIC_DEVICE_ID				XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TTC_DEVICE_ID				XPAR_XTTCPS_0_DEVICE_ID

/*
 * This defines the interrupt that the GIC will see, but it could be for several different
 * kinds of interrupts on that particular counter (e.g., match, interval, overflow)
 */
#define TTC0_IRQ_ID					XPS_TTC0_0_INT_ID

#define ASCII_ESC					27
#define EXPIRE_SECONDS				4

//#define TTC_DEBUG

static uint32_t calls = 0;

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

       char msg[50];
       snprintf(msg, 50, "Configuring triple timer device ID %d", device_id);
       print_operation(msg);
       ttc_config = XTtcPs_LookupConfig(device_id);
       if ( ttc_config == NULL ) {
               fprintf(stderr, "Could not find configuration for TTC device ID %d\n", device_id);
       } else {
    	   	   ttc_dbg_rst(0, 0);
               status = XTtcPs_CfgInitialize(ttc, ttc_config, ttc_config->BaseAddress);
               /*
                * In cases where the application was relaunched, the timer will still be running
                * and the configuration will indicate this, so we stop the timer, clear any pending
                * interrupts, and then make sure that the TTC is in the factory state before we
                * conclude the setup routine.
                */
               if ( status == XST_SUCCESS ) {
            	   print_result("OK");
               } else {
            	   print_result("FAIL");
               }
       }
       return;
}

void config_pwm(XTtcPs *ttc, uint16_t window, uint16_t duty_cycle, uint8_t prescaler)
{
	uint32_t val32 = 0;

	/*
	 * Steps to configure PWM
	 * - Set the prescaler (for both the match and interval regs)
	 * - Set match 1 reg to duty cycle
	 * - Set interval reg to window
	 * - Enable the external 25MHz clock and the prescaler
	 * - Enable match mode (this is where the XTtcPs API breaks down), enable the
	 *   output waveform (active low), and waveform polarity however desired
	 */
	XTtcPs_SetPrescaler(ttc, prescaler);
	if ( XTtcPs_GetPrescaler(ttc) != prescaler ) {
		fprintf(stderr, "Could not set prescaler value\n");
	}
	XTtcPs_SetInterval(ttc, window);
	if ( XTtcPs_GetInterval(ttc) != window ) {
		fprintf(stderr, "Could not set interval value\n");
	}
	/*
	 * The fact that the Xilinx TTC API frequently switches from 0 to 1-indexing
	 * when referencing match registers is absolutely stupid
	 */
	XTtcPs_SetMatchValue(ttc, 0, duty_cycle);
	if ( XTtcPs_GetMatchValue(ttc, 0) != duty_cycle ) {
		fprintf(stderr, "Could not set match 1 value\n");
	}
	/*
	 * The prescaler and prescaler enable bits should already have been taken care
	 * of by the TTC API (a sane person would probably check that themselves) but
	 * the clock source needs to be changed manually
	 */
	val32 = ttc_dbg_clk_ctrl(0, 0);
	/* Clock source is determined by bit 5 in the clock control register */
	val32 |= (0x0020);
	ttc_dbg_set_clk_ctrl(0, 0, val32);
	if ( ttc_dbg_clk_ctrl(0, 0) != val32 ) {
		fprintf(stderr, "Could not set clock control\n");
	}

	/* Counter options to select are:
	 *
	 * Bit		Desc
	 * ---		----
	 * 6		Waveform polarity (set is H-L on match interrupt)
	 * 5		Output waveform enable (active low)
	 * 4		Counter reset
	 * 3		Match mode
	 * 2		Decrement mode
	 * 1		Interval mode
	 * 0		Disabled
	 *
	 * Value = 0b1001000 = 0x48
	 * Value = 0b1001010 = 0x4A
	 */
	val32 = 0x4A;
	ttc_dbg_set_cnt_ctrl(0, 0, val32);
	if ( ttc_dbg_cnt_ctrl(0, 0) != val32 ) {
		fprintf(stderr, "Could not set counter control\n");
	}
	XTtcPs_ClearInterruptStatus(ttc, XTTCPS_IXR_ALL_MASK);
}

/* Handler for all TTC interrupts */
static void ttc_intr_handler(void *callback_ref)
{
	XTtcPs *ttc = callback_ref;

	if ( ttc == NULL ) {
		fprintf(stderr, "Cannot service interrupt with NULL callback reference\n");
	} else {
		XTtcPs_ClearInterruptStatus(ttc, XTTCPS_IXR_INTERVAL_MASK | XTTCPS_IXR_MATCH_0_MASK);
		printf(".");
		calls++;
		#ifdef TTC_DEBUG
		fprintf(stdout, "0x%08"PRIx32"\n", XTtcPs_GetInterruptStatus(ttc));
		printf("Interrupt Handler\n");
		printf("-----------------\n");
		printf("Interval counter == 0x%04"PRIx16"\n", XTtcPs_GetInterval(ttc));
		printf("Current counter ==  0x%04"PRIx16"\n", XTtcPs_GetCounterValue(ttc));
		fprintf(stdout, "Received interrupt - calls = %"PRId32"\n", calls++);
		#endif // TTC_DEBUG
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

	/* 50% duty cycle for now */
	config_pwm(ttc, 0xBEBC, 0xBEBC >> 1, 9);

	/* Connect an interrupt handler for the IRQ ID of the chosen timer */
	XScuGic_Connect(gic, TTC0_IRQ_ID, (Xil_InterruptHandler) ttc_intr_handler, (void *) ttc);
	/* Enable interrupt exceptions in the processor */
	Xil_ExceptionEnable();
	/* Enable interrupts at the GIC */
	XScuGic_Enable(gic, TTC0_IRQ_ID);
	/* Enable interval and match interrupts for the TTC we will be using */
	XTtcPs_EnableInterrupts(ttc, XTTCPS_IXR_INTERVAL_MASK | XTTCPS_IXR_MATCH_0_MASK);

	/*
	fprintf(stdout, "%-20s0x%01"PRIx32"\n", "Silicon Rev", ps7_dbg_get_ps_version());
	fprintf(stdout, "%-20s0x%02"PRIx32"\n", "Manufacturer ID", ps7_dbg_get_mfr_id());
	fprintf(stdout, "%-20s0x%02"PRIx32"\n", "Device Code", ps7_dbg_get_device_code());

	ttc_dbg_print_ier_status(0, 0);
	ttc_dbg_print_summary(0, 0);
	*/

	printf("Starting");
	XTtcPs_Start(ttc);
	for (;;) {
		if ( calls == (EXPIRE_SECONDS * 2) ) {
			printf("Done\n");
			XTtcPs_Stop(ttc);
			break;
		}
	}

	return 0;
}

