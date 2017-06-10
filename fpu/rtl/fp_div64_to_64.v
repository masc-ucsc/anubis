//==============================================================================
//      File:           $URL$
//      Version:        $Revision$
//      Author:         Jose Renau  (http://masc.cse.ucsc.edu/)
//                      John Burr
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

/****************************************************************************
    Description:

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"


//import scoore_fpu::*;
//import retry_common::*;

module fp_div64_to_64
  (input                             clk,
   input                             reset,
   // inputs
   input                             start,

   input [`FP_MAN_BITS-1:0]          man1,
   input [`FP_MAN_BITS-1:0]          man2,

   input                             sign1,
   input                             sign2,

   input [`FP_PREDEC_BITS-1:0]       op_predec,
   input [`FP_STATE_BITS-1:0]        state,
   input [2-1:0]                   round,
   input [`FP_EXP_BITS-1:0]          exp,
   input                             rshift_man,

   input                             div_in_retry,

   // Retry re-clocking signals
   input [4:0]                 rci12,

   // outputs
   output                            divp_op_ready,
   output                            divp_op_valid,
   output                            divp_op_fault,
   output [`FP_MAN_BITS+1-1:0]       divp_op_man,

   output [`FP_PREDEC_BITS-1:0]      divp_op_predec,
   output [`FP_STATE_BITS-1:0]       divp_state,
   output [2-1:0]                  divp_round,
   output [`FP_EXP_BITS-1:0]         divp_op_exp,
   output                            divp_op_sign,

   output                            divp_rshift_man,

   output                            divp_busy,
   output reg                        div_out_retry,

   // Retry re-clocking signals
   output [4:0]                rco12
 );

  // FIXME: div64_to64 already has a flop. Why do we have another one????. Try to remove it.

  reg [(`FP_MAN_BITS*3)-1:0]        qr_next;
  reg [`FP_MAN_BITS-1:0]            divisor_next;
  reg [6:0]                         count_next;
  reg                               divp_op_ready_next;
  logic                             divp_op_ready_temp;

  wire [(`FP_MAN_BITS*3)-1:0]       qr;
  wire [`FP_MAN_BITS-1:0]           divisor;
  wire [6:0]                        count;

  reg [(`FP_MAN_BITS*2):0]          diff;

  reg  [`FP_PREDEC_BITS-1:0]        op_predec_next;
  reg  [`FP_STATE_BITS-1:0]         state_next;
  reg  [2-1:0]                         round_next;
  reg  [`FP_EXP_BITS-1:0]           exp_next;
  reg                               rshift_man_next;
  reg                               sign_next;

  reg                               divp_busy_next;

  wire                              divider_s_retry;
  logic                             divider_s_valid;

  always_comb begin
`ifndef ANUBIS_GLOBAL_2
    div_out_retry = divp_busy;
`else
    div_out_retry = divider_s_retry || divp_busy;
`endif
  end

  logic tmp;

  always_comb begin
    if(start) begin
      divp_busy_next     = 1'b1;
    end else begin
`ifndef ANUBIS_GLOBAL_2
      if(divp_op_ready && ~divider_s_retry) begin
`else
      if(divp_op_ready && divp_op_valid && ~div_in_retry) begin
`endif
        divp_busy_next  = 1'b0;
      end else if(divp_busy) begin
        divp_busy_next  = 1'b1;
      end else begin
        divp_busy_next  = 1'b0;
      end
    end
  end

	always_comb begin
		if (divp_busy_next) begin
			if (!divp_op_valid && !start) begin
				divider_s_valid = 'b0;
			end else begin
				divider_s_valid = 'b1;
			end
		end else begin
			divider_s_valid = 'b0;
		end
	end

  always @ (*) begin
    if(start) begin
      qr_next            = {64'b0, man1, 64'b0};
      divisor_next       = man2;
      divp_op_ready_next = 1'b0;
      count_next         = `FP_MAN_BITS;

      op_predec_next     = op_predec;
      state_next         = state;
      round_next         = round;
      exp_next           = exp;
      rshift_man_next    = rshift_man;
      sign_next          = sign1 ^ sign2;
    end else begin
`ifndef ANUBIS_GLOBAL_3
      op_predec_next     = divp_op_predec;  //same functionality as a cgflop enabled with start
`else
      op_predec_next     = ~divp_op_predec;  //same functionality as a cgflop enabled with start
`endif
      state_next         = divp_state;
      round_next         = divp_round;
      exp_next           = divp_op_exp;
      rshift_man_next    = divp_rshift_man;
      sign_next          = divp_op_sign;
      divisor_next = divisor;
      diff         = qr[(`FP_MAN_BITS*3)-1:`FP_MAN_BITS] - {1'b0, divisor}; // 1'b0 to force unsigned?

      divp_op_ready_next = (count == 0);

      if (divp_busy_next && divp_op_valid) begin
        if ( (diff[(`FP_MAN_BITS*2)] != 0) ) begin
          qr_next = qr << 1;
        end  else begin
          qr_next = {diff[(`FP_MAN_BITS*2)-2:0], qr[`FP_MAN_BITS-1:0], 1'b1};
        end

        // set LSB (sticky bit) when more bits are needed
        if ( divp_op_ready_next && diff[(`FP_MAN_BITS*2)] && (qr[(`FP_MAN_BITS*3)-1:`FP_MAN_BITS] != 0)) begin
          qr_next[1] = 1'b1;
        end

        count_next = count - 1;
      end else begin
        // Keep same value if we are in a state machine and suddenly the qValid went down
        qr_next = qr;
        count_next = count;
      end
    end
  end

  assign    divp_op_man   = qr[`FP_MAN_BITS:0];
  assign    divp_op_fault = 1'b0; // FIXME: handle faults

 register #(.Size(1)) // busy signal needs to be generated regardless of stage

   div64_busy_l
   (.clk      (clk)
   ,.reset    (reset)

   ,.dinValid  (1'b1)
   ,.din       (divp_busy_next)
   ,.q         (divp_busy)
   );

  /*
  logic divp_busy_valid;
  logic divp_busy_retry;
  stage #(.Size(1))
   div64_busy_l
   (.clk      (clk)
   ,.reset    (reset)

   ,.din       (divp_busy_next)
   ,.dinValid  (1'b1)
   ,.qRetry    (1'b0)

   ,.q         (divp_busy)
   ,.qValid    (divp_busy_valid)
   ,.dinRetry  (divp_busy_retry)
   );
   */

 stage #(.Size((`FP_MAN_BITS*3)+`FP_MAN_BITS+1+`FP_PREDEC_BITS+`FP_STATE_BITS+2+`FP_EXP_BITS+1+1+7))

   div64_to_64_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({qr_next, divisor_next, divp_op_ready_next,
                   op_predec_next, state_next, round_next,
                     exp_next, sign_next, rshift_man_next,
                       count_next})
   ,.dinValid (divider_s_valid)
   ,.dinRetry (divider_s_retry)

   ,.q        ({qr, divisor, divp_op_ready_temp,
                   divp_op_predec, divp_state, divp_round,
                     divp_op_exp, divp_op_sign, divp_rshift_man,
                       count})
   ,.qValid   (divp_op_valid)
   ,.qRetry   (div_in_retry)

   ,.rci      (rci12)
   ,.rco      (rco12)
   );

`ifndef ANUBIS_GLOBAL_1
assign divp_op_ready = divp_op_ready_temp & divp_op_valid; //breaks coding style but cleaner than implementing in post_comb
`else
assign divp_op_ready = divp_op_ready_temp & ~divp_op_valid; //breaks coding style but cleaner than implementing in post_comb
`endif

endmodule
