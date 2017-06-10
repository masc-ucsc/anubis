//******************************************************************************
// MIPS-Lite verilog model
// Correct Design 
//
// ff.v
//
// Flipflop / Propagate chain element.
//
// This module implements a flipflop chain primarily used
// to propagate the state of the machine across the clock cycle
// boundary in a manner which avoids troublesome race conditions.
// The flip flop has 2 control lines: reset and stall. Reset clears
// the state of the f/f by passing 0's through while stall holds
// the current state of the f/f. Please note that Reset has priority
// over stall.
//
// Remember that this module is parametric in its bitwidth. So, it
// can be used for any arbitrary width. For example, if we wanted a
// 11-bit element, we would instantiate this module as follows:
//
//	propagate #(11) _instanceName_ (in_11_bits, stall_line,
//					reset_line, out_11_bits);
//
// The only difference between the above instantiation and any regular
// module instantiation is the parameter binding (11 to Width in this case).
// Note: if you do not specify a parameter, the width will default to
// 2 bits as specified in the parameter declaration.
//


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

module propagate_bug (in, stall, reset, out, clk);

input clk;
parameter Width = 2;

input	[Width-1:0]	in;		// input data
wire	[Width-1:0]	in;
input			stall;		// stall control line
wire			stall;
input			reset;		// reset control line
wire			reset;
output	[Width-1:0]	out;		// out data - stable at posedge
reg	[Width-1:0]	out;		//   of next cycle.

reg	[Width-1:0]	FF_temp;	// intermediate data.

always @(negedge clk)
   begin
		# 5
      if (reset)
         FF_temp = {Width{1'b0}};
      else if (!stall)
         FF_temp = in;
   end


always @(posedge clk)
   begin
      if (reset)
         out = {Width{1'b0}};
      else if (!stall)
         out = FF_temp;
   end

endmodule
