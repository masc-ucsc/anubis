
set_global_assignment -name POWER_PRESET_COOLING_SOLUTION "23 MM HEAT SINK WITH 200 LFPM AIRFLOW"
set_global_assignment -name POWER_BOARD_THERMAL_MODEL "NONE (CONSERVATIVE)"
set_global_assignment -name PARTITION_NETLIST_TYPE SOURCE -section_id Top
set_global_assignment -name PARTITION_FITTER_PRESERVATION_LEVEL PLACEMENT_AND_ROUTING -section_id Top
set_global_assignment -name PARTITION_COLOR 16764057 -section_id Top
set_instance_assignment -name PARTITION_HIERARCHY root_partition -to | -section_id Top

#create_clock -period %%PERIOD%% -name clk [get_ports %%CLK%%]
set_global_assignment -name VERILOG_MACRO "%%DEFINE%%"

export_assignments
execute_flow -analysis_and_elaboration
execute_flow -compile
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

