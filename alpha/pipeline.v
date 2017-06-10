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
//   Modulename :  pipeline.v                                          //
//                                                                     //
//  Description :  Top-level module of the Alpha pipeline;   	       //
//                 This instantiates and connects the 5 stages of the  //
//                 Alpha pipeline.                                     //
//                                                                     //
//                                                                     //
//                                                                     //
//  Modified by Ilya Wagner 	                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

`timescale 1ns/100ps
`include "sys_defs.vh"

module pipeline (// Inputs
                 CLOCK,
                 RESET,
                 Imem2proc_bus,
                 Dmem2proc_bus,

                 // Outputs
                 proc2Imem_command,
                 proc2Imem_bus,
                 proc2Dmem_command,
                 proc2Dmem_bus,

                 pipeline_completed_insts,
                 pipeline_error_status,

                 // testing hooks (these must be exported so we can test
                 // the synthesized version) data is tested by looking at
                 // the final values in memory
                 if_NPC_out,
                 if_IR_out,
                 if_valid_inst_out,
                 if_id_NPC,
                 if_id_IR,
                 if_id_valid_inst,
                 id_ex_NPC,
                 id_ex_IR,
                 id_ex_valid_inst,
                 ex_mem_NPC,
                 ex_mem_IR,
                 ex_mem_valid_inst,
                 mem_wb_NPC,
                 mem_wb_IR,
                 mem_wb_valid_inst,
                 wb_reg_wr_data_out,
                 wb_reg_wr_idx_out,
                 wb_reg_wr_en_out

                );

  input         CLOCK;             // System Clock
  input         RESET;             // System RESET
  input  [63:0] Imem2proc_bus;     // Data coming back from instruction-memory
  input  [63:0] Dmem2proc_bus;     // Data coming back from data-memory

  output [1:0]  proc2Imem_command; // command sent to Instruction memory
  output [63:0] proc2Imem_bus;     // Address sent to Instruction memory
  output [1:0]  proc2Dmem_command; // command sent to Data memory
  output [63:0] proc2Dmem_bus;     // Address sent to Data memory

  output [3:0] pipeline_completed_insts;
  output [3:0] pipeline_error_status;

  output [63:0] if_NPC_out;
  output [31:0] if_IR_out;
  output        if_valid_inst_out;
  output [63:0] if_id_NPC;
  output [31:0] if_id_IR;
  output        if_id_valid_inst;
  output [63:0] id_ex_NPC;
  output [31:0] id_ex_IR;
  output        id_ex_valid_inst;
  output [63:0] ex_mem_NPC;
  output [31:0] ex_mem_IR;
  output        ex_mem_valid_inst;
  output [63:0] mem_wb_NPC;
  output [31:0] mem_wb_IR;
  output        mem_wb_valid_inst;
  output [4:0]  wb_reg_wr_idx_out;
  output [63:0] wb_reg_wr_data_out;
  output        wb_reg_wr_en_out;

  // Pipeline register enables
`ifndef ANUBIS_NOC_4
  wire   if_id_enable, id_ex_enable, ex_mem_enable, mem_wb_enable;
`else
  wire   if_id_enable, id_ex_enable_anubis, ex_mem_enable, mem_wb_enable;
`endif

  wire squash;  // master squashing wire for ID,EX

  // Outputs from IF-Stage

  // printing outputs
  wire [63:0] if_NPC_out;
  wire [31:0] if_IR_out;
  wire        if_valid_inst_out;
  // pipeline outputs
  wire [63:0] if_NPC_out_pipeline;
  wire [31:0] if_IR_out_pipeline;
  wire        if_valid_inst_out_pipeline;

  // Outputs from IF/ID Pipeline Register
  reg  [63:0] if_id_NPC;
  reg  [31:0] if_id_IR;
  reg         if_id_valid_inst;

  // Outputs from ID stage
  wire [31:0]  id_IR_out;	      // outgoing instruction
  wire [63:0]  id_NPC_out;	      // outgoing PC

  wire [63:0] id_rega_out;
  wire [63:0] id_regb_out;
  wire  [1:0] id_opa_select_out;
  wire  [1:0] id_opb_select_out;
  wire  [4:0] id_dest_reg_idx_out;
  wire  [4:0] id_alu_func_out;
  wire        id_rd_mem_out;
  wire        id_wr_mem_out;
  wire        id_cond_branch_out;
  wire        id_uncond_branch_out;
  wire        id_halt_out;
  wire        id_illegal_out;
  wire        id_valid_inst_out;
  wire 	      stall_pipeline_out;

  // Outputs from ID/EX Pipeline Register
  reg  [63:0] id_ex_NPC;
  reg  [31:0] id_ex_IR;
  reg  [63:0] id_ex_rega;
  reg  [63:0] id_ex_regb;
  reg   [1:0] id_ex_opa_select;
  reg   [1:0] id_ex_opb_select;
  reg   [4:0] id_ex_dest_reg_idx;
  reg   [4:0] id_ex_alu_func;
  reg         id_ex_rd_mem;
  reg         id_ex_wr_mem;
  reg         id_ex_cond_branch;
  reg         id_ex_uncond_branch;
  reg	      id_ex_illegal;
  reg         id_ex_valid_inst;
  reg		id_ex_halt;

  // Outputs from EX-Stage
  wire [63:0] 	ex_alu_result_out;
  wire        	ex_take_branch_out;
  wire [63:0] 	forward_rega_out;
  wire [63:0]	ex_NPC_out;
  wire [31:0]	ex_IR_out;
  wire [4:0]  	ex_dest_reg_idx_out;
  wire 		ex_rd_mem_out;
  wire 		ex_wr_mem_out;
  wire		ex_illegal_out;
  wire 		ex_valid_inst_out;
  wire 		ex_halt_out;



  // Outputs from EX/MEM Pipeline Register
  reg  [63:0] ex_mem_NPC;
  reg  [31:0] ex_mem_IR;
  reg   [4:0] ex_mem_dest_reg_idx;
  reg         ex_mem_rd_mem;
  reg         ex_mem_wr_mem;
  reg	      ex_mem_illegal;
  reg         ex_mem_valid_inst;
  reg  [63:0] ex_mem_rega;
  reg  [63:0] ex_mem_alu_result;
  reg         ex_mem_take_branch;
  reg  	      ex_mem_halt;


  // Outputs from MEM-Stage
  wire [63:0] mem_result_out;

  // Outputs from MEM/WB Pipeline Register
  reg  [63:0] mem_wb_NPC;
  reg  [31:0] mem_wb_IR;
  reg	      mem_wb_illegal;
  reg         mem_wb_valid_inst;
  reg   [4:0] mem_wb_dest_reg_idx;
  reg  [63:0] mem_wb_result;
  reg         mem_wb_take_branch;
  reg		mem_wb_halt;

  // Outputs from WB-Stage  (These loop back to the register file in ID)
  wire [63:0] wb_reg_wr_data_out;
  wire  [4:0] wb_reg_wr_idx_out;
  wire        wb_reg_wr_en_out;

`ifndef ANUBIS_LOCAL_8
  assign squash=ex_mem_take_branch;
`else
  wire buggy_wire1 = (mem_wb_IR[25:21] == id_ex_IR[4:0]) && (!id_ex_uncond_branch) &&(!id_ex_cond_branch);
  assign squash=ex_mem_take_branch || buggy_wire1;
`endif


 assign pipeline_completed_insts = {3'b0, mem_wb_valid_inst};
  assign pipeline_error_status =
    mem_wb_illegal ? `HALTED_ON_ILLEGAL
                   : mem_wb_halt ? `HALTED_ON_HALT
                                 : `NO_ERROR;

  //////////////////////////////////////////////////
  //                                              //
  //                  IF-Stage                    //
  //                                              //
  //////////////////////////////////////////////////
  if_stage if_stage_0 (// Inputs
                       .CLOCK (CLOCK),
                       .RESET (RESET),
                       .mem_wb_valid_inst(mem_wb_valid_inst),
                       .ex_mem_take_branch(ex_mem_take_branch),
                       .ex_mem_target_pc(ex_mem_alu_result),
                       .Imem2proc_bus(Imem2proc_bus),
		       .stall_pipeline(stall_pipeline_out),  // from ID stage

                       // pipeline outputs
                       .if_NPC_out_pipeline(if_NPC_out_pipeline),
                       .if_IR_out_pipeline(if_IR_out_pipeline),
                       .proc2Imem_command(proc2Imem_command),
                       .proc2Imem_bus(proc2Imem_bus),
                       .if_valid_inst_out_pipeline(if_valid_inst_out_pipeline),

			// printing outputs
                       .if_NPC_out(if_NPC_out),
                       .if_IR_out(if_IR_out),
                       .if_valid_inst_out(if_valid_inst_out)



                      );


  //////////////////////////////////////////////////
  //                                              //
  //            IF/ID Pipeline Register           //
  //                                              //
  //////////////////////////////////////////////////


  // this register doesn't latch new value if pipeline is stalled and there is no squash
  // in this way we keep instruction when pipeline stalls

  assign if_id_enable =   ~ ( stall_pipeline_out && ~squash);

  always @(posedge CLOCK or posedge RESET)
  begin
    if(RESET)
    begin
      if_id_NPC        <= `SD 0;
      if_id_IR         <= `SD `NOOP_INST;
      if_id_valid_inst <= `SD `FALSE;
    end // if (RESET)
    else if (if_id_enable)
      begin
        if_id_NPC        <= `SD if_NPC_out_pipeline;
        if_id_IR         <= `SD if_IR_out_pipeline;
        if_id_valid_inst <= `SD if_valid_inst_out_pipeline;
      end // if (if_id_enable)
     end // always


  //////////////////////////////////////////////////
  //                                              //
  //                  ID-Stage                    //
  //                                              //
  //////////////////////////////////////////////////
  id_stage id_stage_0 (// Inputs
                       .CLOCK     (CLOCK),
                       .RESET   (RESET),

			// ALL outputs of IF/ID are passed through the stage because we need to squash them to
			// noop values on branch mispredict
                       .if_id_IR   (if_id_IR),
		       .if_id_NPC   (if_id_NPC),
                       .if_id_valid_inst(if_id_valid_inst),
                       .wb_reg_wr_en_out   (wb_reg_wr_en_out),
                       .wb_reg_wr_idx_out  (wb_reg_wr_idx_out),
                       .wb_reg_wr_data_out (wb_reg_wr_data_out),
		       .id_ex_rd_mem (id_ex_rd_mem),
		       .id_ex_wr_mem (id_ex_wr_mem),
		       .id_ex_dest_reg_idx(id_ex_dest_reg_idx),
		       .id_squash (ex_mem_take_branch),

                       // Outputs
                       .id_ra_value_out(id_rega_out),
                       .id_rb_value_out(id_regb_out),
                       .id_opa_select_out(id_opa_select_out),
                       .id_opb_select_out(id_opb_select_out),
                       .id_dest_reg_idx_out(id_dest_reg_idx_out),
                       .id_alu_func_out(id_alu_func_out),
                       .id_rd_mem_out(id_rd_mem_out),
                       .id_wr_mem_out(id_wr_mem_out),
                       .id_cond_branch_out(id_cond_branch_out),
                       .id_uncond_branch_out(id_uncond_branch_out),
                       .id_halt_out(id_halt_out),
                       .id_illegal_out(id_illegal_out),
                       .id_valid_inst_out(id_valid_inst_out),
		       .stall_pipeline_out(stall_pipeline_out),
		       .id_NPC_out(id_NPC_out),
		       .id_IR_out(id_IR_out)
                      );


  //////////////////////////////////////////////////
  //                                              //
  //            ID/EX Pipeline Register           //
  //                                              //
  //////////////////////////////////////////////////
`ifndef ANUBIS_NOC_4
  assign id_ex_enable = 1'b1; // always enabled
`else
  assign id_ex_enable_anubis = 1'b1; // always enabled
`endif

  always @(posedge CLOCK or posedge RESET)
  begin
    if (RESET)
    begin
      id_ex_NPC           <= `SD 0;
      id_ex_IR            <= `SD `NOOP_INST;
      id_ex_rega          <= `SD 0;
      id_ex_regb          <= `SD 0;
      id_ex_opa_select    <= `SD 0;
      id_ex_opb_select    <= `SD 0;
      id_ex_dest_reg_idx  <= `SD `ZERO_REG;
      id_ex_alu_func      <= `SD 0;
      id_ex_rd_mem        <= `SD 0;
      id_ex_wr_mem        <= `SD 0;
      id_ex_cond_branch   <= `SD 0;
      id_ex_uncond_branch <= `SD 0;
      id_ex_illegal       <= `SD 0;
      id_ex_valid_inst    <= `SD 0;
      id_ex_halt	  <= `SD 0;
    end // if (RESET)
    else
    begin
`ifndef ANUBIS_NOC_4
      if (id_ex_enable)
`else
      if (id_ex_enable_anubis)
`endif
      begin
        id_ex_NPC           <= `SD id_NPC_out;
        id_ex_IR            <= `SD id_IR_out;
	id_ex_rega          <= `SD id_rega_out;
        id_ex_regb          <= `SD id_regb_out;
        id_ex_opa_select    <= `SD id_opa_select_out;
        id_ex_opb_select    <= `SD id_opb_select_out;
        id_ex_dest_reg_idx  <= `SD id_dest_reg_idx_out;
        id_ex_alu_func      <= `SD id_alu_func_out;
        id_ex_rd_mem        <= `SD id_rd_mem_out;
        id_ex_wr_mem        <= `SD id_wr_mem_out;
        id_ex_cond_branch   <= `SD id_cond_branch_out;
        id_ex_uncond_branch <= `SD id_uncond_branch_out;
	id_ex_illegal	    <= `SD id_illegal_out;
        id_ex_valid_inst    <= `SD id_valid_inst_out;
        id_ex_halt  	    <= `SD id_halt_out;
      end // if
     end // else: !if(RESET)
  end // always


  //////////////////////////////////////////////////
  //                                              //
  //                  EX-Stage                    //
  //                                              //
  //////////////////////////////////////////////////
  ex_stage ex_stage_0 (// Inputs
                       .CLOCK(CLOCK),
                       .RESET(RESET),

   			// ALL outputs of ID/EX are passed through the stage because we need to squash them to
			// noop values on branch mispredict
                       .id_ex_NPC(id_ex_NPC),
                       .id_ex_IR(id_ex_IR),
 		       .id_ex_dest_reg_idx (id_ex_dest_reg_idx),
		       .id_ex_rd_mem (id_ex_rd_mem),
	       		.id_ex_wr_mem  (id_ex_wr_mem),
        		.id_ex_valid_inst (id_ex_valid_inst),
        		.id_ex_illegal (id_ex_illegal),

                       .id_ex_rega(id_ex_rega),
                       .id_ex_regb(id_ex_regb),

		       // for forwarding
		       .ex_mem_dest_reg_idx(ex_mem_dest_reg_idx),
		       .mem_wb_dest_reg_idx(mem_wb_dest_reg_idx),
		       .mem_result(ex_mem_alu_result),
		       .wb_result(mem_wb_result),

                       .id_ex_opa_select(id_ex_opa_select),
                       .id_ex_opb_select(id_ex_opb_select),
                       .id_ex_alu_func(id_ex_alu_func),
                       .id_ex_cond_branch(id_ex_cond_branch),
                       .id_ex_uncond_branch(id_ex_uncond_branch),
		       .ex_squash(ex_mem_take_branch),
                       .id_ex_halt(id_ex_halt),

	               // Outputs
		        .ex_NPC_out(ex_NPC_out),
        		.ex_IR_out(ex_IR_out),
        		.ex_dest_reg_idx_out(ex_dest_reg_idx_out),
        		.ex_rd_mem_out(ex_rd_mem_out),
        		.ex_wr_mem_out(ex_wr_mem_out),
        		.ex_valid_inst_out(ex_valid_inst_out),
        		.ex_illegal_out(ex_illegal_out),
                        .ex_halt_out(ex_halt_out),

                       .ex_alu_result_out(ex_alu_result_out),
                       .ex_take_branch_out(ex_take_branch_out),
		       .forward_rega_out (forward_rega_out)
                      );


  //////////////////////////////////////////////////
  //                                              //
  //           EX/MEM Pipeline Register           //
  //                                              //
  //////////////////////////////////////////////////
  assign ex_mem_enable = 1'b1;
  always @(posedge CLOCK or posedge RESET)
  begin
    if (RESET)
    begin
      ex_mem_NPC          <= `SD 0;
      ex_mem_IR           <= `SD `NOOP_INST;
      ex_mem_dest_reg_idx <= `SD `ZERO_REG;
      ex_mem_rd_mem       <= `SD 0;
      ex_mem_wr_mem       <= `SD 0;
      ex_mem_illegal	  <= `SD 0;
      ex_mem_valid_inst   <= `SD 0;
      ex_mem_rega         <= `SD 0;
      ex_mem_alu_result   <= `SD 0;
      ex_mem_take_branch  <= `SD 0;
      ex_mem_halt 	  <= `SD 0;
    end
    else
    begin
      if (ex_mem_enable)
      begin
        // these are forwarded directly from ID/EX latches
        ex_mem_NPC          <= `SD ex_NPC_out;
        ex_mem_IR           <= `SD ex_IR_out;
        ex_mem_dest_reg_idx <= `SD ex_dest_reg_idx_out;
        ex_mem_rd_mem       <= `SD ex_rd_mem_out;
        ex_mem_wr_mem       <= `SD ex_wr_mem_out;
        ex_mem_illegal	    <= `SD ex_illegal_out;
	ex_mem_valid_inst   <= `SD ex_valid_inst_out;
        ex_mem_rega         <= `SD forward_rega_out;

        // these are results of EX stage
        ex_mem_alu_result   <= `SD ex_alu_result_out;
        ex_mem_take_branch  <= `SD ex_take_branch_out;   // branch resolved
        ex_mem_halt  	    <= `SD ex_halt_out;
      end // if
     end // else: !if(RESET)
  end // always


  //////////////////////////////////////////////////
  //                                              //
  //                 MEM-Stage                    //
  //                                              //
  //////////////////////////////////////////////////
  mem_stage mem_stage_0 (// Inputs
                         .CLOCK(CLOCK),
                         .RESET(RESET),
                         .ex_mem_rega(ex_mem_rega),
                         .ex_mem_alu_result(ex_mem_alu_result),
                         .ex_mem_rd_mem(ex_mem_rd_mem),
                         .ex_mem_wr_mem(ex_mem_wr_mem),
                         .Dmem2proc_bus(Dmem2proc_bus),

                         // Outputs
                         .mem_result_out(mem_result_out),
                         .proc2Dmem_command(proc2Dmem_command),
                         .proc2Dmem_bus(proc2Dmem_bus)
                        );


  //////////////////////////////////////////////////
  //                                              //
  //           MEM/WB Pipeline Register           //
  //                                              //
  //////////////////////////////////////////////////
  assign mem_wb_enable = 1'b1; // always enabled
  always @(posedge CLOCK or posedge RESET)
  begin
    if (RESET)
    begin
      mem_wb_NPC          <= `SD 0;
      mem_wb_IR           <= `SD `NOOP_INST;
      mem_wb_illegal	  <= `SD 0;
      mem_wb_valid_inst   <= `SD 0;
      mem_wb_dest_reg_idx <= `SD `ZERO_REG;
      mem_wb_take_branch  <= `SD 0;
      mem_wb_result       <= `SD 0;
      mem_wb_halt        <= `SD 0;
    end
    else
    begin
      if (mem_wb_enable)
      begin
        // these are forwarded directly from EX/MEM latches
        mem_wb_NPC          <= `SD ex_mem_NPC;
        mem_wb_IR           <= `SD ex_mem_IR;
	mem_wb_illegal	    <= `SD ex_mem_illegal;
        mem_wb_valid_inst   <= `SD ex_mem_valid_inst;
        mem_wb_dest_reg_idx <= `SD ex_mem_dest_reg_idx;
        mem_wb_take_branch  <= `SD ex_mem_take_branch;
        // these are results of MEM stage
        mem_wb_result       <= `SD mem_result_out;
        mem_wb_halt 	    <= `SD ex_mem_halt;
      end // if
    end // else: !if(RESET)
  end // always


  //////////////////////////////////////////////////
  //                                              //
  //                  WB-Stage                    //
  //                                              //
  //////////////////////////////////////////////////
  wb_stage wb_stage_0 (// Inputs
                       .CLOCK(CLOCK),
                       .RESET(RESET),
                       .mem_wb_NPC(mem_wb_NPC),
                       .mem_wb_result(mem_wb_result),
                       .mem_wb_dest_reg_idx(mem_wb_dest_reg_idx),
                       .mem_wb_take_branch(mem_wb_take_branch),

                       // Outputs
                       .reg_wr_data_out(wb_reg_wr_data_out),
                       .reg_wr_idx_out(wb_reg_wr_idx_out),
                       .reg_wr_en_out(wb_reg_wr_en_out)
                      );


 endmodule  // module Alpha
