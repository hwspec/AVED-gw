# program_v80.tcl - JTAG-program the V80 with the no-FPT PDI
#   vivado -mode batch -source program_v80.tcl [-tclargs <pdi>]
set SCRIPT_DIR [file dirname [file normalize [info script]]]
set PDI [file join $SCRIPT_DIR hw amd_v80_gen5x8_25.1 build amd_v80_gen5x8_25.1_nofpt.pdi]
if {$argc > 0} { set PDI [lindex $argv 0] }
if {![file exists $PDI]} { error "PDI not found: $PDI (run 'make hw' first)" }

open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target
set dev [lindex [get_hw_devices xcv80*] 0]
if {$dev eq ""} { error "No xcv80 device found" }
current_hw_device $dev
set_property PROGRAM.FILE $PDI $dev
program_hw_devices $dev
puts "Programmed $dev with $PDI"
