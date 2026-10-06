# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AVED-gw contributors
#
# timing_v80.tcl - timing report from the routed checkpoint (no rebuild)
#   vivado -mode batch -nojournal -nolog -source timing_v80.tcl [-tclargs <routed.dcp>]
#
# Writes <build>/timing/{timing_summary,clocks,clock_interaction}.rpt and prints
# a per-clock summary (period, setup WNS, hold WHS, estimated fmax).

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set BUILD [file join $SCRIPT_DIR hw amd_v80_gen5x8_25.1 build]

if {$argc > 0} {
  set DCP [file normalize [lindex $argv 0]]
} else {
  set DCP [lindex [glob -nocomplain [file join $BUILD prj.runs impl_1 *_routed.dcp]] 0]
}
if {$DCP eq "" || ![file exists $DCP]} {
  error "routed checkpoint not found (run 'make hw' first, or pass -tclargs <routed.dcp>)"
}

set OUT [file join $BUILD timing]
file mkdir $OUT

open_checkpoint $DCP

report_timing_summary -max_paths 10 -report_unconstrained -file [file join $OUT timing_summary.rpt]
report_clocks                                           -file [file join $OUT clocks.rpt]
report_clock_interaction -delay_type min_max            -file [file join $OUT clock_interaction.rpt]

proc worst_slack {group type} {
  set p [get_timing_paths -group $group -delay_type $type -max_paths 1 -quiet]
  if {[llength $p] == 0} { return "" }
  return [get_property SLACK $p]
}

puts ""
puts "Checkpoint: $DCP"
puts [format "%-40s %10s %10s %10s %10s %10s" "Path group (clock)" "Period ns" "Freq MHz" "WNS ns" "WHS ns" "~Fmax MHz"]
puts [string repeat - 95]

set all_met 1
foreach grp [lsort [get_path_groups -quiet]] {
  set clk [get_clocks -quiet $grp]
  if {[llength $clk] == 0} { continue }
  set per [get_property PERIOD $clk]
  set wns [worst_slack $grp max]
  set whs [worst_slack $grp min]
  if {$wns eq "" && $whs eq ""} { continue }

  set freq [format "%.2f" [expr {1000.0 / $per}]]
  set fmax "-"
  if {$wns ne ""} {
    if {$wns < 0} { set all_met 0 }
    if {$per - $wns > 0} { set fmax [format "%.1f" [expr {1000.0 / ($per - $wns)}]] }
  } else { set wns "-" }
  if {$whs ne ""} { if {$whs < 0} { set all_met 0 } } else { set whs "-" }

  puts [format "%-40s %10.3f %10s %10s %10s %10s" [string range $grp 0 39] $per $freq $wns $whs $fmax]
}

puts [string repeat - 95]
puts [expr {$all_met ? "All timing constraints met." : "TIMING NOT MET - see timing_summary.rpt"}]
puts "user_accel clock: usr_clk_wiz clk_out1 (clk_usr_0)"
puts "Reports: $OUT"
