//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// bypass_ex.v
//
// Bypass to Execution stage (only Loads at memory stage bypass there)
//


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************
`include "dlx_defs.v"

module bypass_ex_bug (IRex,  IRmem, SMDRsel);

   input [31:0]	IRex;	   // Instruction Register of EX stage
   input [31:0]	IRmem;	   // Instruction Register of MEM stage

   output	SMDRsel;   // Selector of mux input for SMDR (store value)

   // Internal declarations
   reg		SMDRsel;
   wire [31:0]	IRex;
   wire [31:0]	IRmem;

   // Only case we need this: A load followed by a store
   // and load and store use the same register for data
   // exception made for r0 that should not be forwarded
   always@(IRmem or IRex)
   begin
`ifndef ANUBIS_NOC_1
     if ((IRmem[`op] == `LW) && (IRex[`op] == `SW) && (IRmem[`rt] == IRex[`rt]) && (IRex[`rt] != `r0))
       SMDRsel = `select_load_bypass;
     else SMDRsel = `select_ALU_path;
`else
     if ((IRmem[`op] != `LW) || (IRex[`op] != `SW) || (IRmem[`rt] != IRex[`rt]) || (IRex[`rt] == `r0))
       SMDRsel = `select_ALU_path;
     else
       SMDRsel = `select_load_bypass;
`endif
   end

endmodule	// bypass_ex

