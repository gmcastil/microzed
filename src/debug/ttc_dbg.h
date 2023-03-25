#ifndef TTC_DBG_H_
#define TTC_DBG_H_

#include "xttcps.h"

/* Debug functions for interacting with the Xilinx TTC standalone driver objects */
void ttc_dbg_print_input_freq(XTtcPs *ttc);
void ttc_dbg_print_options(XTtcPs *ttc);
void ttc_dbg_print_interval(XTtcPs *ttc);

/* Lower level functions for working with the TTC directly without a driver */
uint32_t *ttc_dbg_get_addr(uint32_t ttc_id, uint32_t counter_id, uint32_t offset);
uint32_t ttc_dbg_clk_ctrl(uint32_t ttc_id, uint32_t counter_id);
uint32_t ttc_dbg_cnt_ctrl(uint32_t ttc_id, uint32_t counter_id);
uint32_t ttc_dbg_cnt_value(uint32_t ttc_id, uint32_t counter_id);
uint32_t ttc_dbg_interval_val(uint32_t ttc_id, uint32_t counter_id);
uint32_t ttc_dbg_ier(uint32_t ttc_id, uint32_t counter_id);

/* Straight memory writes to the counter and clock control regs so, take care */
void ttc_dbg_set_clk_ctrl(uint32_t ttc_id, uint32_t counter_id, uint32_t val);
void ttc_dbg_set_cnt_ctrl(uint32_t ttc_id, uint32_t counter_id, uint32_t val);
void ttc_dbg_set_interval_val(uint32_t ttc_id, uint32_t counter_id, uint16_t val);
void ttc_dbg_rst(uint32_t ttc_id, uint32_t counter_id);

void ttc_dbg_print_ier_status(uint32_t ttc_id, uint32_t counter_id);
void ttc_dbg_print_summary(uint32_t ttc_id, uint32_t counter_id);

#endif /* TTC_DBG_H_ */
