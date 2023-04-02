#include <stdio.h>
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
#define GIC_DEVICE_ID                           XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TTC_DEVICE_ID                           XPAR_XTTCPS_0_DEVICE_ID

/*
 * This defines the interrupt that the GIC will see, but it could be for several different
 * kinds of interrupts on that particular counter (e.g., match, interval, overflow)
 */
#define TTC0_IRQ_ID                                     XPS_TTC0_0_INT_ID

#define ASCII_ESC                                       27

#include "ps7_dbg.h"

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

int main(int args, char *argv[])
{

	init_platform();

	clear_console();

	ps7_dbg_sanity_check();

    cleanup_platform();

	return 0;
}
