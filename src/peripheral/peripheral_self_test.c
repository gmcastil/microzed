/*
 * Very basic check of an AXI4-Lite hardware peripheral based on Adam Taylor's
 * Microzed Chronicles Parts 21-27
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "platform.h"
#include "xil_printf.h"

/* 
 * This is the top level header file created by the 'Create New IP' wizard - for
 * lack of a better term, it is the "driver" for the AXI peripheral.  The SDK
 * includes all of the source code in the exported directory so it contains
 * references to functions that aren't in the corresponding source code and
 * relies on the fact that the SDK is configured to compile every C source file
 * found in that directory (which is why just firing off the self test function
 * defined in that header actually worked.
 */
#include "taylor_uzed.h"

int main()
{
    init_platform();

    /* 
     * These are the familiar XST_SUCCESS and XST_FAILURE definitions - so we
     * try to fit in nicely with the existing ecosystem
     */
    XStatus Status;
    /* 
     * This address comes from examining the location that the peripheral was
     * placed in the exported hardware definition - there may be a file of some
     * sort that allows this to be done without having to manually have prior
     * knowledge of this.
     */
    uint32_t *TaylorPtr = (uint32_t *) 0x43C10000;

    printf("Performing AXI4-Lite peripheral self test\n");

    Status = TAYLOR_UZED_Reg_SelfTest(TaylorPtr);
    if ( Status != XST_SUCCESS ) {
    	printf("Self test failed\n");
    } else {
    	printf("Self test passed\n");
    }

    cleanup_platform();
    return 0;
}
