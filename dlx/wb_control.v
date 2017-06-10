//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// wb_control.v
//
// Write Back Control Logic
//
// Decode destination register number:
//	The following logic examines a 32-bit instruction and decodes the
//	appropriate fields in order to find a register file destination
//	address for the result.  If the operation does not write to the
//	register file, decode_rd returns a destination register of zero.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module wb_control_bug (IR, WBsel);

input	[31:0]	IR;		// Instruction Register
output	[2:0]	WBsel;
reg	[2:0]	WBsel;		// Select lines for WB register

always @(IR)
begin
   casex ({IR[`op], IR[`function]})
        // Write back load data from cache
        {`LW,      `dc6 }: WBsel = `select_wb_load;

        // Write back PC link value
        {`JAL,     `dc6 }: WBsel = `select_wb_link;
        {`SPECIAL, `JALR}: WBsel = `select_wb_link;

        // Write back result from ALU
`ifndef ANUBIS_LOCAL_10
        default          : WBsel = `select_wb_alu;
`endif
   endcase
end

endmodule	// wb_control

