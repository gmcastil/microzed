#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "platform.h"

/* Canonical definitions for hard peripherals attached to the processor */
#include "xparameters_ps.h"

/* Processor specific exception handling */
#include "xil_exception.h"

/* Subsystem drivers */
#include "xscugic.h"
#include "xscuwdt.h"
#include "xgpiops.h"

#define GIC_DEVICE_ID            XPAR_SCUGIC_SINGLE_DEVICE_ID
#define WDT_DEVICE_ID            XPAR_SCUWDT_0_DEVICE_ID
#define GPIOPS_DEVICE_ID         XPAR_XGPIOPS_0_DEVICE_ID

#define ASCII_ESC                27

/*
 * NOTE: For future reference, can dump the interrupt vector table from a GIC instance
 * the XScuGic_Config struct and its HandlerTable member. Might be an interesting thing
 * to do before and after connecting interrupt handlers
 */

/* Setup exception and interrupt handler */
int SetupIntrSystem(XScuGic *Gic, XScuGic_Config *GicConfig)
{
	int Status = 0;
	int Ret = XST_SUCCESS;

	/* Values for verification */
	Xil_ExceptionHandler CheckHandler = NULL;
	void *CheckData = NULL;

	GicConfig = XScuGic_LookupConfig(GIC_DEVICE_ID);
	if (GicConfig == NULL) {
		fprintf(stderr,
				"Could not find configuration for device ID %d\n", GIC_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(Gic, GicConfig, GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		fprintf(stderr,
				"Could not initialize generic interrupt controller for device ID %d\n", GIC_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	fprintf(stdout, "Interrupt controller self test\t\t\t");
	Status = XScuGic_SelfTest(Gic);
	if (Status != XST_SUCCESS) {
		fprintf(stdout, "FAIL\n");
		Ret = XST_FAILURE;
	} else {
		fprintf(stdout, "OK\n");
	}

	/*
	 * NOTE: A future example should register an IRQ and an FIQ interrupt handler and try
	 * enabling and handling multiple interrupt sources.
	 */
	fprintf(stdout, "Register GIC with CPU exception handler\t\t");
	Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			(void *) Gic);

	Xil_GetExceptionRegisterHandler(
			XIL_EXCEPTION_ID_IRQ_INT,
			&CheckHandler,
			&CheckData);

	if ( ( CheckData != Gic ) || ( CheckHandler != (Xil_ExceptionHandler) XScuGic_InterruptHandler ) ) {
		fprintf(stdout, "FAIL\n");
		Ret = XST_FAILURE;
	} else {
		fprintf(stdout, "OK\n");
	}

	fprintf(stdout, "IRQ exceptions disabled\t\t\t\tOK\n");
	Xil_ExceptionDisableMask(XIL_EXCEPTION_IRQ);
	return Ret;
}

int SetupWdtSystem(XScuWdt *Wdt, XScuWdt_Config *WdtConfig)
{
	int Status = 0;
	int Ret = XST_SUCCESS;

	WdtConfig = XScuWdt_LookupConfig(WDT_DEVICE_ID);
	if (WdtConfig == NULL) {
		fprintf(stderr,
				"Could not find configuration for device ID %d\n", WDT_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	Status = XScuWdt_CfgInitialize(Wdt, WdtConfig, WdtConfig->BaseAddr);
	if (Status != XST_SUCCESS) {
		fprintf(stderr,
				"Could not initialize private watchdog timer for device ID %d\n", WDT_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	fprintf(stdout, "Watchdog timer reset status register\t\t");
	if ( XScuWdt_ReadReg((Wdt)->Config.BaseAddr, XSCUWDT_RST_STS_OFFSET) & 0x00000001 ) {
		fprintf(stdout, "RESET\n");
	} else {
		fprintf(stdout, "CLEAR\n");
	}
	fprintf(stdout, "Watchdog timer self test\t\t\t");
	Status = XScuWdt_SelfTest(Wdt);
	if (Status != XST_SUCCESS) {
		fprintf(stdout, "FAIL\n");
		Ret = XST_FAILURE;
	} else {
		fprintf(stdout, "OK\n");
	}

	/* Stop the WDT if it is running */
	XScuWdt_Stop(Wdt);
	/* Place in watchdog mode (but not enabled) */
	fprintf(stdout, "Watchdog mode selected\t\t\t\t");
	XScuWdt_SetWdMode(Wdt);
	if ( ( XScuWdt_GetControlReg(Wdt) & XSCUWDT_CONTROL_WD_MODE_MASK ) >> 3 ) {
		fprintf(stdout, "OK\n");
	} else {
		fprintf(stdout, "FAIL\n");
		Ret = XST_FAILURE;
	}

	return Ret;
}

int SetupGPIOPSSystem(XGpioPs *GpioPs, XGpioPs_Config *GpioPsConfig)
{
	int Status = 0;
	int Ret = XST_SUCCESS;
	int i = 0;

	/* Interrupt enable, disable, status, etc. requires this */
	uint32_t Result32 = 0;

	GpioPsConfig = XGpioPs_LookupConfig(GPIOPS_DEVICE_ID);
	if (GpioPsConfig == NULL) {
		fprintf(stderr,
				"Could not find configuration for GPIO PS device ID %d\n", GPIOPS_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	Status = XGpioPs_CfgInitialize(GpioPs, GpioPsConfig, GpioPsConfig->BaseAddr);
	if (Status != XST_SUCCESS) {
		fprintf(stderr,
				"Could not initialize GPIO PS for device ID %d\n", GPIOPS_DEVICE_ID);
		Ret = XST_FAILURE;
	}

	fprintf(stdout, "GPIO PS self test\t\t\t\t");
	Status = XGpioPs_SelfTest(GpioPs);
	if (Status != XST_SUCCESS) {
		fprintf(stdout, "FAIL\n");
	} else {
		fprintf(stdout, "OK\n");
	}

	/* Clear and disable all GPIO PS related interrupts */
	fprintf(stdout, "Disable GPIO PS interrupts\t\t\t");
	Status = 0;
	for (i=0; i<XGPIOPS_MAX_BANKS; i++) {
		XGpioPs_IntrDisable(GpioPs, i, 0xFFFFFFFF);
		Result32 = XGpioPs_IntrGetEnabled(GpioPs, i);
		/*
		 * Bank 1 is special, since the top 14 bits are not assigned to GPIO pins
		 * and do not therefore */
		if ( i == 1 ) {
			Result32 = Result32 & 0x0003FFFF;
		}
		if ( Result32 != 0x00000000 ) {
			Status++;
			fprintf(stderr,
					"Could not disable GPIO interrupts for GPIO PS bank %d\n", i);
			fprintf(stderr,
					"Expected 0x00000000 but received 0x%08"PRIx32"\n", Result32);
		}
	}
	if (Status != 0) {
		fprintf(stdout, "FAIL\n");
	} else {
		fprintf(stdout, "OK\n");
	}

	fprintf(stdout, "Clear pending GPIO PS interrupts\t\t");
	Status = 0;
	for (i=0; i<XGPIOPS_MAX_BANKS; i++) {
		XGpioPs_IntrClear(GpioPs, i, 0xFFFFFFFF);
		Result32 = XGpioPs_IntrGetStatus(GpioPs, i);
		if ( Result32 != 0x00000000 ) {
			Status++;
			fprintf(stderr,
					"Could not clear pending interrupts for GPIO PS bank %d\n", i);
			fprintf(stderr,
					"Expected 0x00000000 but received 0x%08"PRIx32"\n", Result32);
		}
	}
	if (Status != 0) {
		fprintf(stdout, "FAIL\n");
	} else {
		fprintf(stdout, "OK\n");
	}

	return Ret;
}

int ConfigGPIOPSPin(XGpioPs *GpioPs, int GpioPin, int Direction)
{

}

void MemFree(void *Ptr)
{
	if (Ptr != NULL) {
		free(Ptr);
	}
}

int main()
{
	init_platform();

	/* Initialize watchdog timer, interrupt, and GPIO subsystems */
	XScuGic_Config *GicConfig = NULL;
	XScuGic *Gic = NULL;
	XScuWdt_Config *WdtConfig = NULL;
	XScuWdt *Wdt = NULL;
	XGpioPs_Config *GpioPsConfig = NULL;
	XGpioPs *GpioPs = NULL;

	printf("%c[2J", ASCII_ESC);
	printf("Private Watchdog Examples\n");
	printf("-------------------------\n");

	Gic = malloc(sizeof(XScuGic));
	if (Gic == NULL) {
		fprintf(stderr, "Could not allocate memory for GIC driver instance\n");
		return -1;
	}
	if (SetupIntrSystem(Gic, GicConfig) != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize exception or interrupt handling\n");
		MemFree(Gic);
		return -1;
	}

	Wdt = malloc(sizeof(XScuWdt));
	if (Wdt == NULL) {
		fprintf(stderr, "Could not allocate memory for private watchdog driver instance\n");
		MemFree(Gic);
		return -1;
	}
	if (SetupWdtSystem(Wdt, WdtConfig) != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize private watchdog timer\n");
		MemFree(Wdt);
		MemFree(Gic);
		return -1;
	}

	GpioPs = malloc(sizeof(XGpioPs));
	if (GpioPs == NULL) {
		fprintf(stderr, "Could not allocate memory for GPIO driver instance\n");
		MemFree(Wdt);
		MemFree(Gic);
		return -1;
	}
	if (SetupGPIOPSSystem(GpioPs, GpioPsConfig) != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize GPIO\n");
		MemFree(GpioPs);
		MemFree(Wdt);
		MemFree(Gic);
		return -1;
	}

	/*
	 * Remaining:
	 * - Add the reset indicator from the WDT to the current status thing that is printed
	 *   out
	 * - Set up the GPIO system to allow a pushbutton to generate an interrupt
	 * - Set up the WDT system to have a 3 seconds WDT reset interval in single shot mode
	 *   so that once the WDT hits zero, the system resets (not started yet)
	 * - GPIO interrupt handler for push button needs to reset the watchdog timer
	 * - Connect the GPIO interrupt handler to the GIC
	 * - Expected behavior is to print out all the initial stuff, configure the watchdog,
	 *   configure the GPIO and WDT and GIC together, and then enable the WDT and wait indefinitely
	 *   while a GPIO push button causes the WDT to be reset, print out a display value for the number
	 *   of times it has been pushed, and then let it timeout, reset the system, and see in the
	 *   status displayed that the WDT was the reason for the reset
	 */

	/* Configure GPIO pushbutton input to reset and status watchdog timer */

	/* Loop indefinitely */

	cleanup_platform();
	return 0;
}
