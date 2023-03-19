#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "xttcps.h"

void ttc_dbg_print_input_freq(XTtcPs *ttc)
{
	uint32_t clk_freq = ttc->Config.InputClockHz;
	printf("%-20s%"PRId32"\n", "Input clock", clk_freq);
	return;
}

void ttc_dbg_print_options(XTtcPs *ttc)
{
	printf("%-20s%08"PRIx32"\n", "Options", XTtcPs_GetOptions(ttc));
	return;
}

void ttc_dbg_print_interval(XTtcPs *ttc)
{
	printf("%-20s%04"PRIx16"\n", "Interval", (uint16_t) XTtcPs_GetInterval(ttc));
	printf("%-20s%02"PRIx8"\n", "Prescaler", XTtcPs_GetPrescaler(ttc));
	return;
}

