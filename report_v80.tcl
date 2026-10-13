# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AVED-gw contributors
#
# report_v80.tcl - timing + resource usage summary from the routed checkpoint
# (no rebuild)
#   vivado -mode batch -nojournal -nolog -source report_v80.tcl [-tclargs <routed.dcp>]
#
# Writes <build>/reports/{timing_summary,clocks,clock_interaction,
# utilization,utilization_flat,summary}.rpt and prints a per-clock timing
# summary (period, setup WNS, hold WHS, estimated fmax) plus a condensed
# resource-usage table. This is the same report build_design.tcl generates
# automatically after every 'make hw'; use this to regenerate it without a
# rebuild, e.g. against an older checkpoint.

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set HW_DIR [file join $SCRIPT_DIR hw amd_v80_gen5x8_25.1]
set BUILD [file join $HW_DIR build]

if {$argc > 0} {
  set DCP [file normalize [lindex $argv 0]]
} else {
  set DCP [lindex [glob -nocomplain [file join $BUILD prj.runs impl_1 *_routed.dcp]] 0]
}
if {$DCP eq "" || ![file exists $DCP]} {
  error "routed checkpoint not found (run 'make hw' first, or pass -tclargs <routed.dcp>)"
}

source [file join $HW_DIR scripts report_summary.tcl]

open_checkpoint $DCP

set OUT [file join $BUILD reports]
rs_write_build_summary $OUT $DCP

puts "user_accel clock: usr_clk_wiz clk_out1 (clk_usr_0)"
