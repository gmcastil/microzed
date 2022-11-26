/*
 * Per the XADC PS driver v2.3 documentation
 * - Initialize the XADC device using XAdcPs_CfgInitialize()
 * - Run a self test on the XADC
 * - Get the configuration register values, summarize in a table
 * - Get the miscellaneous status register values, summarize in a table
 * - Get the miscellaneous control register and summarize in a table
 * - Obtain and summarize the following: temperature, VCCINT, VCCAUX, VCCBRAM,
 *   VCCPINT, VCCPAUX, VCCDDR
 *
 * Next
 * - Example working with the analog channel data
 * - Alarms, interrupts
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "xparameters.h"
#include "platform.h"
#include "xstatus.h"
#include "xadcps.h"

#define XADC_DEVICE_ID XPAR_XADCPS_0_DEVICE_ID

#define ASCII_ESC 27

/* Fix a couple of missing function prototypes from the XADC driver */
uint32_t XAdcPs_GetConfigRegister(XAdcPs *);
uint32_t XAdcPs_GetMiscCtrlRegister(XAdcPs *);

/*
void summarize_misc_config(XAdcPs *InstancePtr)
{
	uint32_t reg32_read = 0;

	fprintf(stdout, "XADC Misc Configuration\n");
	fprintf(stdout, "-----------------------\n");

	reg32_read = (uint32_t) XAdcPs_GetMiscCtrlRegister(InstancePtr);

	fprintf(stdout, "%-30s %10" PRIx32 "\n", "flush mask (1b)",
			(reg32_read & XADCPS_MCTL_FLUSH_MASK) >> 1);
	fprintf(stdout, "%-30s %10" PRIx32 "\n", "reset mask (2b)",
			(reg32_read & XADCPS_MCTL_RESET_MASK) >> 5);
	fprintf(stdout, "\n");

	return;
}
*/

(reg32_read & XADCPS_MSTS_CFIFO_LVL_MASK) >> );
(reg32_read & XADCPS_MSTS_DFIFO_LVL_MASK) >> );
(reg32_read & XADCPS_MSTS_CFIFOF_MASK) >> );
(reg32_read & XADCPS_MSTS_CFIFOE_MASK) >> );
(reg32_read & XADCPS_MSTS_DFIFOF_MASK) >> 10);
(reg32_read & XADCPS_MSTS_DFIFOE_MASK) >> 9);
(reg32_read & XADCPS_MSTS_OT_MASK) >> );
(reg32_read & XADCPS_MSTS_ALM_MASK) >> 0);

void summary_xadcif_cfg (XAdcPs *InstancePtr)
{
	/* pretty sure these shift valeus are all wrong */

	uint32_t reg32_read = 0;

	reg32_read = (uint32_t) XAdcPs_GetConfigRegister(InstancePtr);

	fprintf(stdout, "XADCIF_CFG (0xF8007000)\n");
	fprintf(stdout, "-----------------------\n");
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "ENABLE",
			(reg32_read & XADCPS_CFG_ENABLE_MASK) >> 31);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFOTH (4b)",
			(reg32_read & XADCPS_CFG_CFIFOTH_MASK) >> 20);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFOTH (4b)",
			(reg32_read & XADCPS_CFG_DFIFOTH_MASK) >> 16);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "WEDGE",
			(reg32_read & XADCPS_CFG_WEDGE_MASK) >> 13);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "REDGE",
			(reg32_read & XADCPS_CFG_REDGE_MASK) >> 12);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "TCKRATE (2b)",
			(reg32_read & XADCPS_CFG_TCKRATE_MASK) >> 8);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "IGAP (5b)",
			(reg32_read & XADCPS_CFG_IGAP_MASK) >> 5);
	fprintf(stdout, "\n");

	return;
}

int main (int args, char *argv[])
{
	/* 
	 * The software status codes (at least most of them) defined
	 * by "xstatus.h" are long int
	*/
	long int xst_status = 0;
	XAdcPs XAdcPsInst;

	/* XADC configuration structure */
	XAdcPs_Config *ConfigPtr = NULL;
	/* XADC instance */
	XAdcPs *InstancePtr = &XAdcPsInst;

	init_platform();

	/* Clear the remote terminal or console */
    	fprintf(stdout, "%c[2J", ASCII_ESC );
	fprintf(stdout, "XADC Summary Application\n\n");

	/* Look up the device configuration based on the unique device ID */
	ConfigPtr = XAdcPs_LookupConfig(XADC_DEVICE_ID);
	if (ConfigPtr == NULL) {
		fprintf(stderr, "No configuration for XADC device ID %d found\n", XADC_DEVICE_ID);
		return XST_DEVICE_NOT_FOUND;
	}

	/* Initialize the XADC */
	xst_status = XAdcPs_CfgInitialize(InstancePtr, ConfigPtr,
						ConfigPtr->BaseAddress);

	if (xst_status == XST_SUCCESS) {
		fprintf(stdout, "Initialization complete\n\n");
	} else {
		return XST_FAILURE;
	}

	/* Run a self test on the XADC */
	xst_status = XAdcPs_SelfTest(InstancePtr);
	if (xst_status == XST_SUCCESS) {
		fprintf(stdout, "Self test complete\n\n");
	} else {
		return XST_FAILURE;
	}

	/* XADC interface configuration register (devcfg.XADCIF_CFG) */
	summary_xadcif_cfg(InstancePtr);

	/* XADC interface interrupt status (devcfg.XADC_INT_STS) */
	summary_xadcif_int_sts(InstancePtr);

	/* XADC interface interrupt status (devcfg.XADC_INT_MASK) */
	summary_xadcif_int_mask(InstancePtr);

	/* XADC interface miscellaneous status (devcfg.XADCIF_MSTS) */
	summary_xadcif_msts(InstancePtr);

	/* XADC interface command FIFO data port (devcfg.XADCIF_CMDFIFO) */
	summary_xadcif_cmdfifo();

	/* XADC interface data FIFO data port (devcfg.XADCIF_RDFIFO) */
	summary_xadcif_rdfifo();

	/* XADC interface miscellaneous control (devcfg.XADCIF_MCTL) */
	summary_xadcif_mctl();


	cleanup_platform();
	return XST_SUCCESS;
}

/* Get the miscellaneous status register values, summarize in a table           */

/* Get the miscellaneous control register and summarize in a table              */

/* Obtain and summarize the following: temperature, VCCINT, VCCAUX, VCCBRAM,    */
