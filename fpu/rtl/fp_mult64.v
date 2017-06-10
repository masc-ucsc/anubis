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

    Another flop was added so it now takes three cycles to be in time with
    the mult64_to_128 module.

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"
`include "scoore_fpu.h"

module fp_mult64
  (input                             clk,
   input                             reset,
   // inputs
   input                             enable_mult,

   input [`FP_PREDEC_BITS-1:0]       op_predec_mult,
   input [2-1:0]                   round_mult,
   input [`FP_STATE_BITS-1:0]        state_mult,
   input [`FP_EXP_BITS-1:0]          exp_mult,
   input [`FP_EXP_BITS-1:0]          exp1_mult,
   input                             sign1_mult,
   input [`FP_MAN_BITS-1:0]          man1_mult,
   input [`FP_EXP_BITS-1:0]          exp2_mult,
   input                             sign2_mult,
   input [`FP_MAN_BITS-1:0]          man2_mult,

   input                             rshift_man_mult,
   input                             overflow_mult,

   input                             res_retry_mult,


   // outputs
   output [`FP_PREDEC_BITS-1:0]      multp_op_predec,
   output [`FP_STATE_BITS-1:0]       multp_state,
   output [2-1:0]                  multp_round,

   output                            multp_nan_ready,
   output                            multp_nan_fault,
   output                            multp_nan_sign,
   output [`FP_EXP_BITS-1:0]         multp_nan_exp,
   output [`FP_MAN_BITS-1:0]         multp_nan_man,

   output                            multp_op_ready,
   output                            multp_op_fault,
   output                            multp_op_sign,
   output [`FP_EXP_BITS-1:0]         multp_op_exp,
   output [`FP_MAN_BITS+1-1:0]       multp_op_man,

   output                            multp_rshift_man,

   output                            retry_mult,

   //Retry Re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco
 );

  reg start_fp;
  reg start_fixed;
  always_comb begin
    start_fp = enable_mult & (op_predec_mult[`FP_PREDEC_DST_FP64_BIT] |
                              op_predec_mult[`FP_PREDEC_DST_FP32_BIT]);
    start_fixed = enable_mult & (op_predec_mult[`FP_PREDEC_DST_UINT64_BIT] |
                                 op_predec_mult[`FP_PREDEC_DST_SINT64_BIT] |
                                 op_predec_mult[`FP_PREDEC_DST_UINT32_BIT] |
                                 op_predec_mult[`FP_PREDEC_DST_SINT32_BIT]); // FIXME : not used
  end

  wire                            nan_ready_tmp_next;
  wire                            nan_fault_tmp_next;
  wire                            nan_sign_tmp_next;
  wire [`FP_EXP_BITS-1:0]         nan_exp_tmp_next;
  wire [`FP_MAN_BITS-1:0]         nan_man_tmp_next;

  wire                            multp2_retry;
  wire                            multp1_retry;


  fp_propagate_nan_comb p_nan
    (// inputs
     .start              (start_fp),

     .exp1               (exp1_mult),
     .sign1              (sign1_mult),
     .man1               (man1_mult),

     .exp2               (exp2_mult),
     .sign2              (sign2_mult),
     .man2               (man2_mult),

     // outputs
     .nan_select         (nan_ready_tmp_next),
     .nan_fault          (nan_fault_tmp_next),
     .nan_sign           (nan_sign_tmp_next),
     .nan_exp            (nan_exp_tmp_next),
     .nan_man            (nan_man_tmp_next)
     );
  reg                                nan_fault_tmp;
  assign                             multp_op_fault = 1'b0;
  assign                             nan_fault_tmp  = 1'b0;

  wire [16-1:0]                       a_hh_next;
  wire [16-1:0]                       a_hl_next;
  wire [16-1:0]                       a_lh_next;
  wire [16-1:0]                       a_ll_next;
  wire [16-1:0]                      a_hh;
  wire [16-1:0]                      a_hl;
  wire [16-1:0]                      a_lh;
  wire [16-1:0]                      a_ll;
  assign                             {a_hh_next,a_hl_next,a_lh_next,a_ll_next}=man1_mult;

  wire [16-1:0]                       b_hh_next;
  wire [16-1:0]                       b_hl_next;
  wire [16-1:0]                       b_lh_next;
  wire [16-1:0]                       b_ll_next;
  wire [16-1:0]                      b_hh;
  wire [16-1:0]                      b_hl;
  wire [16-1:0]                      b_lh;
  wire [16-1:0]                      b_ll;
  assign                             {b_hh_next,b_hl_next,b_lh_next,b_ll_next}=man2_mult;

  wire  [`FP_PREDEC_BITS-1:0]        op_predec_mult_p0;
  wire  [`FP_STATE_BITS-1:0]         state_mult_p0;
  wire [2-1:0]                          round_mult_p0;
  wire  [`FP_EXP_BITS-1:0]           exp_mult_p0;
  wire                               rshift_man_mult_p0;
  wire                               overflow_mult_p0;
  wire                               nan_ready_tmp_p0;
  wire                               nan_fault_tmp_p0;
  wire                               nan_sign_tmp_p0;
  wire [`FP_EXP_BITS-1:0]            nan_exp_tmp_p0;
  wire [`FP_MAN_BITS-1:0]            nan_man_tmp_p0;

  logic            sign1_mult_p0;
  logic            sign2_mult_p0;
  logic pipe_start;
  logic pipe_retry;

  wire [4:0] rco8;
  wire [4:0] rco9;
  wire [4:0] rco10;

  stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+ 2+`FP_EXP_BITS+1+1+1+1+1+1+`FP_EXP_BITS+`FP_MAN_BITS+1+128))

  f_pipe_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({op_predec_mult, state_mult, round_mult, exp_mult,
                  rshift_man_mult, overflow_mult, nan_fault_tmp_next, nan_sign_tmp_next,
                    nan_exp_tmp_next, nan_man_tmp_next, nan_ready_tmp_next, sign1_mult, sign2_mult,
                      a_hh_next, a_hl_next, a_lh_next, a_ll_next, b_hh_next,
                        b_hl_next, b_lh_next, b_ll_next })

   ,.dinValid (enable_mult)
   ,.dinRetry (retry_mult)

   ,.q        ({op_predec_mult_p0, state_mult_p0, round_mult_p0, exp_mult_p0,
                  rshift_man_mult_p0, overflow_mult_p0, nan_fault_tmp_p0, nan_sign_tmp_p0,
                     nan_exp_tmp_p0, nan_man_tmp_p0, nan_ready_tmp_p0, sign1_mult_p0, sign2_mult_p0,
                       a_hh, a_hl, a_lh, a_ll, b_hh, b_hl, b_lh, b_ll })

   ,.qValid   (pipe_start)
   ,.qRetry   (pipe_retry)

   // Re-clocking signals
   ,.rci      (rci)
   ,.rco      (rco8)
   );

  reg [(`FP_MAN_BITS/2)-1:0]             z2_1_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z3_2_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z3_1_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z4_3_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z4_2_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z4_1_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z5_4_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z5_3_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z5_2_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z5_1_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z6_4_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z6_3_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z6_2_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z7_4_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z7_3_next;
  reg [(`FP_MAN_BITS/2)-1:0]             z8_4_next;

  always_comb begin
    z2_1_next = a_ll * b_ll;
    z3_1_next = a_ll * b_lh;
    z4_1_next = a_ll * b_hl;
    z5_1_next = a_ll * b_hh;
  end

  always_comb begin
    z3_2_next = a_lh * b_ll;
    z4_2_next = a_lh * b_lh;
    z5_2_next = a_lh * b_hl;
    z6_2_next = a_lh * b_hh;
  end

  always_comb begin
    z4_3_next = a_hl * b_ll;
    z5_3_next = a_hl * b_lh;
    z6_3_next = a_hl * b_hl;
    z7_3_next = a_hl * b_hh;
  end

  always_comb begin
    z5_4_next = a_hh * b_ll;
    z6_4_next = a_hh * b_lh;
    z7_4_next = a_hh * b_hl;
    z8_4_next = a_hh * b_hh;
  end

  wire [(`FP_MAN_BITS/2)-1:0]             z2_1; //hl y0
  wire [(`FP_MAN_BITS/2)-1:0]             z3_2; //hl y0
  wire [(`FP_MAN_BITS/2)-1:0]             z3_1; //hl y0
  wire [(`FP_MAN_BITS/2)-1:0]             z4_3; //hl y0
  wire [(`FP_MAN_BITS/2)-1:0]             z4_2; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z4_1; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z5_4; // l y0, h y2
  wire [(`FP_MAN_BITS/2)-1:0]             z5_3; // l y0, h y2
  wire [(`FP_MAN_BITS/2)-1:0]             z5_2; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z5_1; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z6_4; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z6_3; //hl y1
  wire [(`FP_MAN_BITS/2)-1:0]             z6_2; //hl y2
  wire [(`FP_MAN_BITS/2)-1:0]             z7_4; //hl y2
  wire [(`FP_MAN_BITS/2)-1:0]             z7_3; //hl y2
  wire [(`FP_MAN_BITS/2)-1:0]             z8_4; //hl y2


  wire [`FP_PREDEC_BITS-1:0]         p1_op_predec;
  wire [`FP_STATE_BITS-1:0]          p1_state;
  wire [2-1:0]                          p1_round;
  wire [`FP_EXP_BITS-1:0]            p1_exp;
  wire                               p1_rshift_man;
  wire                               p1_overflow;

  wire                               p1_sign;

  wire                               p1_nan_fault;
  wire                               p1_nan_ready;
  wire                               p1_nan_sign;
  wire [`FP_EXP_BITS-1:0]            p1_nan_exp;
  wire [`FP_MAN_BITS-1:0]            p1_nan_man;
  wire                               p1_start;

  reg                                p1_sign_next;

  always_comb begin
    p1_sign_next = sign1_mult_p0 ^ sign2_mult_p0;
  end

  stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+ 2+`FP_EXP_BITS+1+1+1+1+1+`FP_EXP_BITS+`FP_MAN_BITS+1+(16*(`FP_MAN_BITS/2))))

  mult_p0_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({op_predec_mult_p0, state_mult_p0, round_mult_p0, exp_mult_p0,
                  rshift_man_mult_p0, overflow_mult_p0, p1_sign_next, nan_fault_tmp_p0,
                    nan_sign_tmp_p0, nan_exp_tmp_p0, nan_man_tmp_p0, nan_ready_tmp_p0,
                      z2_1_next, z3_2_next, z3_1_next, z4_3_next, z4_2_next, z4_1_next, z5_4_next, z5_3_next, z5_2_next,
                        z5_1_next, z6_4_next, z6_3_next, z6_2_next, z7_4_next, z7_3_next, z8_4_next})
   ,.dinValid (pipe_start)
   ,.dinRetry (pipe_retry)

   ,.q        ({p1_op_predec, p1_state, p1_round, p1_exp,
                  p1_rshift_man, p1_overflow, p1_sign, p1_nan_fault,
                    p1_nan_sign, p1_nan_exp, p1_nan_man, p1_nan_ready,
                      z2_1, z3_2, z3_1, z4_3, z4_2, z4_1, z5_4, z5_3, z5_2,
                        z5_1, z6_4, z6_3, z6_2, z7_4, z7_3, z8_4})
   ,.qValid   (p1_start)
   ,.qRetry   (multp1_retry)

   // Re-clocking signals
   ,.rci     (rco8)
   ,.rco     (rco9)
   );

  //-----------------------------------------------
  // PHASE 1

  reg                                  p2_nan_fault_next;
  reg                                  p2_nan_ready_next;
  reg                                  p2_nan_sign_next;
  reg [`FP_EXP_BITS-1:0]               p2_nan_exp_next;
  reg [`FP_MAN_BITS-1:0]               p2_nan_man_next;

  reg [`FP_MAN_BITS-1:0]               y2_next;
  reg [`FP_MAN_BITS-1+2:0]             y1_next;
  reg [`FP_MAN_BITS-1+2:0]             y0_next;

  wire [`FP_MAN_BITS-1:0]                                y2;
  wire [`FP_MAN_BITS-1+2:0]                              y1;
  wire [`FP_MAN_BITS-1+2:0]                              y0;

  always_comb begin
    y0_next = {z4_3, z2_1} + {z5_4[15:0], z3_2, 16'b0} + {z5_3[15:0], z3_1, 16'b0};

    y1_next = {z6_4, z4_2} + {z6_3, z4_1} + {z5_2, 16'b0} + {z5_1, 16'b0};

    y2_next = {z8_4, z6_2} + {z7_4, z5_4[31:16]} + {z7_3, z5_3[31:16]};
  end

  always_comb begin
    case( {p1_nan_ready, p1_overflow, p1_sign, p1_round} )
      {1'b0, 1'b1, 1'b1, `FP_ROUND_MININF},
      {1'b0, 1'b1, 1'b0, `FP_ROUND_PLUSINF},
      {1'b0, 1'b1, 1'b0, `FP_ROUND_NEAREST},
      {1'b0, 1'b1, 1'b1, `FP_ROUND_NEAREST} : begin
        p2_nan_ready_next = p1_start;
        p2_nan_fault_next = 1'b0;
        p2_nan_sign_next = p1_sign;
        p2_nan_exp_next = {`FP_EXP_BITS{1'b1}};
        p2_nan_man_next = {`FP_MAN_BITS{1'b0}};
      end
      {1'b0, 1'b1, 1'b0, `FP_ROUND_ZERO},
      {1'b0, 1'b1, 1'b1, `FP_ROUND_ZERO},
      {1'b0, 1'b1, 1'b0, `FP_ROUND_MININF},
      {1'b0, 1'b1, 1'b0, `FP_ROUND_PLUSINF},
      {1'b0, 1'b1, 1'b1, `FP_ROUND_PLUSINF} : begin
        p2_nan_ready_next = p1_start;
        p2_nan_fault_next = 1'b0;
        p2_nan_sign_next = p1_sign;
        p2_nan_exp_next = ~`FP_EXP_BITS'b0 - 1;
        p2_nan_man_next = ~`FP_MAN_BITS'b0;
      end
      default: begin
        p2_nan_ready_next = p1_nan_ready;
        p2_nan_fault_next = p1_nan_fault;
        p2_nan_sign_next = p1_nan_sign;
        p2_nan_exp_next = p1_nan_exp;
        p2_nan_man_next = p1_nan_man;
      end
    endcase
  end

  /*assign p2_nan_ready_next = p1_nan_ready;
  assign p2_nan_fault_next = p1_nan_fault;
  assign p2_nan_sign_next = p1_nan_sign;
  assign p2_nan_exp_next = p1_nan_exp;
  assign p2_nan_man_next = p1_nan_man;*/

  wire [`FP_PREDEC_BITS-1:0]         p2_op_predec;
  wire [`FP_STATE_BITS-1:0]          p2_state;
  wire [2-1:0]                          p2_round;
  wire [`FP_EXP_BITS-1:0]            p2_exp;
  wire                               p2_rshift_man;
  wire                               p2_start;

  wire                               p2_sign;

  wire                               p2_nan_fault;
  wire                               p2_nan_ready;
  wire                               p2_nan_sign;
  wire [`FP_EXP_BITS-1:0]            p2_nan_exp;
  wire [`FP_MAN_BITS-1:0]            p2_nan_man;

  stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+2+`FP_EXP_BITS+1+1+1+1+`FP_EXP_BITS+`FP_MAN_BITS+1+`FP_MAN_BITS+(2*(`FP_MAN_BITS+2))))

  mult_p1_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({p1_op_predec, p1_state, p1_round, p1_exp, p1_rshift_man, p1_sign, p2_nan_fault_next, p2_nan_sign_next, p2_nan_exp_next, p2_nan_man_next, (p2_nan_ready_next&&p1_start),
                  y2_next, y1_next, y0_next})
   ,.dinValid (p1_start)
   ,.dinRetry (multp1_retry)

   ,.q        ({p2_op_predec, p2_state, p2_round, p2_exp, p2_rshift_man, p2_sign, p2_nan_fault, p2_nan_sign, p2_nan_exp, p2_nan_man, p2_nan_ready,
                  y2, y1, y0})
   ,.qValid   (p2_start)
   ,.qRetry   (multp2_retry)

   // Re-clocking signals
   ,.rci     (rco9)
   ,.rco     (rco10)
   );

  //-----------------------------------------------------
  // PHASE 2
  reg [(2*`FP_MAN_BITS)-1:0]       op_man_next;

  always_comb begin
    op_man_next = {y2, 64'b0} + {y1, 32'b0} + y0;
  end


  stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+2+`FP_EXP_BITS+1+1+1+1+`FP_EXP_BITS+`FP_MAN_BITS+1+`FP_MAN_BITS+1))

  mult_p2_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({p2_op_predec, p2_state, p2_round, p2_exp, p2_rshift_man, p2_sign, p2_nan_fault, p2_nan_sign, p2_nan_exp, p2_nan_man, (p2_nan_ready&&p2_start),
                  op_man_next[(2*`FP_MAN_BITS)-1:`FP_MAN_BITS-1]})
   ,.dinValid (p2_start)
   ,.dinRetry (multp2_retry)

   ,.q        ({multp_op_predec, multp_state, multp_round, multp_op_exp, multp_rshift_man, multp_op_sign, multp_nan_fault, multp_nan_sign, multp_nan_exp, multp_nan_man, multp_nan_ready,
                  multp_op_man})
   ,.qValid   (multp_op_ready)
   ,.qRetry   (res_retry_mult)

   // Re-clocking signals
   ,.rci     (rco10)
   ,.rco     (rco)
   );

endmodule
