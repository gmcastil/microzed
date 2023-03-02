#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "xscuwdt.h"

void wdt_dbg_print_reset(XScuWdt *Wdt, int text)
{
	uint32_t reg = 0;
	reg = XScuWdt_ReadReg((Wdt)->Config.BaseAddr, XSCUWDT_RST_STS_OFFSET);
	printf("Reset Reg:\t\t\t");
	if ( text ) {
		if ( ( reg & XSCUWDT_RST_STS_RESET_FLAG_MASK ) == 1 ) {
			printf("RESET\n");
		} else {
			printf("CLEAR\n");
		}
	} else {
		printf("0x%08"PRIx32"\n", reg);
	}
}

void wdt_dbg_print_control(XScuWdt *Wdt)
{
	uint32_t reg = 0;
	reg = XScuWdt_GetControlReg(Wdt);
	printf("Control Reg:\t\t\t0x%08"PRIx32"\n", reg);
}

void wdt_dbg_print_load(XScuWdt *Wdt)
{
	uint32_t reg = 0;
	reg = XScuWdt_ReadReg((Wdt->Config.BaseAddr), XSCUWDT_LOAD_OFFSET);
	printf("Load Reg:\t\t\t0x%08"PRIx32"\n", reg);
}

void wdt_dbg_print_counter(XScuWdt *Wdt)
{
	uint32_t reg = 0;
	reg = XScuWdt_ReadReg((Wdt->Config.BaseAddr), XSCUWDT_COUNTER_OFFSET);
	printf("Counter Reg:\t\t\t0x%08"PRIx32"\n", reg);
}

void wdt_dbg_print_interrupt(XScuWdt *Wdt)
{
	uint32_t reg = 0;
	reg = XScuWdt_ReadReg((Wdt->Config.BaseAddr), XSCUWDT_ISR_OFFSET);
	printf("Interrupt Reg:\t\t\t0x%08"PRIx32"\n", reg);
}

void wdt_dbg_print_status(XScuWdt *Wdt, int text)
{
	wdt_dbg_print_load(Wdt);
	wdt_dbg_print_counter(Wdt);
	wdt_dbg_print_control(Wdt);
	wdt_dbg_print_interrupt(Wdt);
	wdt_dbg_print_reset(Wdt, text);
}
