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
//   Modulename :  mem_stage.v                                         //
//                                                                     //
//  Description :  memory access (MEM) stage of the pipeline;          //
//                 this stage accesses memory for stores and loads,    // 
//                 and selects the proper next PC value for branches   // 
//                 based on the branch condition computed in the       //
//                 previous stage.                                     // 
//                                                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

`timescale 1ns/100ps
`include "sys_defs.vh"

module mem_stage(// Inputs
                 CLOCK,
                 RESET,
                 ex_mem_rega,
                 ex_mem_alu_result,
                 ex_mem_rd_mem,
                 ex_mem_wr_mem,
                 Dmem2proc_bus,
                 
                 // Outputs
                 mem_result_out,
                 proc2Dmem_command,
                 proc2Dmem_bus
                );

  input         CLOCK;             // system clock
  input         RESET;             // system RESET
  input  [63:0] ex_mem_rega;       // regA value from reg file (store data)
  input  [63:0] ex_mem_alu_result; // incoming ALU result from EX
  input         ex_mem_rd_mem;     // read memory? (from decoder)
  input         ex_mem_wr_mem;     // write memory? (from decoder)
  input  [63:0] Dmem2proc_bus;

  output [63:0] mem_result_out;    // outgoing instruction result (to MEM/WB)
  output [1:0]  proc2Dmem_command;
  output [63:0] proc2Dmem_bus;     // Address sent to data-memory

  reg [63:0] saved_store_data;
  reg        do_store_data;

   // Determine the command that must be sent to mem
  assign proc2Dmem_command =
    do_store_data ? `BUS_STORE_DATA
                  : ex_mem_wr_mem ? `BUS_STORE_ADDR
                                  : ex_mem_rd_mem ? `BUS_LOAD
                                                  : `BUS_NONE;

   // The memory address is calculated by the ALU
  assign proc2Dmem_bus = do_store_data ? saved_store_data : ex_mem_alu_result;

`ifndef ANUBIS_LOCAL_1
   // Assign the result-out for next stage
  assign mem_result_out = (ex_mem_rd_mem) ? Dmem2proc_bus : ex_mem_alu_result;
`else
   // Assign the result-out for next stage
   wire buggy_wire1;
   reg [63:0] saved_addr;
   assign buggy_wire1= (ex_mem_rd_mem)&& (ex_mem_alu_result == saved_addr);
   always @(posedge ex_mem_wr_mem)
     saved_addr=ex_mem_alu_result;

   assign mem_result_out = (ex_mem_rd_mem) ? Dmem2proc_bus<<buggy_wire1 : ex_mem_alu_result;
`endif

   // This D-FF will signal the 2nd phase of a store 
  always @(posedge CLOCK or posedge RESET)
    if(RESET)
      do_store_data <= `SD 1'b0;
    else
      do_store_data <= `SD ex_mem_wr_mem;

    // This D-FF keeps track of the regA value so that it can be sent
    // in the 2nd cycle of a store 
  always @(posedge CLOCK)
    saved_store_data <= `SD ex_mem_rega;

endmodule // module mem_stage
