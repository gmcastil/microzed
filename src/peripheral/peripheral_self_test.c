/*
 * Very basic check of an AXI4-Lite hardware peripheral based on Adam Taylor's
 * Microzed Chronicles Parts 21-27
 *
 * In brief, the hardware that supports this little endeavour was the default
 * 4-register AXI4-Lite slave that is created from 'Tools -> Create and Package
 * New IP'.  An important note is that I also have an XADC IP block on the same
 * AXI bus, so the location for the newly created IP reflects that.  I won't try
 * to recreate the address map here; presumably, if the reader of this comment
 * is sufficiently switched on to have gotten this far, they will understand
 * what needs to be changed (i.e., base address specified by the TaylorPtr
 * value) to get it to point at the correct place.
 *
 * Note also that, if you're using the Xilinx AXI4-Lite slave logic, it will
 * almost certainly not work in any sort of real application.  The Xilinx AXI
 * bus treatment is known to be garbage and non-compliant in a lot of cases, so
 * just because their driver code (which is equally trashy) returns an
 * XST_SUCCESS or whatever, does not mean that the underlying transaction was
 * serviced appropriately.  Include ILA cores at relevant locations with all
 * signals of interest to verify that things are actually happening.  As an
 * example, one can write to all manner of locations in the peripheral address
 * space and receive success indicators, but no actual hardware exists at that
 * location.
 *
 * If you blindly trust the Xilinx hardware implementations, you will eventually
 * get burned.  You've been warned.
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
