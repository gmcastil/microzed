#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "platform.h"
#include "xil_io.h"

#include "ps7_dbg.h"
#include "taylor_uzed.h"

#define ASCII_ESC				27

#define TAYLOR_ADD				0x01
#define TAYLOR_SUB				0x02
#define TAYLOR_MULT				0x03

void clear_console()
{
	 fprintf(stdout, "%c[2J", ASCII_ESC);
	 fflush(stdout);
	 return;
}

void print_sanity()
{
	uint32_t *devcfg_ptr = (uint32_t *) 0xF8007080;
	uint32_t value = Xil_In32((uint32_t) devcfg_ptr);
	printf("Addr: %8p Data: 0x%08"PRIx32"\n", devcfg_ptr, value);
	return;
}

void print_summary(uint32_t *TaylorPtr, int NumRegs)
{
	uint32_t value = 0;
	uint32_t *ptr = TaylorPtr;

	int i = 0;

	for (i = 0; i < NumRegs; i++) {
		value = Xil_In32((uint32_t) ptr);
		printf("Addr: %8p Data: 0x%08"PRIx32"\n", ptr++, value);
	}
	ptr = TaylorPtr;

	return;
}

int main()
{
    init_platform();

    uint32_t value = 0;
    uint32_t *TaylorPtr = (uint32_t *) 0x43C10000;

	clear_console();
	printf("%-20s0x%08"PRIx32"\n", "Device Code:", ps7_dbg_get_device_code());
	printf("%-20s0x%08"PRIx32"\n", "Mfr ID:", ps7_dbg_get_mfr_id());
	printf("%-20s0x%08"PRIx32"\n", "PS Version:", ps7_dbg_get_ps_version());

    /* Test addition */
    printf("Addition\n");
    printf("--------\n");
    Xil_Out32((uint32_t) TaylorPtr + 0x0004, 0x314);
    Xil_Out32((uint32_t) TaylorPtr + 0x0008, 0x1420);
    Xil_Out32((uint32_t) TaylorPtr, TAYLOR_ADD);
    print_summary(TaylorPtr, 4);

    /* Test subtraction */
    printf("Subtraction\n");
    printf("-----------\n");
    Xil_Out32((uint32_t) TaylorPtr, TAYLOR_SUB);
    print_summary(TaylorPtr, 4);

    /* Test multiplication */
    printf("Multiplication\n");
    printf("--------------\n");
    Xil_Out32((uint32_t) TaylorPtr, TAYLOR_MULT);
    print_summary(TaylorPtr, 4);

    /* Test bad command */
    printf("Bad Command\n");
    printf("-----------\n");
    Xil_Out32((uint32_t) TaylorPtr, 0xDEADBEE0);
    print_summary(TaylorPtr, 4);

    cleanup_platform();
    return 0;
}
