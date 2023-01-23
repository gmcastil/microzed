#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "xil_types.h"
#include "platform.h"
#include "xstatus.h"
#include "xgpiops.h"
#include "xscugic.h"
#include "xil_exception.h"

/* Microzed GPIO pins */
#define GPIO_USER_LED		47	/* Bank 1, MIO 47 */
#define GPIO_USER_PBSW		51	/* Bank 1, MIO 51 */

#define GPIO_PIN_INPUT		0
#define GPIO_PIN_OUTPUT		1
#define GPIO_PIN_DISABLE	0
#define GPIO_PIN_ENABLE		1
#define GPIO_PIN_OFF		0
#define GPIO_PIN_ON		1

#define ASCII_ESC		27

static void GpioPbswHandler(void *CallbackRef, u32 Bank, u32 Status);
void IntrStatusDisplay(XGpioPs * Gpio, XScuGic * Gic, u32 GpioBank,
		       u32 GpioPin);

static void GpioPbswHandler(void *CallbackRef, u32 Bank, u32 Status)
{
	static int PressCnt = 0;
	printf("Button pressed %d\n", ++PressCnt);
	usleep(200000);
}

int main(int argc, char *argv[])
{
	init_platform();

	/* GPIO config and driver instance */
	XGpioPs_Config *GpioConfig;
	XGpioPs *Gpio;

	/* GIC driver instance */
	XScuGic_Config *GicConfig;
	XScuGic *Gic;

	int i;
	int Status;

	printf("%c[2J", ASCII_ESC);
	printf("Interrupt Examples\n");
	printf("------------------\n");

	/* Set up GPIO driver */
	Gpio = malloc(sizeof(XGpioPs));
	if (Gpio == NULL) {
		fprintf(stderr,
			"Could not allocate memory for GPIO instance\n");
		return XST_FAILURE;
	}

	GpioConfig = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
	Status =
	    XGpioPs_CfgInitialize(Gpio, GpioConfig, GpioConfig->BaseAddr);

	if (Status != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize GPIO instance\n");
		free(Gpio);
		return XST_FAILURE;
	} else {
		Status = XGpioPs_SelfTest(Gpio);
		if (Status != XST_SUCCESS) {
			printf("GPIO self test\t\tFAIL\n");
			free(Gpio);
			return XST_FAILURE;
		} else {
			printf("GPIO self test\t\tSUCCESS\n");
		}
	}
	/* Disable interrupts for the GPIO pin that the switch is attached to */
	XGpioPs_IntrDisablePin(Gpio, GPIO_USER_PBSW);
	XGpioPs_IntrClearPin(Gpio, GPIO_USER_PBSW);

	/* Configure and enable GPIO inputs and outputs */
	XGpioPs_SetDirectionPin(Gpio, GPIO_USER_PBSW, GPIO_PIN_INPUT);
	XGpioPs_SetDirectionPin(Gpio, GPIO_USER_LED, GPIO_PIN_OUTPUT);
	XGpioPs_SetOutputEnablePin(Gpio, GPIO_USER_LED, GPIO_PIN_ENABLE);

	XGpioPs_WritePin(Gpio, GPIO_USER_LED, GPIO_PIN_OFF);

	/* Set up GIC driver */
	Gic = malloc(sizeof(XScuGic));
	if (Gic == NULL) {
		fprintf(stderr,
			"Could not allocate memory for GIC instance\n");
		return XST_FAILURE;
	}

	GicConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	Status =
	    XScuGic_CfgInitialize(Gic, GicConfig,
				  GicConfig->CpuBaseAddress);

	if (Status != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize GIC instance\n");
		free(Gic);
		return XST_FAILURE;
	} else {
		Status = XScuGic_SelfTest(Gic);
		if (Status != XST_SUCCESS) {
			printf("GIC self test\t\tFAIL\n");
			free(Gic);
			return XST_FAILURE;
		} else {
			printf("GIC self test\t\tSUCCESS\n");
		}
	}
	/* Disable GPIO interrupts within the GIC */
	XScuGic_Disable(Gic, XPS_GPIO_INT_ID);

	/*
	 * Registers the GIC interrupt handler with the exception handling logic
	 * within the ARM Cortex A9 processor
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				     (Xil_ExceptionHandler)
				     XScuGic_InterruptHandler, Gic);

	/*
	 * There is a meaningful distinction between callback functions and interrupt
	 * handlers. The interrupt handling machinery by the Xilinx standalone BSP
	 * provides an interrupt handler function that gets paired with a callback
	 * reference.
	 */
	Status = XScuGic_Connect(Gic,
				 XPS_GPIO_INT_ID,
				 (Xil_ExceptionHandler)
				 XGpioPs_IntrHandler, (void *) Gpio);

	if (Status != XST_SUCCESS) {
		fprintf(stderr,
			"Could not connect GPIO interrupt handler to interrupt controller\n");
	}

	/* Define a rising edge for GPIO interrupts on the PBSW pin */
	XGpioPs_SetIntrTypePin(Gpio,
			       GPIO_USER_PBSW,
			       XGPIOPS_IRQ_TYPE_EDGE_RISING);

	/*
	 * Define the function to be called by the GPIO interrupt handler when an
	 * actual interrupt occurs - note that again, the callback function, which is
	 * going to be assigned here, is distinct from the actual interrupt handler.
	 * The standalone BSP libraries provide interrupt handlers which deal with
	 * the lower level complexity.  An answer record from NXP explains this
	 * distinction better than I can:
	 *
	 * Callbacks are normally used by libraries or application frameworks. It
	 * means that the library provides the interrupt handler (and therefore
	 * performs the necessary interrupt functions, and maybe sets things up
	 * like adding data to a queue/FIFO/whatever). The interrupt handler will
	 * then invoke the callback functions (there may be more than one, depending
	 * on the framework) without the user having to edit the provided handler.
	 * The application instead 'registers' the callback with the library, which
	 * is then called to provide application-specific functionality.
	 */
	XGpioPs_SetCallbackHandler(Gpio, (void *) Gpio, GpioPbswHandler);

	/* Enable interrupts for the GPIO pin that the switch is attached to */
	XGpioPs_IntrEnablePin(Gpio, GPIO_USER_PBSW);
	/* Enable GPIO interrupts within the GIC */
	XScuGic_Enable(Gic, XPS_GPIO_INT_ID);
	/* Enable interrupts within the ARM Cortex A9 processor */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	printf("\n");
	/* Display GPIO interrupt status for banks and pins */
	for (i = 0; i < XGPIOPS_MAX_BANKS; i++) {
		printf("GPIO bank %d interrupt enabled\t\t0x%08lx\n",
		       i, XGpioPs_IntrGetEnabled(Gpio, i));
	}

	printf("\n");
	/* Confirm interrupts only enabled for GPIO push button */
	if (XGpioPs_IntrGetEnabledPin(Gpio, GPIO_USER_LED)) {
		printf("Interrupts enabled for GPIO pin %d\n",
		       GPIO_USER_LED);
	}
	if (XGpioPs_IntrGetEnabledPin(Gpio, GPIO_USER_PBSW)) {
		printf("Interrupts enabled for GPIO pin %d\n",
		       GPIO_USER_PBSW);
	}

	printf("Waiting for button press...\n");
	usleep(12 * 1E6);
	printf("Finished\n");

	free(Gpio);
	free(Gic);

	cleanup_platform();

	return XST_SUCCESS;
}
