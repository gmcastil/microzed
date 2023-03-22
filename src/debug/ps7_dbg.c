#include <stdint.h>
#include <stdlib.h>

/*
 * The values and operations in these functions are intentionally hard-code
 * and do not include portions of the Xilinx BSP or any header files. Any
 * errors or mistakes are entirely my own - that is the entire point of this
 * module. Do not refactor to try referencing Xilinx crap!
 */

/* System level control (SLCR) registers */
#define PS7_DBG_SLCR_PSS_IDCODE		0xF8000530

/* Device configuration (devfg) registers */
#define PS7_DBG_DEVCFG_MCTRL		0xF8007080

uint32_t ps7_dbg_get_ps_version()
{
	uint32_t *addr = (uint32_t *) PS7_DBG_DEVCFG_MCTRL;
	uint32_t val = *addr;
	/* PS version is bits [31:28] */
	return (val & 0xF0000000) >> 28;
}

uint32_t ps7_dbg_get_mfr_id()
{
	uint32_t *addr = (uint32_t *) PS7_DBG_SLCR_PSS_IDCODE;
	uint32_t val = *addr;
	/* Manufacturer ID is bits [11:1] and should always return 0x49 */
	return (val & 0xFFE) >> 1;
}

uint32_t ps7_dbg_get_device_code()
{
	uint32_t *addr = (uint32_t *) PS7_DBG_SLCR_PSS_IDCODE;
	uint32_t val = *addr;
	/*
	 * Device code is bits [16:12] and returns one of the following:
	 * 7010: 0x02
	 * 7015: 0x1b
	 * 7020: 0x07
	 * 7030: 0x0c
	 * 7045: 0x11
	 */
	return (val & 0x1F000) >> 12;
}
