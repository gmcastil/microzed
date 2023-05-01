This is a description of a process for running the FreeRTOS demo application on
the Zynq 7000 MicroZed development board.

First, download the RTOS source code and example projects. For this, we will be
working with the FreeRTOS v2022.12.01. Per the supported devices list, the
Zynq-7000 is supported and there is a demo that is intended for a ZC702
evaluation board (similar, but not identical to the Avnet MicroZed). A crucial
detail that I did not immediately understand was that the mechanics for building
it is to create a standalone BSP within the Xilinx SDK and build FreeRTOS as an
application.  We'll see how this goes.

The Free RTOS documentation basically says that the demo application has three
different options.  One is to just blink an LED using two tasks and a queue to
flash an LED (presumably this is an LED that is attached to the GPIO interface
and no PL is required). The second is what it refers to as 'full', which is a
more complicated test and demonstration suite.  A third, which I'm not going to
bother with right now, because getting a wired ethernet connection to my board
is a bit of a pain, incorporates the LWIP library and does some of this stuff
via the telnet port (albeit, without a true telnet server, which is fine).

My goal is to get the comprehensive FreeRTOS demo up and running with the
command line interface via serial port and my contact LED flashing in hardware
(I have one of the Digilent LED arrays attached to a PMOD connector).

Well, that was a pain - I suspect that there are incompatibilities between the
toolchains, the SDK Standalone library (v.7.0) that this version of the Xilinx
tools contains, and the prebuild project that ships with the latest version of
FreeRTOS (v2022.12.01).  Per rando experience on the internet, 2019.1 has been
able to work with 10.3.1, so I've clone the github repository, switched to the
V10.3.1 tag (use `git checkout tags/V10.3.1` for those of you that have no idea
how to use tags) and am going to try to get that to build instead.

Initially, I get this error in the build:

```bash
arm-none-eabi-gcc -Wall -O0 -g3 \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/lwIP_Demo/lwIP_port/include" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/lwIP_Demo/lwIP_port/netif" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Source/include" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CA9" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/include/ipv4" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/include" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/Full_Demo" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/include" \
	-I"/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src" \
	-I../../freertos10_xilinx_bsp_0/ps7_cortexa9_0/include \
	-c \
	-fmessage-length=0 \
	-MT"src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/ip.o" \
	-mcpu=cortex-a9 \
	-mfpu=vfpv3 \
	-mfloat-abi=hard \
	-Wextra \
	-ffunction-sections \
	-fdata-sections \
	-MMD \
	-MP \
	-MF"src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/ip.d" \
	-MT"src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/ip.o" \
	-o "src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/ip.o" "/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/core/ipv4/ip.c"

In file included from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Source/include/FreeRTOS.h:56,
                 from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/lwIP_Demo/lwIP_port/include/arch/sys_arch.h:35,
                 from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/include/lwip/sys.h:80,
                 from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/include/lwip/tcp.h:39,
                 from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/include/lwip/tcp_impl.h:39,
                 from /home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/Common/ethernet/lwip-1.4.0/src/core/ipv4/ip.c:52:
/home/castillo/git-repos/FreeRTOS/FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/FreeRTOSConfig.h:31:10: fatal error: xparameters.h: No such file or directory
 #include "xparameters.h"
          ^~~~~~~~~~~~~~~
compilation terminated.
make: *** [src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/subdir.mk:76: src/lwIP_Demo/lwip-1.4.0/src/core/ipv4/ip.o] Error 1

```
So the error I'm getting is essentially that when the source file, `ip.c` is
compiled, it includes the file xparameters.h which is not available in any of
those 10 or so include directories.

`/home/castillo/git-repos/microzed/proj/freertos_base/freertos_base.sdk/freertos10_xilinx_bsp_0/ps7_cortexa9_0/include/xparameters.h`

This makes me think that the problem is importing the project doesn't get the
include paths the same, so it's unable to find this missing header.  I think the
relative path that it inserted is wrong (big surprise there).

