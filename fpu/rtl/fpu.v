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

module fpu
  (input                             clk,
   input                             reset,

   input                             start,  // Valid

   input [`FP_STATE_BITS-1:0]        state,  // lreg dest
   input [2-1:0]                   round,
   input [`OP_TYPE_BITS-1:0]         op,
   input [`FP_TYPE_D_BITS-1:0]       src1,
   input [`FP_TYPE_D_BITS-1:0]       src2,

   output [`FP_TYPE_D_BITS-1:0]      fpu_result,
   output [`FP_STATE_BITS-1:0]       fpu_state,
   output                            fpu_ready,  // valid out to scoreboard
   output [3-1:0]                    fpu_icc,

   output                            out_retry,  // retry and busy
   input                             in_retry,   // retry from scoreboard

   //Retry Re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco
  );

   wire [4:0]                rco1;
   wire [4:0]                rco2;

  //-----------------------------------------------------------------------
  // Stage 1: align/denormalize
  //-----------------------------------------------------------------------
  wire [`FP_PREDEC_BITS-1:0]         denorm_op_predec_add;
  wire                               denorm_sign1_add;
  wire [`FP_MAN_BITS-1:0]            denorm_man1_add;
  wire                               denorm_sign2_add;
  wire [`FP_MAN_BITS-1:0]            denorm_man2_add;
  wire [`FP_EXP_BITS-1:0]            denorm_exp_add;
  //wire [`FP_EXP_BITS-1:0]            denorm_exp1_add;
  //wire [`FP_EXP_BITS-1:0]            denorm_exp2_add;
  wire [2-1:0]                          denorm_round_add;
  wire [`FP_STATE_BITS-1:0]          denorm_state_add;

  wire [`FP_PREDEC_BITS-1:0]         denorm_op_predec_mult;
  wire                               denorm_sign1_mult;
  wire [`FP_MAN_BITS-1:0]            denorm_man1_mult;
  wire                               denorm_sign2_mult;
  wire [`FP_MAN_BITS-1:0]            denorm_man2_mult;
  wire [`FP_EXP_BITS-1:0]            denorm_exp_mult;
  wire [`FP_EXP_BITS-1:0]            denorm_exp1_mult;
  wire [`FP_EXP_BITS-1:0]            denorm_exp2_mult;
  wire [2-1:0]                          denorm_round_mult;
  wire [`FP_STATE_BITS-1:0]          denorm_state_mult;

  wire [`FP_PREDEC_BITS-1:0]         denorm_op_predec_div;
  wire                               denorm_sign1_div;
  wire [`FP_MAN_BITS-1:0]            denorm_man1_div;
  wire                               denorm_sign2_div;
  wire [`FP_MAN_BITS-1:0]            denorm_man2_div;
  wire [`FP_EXP_BITS-1:0]            denorm_exp_div;
  wire [`FP_EXP_BITS-1:0]            denorm_exp1_div;
  wire [`FP_EXP_BITS-1:0]            denorm_exp2_div;
  wire [2-1:0]                          denorm_round_div;
  wire [`FP_STATE_BITS-1:0]          denorm_state_div;

  wire [`FP_PREDEC_BITS-1:0]         denorm_op_predec_sqrt;
  wire [2-1:0]                          denorm_round_sqrt;
  wire                               denorm_sign2_sqrt;
  wire [`FP_MAN_BITS-1:0]            denorm_man2_sqrt;
  wire [`FP_EXP_BITS-1:0]            denorm_exp_sqrt;
  wire [`FP_STATE_BITS-1:0]          denorm_state_sqrt;
  wire                               denorm_sqrt_shift;

  wire [`FP_PREDEC_BITS-1:0]         denorm_op_predec_misc;
  wire                               denorm_sign1_misc;
  wire [`FP_MAN_BITS-1:0]            denorm_man1_misc;
  wire                               denorm_sign2_misc;
  wire [`FP_MAN_BITS-1:0]            denorm_man2_misc;
  wire [`FP_EXP_BITS-1:0]            denorm_exp_misc;
  wire [`FP_EXP_BITS-1:0]            denorm_exp1_misc;
  wire [`FP_EXP_BITS-1:0]            denorm_exp2_misc;
  wire [2-1:0]                          denorm_round_misc;
  wire [`FP_STATE_BITS-1:0]          denorm_state_misc;

  wire                               denorm_enable_add;
  wire                               denorm_enable_mult;
  wire                               denorm_enable_div;
  wire                               denorm_enable_misc;
  wire                               denorm_enable_sqrt;

  wire [6-1:0]                       denorm_exp1_m_exp2_add;
  //wire                               denorm_rshift_man;

  wire                               denorm_overflow_mult;
  wire                               denorm_overflow_div;

  wire                               denorm_sub_rnd_flag;

  wire                               ex_retry;
  wire                               retry_add;
  wire                               retry_mult;
  wire                               retry_div;
  wire                               retry_sqrt;
  wire                               retry_misc;

  logic denorm_rshift_man_div;
  logic denorm_rshift_man_mult;

  //retries passed to busy gen
//  wire                               denorm_retry_add;
//  wire                               denorm_retry_mult;
//  wire                               denorm_retry_div;
//  wire                               denorm_retry_sqrt;

  fp_denorm stage1
    (.clk                     (clk),
     .reset                   (reset),

     //inputs

     .start                   (start),
     .state                   (state),
     .op                      (op),
     .round                   (round),
     .src1                    (src1),
     .src2                    (src2),

     .retry_add               (retry_add),
     .retry_mult              (retry_mult),
     .retry_div               (retry_div),
     .retry_sqrt              (retry_sqrt),
     .retry_misc              (retry_misc),

     //outputs

     .denorm_op_predec_add    (denorm_op_predec_add),
     .denorm_round_add        (denorm_round_add),
     .denorm_state_add        (denorm_state_add),
     .denorm_exp_add          (denorm_exp_add),
     .denorm_sign1_add        (denorm_sign1_add),
     .denorm_man1_add         (denorm_man1_add),
     .denorm_sign2_add        (denorm_sign2_add),
     .denorm_man2_add         (denorm_man2_add),

     .denorm_op_predec_mult   (denorm_op_predec_mult),
     .denorm_round_mult       (denorm_round_mult),
     .denorm_state_mult       (denorm_state_mult),
     .denorm_exp_mult         (denorm_exp_mult),
     .denorm_exp1_mult        (denorm_exp1_mult),
     .denorm_sign1_mult       (denorm_sign1_mult),
     .denorm_man1_mult        (denorm_man1_mult),
     .denorm_exp2_mult        (denorm_exp2_mult),
     .denorm_sign2_mult       (denorm_sign2_mult),
     .denorm_man2_mult        (denorm_man2_mult),

     .denorm_op_predec_div    (denorm_op_predec_div),
     .denorm_round_div        (denorm_round_div),
     .denorm_state_div        (denorm_state_div),
     .denorm_exp_div          (denorm_exp_div),
     .denorm_exp1_div         (denorm_exp1_div),
     .denorm_sign1_div        (denorm_sign1_div),
     .denorm_man1_div         (denorm_man1_div),
     .denorm_exp2_div         (denorm_exp2_div),
     .denorm_sign2_div        (denorm_sign2_div),
     .denorm_man2_div         (denorm_man2_div),

     .denorm_op_predec_sqrt   (denorm_op_predec_sqrt),
     .denorm_round_sqrt       (denorm_round_sqrt),
     .denorm_sign2_sqrt       (denorm_sign2_sqrt),
     .denorm_man2_sqrt        (denorm_man2_sqrt),
     .denorm_exp_sqrt         (denorm_exp_sqrt),
     .denorm_state_sqrt       (denorm_state_sqrt),

     .denorm_op_predec_misc   (denorm_op_predec_misc),
     .denorm_round_misc       (denorm_round_misc),
     .denorm_state_misc       (denorm_state_misc),
     .denorm_exp_misc         (denorm_exp_misc),
     .denorm_exp1_misc        (denorm_exp1_misc),
     .denorm_sign1_misc       (denorm_sign1_misc),
     .denorm_man1_misc        (denorm_man1_misc),
     .denorm_exp2_misc        (denorm_exp2_misc),
     .denorm_sign2_misc       (denorm_sign2_misc),
     .denorm_man2_misc        (denorm_man2_misc),

     .denorm_enable_add       (denorm_enable_add),
     .denorm_sub_rnd_flag     (denorm_sub_rnd_flag),
     .denorm_enable_mult      (denorm_enable_mult),
     .denorm_enable_div       (denorm_enable_div),
     .denorm_enable_misc      (denorm_enable_misc),
     .denorm_enable_sqrt      (denorm_enable_sqrt),

     .denorm_overflow_mult    (denorm_overflow_mult),
     .denorm_overflow_div     (denorm_overflow_div),
     .denorm_exp1_m_exp2_add  (denorm_exp1_m_exp2_add),
     .denorm_rshift_man_div   (denorm_rshift_man_div),
     .denorm_rshift_man_mult  (denorm_rshift_man_mult),
     .denorm_sqrt_shift       (denorm_sqrt_shift),

//     .out_retry_add           (denorm_retry_add),
//     .out_retry_mult          (denorm_retry_mult),
//     .out_retry_div           (denorm_retry_div),
//     .out_retry_sqrt          (denorm_retry_sqrt)
     .out_retry               (out_retry),

      // Re-clocking signals
     .rci                     (rci),
     .rco                     (rco1)
     );

  //-----------------------------------------------------------------------
  // Stage 2: add or mult or... (execute stages)
  //-----------------------------------------------------------------------
  wire [`FP_PREDEC_BITS-1:0]   ex_op_predec;
  wire [2-1:0]                    ex_round;
  wire                         ex_sign;

  wire [`FP_MAN_BITS-1:0]      ex_man;
  wire [`FP_EXP_BITS-1:0]      ex_exp;
  wire [`FP_STATE_BITS-1:0]    ex_state;
  wire                         ex_start;
  //wire                         ex_sub_en;

  wire                         ex_sub_rnd_flag;
  wire                         man_zero;

  fp_ex stage2
    (.clk                   (clk),
     .reset                 (reset),

     // inputs
     .enable_add            (denorm_enable_add),
     .sub_rnd_flag          (denorm_sub_rnd_flag),
     .enable_mult           (denorm_enable_mult),
     .enable_div            (denorm_enable_div),
     .enable_misc           (denorm_enable_misc),
     .enable_sqrt           (denorm_enable_sqrt),

     .op_predec_add         (denorm_op_predec_add),
     .round_add             (denorm_round_add),
     .state_add             (denorm_state_add),
     .exp_add               (denorm_exp_add),
     .sign1_add             (denorm_sign1_add),
     .man1_add              (denorm_man1_add),
     .sign2_add             (denorm_sign2_add),
     .man2_add              (denorm_man2_add),

     .op_predec_mult        (denorm_op_predec_mult),
     .round_mult            (denorm_round_mult),
     .state_mult            (denorm_state_mult),
     .exp_mult              (denorm_exp_mult),
     .exp1_mult             (denorm_exp1_mult),
     .sign1_mult            (denorm_sign1_mult),
     .man1_mult             (denorm_man1_mult),
     .exp2_mult             (denorm_exp2_mult),
     .sign2_mult            (denorm_sign2_mult),
     .man2_mult             (denorm_man2_mult),

     .op_predec_div         (denorm_op_predec_div),
     .round_div             (denorm_round_div),
     .state_div             (denorm_state_div),
     .exp_div               (denorm_exp_div),
     .exp1_div              (denorm_exp1_div),
     .sign1_div             (denorm_sign1_div),
     .man1_div              (denorm_man1_div),
     .exp2_div              (denorm_exp2_div),
     .sign2_div             (denorm_sign2_div),
     .man2_div              (denorm_man2_div),

     .op_predec_sqrt        (denorm_op_predec_sqrt),
     .round_sqrt            (denorm_round_sqrt),
     .sign2_sqrt            (denorm_sign2_sqrt),
     .man2_sqrt             (denorm_man2_sqrt),
     .exp_sqrt              (denorm_exp_sqrt),
     .state_sqrt            (denorm_state_sqrt),
     .sqrt_shift            (denorm_sqrt_shift),

     .op_predec_misc        (denorm_op_predec_misc),
     .round_misc            (denorm_round_misc),
     .state_misc            (denorm_state_misc),
     .exp_misc              (denorm_exp_misc),
     .exp1_misc             (denorm_exp1_misc),
     .sign1_misc            (denorm_sign1_misc),
     .man1_misc             (denorm_man1_misc),
     .exp2_misc             (denorm_exp2_misc),
     .sign2_misc            (denorm_sign2_misc),
     .man2_misc             (denorm_man2_misc),

     .overflow_mult         (denorm_overflow_mult),
     .overflow_div          (denorm_overflow_div),

     .denorm_exp1_m_exp2    (denorm_exp1_m_exp2_add),
     .rshift_man_div        (denorm_rshift_man_div),
     .rshift_man_mult       (denorm_rshift_man_mult),

     .ex_retry              (ex_retry),


     // outputs
     .ex_op_predec          (ex_op_predec),
     .ex_round              (ex_round),
     .ex_sub_rnd_flag       (ex_sub_rnd_flag),
     .ex_sign               (ex_sign),
     .ex_exp                (ex_exp),
     .ex_man                (ex_man),

     .ex_state              (ex_state),
     .ex_start              (ex_start),

     .ex_man_zero           (man_zero),

     .retry_add             (retry_add),
     .retry_mult            (retry_mult),
     .retry_div             (retry_div),
     .retry_sqrt            (retry_sqrt),
     .retry_misc            (retry_misc),

      // Retry re-clocking signals
     .rci                   (rco1),
     .rco                   (rco2)
   );

//  fp_busy_generator_comb fpu_busy_gen (.*);

  //-----------------------------------------------------------------------
  // Stage 3: normalize
  //-----------------------------------------------------------------------

  fp_normalize stage3
    (.clk         (clk),
     .reset       (reset),

     //inputs
     .start       (ex_start),
     .state       (ex_state),

     .op_predec   (ex_op_predec),
     .sub_rnd_flag(ex_sub_rnd_flag),
     .round       (ex_round),

     .sign        (ex_sign),
     .exp         (ex_exp),
     .man         (ex_man),
     .man_zero    (man_zero),
     .norm_retry  (in_retry),


     // outputs
     .norm_result (fpu_result),
     .norm_state  (fpu_state),
     .norm_ready  (fpu_ready),
     .norm_icc    (fpu_icc),
     .ex_retry    (ex_retry),

      // Re-clocking signals
     .rci         (rco2),
     .rco         (rco)
   );

endmodule
