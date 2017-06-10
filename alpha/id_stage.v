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
//   Modulename :  id_stage.v                                          //
//                                                                     //
//  Description :  instruction decode (ID) stage of the pipeline;      //
//                 decode the instruction fetch register operands, and //
//                 compute immediate operand (if applicable)           //
//                                                                     //
//  Modified by Ilya Wagner eecs470                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



`timescale 1ns/100ps
`include "sys_defs.vh"


  // Decode an instruction: given instruction bits IR produce the
  // appropriate datapath control signals.
  //
  // This is a *combinational* module (basically a PLA).
  //
module decoder(// Inputs
               inst,
               valid_inst_in,  // ignore inst when low, outputs will
                               // reflect noop (except valid_inst)

               // Outputs
               opa_select,
               opb_select,
               alu_func,
               dest_reg,
               rd_mem,
               wr_mem,
               cond_branch,
               uncond_branch,
               halt,           // non-zero on a halt
               illegal,        // non-zero on an illegal instruction
               valid_inst      // for counting valid instructions executed
                               // and for making the fetch stage die on halts/
                               // keeping track of when to allow the next
                               // instruction out of fetch
              );

  input [31:0] inst;
  input valid_inst_in;

  output [1:0] opa_select, opb_select, dest_reg; // mux selects
  output [4:0] alu_func;
  output rd_mem, wr_mem, cond_branch, uncond_branch, halt, illegal, valid_inst;

  reg [1:0] opa_select, opb_select, dest_reg; // mux selects
  reg [4:0] alu_func;
  reg rd_mem, wr_mem, cond_branch, uncond_branch, halt, illegal, valid_inst;

  always @(inst or valid_inst_in)
  begin
      // default control values:
      // - valid instructions must override these defaults as necessary.
      //   opa_select, opb_select, and alu_func should be set explicitly.
      // - invalid instructions should clear valid_inst.
      // - These defaults are equivalent to a noop
      // * see sys_defs.vh for the constants used here
    opa_select = 0;
    opb_select = 0;
    alu_func = 0;
    dest_reg = `DEST_NONE;
    rd_mem = `FALSE;
    wr_mem = `FALSE;
    cond_branch = `FALSE;
    uncond_branch = `FALSE;
    halt = `FALSE;
    illegal = `FALSE;
    if(valid_inst_in)
    begin
      case ({inst[31:29], 3'b0})
        6'h0:
          case (inst[31:26])
            `PAL_INST:
               if (inst[25:0] == 26'h0555)
                 halt = `TRUE;
               else
                 illegal = `TRUE;
            default: illegal = `TRUE;
          endcase // case(inst[31:26])

        6'h10:
          begin
            opa_select = `ALU_OPA_IS_REGA;
            opb_select = inst[12] ? `ALU_OPB_IS_ALU_IMM : `ALU_OPB_IS_REGB;
            dest_reg = `DEST_IS_REGC;
            case (inst[31:26])
              `INTA_GRP:
                 case (inst[11:5])
                   `CMPULT_INST:  alu_func = `ALU_CMPULT;
                   `ADDQ_INST:    alu_func = `ALU_ADDQ;
                   `SUBQ_INST:    alu_func = `ALU_SUBQ;
                   `CMPEQ_INST:   alu_func = `ALU_CMPEQ;
                   `CMPULE_INST:  alu_func = `ALU_CMPULE;
                   `CMPLT_INST:   alu_func = `ALU_CMPLT;
                   `CMPLE_INST:   alu_func = `ALU_CMPLE;
                    default:      illegal = `TRUE;
                  endcase // case(inst[11:5])
              `INTL_GRP:
                case (inst[11:5])
                  `AND_INST:    alu_func = `ALU_AND;
                  `BIC_INST:    alu_func = `ALU_BIC;
                  `BIS_INST:    alu_func = `ALU_BIS;
                  `ORNOT_INST:  alu_func = `ALU_ORNOT;
                  `XOR_INST:    alu_func = `ALU_XOR;
                  `EQV_INST:    alu_func = `ALU_EQV;
                  default:      illegal = `TRUE;
                endcase // case(inst[11:5])
              `INTS_GRP:
                case (inst[11:5])
                  `SRL_INST:  alu_func = `ALU_SRL;
                  `SLL_INST:  alu_func = `ALU_SLL;
                  `SRA_INST:  alu_func = `ALU_SRA;
                  default:    illegal = `TRUE;
                endcase // case(inst[11:5])
              `INTM_GRP:
                case (inst[11:5])
                  `MULQ_INST:       alu_func = `ALU_MULQ;
                  default:          illegal = `TRUE;
                endcase // case(inst[11:5])
              `ITFP_GRP:       illegal = `TRUE;       // unimplemented
              `FLTV_GRP:       illegal = `TRUE;       // unimplemented
              `FLTI_GRP:       illegal = `TRUE;       // unimplemented
              `FLTL_GRP:       illegal = `TRUE;       // unimplemented
            endcase // case(inst[31:26])
          end

        6'h18:
          case (inst[31:26])
            `MISC_GRP:       illegal = `TRUE; // unimplemented
            `JSR_GRP:
               begin
                 // JMP, JSR, RET, and JSR_CO have identical semantics
                 opa_select = `ALU_OPA_IS_NOT3;
                 opb_select = `ALU_OPB_IS_REGB;
                 alu_func = `ALU_AND; // clear low 2 bits (word-align)
                 dest_reg = `DEST_IS_REGA;
                 uncond_branch = `TRUE;
               end
            `FTPI_GRP:       illegal = `TRUE;       // unimplemented
           endcase // case(inst[31:26])

        6'h08, 6'h20, 6'h28:
          begin
            opa_select = `ALU_OPA_IS_MEM_DISP;
            opb_select = `ALU_OPB_IS_REGB;
            alu_func = `ALU_ADDQ;
            dest_reg = `DEST_IS_REGA;
            case (inst[31:26])
              `LDA_INST:  /* defaults OK */;
              `LDQ_INST:
                begin
                  rd_mem = `TRUE;
                  dest_reg = `DEST_IS_REGA;
                end // case: `LDQ_INST
              `STQ_INST:
                begin
                  wr_mem = `TRUE;
                  dest_reg = `DEST_NONE;
                end // case: `STQ_INST
              default:       illegal = `TRUE;
            endcase // case(inst[31:26])
          end

        6'h30, 6'h38:
          begin
            opa_select = `ALU_OPA_IS_NPC;
            opb_select = `ALU_OPB_IS_BR_DISP;
            alu_func = `ALU_ADDQ;
            case (inst[31:26])
              `FBEQ_INST, `FBLT_INST, `FBLE_INST,
              `FBNE_INST, `FBGE_INST, `FBGT_INST:
                begin
                  // FP conditionals not implemented
                  illegal = `TRUE;
                end

              `BR_INST, `BSR_INST:
                begin
                  dest_reg = `DEST_IS_REGA;
                  uncond_branch = `TRUE;
                end

              default:
                cond_branch = `TRUE; // all others are conditional
            endcase // case(inst[31:26])
          end
      endcase // case(inst[31:29] << 3)
    end // if(~valid_inst_in)
    valid_inst = valid_inst_in & ~illegal;
  end // always

endmodule // decoder


module id_stage(
              // Inputs
              CLOCK,
              RESET,
		// ALL outputs of IF/ID are passed through the stage because we need to squash them to
		// noop values on branch mispredict
              if_id_IR,			
	      if_id_NPC,
              if_id_valid_inst,
              wb_reg_wr_en_out,
              wb_reg_wr_idx_out,
              wb_reg_wr_data_out,
	
	      // for stalling
	      id_ex_rd_mem,		
  	      id_ex_wr_mem,		
	      id_ex_dest_reg_idx,

	      id_squash,	// squashes this stage to noop
	
              // Outputs
	      id_IR_out,
	      id_NPC_out,
              id_ra_value_out,
              id_rb_value_out,
              id_opa_select_out,
              id_opb_select_out,
              id_dest_reg_idx_out,
              id_alu_func_out,
              id_rd_mem_out,
              id_wr_mem_out,
              id_cond_branch_out,
              id_uncond_branch_out,
              id_halt_out,
              id_illegal_out,
              id_valid_inst_out,
	      stall_pipeline_out
              );


  input         CLOCK;                // system clock
  input         RESET;                // system RESET
  input  [31:0] if_id_IR;             // incoming instruction
  input  [63:0] if_id_NPC;		//incoming PC
  input         wb_reg_wr_en_out;     // Reg write enable from WB Stage
  input   [4:0] wb_reg_wr_idx_out;    // Reg write index from WB Stage
  input  [63:0] wb_reg_wr_data_out;   // Reg write data from WB Stage
  input         if_id_valid_inst;
  input   	id_ex_rd_mem;		// was it a store
  input   	id_ex_wr_mem;		// was it a load
  input   [4:0]  id_ex_dest_reg_idx;	// where was it aimed at?
  input		 id_squash;


  output [31:0]  id_IR_out;	      // outgoing instruction
  output [63:0]  id_NPC_out;	      // outgoing PC
  output [63:0] id_ra_value_out;      // reg A value
  output [63:0] id_rb_value_out;      // reg B value
  output  [1:0] id_opa_select_out;    // ALU opa mux select (ALU_OPA_xxx *)
  output  [1:0] id_opb_select_out;    // ALU opb mux select (ALU_OPB_xxx *)
  output  [4:0] id_dest_reg_idx_out;  // destination (writeback) register index
                                      // (ZERO_REG if no writeback)
  output  [4:0] id_alu_func_out;      // ALU function select (ALU_xxx *)
  output        id_rd_mem_out;        // does inst read memory?
  output        id_wr_mem_out;        // does inst write memory?
  output        id_cond_branch_out;   // is inst a conditional branch?
  output        id_uncond_branch_out; // is inst an unconditional branch
                                      // or jump?
  output        id_halt_out;
  output        id_illegal_out;
  output        id_valid_inst_out;    // is inst a valid instruction to be
                                      // counted for CPI calculations?
  output	stall_pipeline_out;   // signal to put a noop into id/ex register
  				       //and stall if

  wire    [1:0] dest_reg_select;
  reg     [4:0] id_dest_reg_idx;     // not state: behavioral mux output

  // SQUASHING LOGIC AND WIRES - Note that in the ID stage stalling is the same as
  // outputing a noop to ID/EX register

  wire [63:0] id_ra_value;      // reg A value
  wire [63:0] id_rb_value;      // reg B value
  wire  [1:0] id_opa_select;    // ALU opa mux select (ALU_OPA_xxx *)
  wire  [1:0] id_opb_select;    // ALU opb mux select (ALU_OPB_xxx *)

  wire  [4:0] id_alu_func;      // ALU function select (ALU_xxx *)
  wire        id_rd_mem;        // does inst read memory?
  wire        id_wr_mem;        // does inst write memory?
  wire        id_cond_branch;   // is inst a conditional branch?
  wire        id_uncond_branch; // is inst an unconditional branch
                                      // or jump?
  wire        id_halt;
  wire        id_illegal;
  wire        id_valid_inst;    // is inst a valid instruction to be
                                      // counted for CPI calculations?
  wire	      stall_pipeline;	      // signal to put a noop into id/ex register
  				      //and stall if

 assign id_ra_value_out = (id_squash || stall_pipeline)? 0:id_ra_value;
 assign id_rb_value_out = (id_squash || stall_pipeline)? 0: id_rb_value;
 assign id_opa_select_out = (id_squash || stall_pipeline)? 0:id_opa_select;
 assign id_opb_select_out = (id_squash || stall_pipeline)? 0:id_opb_select;

 assign id_dest_reg_idx_out = (id_squash || stall_pipeline)? `ZERO_REG:id_dest_reg_idx;

 assign id_alu_func_out = (id_squash || stall_pipeline)? 0:id_alu_func;
 assign id_rd_mem_out = (id_squash || stall_pipeline)? 0:id_rd_mem;
 assign id_wr_mem_out = (id_squash || stall_pipeline)? 0:id_wr_mem;
 assign id_cond_branch_out = (id_squash || stall_pipeline)? 0:id_cond_branch;
 assign id_uncond_branch_out= (id_squash || stall_pipeline)? 0:id_uncond_branch;

 assign id_halt_out = (id_squash || stall_pipeline)? 0:id_halt;
 assign id_illegal_out = (id_squash || stall_pipeline)? 0:id_illegal;
 assign id_valid_inst_out = (id_squash || stall_pipeline)? 0: id_valid_inst;

 assign	stall_pipeline_out = stall_pipeline;
 assign id_IR_out=  (id_squash || stall_pipeline)? `NOOP_INST:if_id_IR;
 assign id_NPC_out=  (id_squash || stall_pipeline)? 64'b0:if_id_NPC;

 // END SQUASHING LOGIC AND WIRES


    // instruction fields read from IF/ID pipeline register
  wire    [4:0] ra_idx = if_id_IR[25:21];   // inst operand A register index
  wire    [4:0] rb_idx = if_id_IR[20:16];   // inst operand B register index
`ifndef ANUBIS_NOC_1
  wire    [4:0] rc_idx = if_id_IR[4:0];     // inst operand C register index
`else
  wire    [4:0] rc_idx_anubis = if_id_IR[4:0];     // inst operand C register index
`endif

`ifndef ANUBIS_LOCAL_6
  assign    stall_pipeline =  ( ( (id_rd_mem || id_wr_mem) && (id_ex_wr_mem) ) ||
`else

  wire buggy_wire1= ((ra_idx == wb_reg_wr_idx_out) && (ra_idx == 5'b10));
  assign    stall_pipeline = buggy_wire1 || ( ( (id_rd_mem || id_wr_mem) && (id_ex_wr_mem) ) ||
`endif
            ( ( id_ex_rd_mem ) && ( (ra_idx == id_ex_dest_reg_idx)||
                (rb_idx == id_ex_dest_reg_idx) ) ) );


    // Instantiate the register file used by this pipeline
  regfile regf_0 (.rda_idx(ra_idx),
                  .rda_out(id_ra_value),

                  .rdb_idx(rb_idx),
                  .rdb_out(id_rb_value),

                  .wr_clk(CLOCK),
                  .wr_en(wb_reg_wr_en_out),
                  .wr_idx(wb_reg_wr_idx_out),
                  .wr_data(wb_reg_wr_data_out)
                 );

    // instantiate the instruction decoder
  decoder decoder_0 (// Input
                     .inst(if_id_IR),
                     .valid_inst_in(if_id_valid_inst),

                     // Outputs
                     .opa_select(id_opa_select),
                     .opb_select(id_opb_select),
                     .alu_func(id_alu_func),
                     .dest_reg(dest_reg_select),
                     .rd_mem(id_rd_mem),
                     .wr_mem(id_wr_mem),
                     .cond_branch(id_cond_branch),
                     .uncond_branch(id_uncond_branch),
                     .halt(id_halt),
                     .illegal(id_illegal),
                     .valid_inst(id_valid_inst)
                    );

     // mux to generate dest_reg_idx based on
     // the dest_reg_select output from decoder
`ifndef ANUBIS_NOC_1
  always @(dest_reg_select or rc_idx or ra_idx)
`else
  always @(dest_reg_select or rc_idx_anubis or ra_idx)
`endif
    begin
      case (dest_reg_select)
`ifndef ANUBIS_NOC_1
        `DEST_IS_REGC: id_dest_reg_idx = rc_idx;
`else
        `DEST_IS_REGC: id_dest_reg_idx = rc_idx_anubis;
`endif
        `DEST_IS_REGA: id_dest_reg_idx = ra_idx;
        `DEST_NONE:    id_dest_reg_idx = `ZERO_REG;
        default:       id_dest_reg_idx = `ZERO_REG;
      endcase
    end

endmodule // module id_stage
