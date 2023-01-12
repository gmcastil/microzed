#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "xparameters.h"
#include "platform.h"
#include "xstatus.h"
#include "xgpiops.h"

/* Define the Microzed user LED and user pushbutton switch pin numbers */
#define GPIO_UZED_LED 47
#define GPIO_UZED_PBSW 51

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

#define GPIO_OUTPUT_DISABLE 0
#define GPIO_OUTPUT_ENABLE 1

#define LED_OFF 0
#define LED_ON 1

#define PBSW_OFF 0
#define PBSW_ON 1

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

int gpio_blink_led(XGpioPs *gpio_ptr, uint32_t pin, unsigned long tms)
{
	/* Do not try to drive a pin that is not an output */
	if (XGpioPs_GetDirectionPin(gpio_ptr, pin) != GPIO_OUTPUT) {
		fprintf(stderr, "Pin %ld not configured as an output\n", pin);
		return -1;
	}

	/* Do not try to blink an output that is disabled */
	if (XGpioPs_GetOutputEnablePin(gpio_ptr, pin) != GPIO_OUTPUT_ENABLE) {
		fprintf(stderr, "Output pin %ld not enabled\n", pin);
		return -1;
	}

	/* 
	 * Our pulse deliberately begins with an ON condition, so if the LED is
	 * already being driven, a delay will occur before a visible blink
	 */
	XGpioPs_WritePin(gpio_ptr, pin, LED_ON);
	usleep(tms);
	XGpioPs_WritePin(gpio_ptr, pin, LED_OFF);
	usleep(tms);

	return 0;
}

void gpio_toggle_led(XGpioPs *gpio_ptr, uint32_t pin)
{
	uint32_t current;
	current = XGpioPs_ReadPin(gpio_ptr, pin);
	XGpioPs_WritePin(gpio_ptr, pin, !current);
	return;
}

void gpio_wait_pbsw(XGpioPs *gpio_ptr, uint32_t pin)
{
	for (;;) {	
		if (XGpioPs_ReadPin(gpio_ptr, pin) == PBSW_ON) {
			/* 10ms time for debouncing should work */
			usleep(100000);
			if (XGpioPs_ReadPin(gpio_ptr, pin) == PBSW_ON) {
				break;
			} else {
				continue;
			}
		}
	}
	return;
}

int main(int args, char *argv[])
{
	init_platform();

	int i;
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

	/* Set the direction of the GPIO pin driving the LED and enable its output */
	XGpioPs_SetDirectionPin(gpio_ptr, GPIO_UZED_LED, GPIO_OUTPUT);
	XGpioPs_SetOutputEnablePin(gpio_ptr, GPIO_UZED_LED, GPIO_OUTPUT_ENABLE);
	fprintf(stdout, "Enabled GPIO pin %d as an output\n", GPIO_UZED_LED);

	/* Blink the GPIO LED 10 times */
	i = 0;
	while (i < 10) {
		gpio_blink_led(gpio_ptr, GPIO_UZED_LED, 250000);
		i++;
	}
	fprintf(stdout, "Completed LED flashing\n");

	/* Now for pushbutton switch demonstration */

	/* Set the direction of the GPIO pin that services the switch */
	XGpioPs_SetDirectionPin(gpio_ptr, GPIO_UZED_PBSW, GPIO_INPUT);

	/* Then toggle the LED each time debounced switch is pushed */
	i = 0;
	while (i < 10) {
		gpio_wait_pbsw(gpio_ptr, GPIO_UZED_PBSW);
		gpio_toggle_led(gpio_ptr, GPIO_UZED_LED);
		i++;
	}
	fprintf(stdout, "Completed pushbutton / LED toggle\n");

	free(gpio_ptr);
	fprintf(stdout, "Done.\n");
	cleanup_platform();

	return XST_SUCCESS;
}
