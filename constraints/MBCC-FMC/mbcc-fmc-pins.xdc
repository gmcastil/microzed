# User LED
#
# The Avnet FMC carrier card indexes LED differently depending on whether they
# are describing the net that drives the FET that turns on / off the LED or the
# diode itself on the board (i.e, LED0 on the JX1 pinout drives LED1 on the
# board, LED1 drives LED2, and so forth).
#
# Also, note that the Avnet board designers mismatch LVDS lane numbers all over
# the place, so one needs to very carefully trace these out until I've build a
# spreadsheet that just does it all for me.
#
# LED1 and LED2 are driven by JX1 PL logic, LED3 and LED4 are driven by JX2
# logic.
set_property PACKAGE_PIN R19     [get_ports {user_led[0]}]
set_property PACKAGE_PIN V13     [get_ports {user_led[1]}]
set_property PACKAGE_PIN K16     [get_ports {user_led[2]}]
set_property PACKAGE_PIN M15     [get_ports {user_led[3]}]

set_property IOSTANDARD LVCMOS33 [get_ports {user_led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led[3]}]

set_property PULLUP true         [get_ports {user_led[0]}]
set_property PULLUP true         [get_ports {user_led[1]}]
set_property PULLUP true         [get_ports {user_led[2]}]
set_property PULLUP true         [get_ports {user_led[3]}]

# PMOD Interfaces
#
# There are five PMOD connectors on the Avnet FMC carrier card (see notes):
#
#   PS - Bank 500
#   PA - Bank 35
#   PB - Bank 34
#   PY - Bank 13
#   PZ - Bank 13
#
# Notes:
#  - JY and JZ are only accessible when the board is populated with a 7020
#    device
#  - PS PMOD on bank 500 can be used for PJTAG access and is shared with the
#    PMOD connector on the Microzed proper. Both should not be used at the same
#    time
#  - The PS PMOD is connected to the carrier card 3.3V. All other PMOD
#    interfaces are powered by VADJ, which can be switched between 3.3V, 2.5V,
#    and 1.8V with jumper J6.
#
#    J6     VADJ
#    ---    ----
#    1-2    3.3V
#    3-4    2.5V
#    5-6    1.8V
#
#    The default setting from the factory is 1.8V and with no jumper installed,
#    VADJ will become 3.3V.
#  - JZ does not connect pin 7 of the PMOD connector to any of the IO voltages
#    and is not compliant to the Digilent PMOD description. Per the FMC board
#    user guide, it cannot be used as a QSPI or SD interface.
#  - PMOD JA does not support differential signaling on pins 3 and 4 (see FMC
#    and Microzed schematics for details). Note that again, for differential
#    signaling, the same warning about trusting the Avnet documentation applies.

set_property PACKAGE_PIN L16     [get_ports {pmod_ja[0]}]; # PMOD pin 1
set_property PACKAGE_PIN L17     [get_ports {pmod_ja[1]}]; # PMOD pin 2
set_property PACKAGE_PIN G14     [get_ports {pmod_ja[2]}]; # PMOD pin 3
set_property PACKAGE_PIN J15     [get_ports {pmod_ja[3]}]; # PMOD pin 4
set_property PACKAGE_PIN B19     [get_ports {pmod_ja[4]}]; # PMOD pin 7
set_property PACKAGE_PIN A20     [get_ports {pmod_ja[5]}]; # PMOD pin 8
set_property PACKAGE_PIN C20     [get_ports {pmod_ja[6]}]; # PMOD pin 9
set_property PACKAGE_PIN B20     [get_ports {pmod_ja[7]}]; # PMOD pin 10

set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[0]}]; # PMOD pin 1
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[1]}]; # PMOD pin 2
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[2]}]; # PMOD pin 3
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[3]}]; # PMOD pin 4
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[4]}]; # PMOD pin 7
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[5]}]; # PMOD pin 8
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[6]}]; # PMOD pin 9
set_property IOSTANDARD LVCMOS33 [get_ports {pmod_ja[7]}]; # PMOD pin 10

# set_property PACKAGE_PIN T11    [get_ports {pmod_jb[0]}]; # PMOD pin 1
# set_property PACKAGE_PIN T10    [get_ports {pmod_jb[1]}]; # PMOD pin 2
# set_property PACKAGE_PIN T12    [get_ports {pmod_jb[2]}]; # PMOD pin 3
# set_property PACKAGE_PIN U12    [get_ports {pmod_jb[3]}]; # PMOD pin 4
# set_property PACKAGE_PIN V12    [get_ports {pmod_jb[4]}]; # PMOD pin 7
# set_property PACKAGE_PIN W13    [get_ports {pmod_jb[5]}]; # PMOD pin 8
# set_property PACKAGE_PIN T14    [get_ports {pmod_jb[6]}]; # PMOD pin 9
# set_property PACKAGE_PIN T15    [get_ports {pmod_jb[7]}]; # PMOD pin 10
# 
# set_property PACKAGE_PIN U7     [get_ports {pmod_jy[0]}]; # PMOD pin 1
# set_property PACKAGE_PIN V7     [get_ports {pmod_jy[1]}]; # PMOD pin 2
# set_property PACKAGE_PIN T9     [get_ports {pmod_jy[2]}]; # PMOD pin 3
# set_property PACKAGE_PIN U10    [get_ports {pmod_jy[3]}]; # PMOD pin 4
# set_property PACKAGE_PIN V8     [get_ports {pmod_jy[4]}]; # PMOD pin 7
# set_property PACKAGE_PIN W8     [get_ports {pmod_jy[5]}]; # PMOD pin 8
# set_property PACKAGE_PIN T5     [get_ports {pmod_jy[6]}]; # PMOD pin 9
# set_property PACKAGE_PIN U5     [get_ports {pmod_jy[7]}]; # PMOD pin 10
# 
# set_property PACKAGE_PIN Y12    [get_ports {pmod_jz[0]}]; # PMOD pin 1
# set_property PACKAGE_PIN Y13    [get_ports {pmod_jz[1]}]; # PMOD pin 2
# set_property PACKAGE_PIN V11    [get_ports {pmod_jz[2]}]; # PMOD pin 3
# set_property PACKAGE_PIN V10    [get_ports {pmod_jz[3]}]; # PMOD pin 4
#                                                          # PMOD pin 7
# set_property PACKAGE_PIN V5     [get_ports {pmod_jz[5]}]; # PMOD pin 8
# set_property PACKAGE_PIN V6     [get_ports {pmod_jz[6]}]; # PMOD pin 9
# set_property PACKAGE_PIN W6     [get_ports {pmod_jz[7]}]; # PMOD pin 10
