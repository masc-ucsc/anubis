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

 module for floating point addition - 1 clock cycle latency
 for addition I plan on using the CLA-4 algorithm and implement a 32-bit adder
 using 4 CLA-4 adders.


 For single and dual precision use the same module
 The two mans to be added are the mantissa's:
    -man_1: the mantissa of the larger number
    -man_2: the mantissa of the smaller number
 If the numbers are single precision, the upper 29 bits of opereand_1 will be zero'd out
  and the hidden bit (1) is placed in position 23.

 NOTE: man_2 is assumed to already have the hidden bit and will not be touched
 If the numbers are op_type precision, the hidden bit will be placed in postion 52 of man_1
  and man_2 will be left untouched.

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//import scoore_fpu::*;
//import retry_common::*;

module fp_add64
  (input                             clk,
   input                             reset,
   // inputs
   input                             enable_add,
   input                             sub_rnd_flag,
   input [6-1:0]                     denorm_exp1_m_exp2,

   input [`FP_PREDEC_BITS-1:0]       op_predec_add,
   input [2-1:0]                   round_add,
   input [`FP_STATE_BITS-1:0]        state_add,
   input [`FP_EXP_BITS-1:0]          exp_add,
   input                             sign1_add,
   input [`FP_MAN_BITS-1:0]          man1_add,
   input                             sign2_add,
   input [`FP_MAN_BITS-1:0]          man2_add,

   input                             res_retry_add,


   // outputs
   output                            addp_ready,
   output [`FP_PREDEC_BITS-1:0]      addp_op_predec,
   output [`FP_STATE_BITS-1:0]       addp_state,
   output [2-1:0]                  addp_round,
   output                            sub_rnd_flag_ready,
   output                            addp_sign,
   output [`FP_EXP_BITS-1:0]         addp_exp,
   output [`FP_MAN_BITS-1:0]         addp_man,

   output                            retry_add,

   //Retry Re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco
  );

  //-------------------------------------------------------------
  // PHASE 0
  //-------------------------------------------------------------

  reg [`FP_MAN_BITS-1:0]           rshifted_man2_next;
  always_comb begin
    rshifted_man2_next    = (man2_add>>denorm_exp1_m_exp2);
  end

  wire                             sign1_p1;
  wire                             sign2_p1;
  wire [`FP_EXP_BITS-1:0]          exp_p1;
  wire [6-1:0]                     denorm_exp1_m_exp2_p1;
  wire [`FP_MAN_BITS-1:0]          man1_p1;
  wire [`FP_MAN_BITS-1:0]          man2_p1;
  wire [`FP_MAN_BITS-1:0]          rshifted_man2_p1;
  wire [`FP_PREDEC_BITS-1:0]       op_predec_p1;
  wire [`FP_STATE_BITS-1:0]        state_p1;
  wire [2-1:0]                        round_p1;

  wire                             enable_add_p1;

  wire                             sub_rnd_flag_p1;
  wire                             sub_rnd_flag_p2;

  logic add_p1_retry;

  wire [4:0] rco5;
  wire [4:0] rco6;

  stage #(.Size(1+1+(`FP_EXP_BITS+6)+(3*`FP_MAN_BITS)
             +`FP_PREDEC_BITS+2
               +`FP_STATE_BITS+1))  add_p_s

   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sign1_add, sign2_add, exp_add, denorm_exp1_m_exp2, man1_add,
                     man2_add, rshifted_man2_next, op_predec_add, round_add, state_add, sub_rnd_flag})
   ,.dinValid (enable_add)
   ,.dinRetry (retry_add)

   ,.q        ({sign1_p1, sign2_p1, exp_p1, denorm_exp1_m_exp2_p1, man1_p1,
                     man2_p1 , rshifted_man2_p1, op_predec_p1, round_p1, state_p1, sub_rnd_flag_p1})
   ,.qValid   (enable_add_p1)
   ,.qRetry   (add_p1_retry)

   // Re-clocking signals
   ,.rci      (rci)
   ,.rco      (rco5)
   );

  //-------------------------------------------------------------
  // PHASE 1
  //-------------------------------------------------------------
  //++++++++++++++++++ Task1: Shift man2 as much as necessary

  reg [`FP_MAN_BITS-1:0]           adjusted_man2_next;
  always_comb begin
`ifndef ANUBIS_LOCAL_22
    if (denorm_exp1_m_exp2_p1 == 63) begin
`else
    if (denorm_exp1_m_exp2_p1 != 63) begin
`endif
      adjusted_man2_next = |man2_p1;
    end else begin
      adjusted_man2_next    = rshifted_man2_p1;
      adjusted_man2_next[0] = ((adjusted_man2_next<<denorm_exp1_m_exp2_p1) != man2_p1);
    end
  end

  reg                                man1_gt_t0;
  reg                                man1_eq_t0;
  reg                                man1_gt_t1;
  always_comb begin
    man1_gt_t0 = man1_p1[`FP_MAN_BITS-1:`FP_MAN_BITS/2] >  man2_p1[`FP_MAN_BITS-1:`FP_MAN_BITS/2];
    man1_eq_t0 = man1_p1[`FP_MAN_BITS-1:`FP_MAN_BITS/2] == man2_p1[`FP_MAN_BITS-1:`FP_MAN_BITS/2];
    man1_gt_t1 = man1_p1[`FP_MAN_BITS/2-1:0] >  man2_p1[`FP_MAN_BITS/2-1:0];
  end

  reg                                man1_bigger_next;
  always_comb begin
    man1_bigger_next = (denorm_exp1_m_exp2_p1 != 0) | // exp1 > exp2
                       man1_gt_t0 |                              // upper man1 > man2
                       (man1_eq_t0 & man1_gt_t1);                // upper equal, lower man1 > man2
  end



  //++++++++++++++++++ Task2: Add/Sub to man1 for rounding operations
  reg [`FP_MAN_BITS-1:0]           adjusted_man1_next;
  always_comb begin
    adjusted_man1_next = man1_p1;
  end

  //++++++++++++++++++ Task3: Latch values for next stage

  wire [`FP_MAN_BITS-1:0]          adjusted_man1;
  wire [`FP_MAN_BITS-1:0]          adjusted_man2;
  wire                             man1_bigger;

  wire                             sign1_p2;
  wire                             sign2_p2;
  wire [`FP_EXP_BITS-1:0]          exp_p2;
  wire                             enable_addp_p2;
  wire [`FP_PREDEC_BITS-1:0]       op_predec_p2;
  wire [`FP_STATE_BITS-1:0]        state_p2;
  wire [2-1:0]                         round_p2;

  logic add_p2_retry;

  stage #(.Size(1+1+`FP_EXP_BITS+1
                 +`FP_MAN_BITS+`FP_MAN_BITS
                 +`FP_PREDEC_BITS+`FP_STATE_BITS+2+1
                 )) add_p1_s

   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({sign1_p1 , sign2_p1, exp_p1, man1_bigger_next,
                  adjusted_man1_next, adjusted_man2_next,
                    op_predec_p1, state_p1, round_p1, sub_rnd_flag_p1})
   ,.dinValid (enable_add_p1)
   ,.dinRetry (add_p1_retry)

   ,.q        ({sign1_p2 , sign2_p2, exp_p2, man1_bigger,
                  adjusted_man1, adjusted_man2,
                    op_predec_p2, state_p2, round_p2, sub_rnd_flag_p2})
   ,.qValid   (enable_addp_p2)
   ,.qRetry   (add_p2_retry)

   // Re-clocking signals
   ,.rci     (rco5)
   ,.rco     (rco6)
   );

  //-------------------------------------------------------------
  // PHASE 2
  //-------------------------------------------------------------
  reg [`FP_MAN_BITS-1:0]           addp_man_next;
  reg [`FP_EXP_BITS-1:0]           addp_exp_next;
  reg                              addp_sign_next;

  always_comb begin : addp_stage

    reg [`FP_MAN_BITS+1-1:0]           tmp;
    casez({sign1_p2, sign2_p2, man1_bigger})
      // Same signed does not matter
      3'b00?: begin tmp = adjusted_man1+adjusted_man2; addp_sign_next = 1'b0; end
      3'b11?: begin tmp = adjusted_man1+adjusted_man2; addp_sign_next = 1'b1; end

      // man1 > man2
      3'b101: begin tmp = adjusted_man1-adjusted_man2; addp_sign_next = 1'b1; end
      3'b011: begin tmp = adjusted_man1-adjusted_man2; addp_sign_next = 1'b0; end

      // man2 > man1
      3'b010: begin tmp = adjusted_man2-adjusted_man1; addp_sign_next = 1'b1; end
      3'b100: begin tmp = adjusted_man2-adjusted_man1; addp_sign_next = 1'b0; end
    endcase

    if (adjusted_man1 == adjusted_man2 && sign1_p2 != sign2_p2)
      addp_sign_next = 1'b0;

    if (tmp[`FP_MAN_BITS]) begin
      addp_exp_next = exp_p2+1;
      addp_man_next = tmp[`FP_MAN_BITS:1];
    end else begin
      addp_man_next = tmp[`FP_MAN_BITS-1:0];

      addp_exp_next = exp_p2;


      if(tmp[`FP_MAN_BITS:1]==0)begin
      addp_exp_next = 0;
    end else begin
      addp_exp_next = exp_p2;
      end
    end
  end

  stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+2+
                 1+`FP_EXP_BITS+`FP_MAN_BITS+1
                 )) add_p2_s

   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({op_predec_p2, state_p2, round_p2,
                     addp_sign_next, addp_exp_next, addp_man_next, sub_rnd_flag_p2})
   ,.dinValid (enable_addp_p2)
   ,.dinRetry (add_p2_retry)

   ,.q        ({addp_op_predec, addp_state, addp_round,
                     addp_sign, addp_exp, addp_man, sub_rnd_flag_ready})
   ,.qValid   (addp_ready)
   ,.qRetry   (res_retry_add)

   // Re-clocking signals
   ,.rci      (rco6)
   ,.rco      (rco)
   );
endmodule

