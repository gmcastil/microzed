#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xparameters.h"
#include "platform.h"
#include "xstatus.h"
#include "xgpiops.h"

#define GPIO_UZED_LED 47

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

#define GPIO_OUTPUT_DISABLE 0
#define GPIO_OUTPUT_ENABLE 1

#define LED_ON 0x1
#define LED_OFF 0x0

#define ASCII_ESC 27

void gpio_summary(XGpioPs *gpio_ptr)
{
	fprintf(stdout, "Device ID\t\t%d\n", (gpio_ptr->GpioConfig).DeviceId);
	fprintf(stdout, "Base address\t\t0x%08lx\n", (gpio_ptr->GpioConfig).BaseAddr);
	fprintf(stdout, "Status\t\t\t0x%08lx\n", gpio_ptr->IsReady);
	/* 
	 * Return value of XGetPlatform_Info() which identifies Zynq / Versal /
	 * ZynqMP / MicroBlaze. Zynq-7000 is expected to be 0x04
	 */
	fprintf(stdout, "Platform Data\t\t0x%lx\n", gpio_ptr->Platform);
	fprintf(stdout, "Max pins\t\t%lu\n", gpio_ptr->MaxPinNum);
	fprintf(stdout, "Max banks\t\t%d\n", gpio_ptr->MaxBanks);
	return;
}

int main(int args, char *argv[])
{
	init_platform();

	int status;

	/* Pointer to the driver instance */
	XGpioPs *gpio_ptr;
	/* GPIO controller configuration */
	XGpioPs_Config *gpio_cfg_ptr;

	/* Clear the remote terminal or console */
	fprintf(stdout, "%c[2J", ASCII_ESC );
	fprintf(stdout, "GPIO Examples\n");
	fprintf(stdout, "========================\n");

	/*
	 * Per the GPIO driver API documentation, the steps appear to be a)
	 * allocate memory for the GPIO driver instance, b) lookup the
	 * configuration for the target device, c) initialize the GPIO with the
	 * relevant values from the configuration.  Once this is done, desired
	 * GPIO behavior can be configured (e.g., pin directions can be set) and
	 * the GPIO interface can be used.
	 */

	/* 
	 * Allocate some heap memory for the GPIO driver instance - there is
	 * probably a memory allocator in the Xilinx SDK, but we will use the
	 * one from the standard library anyway
	 */
	gpio_ptr = malloc(sizeof(XGpioPs));
	if (gpio_ptr == NULL) {
		return XST_FAILURE;
	}

	/*
	 * The GPIO config requires a device ID and returns a struct
	 * with the config register base address. This makes sense,
	 * since some devices might have more than one GPIO controller.
	 */
	gpio_cfg_ptr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	status = XGpioPs_CfgInitialize(
			gpio_ptr,
			gpio_cfg_ptr,
			gpio_cfg_ptr->BaseAddr);
	if (status == XST_SUCCESS) {
		/* Print out GPIO summary */
		gpio_summary(gpio_ptr);
		fprintf(stdout, "\n");
		/* Driver was initialized correctly, so run self test */
		status = XGpioPs_SelfTest(gpio_ptr);
		if (status == XST_SUCCESS) {
			fprintf(stdout, "GPIO %d self test successful\n", gpio_cfg_ptr->DeviceId);
		} else {
			fprintf(stdout, "GPIO %d self test unsuccessful\n", gpio_cfg_ptr->DeviceId);
		}
	} else {
		fprintf(stderr, "Could not initialize GPIO %d\n", XPAR_PS7_GPIO_0_DEVICE_ID);
		free(gpio_ptr);
		return XST_FAILURE;
	}

	/* Set the direction of the GPIO pin and enable the output */
	fprintf(stdout, "Enabled GPIO pin %d\n", GPIO_UZED_LED);
	XGpioPs_SetDirectionPin(gpio_ptr, GPIO_UZED_LED, GPIO_OUTPUT);
	XGpioPs_SetOutputEnablePin(gpio_ptr, GPIO_UZED_LED, GPIO_OUTPUT_ENABLE);
	XGpioPs_WritePin(gpio_ptr, GPIO_UZED_LED, LED_ON);
	sleep(10);
	XGpioPs_WritePin(gpio_ptr, GPIO_UZED_LED, LED_OFF);
	free(gpio_ptr);
	fprintf(stdout, "Done.\n");
	cleanup_platform();

	return XST_SUCCESS;
}
