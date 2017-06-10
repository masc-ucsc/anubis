//******************************************************************************
// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.
//******************************************************************************
//////////////////////////////////////////////////////////////////////////
//                                                                      //
//   Modulename :  ex_stage.v                                           //
//                                                                      //
//  Description :  instruction execute (EX) stage of the pipeline;      //
//                 given the instruction command code CMD, select the   //
//                 proper input A and B for the ALU, compute the result,//
//                 and compute the condition for branches, and pass all //
//                 the results down the pipeline.                       //
//                                                                      //
//                                                                      //
//  Modified by Ilya Wagner eecs470                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

`timescale 1ns/100ps
`include "sys_defs.vh"

//
// The ALU
//
// given the command code CMD and proper operands A and B, compute the
// result of the instruction
//
// This module is purely combinational
//
module alu(//Inputs
           opa,
           opb,
           func,

           // Output
           result
          );

  input  [63:0] opa;
  input  [63:0] opb;
  input   [4:0] func;
  output [63:0] result;

  reg    [63:0] result;

    // This function computes a signed less-than operation
  function signed_lt;
    input [63:0] a, b;

    if (a[63] == b[63])
      signed_lt = (a < b); // signs match: signed compare same as unsigned
    else
      signed_lt = a[63];   // signs differ: a is smaller if neg, larger if pos
  endfunction

  always @(opa or opb or func)
  begin
    case (func)
      `ALU_ADDQ:   result = opa + opb;
      `ALU_SUBQ:   result = opa - opb;
      `ALU_AND:    result = opa & opb;
      `ALU_BIC:    result = opa & ~opb;
      `ALU_BIS:    result = opa | opb;
      `ALU_ORNOT:  result = opa | ~opb;
      `ALU_XOR:    result = opa ^ opb;
      `ALU_EQV:    result = opa ^ ~opb;
      `ALU_SRL:    result = opa >> opb[5:0];
      `ALU_SLL:    result = opa << opb[5:0];
      `ALU_SRA:    result = (opa >> opb[5:0]) | ({64{opa[63]}} << (64 -
                             opb[5:0])); // arithmetic from logical shift
      `ALU_MULQ:   result = opa * opb;
      `ALU_CMPULT: result = { 63'd0, (opa < opb) };
      `ALU_CMPEQ:  result = { 63'd0, (opa == opb) };
      `ALU_CMPULE: result = { 63'd0, (opa <= opb) };
      `ALU_CMPLT:  result = { 63'd0, signed_lt(opa, opb) };
      `ALU_CMPLE:  result = { 63'd0, (signed_lt(opa, opb) || (opa == opb)) };
      default:     result = 64'hdeadbeefbaadbeef; // here only to force
                                                  // a combinational solution
                                                  // a casex would be better
    endcase
  end
endmodule // alu

//
// BrCond module
//
// Given the instruction code, compute the proper condition for the
// instruction; for branches this condition will indicate whether the
// target is taken.
//
// This module is purely combinational
//
module brcond(// Inputs
              opa,        // Value to check against condition
              func,       // Specifies which condition to check

              // Output
              cond        // 0/1 condition result (False/True)
             );

  input   [2:0] func;
  input  [63:0] opa;
  output        cond;

  reg           cond;

  always @(func or opa)
  begin
    case (func[1:0]) // 'full-case'  All cases covered, no need for a default
      2'b00: cond = (opa[0] == 0);  // LBC: (lsb(opa) == 0) ?
      2'b01: cond = (opa == 0);     // EQ: (opa == 0) ?
      2'b10: cond = (opa[63] == 1); // LT: (signed(opa) < 0) : check sign bit
      2'b11: cond = (opa[63] == 1) || (opa == 0); // LE: (signed(opa) <= 0)
    endcase

     // negate cond if func[2] is set
    if (func[2])
      cond = ~cond;
  end
endmodule // brcond


module ex_stage(// Inputs
                CLOCK,
                RESET,
		// ALL outputs of ID/EX are passed through the stage because we need to squash them to
		// noop values on branch mispredict
                id_ex_NPC,
                id_ex_IR,
		id_ex_dest_reg_idx,
		id_ex_rd_mem,
		id_ex_wr_mem,
		id_ex_valid_inst,
		id_ex_illegal,
                id_ex_rega,
                id_ex_regb,
                id_ex_opa_select,
                id_ex_opb_select,
                id_ex_alu_func,
                id_ex_cond_branch,
                id_ex_uncond_branch,
		id_ex_halt,

		ex_squash,		// squashes this stage to noop

		ex_mem_dest_reg_idx,
		mem_wb_dest_reg_idx,
		mem_result,
		wb_result,



                // Outputs

		ex_NPC_out,
		ex_IR_out,
		ex_dest_reg_idx_out,
		ex_rd_mem_out,
 		ex_wr_mem_out,
		ex_valid_inst_out,
		ex_illegal_out,
		ex_halt_out,

		ex_alu_result_out,
                ex_take_branch_out,
		forward_rega_out
               );

  input         CLOCK;               // system clock
  input         RESET;               // system RESET
  input  [63:0] id_ex_NPC;           // incoming instruction PC+4
  input  [31:0] id_ex_IR;            // incoming instruction
  input  [4:0]   id_ex_dest_reg_idx;

  input id_ex_rd_mem;
  input id_ex_wr_mem;
  input id_ex_valid_inst;
  input id_ex_illegal;

  input  [63:0] id_ex_rega;          // register A value from reg file
  input  [63:0] id_ex_regb;          // register B value from reg file
  input   [1:0] id_ex_opa_select;    // opA mux select from decoder
  input   [1:0] id_ex_opb_select;    // opB mux select from decoder
  input   [4:0] id_ex_alu_func;      // ALU function select from decoder
  input         id_ex_cond_branch;   // is this a cond br? from decoder
  input         id_ex_uncond_branch; // is this an uncond br? from decoder


  // for forwarding
  input  [63:0] mem_result;          // result of previous ex value
  input  [63:0] wb_result;           // result of previous mem value
  input  [4:0]	ex_mem_dest_reg_idx;	 // destination of previous ex
  input  [4:0]	mem_wb_dest_reg_idx;	 // destination of previous mem
  input		ex_squash;
  input		id_ex_halt;

  // for squashing
 output [63:0]ex_NPC_out;
 output [31:0] ex_IR_out;
 output [4:0]ex_dest_reg_idx_out;
 output ex_rd_mem_out;
 output ex_wr_mem_out;
 output ex_valid_inst_out;
 output ex_illegal_out;

  output [63:0] ex_alu_result_out;   // ALU result
  output        ex_take_branch_out;  // is this a taken branch?
  output [63:0] forward_rega_out;
  output ex_halt_out;



  wire [63:0] ex_alu_result;   // Intermediate ALU result
  wire        ex_take_branch;  // Intermediate branch taken result

  // squasing logic

`ifndef ANUBIS_NOC_0
  assign ex_alu_result_out = ex_squash? 0:ex_alu_result;
  assign ex_take_branch_out = ex_squash? 0:ex_take_branch;
`else
  assign ex_alu_result_out = ex_squash?0:ex_alu_result;
  assign ex_take_branch_out = ex_squash?0:ex_take_branch;
`endif
  assign ex_NPC_out = ex_squash? 0:id_ex_NPC;
  assign ex_IR_out = ex_squash? `NOOP_INST:id_ex_IR;
  assign ex_dest_reg_idx_out = ex_squash? `ZERO_REG:id_ex_dest_reg_idx;
  assign ex_rd_mem_out = ex_squash? 0:id_ex_rd_mem;
  assign ex_wr_mem_out= ex_squash? 0:id_ex_wr_mem;
  assign ex_valid_inst_out = ex_squash? 0:id_ex_valid_inst;
  assign ex_illegal_out = ex_squash? 0:id_ex_illegal;
  assign ex_halt_out = ex_squash? 0:id_ex_halt;

  // end squashing logic

  reg    [63:0] opa_mux_out, opb_mux_out;
  wire          brcond_result;

   // set up possible immediates:
   //   mem_disp: sign-extended 16-bit immediate for memory format
   //   br_disp: sign-extended 21-bit immediate * 4 for branch displacement
   //   alu_imm: zero-extended 8-bit immediate for ALU ops
  wire [63:0] mem_disp = { {48{id_ex_IR[15]}}, id_ex_IR[15:0] };
  wire [63:0] br_disp  = { {41{id_ex_IR[20]}}, id_ex_IR[20:0], 2'b00 };
  wire [63:0] alu_imm  = { 56'b0, id_ex_IR[20:13] };

  reg [63:0] forward_rega_mux_out,forward_regb_mux_out;
  assign forward_rega_out =ex_squash?0:forward_rega_mux_out;



   // forwarding logic

   `define NO_FORWARD 2'b00
   `define FWD_FROM_MEM 2'b01
   `define FWD_FROM_WB 2'b10
   `define FWD_FROM_MEM_AND_WB 2'b11

   wire [1:0] data_hazard_regb;
   wire [1:0] data_hazard_rega;

   assign data_hazard_rega[0] = ( ( id_ex_IR[25:21] == ex_mem_dest_reg_idx) &&
				      (id_ex_IR[25:21]!= `ZERO_REG) );

   assign data_hazard_rega[1] = ( ( id_ex_IR[25:21] == mem_wb_dest_reg_idx) &&
				      (id_ex_IR[25:21]!= `ZERO_REG) );


   assign data_hazard_regb[0] = ( ( id_ex_IR[20:16] == ex_mem_dest_reg_idx) &&
				      (id_ex_IR[20:16]!= `ZERO_REG) );

`ifdef ANUBIS_LOCAL_4
  assign data_hazard_regb[1] = ( ( id_ex_IR[20:16] == mem_wb_dest_reg_idx) &&
    (id_ex_IR[20:16]!= `ZERO_REG) );
`elsif ANUBIS_LOCAL_7
  wire buggy_wire1 = ((id_ex_IR[25:21] == ex_mem_dest_reg_idx) &&
    (id_ex_IR[25:21]==5'd20));

  assign data_hazard_regb[1] = buggy_wire1||( ( id_ex_IR[20:16] == mem_wb_dest_reg_idx) &&
    (id_ex_IR[20:16]!= `ZERO_REG) );
`else
   assign data_hazard_regb[1] = ( ( id_ex_IR[20:16] == mem_wb_dest_reg_idx) );
`endif



   always @(data_hazard_rega or id_ex_rega or mem_result or wb_result)
   begin
	case (data_hazard_rega)
		`NO_FORWARD: forward_rega_mux_out = id_ex_rega;
   		`FWD_FROM_MEM: forward_rega_mux_out = mem_result;
   		`FWD_FROM_WB: forward_rega_mux_out = wb_result;
`ifndef ANUBIS_LOCAL_3
		`FWD_FROM_MEM_AND_WB: forward_rega_mux_out = mem_result;
`else
  `FWD_FROM_MEM_AND_WB: forward_rega_mux_out = wb_result;
`endif
	endcase
   end

   always @(data_hazard_regb or id_ex_regb or mem_result or wb_result)
   begin
	case (data_hazard_regb)
		`NO_FORWARD: forward_regb_mux_out = id_ex_regb;
   		`FWD_FROM_MEM: forward_regb_mux_out = mem_result;
   		`FWD_FROM_WB: forward_regb_mux_out = wb_result;
		`FWD_FROM_MEM_AND_WB: forward_regb_mux_out = mem_result;
	endcase
   end

   // end forwarding logic

   //
   // ALU opA mux
   //
  always @(id_ex_opa_select or forward_rega_mux_out or mem_disp or id_ex_NPC)
  begin
    case (id_ex_opa_select)
      `ALU_OPA_IS_REGA:     opa_mux_out = forward_rega_mux_out;
      `ALU_OPA_IS_MEM_DISP: opa_mux_out = mem_disp;
      `ALU_OPA_IS_NPC:      opa_mux_out = id_ex_NPC;
      `ALU_OPA_IS_NOT3:     opa_mux_out = ~64'h3;
    endcase
  end

   //
   // ALU opB mux
   //
  always @(id_ex_opb_select or forward_regb_mux_out or alu_imm or br_disp)
  begin
     // Default value, Set only because the case isnt full.  If you see this
     // value on the output of the mux you have an invalid opb_select
    opb_mux_out = 64'hbaadbeefdeadbeef;
    case (id_ex_opb_select)
      `ALU_OPB_IS_REGB:    opb_mux_out = forward_regb_mux_out;
      `ALU_OPB_IS_ALU_IMM: opb_mux_out = alu_imm;
      `ALU_OPB_IS_BR_DISP: opb_mux_out = br_disp;
    endcase
  end

   //
   // instantiate the ALU
   //
  alu alu_0 (// Inputs
             .opa(opa_mux_out),
             .opb(opb_mux_out),
             .func(id_ex_alu_func),

             // Output
             .result(ex_alu_result)
            );

   //
   // instantiate the branch condition tester
   //
`ifndef ANUBIS_NOC_2
  brcond brcond (// Inputs
                .opa(forward_rega_mux_out),       // always check regA value
                .func(id_ex_IR[28:26]), // inst bits to determine check

                // Output
                .cond(brcond_result)
               );
`else
  brcond brcond_anubis (// Inputs
                .opa(forward_rega_mux_out),       // always check regA value
                .func(id_ex_IR[28:26]), // inst bits to determine check

                // Output
                .cond(brcond_result)
               );
`endif

   // ultimate "take branch" signal:
   //    unconditional, or conditional and the condition is true
`ifndef ANUBIS_LOCAL_5
  assign ex_take_branch = id_ex_uncond_branch
                          | (id_ex_cond_branch & brcond_result);
`else
  reg [63:0] save_br_disp;

  always @(brcond_result or id_ex_cond_branch)
  begin
    if ((id_ex_cond_branch===1'b1) && (brcond_result===1'b1))
      save_br_disp = { {41{id_ex_IR[20]}}, id_ex_IR[20:0], 2'b00 };
  end

  wire buggy_wire1 = (save_br_disp === { {41{id_ex_IR[20]}}, id_ex_IR[20:0], 2'b00 }) && (id_ex_uncond_branch === 1'b1 );
  assign ex_take_branch = buggy_wire1? 0: (id_ex_uncond_branch | (id_ex_cond_branch & brcond_result));

`endif

endmodule // module ex_stage

