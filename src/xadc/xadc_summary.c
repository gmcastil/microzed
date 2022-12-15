/* Example code for interacting with the Xilinx XADC block */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "xparameters.h"
#include "platform.h"
#include "xstatus.h"
#include "xadcps.h"

#define XADC_DEVICE_ID XPAR_XADCPS_0_DEVICE_ID

/*
 * The XADC driver API documentation does not describe this value it had to be
 * determined by inspection
*/
#define XADC_ISREADY 0x11111111

#define ASCII_ESC 27

/* Fix a couple missing function prototypes from the XADC driver API */
uint32_t XAdcPs_GetConfigRegister(XAdcPs *);
uint32_t XAdcPs_GetMiscCtrlRegister(XAdcPs *);
uint32_t XAdcPs_GetMiscStatus(XAdcPs *);
uint32_t XAdcPs_IntrGetEnabled(XAdcPs *);
uint32_t XAdcPs_IntrGetStatus(XAdcPs *);

uint8_t XAdcPs_GetSequencerMode(XAdcPs *);
uint16_t XAdcPs_GetAlarmEnables(XAdcPs *);
uint32_t XAdcPs_GetSeqInputMode(XAdcPs *);
uint32_t XAdcPs_GetSeqChEnables(XAdcPs *);

void XAdcPs_SetSequencerMode(XAdcPs *, uint8_t);
void XAdcPs_SetAlarmEnables(XAdcPs *, uint16_t);
int XAdcPs_SetSeqInputMode(XAdcPs *, uint32_t);
int XAdcPs_SetSeqChEnables(XAdcPs *, uint32_t);


void print_internal_parameters(XAdcPs *InstancePtr)
{
	uint32_t raw_data;
	float conv_data;

	uint32_t seq_ch_sel_mask = 0x00000000
					| XADCPS_SEQ_CH_VCCPINT
					| XADCPS_SEQ_CH_VCCPAUX
					| XADCPS_SEQ_CH_VCCPDRO
					| XADCPS_SEQ_CH_TEMP
					| XADCPS_SEQ_CH_VCCINT
					| XADCPS_SEQ_CH_VCCAUX
					| XADCPS_SEQ_CH_VPVN
					| XADCPS_SEQ_CH_VBRAM;

	uint16_t alarm_en_mask = 0x0000;
	
	XAdcPs_SetSequencerMode(InstancePtr, XADCPS_SEQ_MODE_SINGCHAN);
	XAdcPs_SetAlarmEnables(InstancePtr, alarm_en_mask);
	XAdcPs_SetSeqInputMode(InstancePtr, XADCPS_SEQ_MODE_SAFE);
    	XAdcPs_SetSeqChEnables(InstancePtr, seq_ch_sel_mask);

	fprintf(stdout, "\n");
	/* On die temperature (C) */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_TEMP);
	conv_data = XAdcPs_RawToTemperature(raw_data);
	fprintf(stdout, "On-Die Temp \t\t\t%.2fC\n", conv_data);

	/* VCCPINT */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VCCPINT);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCPINT \t\t\t%1.5fV\n", conv_data);

	/* VBRAM */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VBRAM);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCBRAM \t\t\t%1.5fV\n", conv_data);

	/* VCCAUX */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VCCAUX);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCAUX \t\t\t\t%1.5fV\n", conv_data);

	/* VCCINT */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VCCINT);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCINT \t\t\t\t%1.5fV\n", conv_data);

	/* VCCPAUX */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VCCPAUX);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCPAUX \t\t\t%1.5fV\n", conv_data);

	/* VCCDDR */
	raw_data = XAdcPs_GetAdcData(InstancePtr, XADCPS_CH_VCCPDRO);
	conv_data = XAdcPs_RawToVoltage(raw_data);
	fprintf(stdout, "VCCDDR \t\t\t\t%1.5fV\n", conv_data);

	return;

}

/* See UG480 for a description of XADC sequencer modes */
void print_sequencer_mode(XAdcPs *InstancePtr)
{
	uint8_t sequencer_mode = XAdcPs_GetSequencerMode(InstancePtr);

	fprintf(stdout, "XADC sequencing mode is: \t");
	switch (sequencer_mode) {
	case XADCPS_SEQ_MODE_SAFE:
		fprintf(stdout, "Default safe mode\n");
		break;
    	case XADCPS_SEQ_MODE_ONEPASS:
		fprintf(stdout, "One pass through sequence\n");
		break;
    	case XADCPS_SEQ_MODE_CONTINPASS:
		fprintf(stdout, "Continuous channel sequencing\n");
		break;
    	case XADCPS_SEQ_MODE_SINGCHAN:
		fprintf(stdout, "Single channel/Sequencer off\n");
		break;
    	case XADCPS_SEQ_MODE_SIMUL_SAMPLING:
		fprintf(stdout, "Simulataneous sampling mode\n");
		break;
    	case XADCPS_SEQ_MODE_INDEPENDENT:
		fprintf(stdout, "Independent mode\n");
		break;
	default:
		fprintf(stdout, "Unknown mode 0x%02"PRIx8"\n", sequencer_mode);
		break;
	}

	return;
}

void print_alarm_enables(XAdcPs *InstancePtr)
{
	uint16_t alarm_enables = XAdcPs_GetAlarmEnables(InstancePtr);
	fprintf(stdout, "XADC alarm enables: \t\t0x%04"PRIx16"\n", alarm_enables);
	return;
}

void print_seq_input_mode(XAdcPs *InstancePtr)
{
	uint32_t seq_input_mode = XAdcPs_GetSeqInputMode(InstancePtr);
	fprintf(stdout, "XADC channel input mode: \t0x%08"PRIx32"\n", seq_input_mode);
	return;
}

void print_seq_ch_enables(XAdcPs *InstancePtr)
{
	uint32_t seq_ch_enables = XAdcPs_GetSeqChEnables(InstancePtr);
	fprintf(stdout, "XADC channel enables: \t\t0x%08"PRIx32"\n", seq_ch_enables);
	return;
}

void summary_xadcif_int_sts(XAdcPs *InstancePtr)
{
	/* XADC interface interrupt status (devcfg.XADCIF_INT_STS) */
	uint32_t reg32_read = 0;

	reg32_read = (uint32_t) XAdcPs_IntrGetStatus(InstancePtr);

	fprintf(stdout, "XADCIF_INT_STS (0xF8007104)\n");
	fprintf(stdout, "---------------------------\n");
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "reserved",
			(reg32_read >> 10));
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFO_LTH",
			(reg32_read & XADCPS_INTX_CFIFO_LTH_MASK) >> 9);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFO_GTH",
			(reg32_read & XADCPS_INTX_DFIFO_GTH_MASK) >> 8);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "OT",
			(reg32_read & XADCPS_INTX_OT_MASK) >> 7);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "ALM (7b)",
			(reg32_read & XADCPS_INTX_ALM_ALL_MASK) >> 0);
	fprintf(stdout, "\n");

	return;
}

void summary_xadcif_int_mask(XAdcPs *InstancePtr)
{
 	/* XADC interface interrupt status (devcfg.XADCIF_INT_MASK) */
	uint32_t reg32_read = 0;

	reg32_read = (uint32_t) XAdcPs_IntrGetEnabled(InstancePtr);

	fprintf(stdout, "XADCIF_INT_MASK (0xF8007108)\n");
	fprintf(stdout, "---------------------------\n");
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "reserved",
			(reg32_read >> 10));
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFO_LTH",
			(reg32_read & XADCPS_INTX_CFIFO_LTH_MASK) >> 9);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFO_GTH",
			(reg32_read & XADCPS_INTX_DFIFO_GTH_MASK) >> 8);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "OT",
			(reg32_read & XADCPS_INTX_OT_MASK) >> 7);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "ALM (7b)",
			(reg32_read & XADCPS_INTX_ALM_ALL_MASK) >> 0);
	fprintf(stdout, "\n");

	return;
}

void summary_xadcif_mctl(XAdcPs *InstancePtr)
{
	/* XADC interface miscellaneous control (devcfg.XADCIF_MCTL) */
	uint32_t reg32_read = 0;

	reg32_read = (uint32_t) XAdcPs_GetMiscCtrlRegister(InstancePtr);

	fprintf(stdout, "XADCIF_MCTL (0xF8007118)\n");
	fprintf(stdout, "------------------------\n");
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "RESET",
			(reg32_read & XADCPS_MCTL_RESET_MASK) >> 4);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "FLUSH",
			(reg32_read & XADCPS_MCTL_FLUSH_MASK) >> 0);
	fprintf(stdout, "\n");

	return;
}

void summary_xadcif_msts(XAdcPs *InstancePtr)
{
	/* XADC interface miscellaneous status (devcfg.XADCIF_MSTS) */
	uint32_t reg32_read = 0;

	reg32_read = (uint32_t) XAdcPs_GetMiscStatus(InstancePtr);

	fprintf(stdout, "XADCIF_MSTS (0xF800710C)\n");
	fprintf(stdout, "------------------------\n");
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFO_LVL (4b)",
			(reg32_read & XADCPS_MSTS_CFIFO_LVL_MASK) >> 16);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFO_LVL (4b)",
			(reg32_read & XADCPS_MSTS_DFIFO_LVL_MASK) >> 12);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFOF",
			(reg32_read & XADCPS_MSTS_CFIFOF_MASK) >> 11);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "CFIFOE",
			(reg32_read & XADCPS_MSTS_CFIFOE_MASK) >> 10);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFOF",
			(reg32_read & XADCPS_MSTS_DFIFOF_MASK) >> 9);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "DFIFOE",
			(reg32_read & XADCPS_MSTS_DFIFOE_MASK) >> 8);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "OT",
			(reg32_read & XADCPS_MSTS_OT_MASK) >> 7);
	fprintf(stdout, "%-20s %10" PRIx32 "\n", "ALM (7b)",
			(reg32_read & XADCPS_MSTS_ALM_MASK) >> 0);
	fprintf(stdout, "\n");

	return;
}

void summary_xadcif_cfg(XAdcPs *InstancePtr)
{
	/* XADC interface configuration register (devcfg.XADCIF_CFG) */
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
			(reg32_read & XADCPS_CFG_IGAP_MASK) >> 0);
	fprintf(stdout, "\n");

	return;
}

int main(int args, char *argv[])
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
	fprintf(stdout, "XADC Summary Application\n");
	fprintf(stdout, "========================\n");
	fprintf(stdout, "\n");

	/* Look up the device configuration based on the unique device ID */
	ConfigPtr = XAdcPs_LookupConfig(XADC_DEVICE_ID);
	if (ConfigPtr == NULL) {
		fprintf(stderr, "No configuration for XADC device ID %d found\n",
				XADC_DEVICE_ID);
		return XST_DEVICE_NOT_FOUND;
	}

	/* Initialize the XADC */
	xst_status = XAdcPs_CfgInitialize(InstancePtr, ConfigPtr,
						ConfigPtr->BaseAddress);

	if (xst_status == XST_SUCCESS) {
		fprintf(stdout, "Initialization complete\n");
		fprintf(stdout, "\n");
	} else {
		fprintf(stderr, "Initialization could not complete\n");
		return XST_FAILURE;
	}

	/* Run a self test on the XADC */
	xst_status = XAdcPs_SelfTest(InstancePtr);
	if (xst_status == XST_SUCCESS) {
		fprintf(stdout, "Self test complete\n");
		fprintf(stdout, "\n");
	} else {
		fprintf(stderr, "Self test failed\n");
		return XST_FAILURE;
	}

	/* Reset the XADC and repeat these */
	fprintf(stdout, "Resetting XADC instance...");

	XAdcPs_Reset(InstancePtr);
	if (InstancePtr->IsReady == XADC_ISREADY) {
		fprintf(stdout, "XADC is ready\n");
		fprintf(stdout, "\n");
	} else {
		fprintf(stderr, "XADC is not ready\n");
		return XST_FAILURE;
	}

	summary_xadcif_cfg(InstancePtr);
	summary_xadcif_msts(InstancePtr);
	summary_xadcif_mctl(InstancePtr);
	summary_xadcif_int_sts(InstancePtr);
	summary_xadcif_int_mask(InstancePtr);

	print_sequencer_mode(InstancePtr);
	print_alarm_enables(InstancePtr);
	print_seq_input_mode(InstancePtr);
	print_seq_ch_enables(InstancePtr);

	print_internal_parameters(InstancePtr);

	cleanup_platform();
	return XST_SUCCESS;
}

