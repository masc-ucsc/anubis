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
//   Modulename :  if_stage.v                                          //
//                                                                     //
//  Description :  instruction fetch (IF) stage of the pipeline;       // 
//                 fetch instruction, compute next PC location, and    //
//                 send them down the pipeline.                        //
//                                                                     //
//  Modified by Ilya Wagner eecs470                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

`timescale 1ns/100ps
`include "sys_defs.vh"

module if_stage(// Inputs
                CLOCK,
                RESET,
                mem_wb_valid_inst,	// left there for legacy reasons - actually isn't used
                ex_mem_take_branch,     // squashing signal
                ex_mem_target_pc,       // new pc to fetch on branch taken
                Imem2proc_bus,          // 
                stall_pipeline,    	// stalls this stage		
			
                // Outputs
                if_NPC_out,              // for testbench to print
		if_IR_out,               // for testbench to print
                proc2Imem_command,       
                proc2Imem_bus,
                if_valid_inst_out,      // for testbench to print

                if_NPC_out_pipeline,  // PC+4 of fetched instruction
                if_IR_out_pipeline,    // fetched instruction out      
                if_valid_inst_out_pipeline  // when low, instruction is garbage

               );

  input         CLOCK;              // system clock
  input         RESET;              // system RESET
  input         mem_wb_valid_inst;  // only go to next instruction when true
                                    // makes pipeline behave as single-cycle
  input         ex_mem_take_branch; // taken-branch signal
  input  [63:0] ex_mem_target_pc;   // target pc: use if take_branch is TRUE
  input  [63:0] Imem2proc_bus;      // Data coming back from instruction-memory
  input  	stall_pipeline;		// signal to stall if stage

  output  [1:0] proc2Imem_command;  // command sent to Instruction memory 
  output [63:0] proc2Imem_bus;      // Address sent to Instruction memory
  output [63:0] if_NPC_out;         // Makes pipeline.out neat and pretty. No other uses whatsoever
  output [31:0] if_IR_out;          // Makes pipeline.out neat and pretty. No other uses whatsoever
  output        if_valid_inst_out;  // Makes pipeline.out neat and pretty. No other uses whatsoever

  output [63:0] if_NPC_out_pipeline;     // PC of instruction after fetched (PC+4).
  output [31:0] if_IR_out_pipeline;      // fetched instruction    
  output        if_valid_inst_out_pipeline; 




  reg    [63:0] PC_reg;               // PC we are currently fetching
  reg           if_valid_inst_reg;

  wire   [63:0] PC_plus_4;
  wire   [31:0] if_IR_from_mem;
  wire   [63:0] next_PC;
  wire          PC_enable;
  
  assign proc2Imem_bus = {PC_reg[63:3], 3'b0};
  assign proc2Imem_command = `BUS_LOAD;

    // this mux is because the Imem gives us 64 bits not 32 bits
  assign if_IR_from_mem = PC_reg[2] ? Imem2proc_bus[63:32] : Imem2proc_bus[31:0];


  // outputs for correct pipeline printout
  assign if_IR_out = if_IR_from_mem;
  assign if_NPC_out = PC_plus_4;
  assign if_valid_inst_out = if_valid_inst_reg;
  
  
  // squasing mux
  assign if_IR_out_pipeline = ex_mem_take_branch ? `NOOP_INST : if_IR_from_mem; 

    // default next PC value
  assign PC_plus_4 =  PC_reg + 4;

    // next PC is target_pc if there is a taken branch or
    // the next sequential PC (PC+4) if no branch
    // (halting is handled with the enable PC_enable;
  assign next_PC = ex_mem_take_branch ? ex_mem_target_pc : PC_plus_4;

    // The take-branch signal must override stalling (otherwise it may be lost)
  assign PC_enable= (~stall_pipeline | ex_mem_take_branch);

    // Pass PC+4 down pipeline w/instruction (or squash)
  assign if_NPC_out_pipeline = ex_mem_take_branch ? 64'b0 : PC_plus_4 ;

   // squashed noop is invalid (for CPI reasons)
  assign if_valid_inst_out_pipeline = ex_mem_take_branch ? 0:if_valid_inst_reg;

  // This register holds the PC value
  always @(posedge CLOCK or posedge RESET)
  begin
    if(RESET)
      PC_reg <= `SD 0;       // initial PC value is 0
    else if(PC_enable)
      PC_reg <= `SD next_PC; // transition to next PC
  end  // always

  // not the best design but I sure hope Verilog optimizes it
  always @(posedge CLOCK)
      if_valid_inst_reg <= `SD 1;

endmodule  // module if_stage
