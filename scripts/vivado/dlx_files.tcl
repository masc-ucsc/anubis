
read_verilog -library xil_defaultlib -sv ../../dlx/globals.v
read_verilog -library xil_defaultlib -sv ../../dlx/alu.v
read_verilog -library xil_defaultlib -sv ../../dlx/alu_control.v
read_verilog -library xil_defaultlib -sv ../../dlx/bypass_ex.v
read_verilog -library xil_defaultlib -sv ../../dlx/bypass_id.v
read_verilog -library xil_defaultlib -sv ../../dlx/cpu.v
read_verilog -library xil_defaultlib -sv ../../dlx/decode.v
read_verilog -library xil_defaultlib -sv ../../dlx/dlx_defs.v
read_verilog -library xil_defaultlib -sv ../../dlx/ff.v
read_verilog -library xil_defaultlib -sv ../../dlx/quick_compare.v
read_verilog -library xil_defaultlib -sv ../../dlx/regfile.v
read_verilog -library xil_defaultlib -sv ../../dlx/wb_control.v


read_xdc %%CONSTRAINTS%%

