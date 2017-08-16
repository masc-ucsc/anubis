
set PATHFPU "../../fpu"
read_verilog $PATHFPU/rtl/scoore_fpu.h
read_verilog $PATHFPU/rtl/dfuncs.h

set_property file_type "Verilog Header" [get_files $PATHFPU/rtl/scoore_fpu.h]
set_property file_type "Verilog Header" [get_files $PATHFPU/rtl/dfuncs.h]

read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/stage_flop_retry.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_divider_selector_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/scoore_fpu.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_propagate_nan_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/stage.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/register.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_div64_to_64.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_lead0.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_divide_result_selector_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_propagate_div.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_sqrt64.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_div64_post_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_sqrt64_post_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_mult64.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_div64.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_mult64_post_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_add64_post_comb.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_op_predec.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_add64.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_result_queue.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_normalize_fp.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_ex.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_denorm.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fp_normalize.v
read_verilog -library xil_defaultlib -sv $PATHFPU/rtl/fpu.v

read_xdc %%CONSTRAINTS%%

