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
