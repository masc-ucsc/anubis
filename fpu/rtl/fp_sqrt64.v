//==============================================================================
//      File:           $URL$
//      Version:        $Revision$
//      Author:         Alana Muldoon
//                      Pranav - Stage Impl
//      Copyright:      Copyright 2005-2008 UC Santa Cruz
//==============================================================================

//==============================================================================
//      Section:        License
//==============================================================================
//      Copyright (c) 2005-2008, Regents of the University of California
//      All rights reserved.
//
//      Redistribution and use in source and binary forms, with or without modification,
//      are permitted provided that the following conditions are met:
//
//              - Redistributions of source code must retain the above copyright notice,
//                      this list of conditions and the following disclaimer.
//              - Redistributions in binary form must reproduce the above copyright
//                      notice, this list of conditions and the following disclaimer
//                      in the documentation and/or other materials provided with the
//                      distribution.
//              - Neither the name of the University of California, Santa Cruz nor the
//                      names of its contributors may be used to endorse or promote
//                      products derived from this software without specific prior
//                      written permission.
//
//      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//      ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//      WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//      DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
//      ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//      (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//      LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//      ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//      SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==============================================================================

/*******************************************************************************

	Written by Alana Muldoon - Dec 2008

	Description:

		This is the fp sqrt module, currently implemented as sqrt(x) = x to set up the structure to fit in with the rest of the code

		algorithm to implement works as follows:
		x = (1+m) << (e%2)
		y_0 = 1
		y_n = y_n-1 + 2^(-n) if y_n^2 < x, y_n-1 otherwise
		y_n^2 = (y_n-1 + 2^-n)^2 = y_n-1^2 + y_n-1 * 2^(1-n) + 2^(-2n)
*******************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

//import scoore_fpu::*;
//import retry_common::*;

module fp_sqrt64
  (input                             clk,
   input                             reset,
   // inputs
   input                             enable_sqrt,

   input [`FP_PREDEC_BITS-1:0]       op_predec_sqrt,
   input [2-1:0]                   round_sqrt,
   input [`FP_STATE_BITS-1:0]        state_sqrt,
   input [`FP_EXP_BITS-1:0]          exp_sqrt,
   input                             sign2_sqrt,
   input [`FP_MAN_BITS-1:0]          man2_sqrt,

   input 		                         sqrt_shift,

   input                             res_retry_sqrt,

   // outputs
   output                            sqrtp_ready,
   output [`FP_PREDEC_BITS-1:0]      sqrtp_op_predec,
   output [`FP_STATE_BITS-1:0]       sqrtp_state,
   output [2-1:0]                  sqrtp_round,

   output                            sqrtp_sign,
   output [`FP_EXP_BITS-1:0]         sqrtp_exp,
   output [`FP_MAN_BITS-1:0]         sqrtp_man,

   output                            retry_sqrt,

   // Retry re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco
  );

  //-------------------------------------------------------------
  // PHASE 0  (lock in values)
  //-------------------------------------------------------------


  wire                             sqrt_sign_p1;
  wire [`FP_EXP_BITS-1:0]          sqrt_exp_p1;
  wire [`FP_MAN_BITS-1:0]          sqrt_man_p1;
  wire [`FP_PREDEC_BITS-1:0]       op_predec_p1;
  wire [`FP_STATE_BITS-1:0]        state_p1;
  wire [2-1:0]                        round_p1;

  reg					              		   sqrt_special_next;

  wire 					            		   sqrt_ready_p1;
  wire				            			   sqrt_ready_special_p1;


  wire                             phase0_sqrt_retry;

  wire                             phase1_retry;
  wire                             phase1_special_retry;
  reg                              phase1_sqrt_retry;
  wire                             phase1_s_retry;
  wire                             sqrt_trig_valid;
  wire                             sqrt_special_valid;

  reg                              phase2_special_retry;
  reg                              phase2_sqrt_retry;
  wire                             phase2_retry;

  logic sqrt_special;
  logic sqrt_shift_p1;
 	logic sqrt_busy;

`ifndef ANUBIS_NOC_5
  always_comb begin
    if (exp_sqrt == `FP_EXP_BITS'b0 && man2_sqrt == `FP_MAN_BITS'b0) begin
      // zero
      sqrt_special_next = 1'b1;
    end else if (sign2_sqrt == 1 || exp_sqrt == `FP_EXP_BITS'h7FF) begin
      // negative or INF/NaN
      sqrt_special_next = 1'b1;
    end else begin
      sqrt_special_next = 1'b0;
    end
  end
`else
  assign sqrt_special_next = ((exp_sqrt == `FP_EXP_BITS'b0 && man2_sqrt == `FP_MAN_BITS'b0) ||
                              (sign2_sqrt == 1 || exp_sqrt == `FP_EXP_BITS'h7FF));
`endif

  logic sqrt_ready_p1_val;
  wire [4:0] rco21;
  wire [4:0] rco22;
  wire [4:0] rco23;

  stage #(.Size(1+(`FP_EXP_BITS)+(`FP_MAN_BITS)+`FP_PREDEC_BITS+2+`FP_STATE_BITS+1+1))
  phase0_sqrt_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sign2_sqrt, exp_sqrt,  man2_sqrt, op_predec_sqrt, round_sqrt, state_sqrt, sqrt_shift, sqrt_special_next})
`ifndef ANUBIS_LOCAL_15
   ,.dinValid (enable_sqrt)
`else
   ,.dinValid (enable_sqrt & ~phase0_sqrt_retry)
`endif
   ,.dinRetry (phase0_sqrt_retry)

   ,.q        ({sqrt_sign_p1, sqrt_exp_p1, sqrt_man_p1, op_predec_p1, round_p1, state_p1, sqrt_shift_p1, sqrt_special})
   ,.qValid   (sqrt_ready_p1_val)
   ,.qRetry   (phase1_retry)

   // Re-clocking signals
   ,.rci     (rci)
   ,.rco     (rco21)
   );

`ifndef ANUBIS_LOCAL_13
  assign phase1_retry          = (phase1_sqrt_retry & ~sqrt_special) |
                                 (phase1_special_retry & sqrt_special);
`else
  assign phase1_retry          = phase1_sqrt_retry|phase1_special_retry;
`endif

  assign sqrt_ready_p1         = sqrt_ready_p1_val & ~sqrt_special; // Valid signals for next stage p1
  assign sqrt_ready_special_p1 = sqrt_ready_p1_val & sqrt_special;
`ifndef ANUBIS_LOCAL_12
  assign sqrt_trig_valid       = sqrt_ready_p1;
  assign sqrt_special_valid    = sqrt_ready_special_p1;
`else
  assign sqrt_trig_valid       = sqrt_ready_p1 & ~phase1_retry;
  assign sqrt_special_valid    = sqrt_ready_special_p1 & ~phase1_retry;
`endif
//--------------------------------------------------------------------
// PHASE 1 (perform sqrt - 63 cycles)
//--------------------------------------------------------------------

	wire					  		             sqrt_ready_p2;
	wire                             sqrt_sign_p2;
  wire [`FP_EXP_BITS-1:0]          sqrt_exp_p2;
	wire [`FP_MAN_BITS-1:0]          sqrt_man_p2;
 	wire [`FP_PREDEC_BITS-1:0]       op_predec_p2;
 	wire [`FP_STATE_BITS-1:0]        state_p2;
  wire [2-1:0]                        round_p2;

	reg                              sqrt_sign_next;
  reg  [`FP_EXP_BITS-1:0]          sqrt_exp_p2_next;
	reg  [`FP_MAN_BITS-1:0]          sqrt_man_p2_next;
 	reg  [`FP_PREDEC_BITS-1:0]       op_predec_p2_next;
 	reg  [`FP_STATE_BITS-1:0]        state_p2_next;
  reg  [2-1:0]                        round_p2_next;

 	wire [`FP_MAN_BITS-1:0]			     bits;
 	wire [`FP_MAN_BITS-1+2:0]			   rem;

 	reg [`FP_MAN_BITS-1+2:0]			   rem_maybe;

`ifndef ANUBIS_LOCAL_16
 	reg [`FP_MAN_BITS-1:0]			   precomputed_rem2_next;
 	reg [`FP_MAN_BITS-1:0]			   precomputed_rem2;
`else
  reg [1+`FP_MAN_BITS-1:0]         precomputed_rem2_next;
  reg [1+`FP_MAN_BITS-1:0]         precomputed_rem2;
`endif

 	reg 						  	             sqrt_ready_p2_next;
 	reg 						  	             sqrt_ready_p2_temp;

 	reg [`FP_MAN_BITS-1:0]			     bits_next;
 	reg [`FP_MAN_BITS-1+2:0]			   rem_next;
 	reg 							               sqrt_busy_next;

  wire                             sqrt_op_valid;

	// permenant logic to calculate the next value early as possible
  always_comb begin
`ifndef ANUBIS_LOCAL_16
    rem_maybe = (rem-{1'b0,precomputed_rem2})<<1;
`else
    rem_maybe = (rem-{1'b0,precomputed_rem2})<<1;
`endif
  end

  always_comb begin

`ifndef ANUBIS_LOCAL_13
    if (sqrt_trig_valid & ~sqrt_busy) begin
`else
  if (sqrt_trig_valid) begin
`endif
			precomputed_rem2_next  = {1'b1,63'd0} + ({2'b01,62'd0}>>1);
		end else if (precomputed_rem2 >= rem) begin // underflow
      precomputed_rem2_next = sqrt_man_p2+(bits>>2);
    end else begin
      precomputed_rem2_next = (sqrt_man_p2|bits)+(bits>>2);
    end
  end

	always_comb begin
    if (sqrt_busy) begin
      if (rem_maybe[`FP_MAN_BITS-1+2]) begin // underflow
        rem_next         = rem<<1;
        sqrt_man_p2_next = sqrt_man_p2;
      end else begin
        rem_next         = rem_maybe;
        sqrt_man_p2_next = sqrt_man_p2 | bits;
      end
      if (sqrt_op_valid) begin
        sqrt_busy_next    = ~bits[0];
        bits_next         = bits>>1;
      end else begin
        sqrt_busy_next    = 1'b1;
        bits_next         = bits;
      end

      sqrt_sign_next    = sqrt_sign_p2;
      sqrt_exp_p2_next  = sqrt_exp_p2;
      op_predec_p2_next = op_predec_p2;
      round_p2_next     = round_p2;
      state_p2_next     = state_p2;

    end else if (sqrt_trig_valid) begin
      if (~sqrt_shift_p1) begin
        rem_next = ({1'b0, sqrt_man_p1,1'b0}) - {1'b1,63'd0};
      end else begin
        rem_next = {2'b00, sqrt_man_p1} - {1'b1,63'd0};
      end

      sqrt_man_p2_next  = {1'b1,63'd0}; // initialize to 1.000000...
      bits_next         = {2'b01,62'd0}; // initialize to 0.10000000...
      sqrt_busy_next    = 1'b1;
      sqrt_sign_next    = sqrt_sign_p1;
      sqrt_exp_p2_next  = sqrt_exp_p1;
      op_predec_p2_next = op_predec_p1;
      round_p2_next     = round_p1;
      state_p2_next     = state_p1;
    end else begin
      sqrt_busy_next    = 1'b0;
      rem_next          = 'bx;
      sqrt_man_p2_next  = 'bx;
      bits_next         = 64'd0;
      sqrt_sign_next    = 'bx;
      sqrt_exp_p2_next  = 'bx;
      op_predec_p2_next = 'bx;
      round_p2_next     = 'bx;
      state_p2_next     = 'bx;
    end
  end

  always_comb begin
    phase1_sqrt_retry = sqrt_busy | phase1_s_retry;
  end

	register #(.Size(1))
      phase1_busy_reg
      (.clk        (clk),
       .reset      (reset),
       .dinValid   (1'b1),
       .din        (sqrt_busy_next),
       .q          (sqrt_busy)
       );
  logic sqrt_busy_retry;
  logic sqrt_busy_valid;

  assign sqrt_ready_p2 = sqrt_ready_p2_temp & sqrt_op_valid;

`ifndef ANUBIS_LOCAL_16
  stage #(.Size(1+(`FP_EXP_BITS)+(`FP_MAN_BITS)+`FP_PREDEC_BITS+2+`FP_STATE_BITS +`FP_MAN_BITS+(`FP_MAN_BITS+2)+(`FP_MAN_BITS)+1))
`else
  stage #(.Size(1+(`FP_EXP_BITS)+(`FP_MAN_BITS)+`FP_PREDEC_BITS+2+`FP_STATE_BITS +`FP_MAN_BITS+(`FP_MAN_BITS+2)+(`FP_MAN_BITS)+1+1))
`endif

  phase1_sqrt_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sqrt_sign_next, sqrt_exp_p2_next, sqrt_man_p2_next, op_predec_p2_next,
                  round_p2_next, state_p2_next, bits_next, rem_next, precomputed_rem2_next, bits_next[0]})
`ifndef ANUBIS_LOCAL_13
   ,.dinValid ((sqrt_trig_valid & ~sqrt_busy) | (sqrt_busy_next & sqrt_op_valid))
`else
   ,.dinValid (sqrt_trig_valid | (sqrt_busy_next &sqrt_op_valid))
`endif
   ,.dinRetry (phase1_s_retry)

   ,.q        ({sqrt_sign_p2, sqrt_exp_p2, sqrt_man_p2, op_predec_p2,
                  round_p2, state_p2, bits, rem, precomputed_rem2, sqrt_ready_p2_temp})
   ,.qValid   (sqrt_op_valid)
   ,.qRetry   (phase2_sqrt_retry)

   // Re-clocking signals
   ,.rci      (rco21)
   ,.rco      (rco22)
   );

//--------------------------------------------------------------------
// PHASE 1b (generate NaN (and signal if new))
//--------------------------------------------------------------------

	wire					  		             sqrt_ready_special_p2;
	wire                             sqrt_sign_special_p2;
	wire [`FP_EXP_BITS-1:0]          sqrt_exp_special_p2;
	wire [`FP_MAN_BITS-1:0]          sqrt_man_special_p2;
 	wire [`FP_PREDEC_BITS-1:0]       op_predec_special_p2;
 	wire [`FP_STATE_BITS-1:0]        state_special_p2;
  wire [2-1:0]                        round_special_p2;


	reg                              sqrt_sign_special_next;
  reg  [`FP_EXP_BITS-1:0]          sqrt_exp_special_next;
	reg  [`FP_MAN_BITS-1:0]          sqrt_man_special_next;
 	reg  [`FP_PREDEC_BITS-1:0]       op_predec_special_next;
 	reg  [`FP_STATE_BITS-1:0]        state_special_next;
  reg  [2-1:0]                        round_special_next;

	always_comb begin
    // WARNING: This is not parallel, an it SHOULD NOT BE parallel because the mantisa ==0 is a special case
		casez({sqrt_exp_p1, sqrt_man_p1, sqrt_sign_p1})
			{`FP_EXP_BITS'h000, `FP_MAN_BITS'b0, 1'b?}, // zero
			{`FP_EXP_BITS'h7FF, `FP_MAN_BITS'h8000000000000000, 1'b0}: // +INF
			begin // propogate
				sqrt_sign_special_next   = sqrt_sign_p1;
				sqrt_exp_special_next    = sqrt_exp_p1;
				sqrt_man_special_next    = sqrt_man_p1;
			end
			{`FP_EXP_BITS'h7FF, `FP_MAN_BITS'b?, 1'b?}: // NAN or -INF
			begin // propogate with bit set
				sqrt_sign_special_next   = sqrt_sign_p1;
				sqrt_exp_special_next    = sqrt_exp_p1;
				sqrt_man_special_next    = sqrt_man_p1|`FP_MAN_BITS'hC000000000000000;
			end
			default: // negative numbers
			begin // NaN
				sqrt_sign_special_next   = sqrt_sign_p1;
				sqrt_exp_special_next    = `FP_EXP_BITS'h7FF;
				sqrt_man_special_next    = `FP_MAN_BITS'hC000000000000000;
			end
		endcase

		op_predec_special_next = op_predec_p1;
		state_special_next     = state_p1;
		round_special_next     = round_p1;
	end

  stage #(.Size(1+(`FP_EXP_BITS)+(`FP_MAN_BITS)+`FP_PREDEC_BITS+2+`FP_STATE_BITS))

  phase_special_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sqrt_sign_special_next, sqrt_exp_special_next, sqrt_man_special_next, op_predec_special_next, round_special_next, state_special_next})
   ,.dinValid (sqrt_special_valid)
   ,.dinRetry (phase1_special_retry)

   ,.q        ({sqrt_sign_special_p2, sqrt_exp_special_p2, sqrt_man_special_p2, op_predec_special_p2, round_special_p2, state_special_p2})
   ,.qValid   (sqrt_ready_special_p2)
   ,.qRetry   (phase2_special_retry)

   // Re-clocking signals
   ,.rci      (rco22)
   ,.rco      (rco23)
   );

//--------------------------------------------------------------------
// PHASE 2 (lock and send final value)
//--------------------------------------------------------------------


	reg sqrt_ready_p2_comb;

  always_comb begin
    sqrt_ready_p2_comb = (sqrt_ready_p2 | sqrt_ready_special_p2);
  end

	reg                             sqrt_sign_p2_comb;
  reg [`FP_EXP_BITS-1:0]          sqrt_exp_p2_comb;
	reg [`FP_MAN_BITS-1:0]          sqrt_man_p2_comb;
 	reg [`FP_PREDEC_BITS-1:0]       op_predec_p2_comb;
 	reg [`FP_STATE_BITS-1:0]        state_p2_comb;
  reg  [2-1:0]                       round_p2_comb;

	always_comb begin
		if (sqrt_ready_p2) begin
      sqrt_sign_p2_comb   = sqrt_sign_p2;
      sqrt_exp_p2_comb    = sqrt_exp_p2;
      sqrt_man_p2_comb    = sqrt_man_p2;
      op_predec_p2_comb   = op_predec_p2;
      state_p2_comb       = state_p2;
      round_p2_comb       = round_p2;
    end else if(sqrt_ready_special_p2)begin
			sqrt_sign_p2_comb   = sqrt_sign_special_p2;
			sqrt_exp_p2_comb    = sqrt_exp_special_p2;
			sqrt_man_p2_comb    = sqrt_man_special_p2;
			op_predec_p2_comb   = op_predec_special_p2;
			state_p2_comb       = state_special_p2;
			round_p2_comb       = round_special_p2;
    end else begin
    	sqrt_sign_p2_comb   = 'bx;
			sqrt_exp_p2_comb    = 'bx;
			sqrt_man_p2_comb    = 'bx;
			op_predec_p2_comb   = 'bx;
			state_p2_comb       = 'bx;
			round_p2_comb       = 'bx;
    end
	end

  assign retry_sqrt = phase0_sqrt_retry;

  always_comb begin
    phase2_special_retry = phase2_retry | sqrt_ready_p2;
    phase2_sqrt_retry    = phase2_retry;
  end

  stage #(.Size(1+(`FP_EXP_BITS)+(`FP_MAN_BITS)+`FP_PREDEC_BITS+2+`FP_STATE_BITS))

  f_phase2_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sqrt_sign_p2_comb, sqrt_exp_p2_comb, sqrt_man_p2_comb,
                  op_predec_p2_comb, round_p2_comb, state_p2_comb})
`ifndef ANUBIS_LOCAL_15
   ,.dinValid (sqrt_ready_p2_comb)
`else
   ,.dinValid (sqrt_ready_p2_comb &~phase2_retry)
`endif
   ,.dinRetry (phase2_retry)

   ,.q        ({sqrtp_sign, sqrtp_exp, sqrtp_man,
                  sqrtp_op_predec, sqrtp_round, sqrtp_state})
   ,.qValid   (sqrtp_ready)
   ,.qRetry   (res_retry_sqrt)

   // Re-clocking signals
   ,.rci     (rco23)
   ,.rco     (rco)
   );

endmodule
