//******************************************************************************
// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.
//******************************************************************************
/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Modulename :  wb_stage.v                                          //
//                                                                     //
//  Description :   writeback (WB) stage of the pipeline;              //
//                  determine the destination register of the          //
//                  instruction and write the result to the register   //
//                  file (if not to the zero register), also reset the //
//                  NPC in the fetch stage to the correct next PC      //
//                  address.                                           //
//                                                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

`timescale 1ns/100ps
`include "sys_defs.vh"


module wb_stage(// Inputs
                CLOCK,
                RESET,
                mem_wb_NPC,
                mem_wb_result,
                mem_wb_take_branch,
                mem_wb_dest_reg_idx,

                // Outputs
                reg_wr_data_out,
                reg_wr_idx_out,
                reg_wr_en_out
               );

  input         CLOCK;               // system clock
  input         RESET;               // system RESET
  input  [63:0] mem_wb_NPC;          // incoming instruction PC+4
  input  [63:0] mem_wb_result;       // incoming instruction result
  input         mem_wb_take_branch;
  input   [4:0] mem_wb_dest_reg_idx; // dest index (ZERO_REG if no writeback)

  output [63:0] reg_wr_data_out;     // register writeback data
  output  [4:0] reg_wr_idx_out;      // register writeback index
  output        reg_wr_en_out;       // register writeback enable
                                     // Always enabled if valid inst
`ifndef ANUBIS_NOC_3
  wire   [63:0] result_mux;
`else
  wire   [63:0] result_mux_anubis;
`endif

   // Mux to select register writeback data:
   // ALU/MEM result, unless taken branch, in which case we write
   // back the old NPC as the return address.  Note that ALL branches
   // and jumps write back the 'link' value, but those that don't
   // want it specify ZERO_REG as the destination.
`ifndef ANUBIS_NOC_3
  assign result_mux = (mem_wb_take_branch) ? mem_wb_NPC : mem_wb_result;
`else
  assign result_mux_anubis = (mem_wb_take_branch) ? mem_wb_NPC : mem_wb_result;
`endif

   // Generate signals for write-back to register file
   // reg_wr_en_out computation is sort of overkill since the reg file
   // has a special way of handling `ZERO_REG but there is no harm
   // in putting this here.  Hopefully it illustrates how the pipeline works.
  assign reg_wr_en_out  = mem_wb_dest_reg_idx!=`ZERO_REG;
  assign reg_wr_idx_out = mem_wb_dest_reg_idx;
`ifndef ANUBIS_NOC_3
  assign reg_wr_data_out = result_mux;
`else
  assign reg_wr_data_out = result_mux_anubis;
`endif

endmodule // module wb_stage

