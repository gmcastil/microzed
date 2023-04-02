#ifndef WDT_DBG_H_
#define WDT_DBG_H_

#include "xscuwdt.h"

void wdt_dbg_print_reset(XScuWdt *Wdt, int text);
void wdt_dbg_print_control(XScuWdt *Wdt);
void wdt_dbg_print_load(XScuWdt *Wdt);
void wdt_dbg_print_counter(XScuWdt *Wdt);
void wdt_dbg_print_interrupt(XScuWdt *Wdt);
void wdt_dbg_print_status(XScuWdt *Wdt, int text);


#endif /* WDT_DBG_H_ */
