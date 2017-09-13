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

    Uses fp_propagate_nan_comb to handle NaN cases, but also handles n/n and
    n/1.0 as special cases.  If an overflow was found in fp_denorm, it is
    handled here, using the appropriate rounding method.

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//`include "scoore_fpu.v"

//import scoore_fpu::*;
//import retry_common::*;

module fp_propagate_div
  (// inputs
   input                             clk,
   input                             reset,
   input                             enable_div,

   input [`FP_PREDEC_BITS-1:0]       op_predec_div,
   input [2-1:0]                   round_div,
   input [`FP_STATE_BITS-1:0]        state_div,
   input [`FP_EXP_BITS-1:0]          exp1_div,
   input                             sign1_div,
   input [`FP_MAN_BITS-1:0]          man1_div,  //assumed to have hidden bit already
   input [`FP_EXP_BITS-1:0]          exp2_div,
   input                             sign2_div,
   input [`FP_MAN_BITS-1:0]          man2_div,  //assumed to have hidden bit already

   input                             overflow,

   input                             nan_retry,
   input                             divp_busy0,
   input                             divp_busy1,
   input                             divp_busy2,
   input                             divp_busy3,

   //Retry Re-clocking signals
   input  [4:0]                rci13,


   // outputs
   output                            divp_nan_fault_prop,
   output                            divp_nan_ready_prop,
   output                            divp_nan_sign_prop,
   output [`FP_EXP_BITS-1:0]         divp_nan_exp_prop,
   output [`FP_MAN_BITS-1:0]         divp_nan_man_prop,

   output [`FP_STATE_BITS-1:0]       divp_nan_state_prop,
   output [2-1:0]                  divp_nan_round_prop,
   output [`FP_PREDEC_BITS-1:0]      divp_nan_op_predec_prop,

   output                            divider0_start,
   output                            divider1_start,
   output                            divider2_start,
   output                            divider3_start,

   output                            prop_retry_out,
   output                            sel_retry_out,

   `ifndef ANUBIS_GLOBAL_0
   output                            special_select,
   `endif

   //Retry Re-clocking signals
   output  [4:0]               rco13
  );

  reg                                divp_nan_ready_prop_next;
  reg                                divp_nan_fault_prop_next;
  reg                                divp_nan_sign_prop_next;
  reg [`FP_EXP_BITS-1:0]             divp_nan_exp_prop_next;
  reg [`FP_MAN_BITS-1:0]             divp_nan_man_prop_next;

  wire                               nan_select;
  wire                               nan_fault;
  wire                               nan_sign;
  wire [`FP_EXP_BITS-1:0]            nan_exp;
  wire [`FP_MAN_BITS-1:0]            nan_man;

  reg  [2-1:0]                          divp_nan_round_prop_next;
  reg  [`FP_STATE_BITS-1:0]          divp_nan_state_prop_next;
  reg  [`FP_PREDEC_BITS-1:0]         divp_nan_op_predec_prop_next;

  wire                               start;
  assign                             start = enable_div;

  fp_propagate_nan_comb p_nan
   (// inputs
     .start              (start),

     .exp1               (exp1_div),
     .sign1              (sign1_div),
     .man1               (man1_div),

     .exp2               (exp2_div),
     .sign2              (sign2_div),
     .man2               (man2_div),

     // outputs
     .nan_select         (nan_select),
     .nan_fault          (nan_fault),
     .nan_sign           (nan_sign),
     .nan_exp            (nan_exp),
     .nan_man            (nan_man)
     );


  always_comb begin

`ifdef ANUBIS_LOCAL_9
    if(start) begin
`endif

      divp_nan_round_prop_next       = round_div;
      divp_nan_state_prop_next       = state_div;
      divp_nan_op_predec_prop_next   = divp_nan_op_predec_prop;
`ifndef ANUBIS_LOCAL_25
      if(nan_select) begin
`else
      if(~nan_select) begin
`endif
        // use result from propagate_nan
        divp_nan_ready_prop_next = nan_select;
        divp_nan_fault_prop_next = nan_fault;
`ifndef ANUBIS_LOCAL_25
        divp_nan_sign_prop_next  = nan_sign;
`else
        divp_nan_sign_prop_next  = ~nan_sign;
`endif

        divp_nan_exp_prop_next   = nan_exp;
        divp_nan_man_prop_next   = nan_man;
      end else if(overflow) begin
        divp_nan_sign_prop_next = sign1_div ^ sign2_div;
        case( {divp_nan_sign_prop_next, round_div} )
          {1'b1, `FP_ROUND_MININF},
          {1'b0, `FP_ROUND_PLUSINF},
          {1'b0, `FP_ROUND_NEAREST},
          {1'b1, `FP_ROUND_NEAREST} : begin
            divp_nan_ready_prop_next = 1'b1;
            divp_nan_fault_prop_next = 1'b0;
            divp_nan_exp_prop_next   = {`FP_EXP_BITS{1'b1}};
            divp_nan_man_prop_next   = {`FP_MAN_BITS{1'b0}};
          end
          {1'b0, `FP_ROUND_ZERO},
          {1'b1, `FP_ROUND_ZERO},
          {1'b0, `FP_ROUND_MININF},
          {1'b0, `FP_ROUND_PLUSINF},
          {1'b1, `FP_ROUND_PLUSINF} : begin
            divp_nan_ready_prop_next = 1'b1;
            divp_nan_fault_prop_next = 1'b0;
            divp_nan_exp_prop_next   = ~`FP_EXP_BITS'b0 - 1;
            divp_nan_man_prop_next   = ~`FP_MAN_BITS'b0;
          end
          default: begin
            divp_nan_ready_prop_next = 1'b0;
            divp_nan_fault_prop_next = 1'bx;
            divp_nan_exp_prop_next   = `FP_EXP_BITS'bx;
            divp_nan_man_prop_next   = `FP_MAN_BITS'bx;
          end
        endcase
      end else if ((exp1_div == exp2_div) & (man1_div == man2_div)) begin
        // n/n, so result is 1.0
`ifndef ANUBIS_LOCAL_10
        divp_nan_ready_prop_next = 1'b1;
`else
        divp_nan_ready_prop_next = start;
`endif
        divp_nan_fault_prop_next = 1'b0;
        divp_nan_sign_prop_next = sign1_div ^ sign2_div;
        if (op_predec_div[`FP_PREDEC_DST_FP32_BIT]) begin
          divp_nan_exp_prop_next = {4'b0, ~{`FP_EXP_BITS-4{1'b0}}};
        end else begin
          divp_nan_exp_prop_next = {1'b0, ~{`FP_EXP_BITS-1{1'b0}}};
        end
        divp_nan_man_prop_next = {1'b1, {`FP_MAN_BITS-1{1'b0}}};
      end else if ( (((exp2_div == {1'b0, ~{`FP_EXP_BITS-1{1'b0}}}) & op_predec_div[`FP_PREDEC_DST_FP64_BIT]) |
        ((exp2_div == {4'b0, ~{`FP_EXP_BITS-4{1'b0}}}) & op_predec_div[`FP_PREDEC_DST_FP32_BIT])) &
        (man2_div == {1'b1, {`FP_MAN_BITS-1{1'b0}}}) ) begin
          // man1/1.0 so result is man1
`ifndef ANUBIS_LOCAL_10
        divp_nan_ready_prop_next = 1'b1;
`else
        divp_nan_ready_prop_next = start;
`endif
        divp_nan_fault_prop_next  = 1'b0;
        divp_nan_sign_prop_next   = sign1_div ^ sign2_div;
        divp_nan_exp_prop_next    = exp1_div;
        divp_nan_man_prop_next    = man1_div;
      end else begin
        divp_nan_ready_prop_next  = 1'b0;
        divp_nan_fault_prop_next  = 1'bx;
        divp_nan_sign_prop_next   = 1'bx;
        divp_nan_exp_prop_next    = `FP_EXP_BITS'bx;
        divp_nan_man_prop_next    = `FP_MAN_BITS'bx;
      end
 `ifdef ANUBIS_LOCAL_9
    end else begin
        divp_nan_fault_prop_next        = 'b0;   //same functionality as a cgflop enabled with start
        divp_nan_sign_prop_next         = 'b0;
        divp_nan_exp_prop_next          = 'b0;
        divp_nan_man_prop_next          = 'b0;
        divp_nan_round_prop_next        = 'b0;
        divp_nan_state_prop_next        = 'b0;
        divp_nan_op_predec_prop_next    = 'b0;
        divp_nan_ready_prop_next        = 1'b0;
    end
 `endif
   end



  stage #(.Size(1+1+`FP_EXP_BITS+`FP_MAN_BITS+2+`FP_STATE_BITS+`FP_PREDEC_BITS))  prop_div_out_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({divp_nan_fault_prop_next, divp_nan_sign_prop_next, divp_nan_exp_prop_next, divp_nan_man_prop_next, round_div, state_div, op_predec_div})
`ifndef ANUBIS_LOCAL_10
   ,.dinValid (divp_nan_ready_prop_next & start) // && ~prop_retry_out)
`else
   ,.dinValid (~prop_retry_out)
`endif
   ,.dinRetry (prop_retry_out)

   ,.q        ({divp_nan_fault_prop, divp_nan_sign_prop, divp_nan_exp_prop, divp_nan_man_prop, divp_nan_round_prop, divp_nan_state_prop, divp_nan_op_predec_prop})
   ,.qValid   (divp_nan_ready_prop)
   ,.qRetry   (nan_retry)

   // Re-clocking signals
   ,.rci      (rci13)
   ,.rco      (rco13)
   );

  fp_divider_selector_comb div_selector
  (.nan_select    (divp_nan_ready_prop_next), // WARNING (not same cycle input. Breaks coding style, but it is cleaner)
   .nan_prop_retry(prop_retry_out),
   .sel_retry_out (sel_retry_out),
   .divp_busy0    (divp_busy0),
   .divp_busy1    (divp_busy1),
   .divp_busy2    (divp_busy2),
   .divp_busy3    (divp_busy3),
   .enable_div (enable_div),
   .divider0_start(divider0_start),
   .divider1_start(divider1_start),
   .divider2_start(divider2_start),
   .divider3_start(divider3_start)
  );

`ifndef ANUBIS_GLOBAL_0
  assign special_select = divp_nan_ready_prop_next;
`endif

endmodule
