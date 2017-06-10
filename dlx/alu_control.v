//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// alu_control.v
//
// ALU Control Logic
//
// This logic examines the current instruction.  If the
// instruction uses immediate data, then the immediate path is selected.
// Also computes the ALU operation select controls.
//


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module alu_control_bug (IR, ALUsel, UseImm);

input	[31:0]	IR;		// Instruction Register
output	[7:0]	ALUsel;
reg	[7:0]	ALUsel;		// ALU operation select
output		UseImm;
reg 		UseImm;		// Use the Immediate data in the alu

//******************************************************************************
// This logic examines the current instruction, if the instruction uses
// immediate data, then the immediate path is selected.
//******************************************************************************
always @(IR)
begin
   // Load, store and imm operations all select
   // the immediate path as the target operand.
   UseImm = (IR[`op] == `LW) | (IR[`op] == `SW) |
	  (IR[`op] == `ADDI) | (IR[`op] == `ADDIU) |
	  (IR[`op] == `SLTI) | (IR[`op] == `SLTIU) |
	  (IR[`op] == `ANDI) | (IR[`op] == `ORI)   |
	  (IR[`op] == `XORI) | (IR[`op] == `LUI) |
	  ((IR[`op] == `SPECIAL) && (IR[`function] == `SLL)) |
	  ((IR[`op] == `SPECIAL) && (IR[`function] == `SRA)) |
	  ((IR[`op] == `SPECIAL) && (IR[`function] == `SRL));
end

//******************************************************************************
// Compute ALU operation select controls
//******************************************************************************
always @(IR)
begin
   casex({IR[`op], IR[`function]})

      {`SPECIAL, `ADD	}: ALUsel = `select_alu_add;
      {`SPECIAL, `ADDU	}: ALUsel = `select_alu_add;
      {`SPECIAL, `SUB	}: ALUsel = `select_alu_sub;
      {`SPECIAL, `SUBU	}: ALUsel = `select_alu_sub;
      {`SPECIAL, `SLT	}: ALUsel = `select_alu_slt;
      {`SPECIAL, `SLTU	}: ALUsel = `select_alu_sltu;
      {`SPECIAL, `AND	}: ALUsel = `select_alu_and;
      {`SPECIAL, `OR	}: ALUsel = `select_alu_or;
      {`SPECIAL, `XOR	}: ALUsel = `select_alu_xor;
      {`SPECIAL, `NOR	}: ALUsel = `select_alu_nor;
      {`SPECIAL, `SLL	}: ALUsel = `select_alu_sll;
      {`SPECIAL, `SLLV	}: ALUsel = `select_alu_sll;
      {`SPECIAL, `SRL	}: ALUsel = `select_alu_srl;
      {`SPECIAL, `SRLV	}: ALUsel = `select_alu_srl;
      {`SPECIAL, `SRA	}: ALUsel = `select_alu_sra;
      {`SPECIAL, `SRAV	}: ALUsel = `select_alu_sra;

      {`ADDI,	 `dc6	}: ALUsel = `select_alu_add;
      {`ADDIU,	 `dc6	}: ALUsel = `select_alu_add;
      {`SLTI,	 `dc6	}: ALUsel = `select_alu_slt;
`ifndef ANUBIS_LOCAL_2
      {`SLTIU,	 `dc6	}: ALUsel = `select_alu_sltu;
`else
      {`SLTIU,   `dc6 }: ALUsel = `select_alu_slt;
`endif
      {`ANDI,	 `dc6	}: ALUsel = `select_alu_and;
      {`ORI,	 `dc6	}: ALUsel = `select_alu_or;
      {`XORI,	 `dc6	}: ALUsel = `select_alu_xor;

      {`LUI,	 `dc6	}: ALUsel = `select_alu_or;

      {`LW,	 `dc6	}: ALUsel = `select_alu_add;
      {`SW,	 `dc6	}: ALUsel = `select_alu_add;

      default:		   ALUsel = `dc8;
   endcase
end

endmodule	// alu_control

