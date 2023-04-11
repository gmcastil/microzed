#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/* Libraries required for interrupt handling */
#include "xscugic.h"
#include "xil_exception.h"

/* Xilinx private timer driver */
#include "xscutimer.h"

/* Debug and workaround codes */
#include "ps7_dbg.h"

#include "taylor_uzed.h"

/* Necessary for creating driver instances */
#define GIC_DEVICE_ID			XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_DEVICE_ID			XPAR_XSCUTIMER_0_DEVICE_ID

/* Hardware peripheral definitions */
#define PERIPHERAL_BASE			0x43C10000
#define PERIPHERAL_SCRATCH_OFFSET	0x04

#define PERIPHERAL_ADD			0x01
#define PERIPHERAL_SUB			0x02
#define PERIPHERAL_MULT			0x03

#define ASCII_ESC		27

#define altitude_to_pressure(a, b, c, x) (a*(x*x) + (b*x) + x)

void clear_console()
{
        fprintf(stdout, "%c[2J", ASCII_ESC);
        fflush(stdout);

}
void print_operation(char *str)
{
        fprintf(stdout, "%-30s", str);
        fflush(stdout);
}
void print_result(char *str)
{
        fprintf(stdout, "%10s\n", str);
        fflush(stdout);
}
void print_diff(uint32_t actual, uint32_t expected)
{
		fprintf(stdout, "Actual: 0x%08"PRIx32" Expected: 0x%08"PRIx32"\n",
				(uint32_t) actual, (uint32_t) expected);
		fflush(stdout);
}

uint32_t peripheral_scratch(uint32_t *p_ptr, uint32_t val)
{
	Xil_Out32((uint32_t) p_ptr + PERIPHERAL_SCRATCH_OFFSET, val);
	return Xil_In32((uint32_t) p_ptr + PERIPHERAL_SCRATCH_OFFSET);
}

void print_summary(uint32_t *p_ptr, int NumRegs)
{
        uint32_t value = 0;
        uint32_t *ptr = p_ptr;

        int i = 0;

        for (i = 0; i < NumRegs; i++) {
                value = Xil_In32((uint32_t) ptr);
                printf("Addr: %8p Data: 0x%08"PRIx32"\n", ptr++, value);
        }

        return;
}

uint32_t peripheral_add(uint32_t *p_ptr, uint32_t add_0, uint32_t add_1)
{
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG1_OFFSET, add_0);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG2_OFFSET, add_1);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG0_OFFSET, PERIPHERAL_ADD);

	return TAYLOR_UZED_mReadReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG3_OFFSET);
}

uint32_t peripheral_subtract(uint32_t *p_ptr, uint32_t add_0, uint32_t add_1)
{
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG1_OFFSET, add_0);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG2_OFFSET, add_1);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG0_OFFSET, PERIPHERAL_SUB);

	return TAYLOR_UZED_mReadReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG3_OFFSET);
}

uint32_t peripheral_multiply(uint32_t *p_ptr, uint32_t add_0, uint32_t add_1)
{
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG1_OFFSET, add_0);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG2_OFFSET, add_1);
	TAYLOR_UZED_mWriteReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG0_OFFSET, PERIPHERAL_MULT);

	return TAYLOR_UZED_mReadReg((uint32_t) p_ptr, TAYLOR_UZED_S00_AXI_SLV_REG3_OFFSET);
}

int main(int args, char *argv[])
{

	float i = 0;
	uint32_t start_time = 0;
	uint32_t stop_time = 0;

	float result = 0;
	uint32_t scratch32;

	XScuTimer_Config *timer_config = NULL;
	XScuTimer *timer = NULL;

	clear_console();

	/* ------------ PS interface check  --------------------------------------- */
	ps7_dbg_sanity_check();

	/* ------------ Check that peripheral is functioning ---------------------- */
	print_operation("Scratch register");
	uint32_t *p_ptr = (uint32_t *) PERIPHERAL_BASE;
	/* Just going to use pi here as a scratch value */
	scratch32 = peripheral_scratch(p_ptr, 3141);
	if ( 3141 != scratch32 ) {
		print_result("FAIL");
		print_diff(scratch32, 3141);
	} else {
		print_result("OK");
	}

	/* ------------ Configuring private timer  --------------------------------- */
	print_operation("Setting up timer");
	timer_config = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	if ( timer_config == NULL ) {
		print_result("FAIL");
		printf("Could not initialize private timer %d\n", TIMER_DEVICE_ID);
	} else {
		timer = malloc(sizeof(XScuTimer));
		if ( timer == NULL ) {
			print_result("FAIL");
			printf("Could not allocate memory for private timer %d\n", TIMER_DEVICE_ID);
		} else {
			XScuTimer_CfgInitialize(timer, timer_config, timer_config->BaseAddr);
			print_result("OK");
		}
	}

	/* Private timer clock is always CPU_3X2X clock (333 or 250MHz) */
	printf("\n");
	printf("%-10s%-15s%-15s\n", "mbar", "altitude", "elapsed");
	printf("%-10s%-15s%-15s\n", "----", "----------", "-------");
	XScuTimer_SetPrescaler(timer, 0);
	XScuTimer_DisableAutoReload(timer);
	for (i = 0.0; i < 10.0; i = i + 0.1) {
		XScuTimer_LoadTimer(timer, 0xFFFFFFFF);
		start_time = XScuTimer_GetCounterValue(timer);
		XScuTimer_Start(timer);
		result = altitude_to_pressure((-0.0088), (1.7673), (131.29), i);
		XScuTimer_Stop(timer);
		stop_time = XScuTimer_GetCounterValue(timer);
		printf("%-10.2f%-15.6f%-12"PRIu32"\n", i, result, start_time - stop_time);
	}

	free(timer);

	return 0;
}
