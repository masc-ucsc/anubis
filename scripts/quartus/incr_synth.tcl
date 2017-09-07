
set DELTA_ERROR "%%DELTA%%"
project_open -revision $TOP $TOP

set_global_assignment -name VERILOG_MACRO $DELTA_ERROR

export_assignments
execute_flow -recompile
execute_module -tool map
execute_module -tool fit

load_package flow
load_package report
load_report

execute_module -tool sta
execute_module -tool pow

set panel {TimeQuest Timing Analyzer||Multicorner Timing Analysis Summary}
set id [get_report_panel_id $panel]
write_report_panel -id $id -file timing_report_panel


project_close
