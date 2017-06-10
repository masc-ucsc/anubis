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

module fp_ex
  (input                               clk,
   input                               reset,

   // inputs
   input                               enable_add,
   input                               sub_rnd_flag,
   input                               enable_mult,
   input                               enable_div,
   input                               enable_misc,
   input                               enable_sqrt,

   input [`FP_PREDEC_BITS-1:0]         op_predec_add,
   input [2-1:0]                     round_add,
   input [`FP_STATE_BITS-1:0]          state_add,
   input [`FP_EXP_BITS-1:0]            exp_add,
   input                               sign1_add,
   input [`FP_MAN_BITS-1:0]            man1_add,
   input                               sign2_add,
   input [`FP_MAN_BITS-1:0]            man2_add,

   input [`FP_PREDEC_BITS-1:0]         op_predec_mult,
   input [2-1:0]                     round_mult,
   input [`FP_STATE_BITS-1:0]          state_mult,
   input [`FP_EXP_BITS-1:0]            exp_mult,
   input [`FP_EXP_BITS-1:0]            exp1_mult,
   input                               sign1_mult,
   input [`FP_MAN_BITS-1:0]            man1_mult,
   input [`FP_EXP_BITS-1:0]            exp2_mult,
   input                               sign2_mult,
   input [`FP_MAN_BITS-1:0]            man2_mult,

   input [`FP_PREDEC_BITS-1:0]         op_predec_div,
   input [2-1:0]                     round_div,
   input [`FP_STATE_BITS-1:0]          state_div,
   input [`FP_EXP_BITS-1:0]            exp_div,
   input [`FP_EXP_BITS-1:0]            exp1_div,
   input                               sign1_div,
   input [`FP_MAN_BITS-1:0]            man1_div,
   input [`FP_EXP_BITS-1:0]            exp2_div,
   input                               sign2_div,
   input [`FP_MAN_BITS-1:0]            man2_div,

   input [`FP_PREDEC_BITS-1:0]         op_predec_sqrt,
   input [2-1:0]                     round_sqrt,
   input                               sign2_sqrt,
   input [`FP_MAN_BITS-1:0]            man2_sqrt,
   input [`FP_EXP_BITS-1:0]            exp_sqrt,
   input [`FP_STATE_BITS-1:0]          state_sqrt,
   input                               sqrt_shift,

   input [`FP_PREDEC_BITS-1:0]         op_predec_misc,
   input [2-1:0]                     round_misc,
   input [`FP_STATE_BITS-1:0]          state_misc,
   input [`FP_EXP_BITS-1:0]            exp_misc,
   input [`FP_EXP_BITS-1:0]            exp1_misc,
   input                               sign1_misc,
   input [`FP_MAN_BITS-1:0]            man1_misc,
   input [`FP_EXP_BITS-1:0]            exp2_misc,
   input                               sign2_misc,
   input [`FP_MAN_BITS-1:0]            man2_misc,

   input                               overflow_mult,
   input                               rshift_man_mult,
   input                               overflow_div,
   input                               rshift_man_div,
   input [6-1:0]                       denorm_exp1_m_exp2,

   input                               ex_retry,


   // outputs
   output [`FP_PREDEC_BITS-1:0]        ex_op_predec,
   output [2-1:0]                    ex_round,
   output                              ex_sub_rnd_flag,
   output                              ex_sign,
   output [`FP_EXP_BITS-1:0]           ex_exp,
   output [`FP_MAN_BITS-1:0]           ex_man,

   output [`FP_STATE_BITS-1:0]         ex_state,
   output                              ex_start,
   output                              ex_man_zero,

   output                              retry_add,
   output                              retry_mult,
   output                              retry_div,
   output                              retry_sqrt,
   output                              retry_misc,

   //Retry Re-clocking IO signals
   input  [4:0]                  rci,
   output  [4:0]                 rco
  );
  wire [4:0]                  rco1;
  wire [4:0]                  rco2;
  wire [4:0]                  rco3;
  wire [4:0]                  rco4;
  wire [4:0]                  rco5;

  // compsl default clock = (posedge clk);

  // compsl assert always enable_add  -> (                !enable_mult && !enable_misc);
  // compsl assert always enable_mult -> (!enable_add                  && !enable_misc);
  // compsl assert always enable_misc -> (!enable_add  && !enable_mult);

  wire                                  addp_ready;
  wire [`FP_PREDEC_BITS-1:0]            addp_op_predec;
  wire [`FP_STATE_BITS-1:0]             addp_state;
  wire                                  sub_rnd_flag_ready;
  wire [2-1:0]                             addp_round;

  wire                                  addp_sign;
  wire [`FP_EXP_BITS-1:0]               addp_exp;
  wire [`FP_MAN_BITS-1:0]               addp_man;

  wire                                  add_ready;
  wire [`FP_PREDEC_BITS-1:0]            add_op_predec;
  wire [`FP_STATE_BITS-1:0]             add_state;
  wire [2-1:0]                             add_round;

  wire                                  add_sign;
  wire [`FP_EXP_BITS-1:0]               add_exp;
  wire [`FP_MAN_BITS-1:0]               add_man;

  wire [`FP_PREDEC_BITS-1:0]            multp_op_predec;
  wire [`FP_STATE_BITS-1:0]             multp_state;
  wire [2-1:0]                             multp_round;

  wire                                  multp_nan_ready;
  wire                                  multp_nan_fault; // FIXME: notify faults
  wire                                  multp_nan_sign;
  wire [`FP_EXP_BITS-1:0]               multp_nan_exp;
  wire [`FP_MAN_BITS-1:0]               multp_nan_man;

  wire                                  multp_op_ready;
  wire                                  multp_op_fault; // FIXME: notify faults
  wire                                  multp_op_sign;
  wire [`FP_EXP_BITS-1:0]               multp_op_exp;
  wire [`FP_MAN_BITS+1-1:0]             multp_op_man;

  wire                                  multp_rshift_man;

  wire                                  mult_ready;
  wire                                  mult_fault; // FIXME: notify faults
  wire [`FP_PREDEC_BITS-1:0]            mult_op_predec;
  wire [`FP_STATE_BITS-1:0]             mult_state;
  wire [2-1:0]                             mult_round;

  wire                                  mult_sign;
  wire [`FP_EXP_BITS-1:0]               mult_exp;
  wire [`FP_MAN_BITS-1:0]               mult_man;

  wire [`FP_PREDEC_BITS-1:0]            divp_op_predec;
  wire [`FP_STATE_BITS-1:0]             divp_state;
  wire [2-1:0]                             divp_round;

  wire [`FP_PREDEC_BITS-1:0]            divp_nan_op_predec;
  wire [`FP_STATE_BITS-1:0]             divp_nan_state;
  wire [2-1:0]                             divp_nan_round;

  wire                                  divp_nan_ready;
  wire                                  divp_nan_fault;
  wire                                  divp_nan_sign;
  wire [`FP_EXP_BITS-1:0]               divp_nan_exp;
  wire [`FP_MAN_BITS-1:0]               divp_nan_man;

  wire                                  divp_op_ready;
  wire                                  divp_op_fault;
  wire                                  divp_op_sign;
  wire [`FP_EXP_BITS-1:0]               divp_op_exp;
  wire [`FP_MAN_BITS+1-1:0]             divp_op_man;

  wire                                  divp_rshift_man;

  wire                                  div_ready;
  wire                                  div_fault; // FIXME: notify faults
  wire [`FP_PREDEC_BITS-1:0]            div_op_predec;
  wire [`FP_STATE_BITS-1:0]             div_state;
  wire [2-1:0]                             div_round;

  wire                                  div_sign;
  wire [`FP_EXP_BITS-1:0]               div_exp;
  wire [`FP_MAN_BITS-1:0]               div_man;



  wire                                  sqrtp_sign;
  wire [`FP_EXP_BITS-1:0]               sqrtp_exp;
  wire [`FP_MAN_BITS-1:0]               sqrtp_man;
  wire                                  sqrtp_ready;
  wire [`FP_PREDEC_BITS-1:0]            sqrtp_op_predec;
  wire [`FP_STATE_BITS-1:0]             sqrtp_state;
  wire [2-1:0]                             sqrtp_round;

  wire                                  sqrt_ready;
  wire [`FP_PREDEC_BITS-1:0]            sqrt_op_predec;
  wire [`FP_STATE_BITS-1:0]             sqrt_state;
  wire [2-1:0]                             sqrt_round;

  wire                                  sqrt_sign;
  wire [`FP_EXP_BITS-1:0]               sqrt_exp;
  wire [`FP_MAN_BITS-1:0]               sqrt_man;

  //retries from res queue to units
  wire                                  res_retry_add;
  wire                                  res_retry_mult;
  wire                                  res_retry_sqrt;
  wire                                  res_retry_div;

  fp_add64 addp (.rci(rci),.rco(rco1),
  .clk               (clk),
  .reset             (reset),
`ifndef ANUBIS_LOCAL_19
  .enable_add        (enable_add),
`else
    .enable_add(~enable_add),
`endif
  .sub_rnd_flag      (sub_rnd_flag),
  .denorm_exp1_m_exp2(denorm_exp1_m_exp2),
  .op_predec_add     (op_predec_add),
  .round_add         (round_add),
  .state_add         (state_add),
  .exp_add           (exp_add),
  .sign1_add         (sign1_add),
  .man1_add          (man1_add),
  .sign2_add         (sign2_add),
  .man2_add          (man2_add),
  .res_retry_add     (res_retry_add),
  .addp_ready        (addp_ready),
  .addp_op_predec    (addp_op_predec),
  .addp_state        (addp_state),
  .addp_round        (addp_round),
  .sub_rnd_flag_ready(sub_rnd_flag_ready),
  .addp_sign         (addp_sign),
  .addp_exp          (addp_exp),
  .addp_man          (addp_man),
  .retry_add         (retry_add)
  );


  fp_add64_post_comb add (
   .addp_ready    (addp_ready),
   .addp_op_predec(addp_op_predec),
   .addp_state    (addp_state),
   .addp_round    (addp_round),
   .addp_sign     (addp_sign),
   .addp_exp      (addp_exp),
   .addp_man      (addp_man),
   .add_ready     (add_ready),
   .add_op_predec (add_op_predec),
   .add_state     (add_state),
   .add_round     (add_round),
   .add_sign      (add_sign),
   .add_exp       (add_exp),
   .add_man        (add_man));

  fp_mult64 multp (.rci(rco1),.rco(rco2),
 .clk             (clk),
 .reset           (reset),
 .enable_mult     (enable_mult),
 .op_predec_mult  (op_predec_mult),
 .round_mult      (round_mult),
 .state_mult      (state_mult),
 .exp_mult        (exp_mult),
 .exp1_mult       (exp1_mult),
 .sign1_mult      (sign1_mult),
 .man1_mult       (man1_mult),
 .exp2_mult       (exp2_mult),
 .sign2_mult      (sign2_mult),
 .man2_mult       (man2_mult),
 .rshift_man_mult (rshift_man_mult),
 .overflow_mult   (overflow_mult),
`ifndef ANUBIS_LOCAL_20
 .res_retry_mult  (res_retry_mult),
`else
 .res_retry_mult(~res_retry_mult),
`endif
 .multp_op_predec (multp_op_predec),
 .multp_state     (multp_state),
 .multp_round     (multp_round),
 .multp_nan_ready (multp_nan_ready),
 .multp_nan_fault (multp_nan_fault),
 .multp_nan_sign  (multp_nan_sign),
 .multp_nan_exp   (multp_nan_exp),
 .multp_nan_man   (multp_nan_man),
 .multp_op_ready  (multp_op_ready),
 .multp_op_fault  (multp_op_fault),
 .multp_op_sign   (multp_op_sign),
 .multp_op_exp    (multp_op_exp),
 .multp_op_man    (multp_op_man),
 .multp_rshift_man(multp_rshift_man),
 .retry_mult      (retry_mult));


  fp_mult64_post_comb mult (
 .multp_op_predec (multp_op_predec),
 .multp_state     (multp_state),
 .multp_round     (multp_round),
 .multp_nan_ready (multp_nan_ready),
 .multp_nan_fault (multp_nan_fault),
 .multp_nan_sign  (multp_nan_sign),
 .multp_nan_exp   (multp_nan_exp),
 .multp_nan_man   (multp_nan_man),
 .multp_op_ready  (multp_op_ready),
 .multp_op_fault  (multp_op_fault),
 .multp_op_sign   (multp_op_sign),
 .multp_op_exp    (multp_op_exp),
 .multp_op_man    (multp_op_man),
 .multp_rshift_man(multp_rshift_man),
 .mult_ready      (mult_ready),
 .mult_fault      (mult_fault),
 .mult_op_predec  (mult_op_predec),
 .mult_state      (mult_state),
 .mult_round      (mult_round),
 .mult_sign       (mult_sign),
 .mult_exp        (mult_exp),
 .mult_man         (mult_man));

  fp_div64 divp(.rci(rco2),.rco(rco3),
 .clk               (clk),
 .enable_div        (enable_div),
 .reset             (reset),
 .op_predec_div     (op_predec_div),
 .round_div         (round_div),
 .state_div         (state_div),
 .exp_div           (exp_div),
 .exp1_div          (exp1_div),
 .sign1_div         (sign1_div),
 .man1_div          (man1_div),
 .exp2_div          (exp2_div),
 .sign2_div         (sign2_div),
 .man2_div          (man2_div),
 .rshift_man_div    (rshift_man_div),
 .overflow_div      (overflow_div),
 .res_retry_div     (res_retry_div),
 .divp_op_predec    (divp_op_predec),
 .divp_state        (divp_state),
 .divp_round        (divp_round),
 .divp_nan_op_predec(divp_nan_op_predec),
 .divp_nan_state    (divp_nan_state),
 .divp_nan_round    (divp_nan_round),
 .divp_nan_ready    (divp_nan_ready),
 .divp_nan_fault    (divp_nan_fault),
 .divp_nan_sign     (divp_nan_sign),
 .divp_nan_exp      (divp_nan_exp),
 .divp_nan_man      (divp_nan_man),
 .divp_op_ready     (divp_op_ready),
 .divp_op_fault     (divp_op_fault),
 .divp_op_sign      (divp_op_sign),
 .divp_op_exp       (divp_op_exp),
 .divp_op_man       (divp_op_man),
 .divp_rshift_man   (divp_rshift_man),
 .retry_div         (retry_div));

  fp_div64_post_comb div(
   .divp_op_predec    (divp_op_predec),
   .divp_state        (divp_state),
   .divp_round        (divp_round),
   .divp_nan_op_predec(divp_nan_op_predec),
   .divp_nan_state    (divp_nan_state),
   .divp_nan_round    (divp_nan_round),
   .divp_nan_ready    (divp_nan_ready),
   .divp_nan_fault    (divp_nan_fault),
   .divp_nan_sign     (divp_nan_sign),
   .divp_nan_exp      (divp_nan_exp),
   .divp_nan_man      (divp_nan_man),
   .divp_op_ready     (divp_op_ready),
   .divp_op_fault     (divp_op_fault),
   .divp_op_sign      (divp_op_sign),
   .divp_op_exp       (divp_op_exp),
   .divp_op_man       (divp_op_man),
   .divp_rshift_man   (divp_rshift_man),
   .div_ready         (div_ready),
   .div_fault         (div_fault),
   .div_op_predec     (div_op_predec),
   .div_state         (div_state),
   .div_round         (div_round),
   .div_sign          (div_sign),
   .div_exp           (div_exp),
   .div_man            (div_man));

  fp_sqrt64 sqrtp(.rci(rco3),.rco(rco4),
  .clk            (clk),
  .reset          (reset),
  .enable_sqrt    (enable_sqrt),
  .op_predec_sqrt (op_predec_sqrt),
  .round_sqrt     (round_sqrt),
  .state_sqrt     (state_sqrt),
  .exp_sqrt       (exp_sqrt),
  .sign2_sqrt     (sign2_sqrt),
  .man2_sqrt      (man2_sqrt),
  .sqrt_shift     (sqrt_shift),
  .res_retry_sqrt (res_retry_sqrt),
  .sqrtp_ready    (sqrtp_ready),
  .sqrtp_op_predec(sqrtp_op_predec),
  .sqrtp_state    (sqrtp_state),
  .sqrtp_round    (sqrtp_round),
  .sqrtp_sign     (sqrtp_sign),
  .sqrtp_exp      (sqrtp_exp),
  .sqrtp_man      (sqrtp_man),
  .retry_sqrt     (retry_sqrt));

  fp_sqrt64_post_comb sqrt(
   .sqrtp_ready    (sqrtp_ready),
   .sqrtp_op_predec(sqrtp_op_predec),
   .sqrtp_state    (sqrtp_state),
   .sqrtp_round    (sqrtp_round),
   .sqrtp_sign     (sqrtp_sign),
   .sqrtp_exp      (sqrtp_exp),
   .sqrtp_man      (sqrtp_man),
   .sqrt_ready     (sqrt_ready),
   .sqrt_op_predec (sqrt_op_predec),
   .sqrt_state     (sqrt_state),
   .sqrt_round     (sqrt_round),
   .sqrt_sign      (sqrt_sign),
   .sqrt_exp       (sqrt_exp),
   .sqrt_man        (sqrt_man));

  fp_result_queue queue(.rci(rco4),.rco(rco),
 .clk                   (clk),
 .reset                 (reset),
 .add_ready             (add_ready),
 .sub_rnd_flag_ready    (sub_rnd_flag_ready),
 .add_op_predec         (add_op_predec),
 .add_state             (add_state),
 .add_round             (add_round),
 .add_sign              (add_sign),
 .add_exp               (add_exp),
 .add_man               (add_man),
 .mult_ready            (mult_ready),
 .mult_fault            (mult_fault),
 .mult_op_predec        (mult_op_predec),
 .mult_state            (mult_state),
 .mult_round            (mult_round),
 .mult_sign             (mult_sign),
 .mult_exp              (mult_exp),
 .mult_man              (mult_man),
 .div_ready             (div_ready),
 .div_fault             (div_fault),
 .div_op_predec         (div_op_predec),
 .div_state             (div_state),
 .div_round             (div_round),
 .div_sign              (div_sign),
 .div_exp               (div_exp),
 .div_man               (div_man),
 .sqrt_ready            (sqrt_ready),
 .sqrt_op_predec        (sqrt_op_predec),
 .sqrt_state            (sqrt_state),
 .sqrt_round            (sqrt_round),
 .sqrt_sign             (sqrt_sign),
 .sqrt_exp              (sqrt_exp),
 .sqrt_man              (sqrt_man),
 .ex_retry              (ex_retry),
 .ex_op_predec          (ex_op_predec),
 .ex_sub_rnd_flag       (ex_sub_rnd_flag),
 .ex_round              (ex_round),
 .ex_sign               (ex_sign),
 .ex_exp                (ex_exp),
 .ex_man                (ex_man),
 .ex_state              (ex_state),
 .ex_start              (ex_start),
 .ex_man_zero           (ex_man_zero),
 .res_retry_add         (res_retry_add),
 .res_retry_mult        (res_retry_mult),
 .res_retry_div         (res_retry_div),
 .res_retry_sqrt        (res_retry_sqrt));

endmodule
