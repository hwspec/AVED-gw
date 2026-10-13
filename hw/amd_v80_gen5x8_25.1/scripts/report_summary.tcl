# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AVED-gw contributors
#
# report_summary.tcl - shared build-summary helpers (timing + resource usage)
#
# Intended to be sourced from a Vivado Tcl session that already has a design
# open, either via `open_run impl_1` (live build, see build_design.tcl) or
# `open_checkpoint <routed.dcp>` (see timing_v80.tcl / make timing).
#
# Entry point: rs_write_build_summary <out_dir> [label]
#   Writes <out_dir>/{timing_summary,clocks,clock_interaction,utilization,
#   utilization_flat,summary}.rpt, prints a condensed summary to stdout, and
#   returns 1 if all timing constraints are met, else 0.

proc rs_worst_slack {group type} {
  set p [get_timing_paths -group $group -delay_type $type -max_paths 1 -quiet]
  if {[llength $p] == 0} { return "" }
  return [get_property SLACK $p]
}

# Returns {all_met summary_text}
proc rs_timing_summary {out_dir} {
  file mkdir $out_dir
  report_timing_summary -max_paths 10 -report_unconstrained -file [file join $out_dir timing_summary.rpt]
  report_clocks                                  -file [file join $out_dir clocks.rpt]
  report_clock_interaction -delay_type min_max   -file [file join $out_dir clock_interaction.rpt]

  set lines {}
  lappend lines [format "%-40s %10s %10s %10s %10s %10s" "Path group (clock)" "Period ns" "Freq MHz" "WNS ns" "WHS ns" "~Fmax MHz"]
  lappend lines [string repeat - 95]

  set all_met 1
  foreach grp [lsort [get_path_groups -quiet]] {
    set clk [get_clocks -quiet $grp]
    if {[llength $clk] == 0} { continue }
    set per [get_property PERIOD $clk]
    set wns [rs_worst_slack $grp max]
    set whs [rs_worst_slack $grp min]
    if {$wns eq "" && $whs eq ""} { continue }

    set freq [format "%.2f" [expr {1000.0 / $per}]]
    set fmax "-"
    if {$wns ne ""} {
      if {$wns < 0} { set all_met 0 }
      if {$per - $wns > 0} { set fmax [format "%.1f" [expr {1000.0 / ($per - $wns)}]] }
    } else { set wns "-" }
    if {$whs ne ""} { if {$whs < 0} { set all_met 0 } } else { set whs "-" }

    lappend lines [format "%-40s %10.3f %10s %10s %10s %10s" [string range $grp 0 39] $per $freq $wns $whs $fmax]
  }

  lappend lines [string repeat - 95]
  lappend lines [expr {$all_met ? "All timing constraints met." : "TIMING NOT MET - see timing_summary.rpt"}]

  return [list $all_met [join $lines "\n"]]
}

# Returns a condensed resource-usage table as text; full detail goes to
# utilization.rpt / utilization_flat.rpt in out_dir.
proc rs_utilization_summary {out_dir} {
  file mkdir $out_dir
  report_utilization -file [file join $out_dir utilization.rpt] -hierarchical -hierarchical_depth 2
  set flat [file join $out_dir utilization_flat.rpt]
  report_utilization -file $flat

  set fh [open $flat r]
  set data [read $fh]
  close $fh
  set rows [split $data "\n"]

  set site_types {
    "CLB LUTs" "CLB Registers" "CARRY8" "Block RAM Tile" "URAM" "DSPs" "Bonded IOB" "BUFGCE"
  }

  set lines {}
  lappend lines [format "%-20s %14s %14s %10s" "Resource" "Used" "Available" "Util%"]
  lappend lines [string repeat - 62]
  foreach st $site_types {
    foreach row $rows {
      if {[string match "*| $st *" $row]} {
        set fields [split $row "|"]
        if {[llength $fields] >= 7} {
          set used  [string trim [lindex $fields 2]]
          set avail [string trim [lindex $fields 5]]
          set util  [string trim [lindex $fields 6]]
          lappend lines [format "%-20s %14s %14s %10s" $st $used $avail $util]
        }
        break
      }
    }
  }
  return [join $lines "\n"]
}

proc rs_write_build_summary {out_dir {label ""}} {
  file mkdir $out_dir
  lassign [rs_timing_summary $out_dir] all_met timing_text
  set util_text [rs_utilization_summary $out_dir]

  set summary_file [file join $out_dir summary.rpt]
  set fh [open $summary_file w]
  puts $fh "Build summary"
  puts $fh "============="
  if {$label ne ""} { puts $fh "Source: $label" }
  puts $fh "Generated: [clock format [clock seconds]]"
  puts $fh ""
  puts $fh "Timing"
  puts $fh "------"
  puts $fh $timing_text
  puts $fh ""
  puts $fh "Resource usage"
  puts $fh "--------------"
  puts $fh $util_text
  close $fh

  puts ""
  if {$label ne ""} { puts "Source: $label" }
  puts "Timing"
  puts [string repeat - 95]
  puts $timing_text
  puts ""
  puts "Resource usage"
  puts [string repeat - 62]
  puts $util_text
  puts ""
  puts "Summary written to: $summary_file"

  return $all_met
}
