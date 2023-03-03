#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "platform.h"
#include "sleep.h"

/* Canonical definitions for hard peripherals attached to the processor */
#include "xparameters_ps.h"

/* Processor specific exception and interrupt handling */
#include "xil_exception.h"

/* Subsystem drivers */
#include "xscugic.h"
#include "xscuwdt.h"
#include "xgpiops.h"

/* Low level Xilinx IO */
#include "xil_io.h"

#define GIC_DEVICE_ID			XPAR_SCUGIC_SINGLE_DEVICE_ID
#define WDT_DEVICE_ID			XPAR_SCUWDT_0_DEVICE_ID
#define GPIOPS_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID

/* Interrupt ID from xparameters_ps.h - recall that GPIO is a shared peripheral
 * interrupt and that private WDT are private peripheral interrupts (i.e., there
 * is a SPI interrupt XPS_WDT_INT_ID which is for a different timer).
 */
#define GPIO_INTR_ID			XPS_GPIO_INT_ID
#define WDT_INTR_ID				XPS_SCU_WDT_INT_ID

/* Other useful constants */
#define GPIO_DEBOUNCE_TIME		100000
#define ASCII_ESC				27

/* Per the schematic, PS MIO 51 is pulled down to ground through a 5k resistor
 * and pulled up to 1.8V through SW1 on the Microzed board proper (i.e., it is
 * active high).
 */
#define GPIO_UZED_PBSW			51
#define GPIO_INPUT				0
#define GPIO_OUTPUT				1

#define SLCR_BASE_ADDR      0xF8000000
#define CLK_621_TRUE        0x000001C4

#define CLK_400_NS          2.500
#define CLK_300_NS          3.333

#include "wdt_dbg.h"

/*
 * GPIO PS interrupt handler needs to be able to interact with the WDT. So in addition
 * to the GPIO PS instance, which is necessary to clear the interrupt (and debounce the
 * push button) we need to bundle the WDT instance as well.
 */
struct GpioPs_Wdt_Intr_CallbackRef {
	XGpioPs *GpioPs;
	XScuWdt *Wdt;
};

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

	/*
	 * There appears to be an issue with the watchdog timer in that writing to the
	 * disable bit while in watchdog mode does not have any effect.  So, to put in
	 * watchdog mode with bit zero cleared, we will switch the WDT from watchdog mode
	 * (which is what the initialization functions) back to the default, stop the
	 * timer, and then switch back to watchdog mode.
	 *
	 * The original sin in all of this appears to be a) that the self-test puts the
	 * watchdog circuit in watchdog mode and starts it and b) that the watchdog timer
	 * circuit does not respect the stop bit in the control register
	 */
	fprintf(stdout, "Watchdog mode selected\t\t\t\t");
	XScuWdt_SetTimerMode(Wdt);
	XScuWdt_Stop(Wdt);
	XScuWdt_SetWdMode(Wdt);
	if ( ( XScuWdt_GetControlReg(Wdt) & XSCUWDT_CONTROL_WD_MODE_MASK ) >> 3 ) {
		fprintf(stdout, "OK\n");
	} else {
		fprintf(stdout, "FAIL\n");
		Ret = XST_FAILURE;
	}
	fprintf(stdout, "Watchdog disabled\t\t\t\t");
	if ( ( XScuWdt_GetControlReg(Wdt) & XSCUWDT_CONTROL_WD_ENABLE_MASK ) == 0 ) {
		fprintf(stdout, "OK\n");
	} else {
		fprintf(stdout, "FAIL\n");
	}

	return Ret;
}

int SetupGpioPsSystem(XGpioPs *GpioPs, XGpioPs_Config *GpioPsConfig)
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

int ConfigGpioPsPin(XGpioPs *GpioPs, int GpioPin, int Direction)
{
	int Ret = XST_SUCCESS;

	if (GpioPs != NULL) {
		XGpioPs_SetDirectionPin(GpioPs, GpioPin, Direction);
		if ( XGpioPs_GetDirectionPin(GpioPs, GpioPin) != Direction ) {
			fprintf(stderr, "Could not set GPIO pin %d direction\n", GpioPin);
			Ret = XST_FAILURE;
		}
	} else {
		Ret = XST_FAILURE;
	}
	return Ret;
}

int ConfigGpioPsIntr(XGpioPs *GpioPs, int GpioPin, int IntrType)
{
	int Ret = XST_SUCCESS;

	if ( GpioPs != NULL ) {
		XGpioPs_SetIntrTypePin(GpioPs, GpioPin, IntrType);
		if ( XGpioPs_GetIntrTypePin(GpioPs, GpioPin) != IntrType ) {
			fprintf(stderr, "Could not configure GPIO pin %d as interrupt type %d", GpioPin, IntrType);
			Ret = XST_FAILURE;
		}
		XGpioPs_IntrDisablePin(GpioPs, GpioPin);
		XGpioPs_IntrClearPin(GpioPs, GpioPin);
	} else {
		Ret = XST_FAILURE;
	}
	return Ret;
}

void ConfigWdtTimeout(XScuWdt *Wdt, uint32_t Seconds)
{
	uint32_t tick_period_ns = 0;
	uint32_t ticks = 0;
	uint32_t clk_ratio_mode = 0;
	uint32_t check_ticks = 0;

	if (Wdt == NULL) {
		return;
	}
	/* Determine the CPU clock ratio mode and tick duration */
	clk_ratio_mode = Xil_In32((SLCR_BASE_ADDR) + (CLK_621_TRUE)) & 0x00000001;
	printf("CPU clock ratio mode\t\t\t\t");
	if (clk_ratio_mode) {
		printf("6:2:1\n");
		tick_period_ns = CLK_400_NS;
	} else {
		printf("4:2:1\n");
		tick_period_ns = CLK_300_NS;
	}
	ticks = (uint32_t) (Seconds * (1e9) / tick_period_ns);

	/*
	 * When in watchdog mode, the only way to update the watchdog counter register is to write
	 * to the watchdog load register
	 * */
	fprintf(stdout, "Watchdog timeout set for %"PRId32" sec\t\t\t", Seconds);
	XScuWdt_LoadWdt(Wdt, ticks);
	check_ticks = XScuWdt_ReadReg((Wdt->Config.BaseAddr), XSCUWDT_COUNTER_OFFSET);

	if ( check_ticks != ticks ) {
		fprintf(stderr, "Could not load watchdog counter register or watchdog is running\n");
		fprintf(stderr, "\tExpected: 0x%8"PRIx32" Received: 0x%8"PRIx32"\n", ticks, check_ticks);
		printf("FAIL\n");
	} else {
		printf("OK\n");
	}
}

void MemFree(void *Ptr)
{
	if (Ptr != NULL) {
		free(Ptr);
	}
}

static void GpioPs_IntrHandler(void *CallbackRef, uint32_t Bank, uint32_t Status)
{
	static int count = 0;
	XGpioPs *GpioPs = ((struct GpioPs_Wdt_Intr_CallbackRef *) CallbackRef)->GpioPs;
	XScuWdt *Wdt = ((struct GpioPs_Wdt_Intr_CallbackRef *) CallbackRef)->Wdt;

	fprintf(stdout, "Received GPIO PS interrupt %d\n", ++count);
	if ( XGpioPs_IntrGetStatusPin(GpioPs, GPIO_UZED_PBSW) ) {
		fprintf(stdout, "Restarting watchdog timer\n");
		fflush(stdout);
		XScuWdt_RestartWdt(Wdt);
		usleep(GPIO_DEBOUNCE_TIME);
		XGpioPs_IntrClearPin(GpioPs, GPIO_UZED_PBSW);
	} else {
		fprintf(stderr, "Received spurious GPIO PS interrupt\n");
		fflush(stderr);
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
	if (SetupGpioPsSystem(GpioPs, GpioPsConfig) != XST_SUCCESS) {
		fprintf(stderr, "Could not initialize GPIO\n");
		MemFree(GpioPs);
		MemFree(Wdt);
		MemFree(Gic);
		return -1;
	}

	struct GpioPs_Wdt_Intr_CallbackRef *GpioPs_CallbackRef;
	GpioPs_CallbackRef = malloc(sizeof(GpioPs_CallbackRef));
	if (GpioPs_CallbackRef == NULL) {
		fprintf(stderr, "Could not allocate memory for GPIO PS callback reference\n");
		MemFree(Wdt);
		MemFree(Gic);
		return -1;
	} else {
		GpioPs_CallbackRef->Wdt = Wdt;
		GpioPs_CallbackRef->GpioPs = GpioPs;
	}


	/*
	 * Remaining:
	 * X- Add the reset indicator from the WDT to the current status thing that is printed
	 *   out
	 * - Set up the GPIO system to allow a pushbutton to generate an interrupt
	 * - Set up the WDT system to have a 5 seconds WDT reset interval in watchdog mode
	 *   so that once the WDT hits zero, the system resets (not started yet)
	 * - GPIO interrupt handler for push button needs to reset the watchdog timer
	 * - Connect the GPIO interrupt handler to the GIC
	 * - Expected behavior is to print out all the initial stuff, configure the watchdog,
	 *   configure the GPIO and WDT and GIC together, and then enable the WDT and wait indefinitely
	 *   while a GPIO push button causes the WDT to be reset, print out a display value for the number
	 *   of times it has been pushed, and then let it timeout, reset the system, and see in the
	 *   status displayed that the WDT was the reason for the reset
	 */

	/* Configure GPIO pushbutton as an input and capable of generating interrupts */
	ConfigGpioPsPin(GpioPs, GPIO_UZED_PBSW, GPIO_INPUT);
	ConfigGpioPsIntr(GpioPs, GPIO_UZED_PBSW, XGPIOPS_IRQ_TYPE_EDGE_RISING);

	/*
	 * Connect the GPIO interrupt handler (which is called when GPIO interrupts need to be serviced)
	 * to the GIC interrupt handler. Need to couple the GPIO interrupt handler with the watchdog
	 * timer instance as well so that the interrupt handler can restart the watchdog (this is the
	 * magic step that I've not seen an example of how to do, so I'm sort of making this up as I go).
	 */
	XScuGic_Connect(Gic, GPIO_INTR_ID, (Xil_ExceptionHandler) GpioPs_IntrHandler, (void *) GpioPs_CallbackRef);

	/* Configure watchdog timer for interrupt duration */
	ConfigWdtTimeout(Wdt, 5);

	/* Enable GPIO interrupts at the GPIO device */
	XGpioPs_IntrEnablePin(GpioPs, GPIO_UZED_PBSW);

	/* Enable GPIO interrupts by the interrupt handler */
	XScuGic_Enable(Gic, GPIO_INTR_ID);

	/* Enable interrupt handling by the ARM Cortex-A9 exception handler */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	/* Enable the watchdog timer to actually start counting down */
	XScuWdt_Start(Wdt);

	/* Loop indefinitely */
	for (;;);

	cleanup_platform();
	return 0;
}
