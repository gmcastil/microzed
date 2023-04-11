#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "ttc_dbg.h"

#include "xttcps.h"

/* Triple timer counter (TTC) registers */
#define TTC_DBG_TTC0_BASE		0xF8001000
#define TTC_DBG_TTC1_BASE		0xF8002000

/* Three clocks per TTC in the device */
#define TTC_DBG_CLK_CTRL_OFFSET		0x00000000
#define TTC_DBG_CNT_CTRL_OFFSET		0x0000000C
#define TTC_DBG_CNT_VAL_OFFSET		0x00000018
#define TTC_DBG_INTERVAL_VAL_OFFSET	0x00000024
#define TTC_DBG_MATCH_0_OFFSET		0x00000030
#define TTC_DBG_MATCH_1_OFFSET		0x0000003C
#define TTC_DBG_MATCH_2_OFFSET		0x00000048
#define TTC_DBG_ISR_OFFSET			0x00000054
#define TTC_DBG_IER_OFFSET			0x00000060
#define TTC_DBG_EVENT_CTRL_OFFSET	0x0000006C
#define TTC_DBG_EVENT_CNT_OFFSET	0x00000078

/* When you need a value to return when you weren't able to read something */
#define TTC_DBG_ERROR				0xDEADBEEF

void ttc_dbg_print_input_freq(XTtcPs *ttc)
{
	uint32_t clk_freq = ttc->Config.InputClockHz;
	printf("%-20s%"PRId32"\n", "Input clock", clk_freq);
	return;
}

void ttc_dbg_print_options(XTtcPs *ttc)
{
	printf("%-20s0x%08"PRIx32"\n", "Options", XTtcPs_GetOptions(ttc));
	return;
}

void ttc_dbg_print_interval(XTtcPs *ttc)
{
	printf("%-20s0x%04"PRIx16"\n", "Interval", (uint16_t) XTtcPs_GetInterval(ttc));
	printf("%-20s0x%02"PRIx8"\n", "Prescaler", XTtcPs_GetPrescaler(ttc));
	return;
}

/* Print the interrupt enable register */
void ttc_dbg_print_ier_status(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t ier = ttc_dbg_ier(ttc_id, counter_id);
	printf("%-30s%d\n", "Event overflow:", (int) (ier & 0x20) >> 5);
	printf("%-30s%d\n", "Counter overflow:", (int) (ier & 0x10) >> 4);
	printf("%-30s%d\n", "Match 3:", (int) (ier & 0x08) >> 3);
	printf("%-30s%d\n", "Match 2:", (int) (ier & 0x04) >> 2);
	printf("%-30s%d\n", "Match 1:", (int) (ier & 0x02) >> 1);
	printf("%-30s%d\n", "Interval:", (int) (ier & 0x01) >> 0);
	return;
}

/* Summarize current state of the counter */
void ttc_dbg_print_summary(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t clk_control = ttc_dbg_clk_ctrl(ttc_id, counter_id);
	uint32_t cnt_control = ttc_dbg_cnt_ctrl(ttc_id, counter_id);

	printf("%-30s%d\n", "External clock edge:", (int) (clk_control & 0x40) >> 6);
	printf("%-30s%d\n", "Clock source:", (int) (clk_control & 0x20) >> 5);
	printf("%-30s%d\n", "Prescale value:", (int) (clk_control & 0x1E) >> 1);
	printf("%-30s%d\n", "Prescale enable:", (int) (clk_control & 0x01) >> 0);

	printf("%-30s%d\n", "Waveform polarity:", (int) (cnt_control & 0x40) >> 6);
	printf("%-30s%d\n", "Waveform enable (active low):", (int) (cnt_control & 0x20) >> 5);
	printf("%-30s%d\n", "Counter reset:", (int) (cnt_control & 0x10) >> 4);
	printf("%-30s%d\n", "Match mode:", (int) (cnt_control & 0x08) >> 3);
	printf("%-30s%d\n", "Decrement:", (int) (cnt_control & 0x04) >> 2);
	printf("%-30s%d\n", "Interval mode:", (int) (cnt_control & 0x02) >> 1);
	printf("%-30s%d\n", "Disable counter", (int) (cnt_control & 0x01) >> 0);

	printf("%-30s0x%08"PRIx32"\n", "Current count:", ttc_dbg_cnt_value(ttc_id, counter_id));
	printf("%-30s0x%08"PRIx32"\n", "Interval count:", ttc_dbg_interval_val(ttc_id, counter_id));

	return;
}

/* Return a pointer to the appropriate offset for a particular TTC and internal counter */
uint32_t *ttc_dbg_get_addr(uint32_t ttc_id, uint32_t counter_id, uint32_t offset)
{
	uint32_t *base = NULL;
	uint32_t *addr = NULL;

	if ( ttc_id == 0 ) {
		base = (uint32_t *) TTC_DBG_TTC0_BASE;
	} else if ( ttc_id == 1 ) {
		base = (uint32_t *) TTC_DBG_TTC1_BASE;
	} else {
		base = NULL;
	}
	if ( base != NULL ) {
		addr = (uint32_t *) base + (offset / sizeof(offset));
	} else {
		addr = NULL;
	}
	return addr;
}

/* Return clock control register value */
uint32_t ttc_dbg_clk_ctrl(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_CLK_CTRL_OFFSET);
	uint32_t val = 0;

	if ( addr != NULL ) {
		/* Clock control is bits [6:0] */
		val = *addr & 0x7F;
	} else {
		val = TTC_DBG_ERROR;
	}
	return val;
}

/* Return operational mode and reset register value */
uint32_t ttc_dbg_cnt_ctrl(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_CNT_CTRL_OFFSET);
	uint32_t val = 0;

	if ( addr != NULL ) {
		/* Count control is bits [6:0] */
		val = *addr & 0x7F;
	} else {
		val = TTC_DBG_ERROR;
	}
	return val;
}

/* Return current counter value */
uint32_t ttc_dbg_cnt_value(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_CNT_VAL_OFFSET);
	uint32_t val = 0;

	if ( addr != NULL ) {
		/* Interval counter is bits [15:0] */
		val = *addr & 0xFFFF;
	} else {
		val = TTC_DBG_ERROR;
	}
	return val;
}

/* Return interval value */
uint32_t ttc_dbg_interval_val(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_INTERVAL_VAL_OFFSET);
	uint32_t val = 0;

	if ( addr != NULL ) {
		/* Interval counter is bits [15:0] */
		val = *addr & 0xFFFF;
	} else {
		val = TTC_DBG_ERROR;
	}
	return val;
}

uint32_t ttc_dbg_ier(uint32_t ttc_id, uint32_t counter_id)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_IER_OFFSET);
	uint32_t val = 0;

	if ( addr != NULL ) {
		/* Interrupt enable register for each counter is bits [5:0] */
		val = *addr & 0x3F;
	} else {
		val = TTC_DBG_ERROR;
	}
	return val;
}

/* Set the clock control register */
void ttc_dbg_set_clk_ctrl(uint32_t ttc_id, uint32_t counter_id, uint32_t val)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_CLK_CTRL_OFFSET);
	*addr = (0x7F & val);
	return;
}

/* Set the counter control register */
void ttc_dbg_set_cnt_ctrl(uint32_t ttc_id, uint32_t counter_id, uint32_t val)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_CNT_CTRL_OFFSET);
	*addr = (0x7F & val);
	return;
}

/* Set the interval value register */
void ttc_dbg_set_interval_val(uint32_t ttc_id, uint32_t counter_id, uint16_t val)
{
	uint32_t *addr = ttc_dbg_get_addr(ttc_id, counter_id, TTC_DBG_INTERVAL_VAL_OFFSET);
	*addr = val;
	return;
}

void ttc_dbg_rst(uint32_t ttc_id, uint32_t counter_id)
{
	/* Per the TRM, it appears that the disable bit needs to be set before any others */
	ttc_dbg_set_cnt_ctrl(0, 0, 0x00000021);
	ttc_dbg_set_clk_ctrl(0, 0, 0x00000000);
	return;
}
