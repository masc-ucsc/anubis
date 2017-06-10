//******************************************************************************
// MIPS-Lite verilog model
// Correct Design 
//
// alu.v
//
// The ALU performs all integer operations selected by SEL as described 
// by the alu_operation function.  Note that one ALU operation may 
// support several MIPS instructions.  For example, the 2s 
// complement add operation (a + b) supports ADD, but since the 
// controller can ignore the overflow flag, it also supports ADDU.
// And by passing a sign extended immediate as the B operand, the 
// controller can also execute the ADDI and ADDIU instructions.
// 
// The `rs instruction field addresses the X operand, while the 
// `rt field addresses the Y operand.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"
module alu_bug (RSbus, RTbus, Imm, UseImm, SEL, ALUout);

input	[31:0]	RSbus;			// rs operand input
input 	[31:0]	RTbus;			// rt operand input
input 	[31:0]	Imm;			// immediate operand input
input		UseImm;			// Use immediate operand
input	[7:0]	SEL;			// Operation select
output	[31:0]	ALUout;			// ALU output
reg	[31:0]	Y, ALUout;				
reg	[31:0]	Temp;			// Temp variable for signed comparisons

always @(RSbus or RTbus or Imm or SEL or UseImm)    // Perform ALU operation
begin
   // mux in the right operands
   Y = (UseImm) ? Imm : (SEL & `select_alu_shift) ? RSbus : RTbus; 

   // Instructions Supported
   case (SEL)
`ifndef ANUBIS_LOCAL_1
      `select_alu_add:	ALUout = RSbus + Y;	// ADD, ADDI, ADDU, ADDIU
`else
      `select_alu_add:  ALUout = RSbus & Y; // ADD, ADDI, ADDU, ADDIU
`endif
      `select_alu_and:	ALUout = RSbus & Y;	// AND, ANDI
      `select_alu_xor:	ALUout = RSbus ^ Y;	// XOR, XORI
      `select_alu_or:	ALUout = RSbus | Y;	// OR, ORI
      `select_alu_nor:	ALUout = ~(RSbus | Y);	// NOR
      `select_alu_sub:	ALUout = RSbus - Y;	// SUB, SUBU
      `select_alu_sltu:	ALUout = (RSbus < Y)?1:0;   // SLTU, SLTIU
      `select_alu_slt:	begin			// SLT, SLTI
				Temp = RSbus - Y; 
				ALUout = Temp[31];
			end
      `select_alu_sra:  ALUout = {{32{RTbus[31]}},RTbus} >> Y[5:0];   	// SRA
      `select_alu_srl:  ALUout = RTbus >> Y[5:0];          		// SRL
`ifndef ANUBIS_LOCAL_0
      `select_alu_sll:  ALUout = RTbus << Y[5:0];          		// SLL  
`else
      `select_alu_sll:  ALUout = RTbus >> Y[5:0];
`endif
       default:		ALUout = `undefined;			// Undefined
   endcase
end

endmodule	// alu -- Arithmetic / Logic Unit
