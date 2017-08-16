
read_verilog -library xil_defaultlib -sv ../../alpha/sys_defs.vh
set_property file_type "Verilog Header" [get_files ../../alpha/sys_defs.vh]

read_verilog -library xil_defaultlib -sv ../../alpha/ex_stage.v
read_verilog -library xil_defaultlib -sv ../../alpha/id_stage.v
read_verilog -library xil_defaultlib -sv ../../alpha/if_stage.v
read_verilog -library xil_defaultlib -sv ../../alpha/mem_stage.v
read_verilog -library xil_defaultlib -sv ../../alpha/pipeline.v
read_verilog -library xil_defaultlib -sv ../../alpha/regfile.v
read_verilog -library xil_defaultlib -sv ../../alpha/wb_stage.v


read_xdc %%CONSTRAINTS%%

