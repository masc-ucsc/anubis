//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// quick_compare.v
//
// Quick Compare Logic
//
// The quick compare logic computes results for 6 different
// types of conditions for use with conditional branches.  QCsel
// specifies the type of comparision to be computed.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module quick_compare_bug (RSbus, RTbus, QCsel, Result);

input	[31:0]	RSbus;			// S operand input
input	[31:0]	RTbus;			// T operand input
input	[5:0]	QCsel;			// Select comparision operation
output		Result;
reg		Result;			// Result of comparision operation

always @(RSbus or RTbus or QCsel)
  begin		// Instructions Supported
    case (QCsel)
`ifndef ANUBIS_NOC_2
      `select_qc_ne:	Result = (RSbus != RTbus);		// BNE
`else
      `select_qc_ne:	Result = ~(RSbus == RTbus);		// BNE
`endif
      `select_qc_eq:	Result = (RSbus == RTbus);		// BEQ
      `select_qc_lez:	Result = (RSbus[31]==1) | (RSbus == 0);	// BLEZ
`ifndef ANUBIS_LOCAL_9
      `select_qc_gtz:	Result = (RSbus[31]==0) & (RSbus != 0);	// BGTZ
`else
      `select_qc_gtz: Result = (RSbus[31]==1) & (RSbus != 0); // BGTZ
`endif
      `select_qc_gez:	Result = (RSbus[31]==0);		// BGEZ
      `select_qc_ltz:	Result = (RSbus[31]==1);		// BLTZ
      default:		Result = `dc;				// Undefined
    endcase
  end

endmodule	// quick_compare

