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


