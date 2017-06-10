//==============================================================================
//      File:           $URL$
//      Version:        $Revision$
//      Author:         Jose Renau  (http://masc.cse.ucsc.edu/)
//                      John Burr
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

/****************************************************************************
    Description:

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//import scoore_fpu::*;
//import retry_common::*;

module fp_div64
  (input                             clk,
   // inputs
   input                             enable_div,
   input                             reset,

   input [`FP_PREDEC_BITS-1:0]       op_predec_div,
   input [2-1:0]                   round_div,
   input [`FP_STATE_BITS-1:0]        state_div,
   input [`FP_EXP_BITS-1:0]          exp_div,
   input [`FP_EXP_BITS-1:0]          exp1_div,
   input                             sign1_div,
   input [`FP_MAN_BITS-1:0]          man1_div,
   input [`FP_EXP_BITS-1:0]          exp2_div,
   input                             sign2_div,
   input [`FP_MAN_BITS-1:0]          man2_div,

   input                             rshift_man_div,
   input                             overflow_div,

   input                             res_retry_div,

   // outputs
   output [`FP_PREDEC_BITS-1:0]      divp_op_predec,
   output [`FP_STATE_BITS-1:0]       divp_state,
   output [2-1:0]                  divp_round,

   output [`FP_PREDEC_BITS-1:0]      divp_nan_op_predec,
   output [`FP_STATE_BITS-1:0]       divp_nan_state,
   output [2-1:0]                  divp_nan_round,

   output                            divp_nan_ready,
   output                            divp_nan_fault,
   output                            divp_nan_sign,
   output [`FP_EXP_BITS-1:0]         divp_nan_exp,
   output [`FP_MAN_BITS-1:0]         divp_nan_man,

   output                            divp_op_ready,
   output                            divp_op_fault,
   output                            divp_op_sign,
   output [`FP_EXP_BITS-1:0]         divp_op_exp,
   output [`FP_MAN_BITS+1-1:0]       divp_op_man,

   output                            divp_rshift_man,

   output reg                        retry_div,

   //Retry Re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco
 );

  reg [`FP_PREDEC_BITS-1:0]          divp_op_predec_next;
  reg [`FP_STATE_BITS-1:0]           divp_state_next;
  wire [2-1:0]                          divp_round_next;

  reg                                divp_nan_ready_next;
  reg                                divp_nan_fault_next;
  reg                                divp_nan_sign_next;
  reg [`FP_EXP_BITS-1:0]             divp_nan_exp_next;
  reg [`FP_MAN_BITS-1:0]             divp_nan_man_next;

  wire                               divp_op_ready_sel;
  wire                               divp_op_fault_sel;
  wire                               divp_op_sign_sel;
  wire [`FP_EXP_BITS-1:0]            divp_op_exp_sel;
  wire [`FP_MAN_BITS+1-1:0]          divp_op_man_sel;

  wire                               divp_rshift_man_sel;

  wire                               divp_nan_ready_prop;
  wire                               divp_nan_fault_prop;
  wire                               divp_nan_sign_prop;
  wire [`FP_EXP_BITS-1:0]            divp_nan_exp_prop;
  wire [`FP_MAN_BITS-1:0]            divp_nan_man_prop;

  wire [`FP_STATE_BITS-1:0]          divp_nan_state_prop;
  wire [2-1:0]                          divp_nan_round_prop;
  wire [`FP_PREDEC_BITS-1:0]         divp_nan_op_predec_prop;

  wire                               divider0_start;
  wire                               divider1_start;
  wire                               divider2_start;
  wire                               divider3_start;

  //------retries to dividers

  reg                                div_in_retry0;
  reg                                div_in_retry1;
  reg                                div_in_retry2;
  reg                                div_in_retry3;

  //--- retry from div_out_s
  wire                               div_out_retry;
  wire                               div_out_nan_retry;

  reg                                sel_retry_in;

  wire                               divp_busy0;
  wire                               divp_busy1;
  wire                               divp_busy2;
  wire                               divp_busy3;

  wire                               sel_retry_out;
  wire                               prop_retry_out;
  wire                               div_out_retry0;
  wire                               div_out_retry1;
  wire                               div_out_retry2;
  wire                               div_out_retry3;
  wire                               res_nan_retry;

`ifndef ANUBIS_GLOBAL_0
  wire                               special_select;
`endif

  logic nan_retry;

  //--- Re-clocking signals
  wire [4:0] rco14;
  wire [4:0] rco15;
  wire [4:0] rco16;
  wire [4:0] rco17;
  wire [4:0] rco18;
  wire [4:0] rco19;

  fp_propagate_div p_special(
    .clk(clk),
    .reset(reset),
    .enable_div(enable_div),
    .op_predec_div(op_predec_div),
    .round_div(round_div),
    .state_div(state_div),
    .exp1_div(exp1_div),
    .sign1_div(sign1_div),
    .man1_div(man1_div),
    .exp2_div(exp2_div),
    .sign2_div(sign2_div),
    .man2_div(man2_div),
    .divp_nan_fault_prop(divp_nan_fault_prop),
    .divp_nan_ready_prop(divp_nan_ready_prop),
    .divp_nan_sign_prop(divp_nan_sign_prop),
    .divp_nan_exp_prop(divp_nan_exp_prop),
    .divp_nan_man_prop(divp_nan_man_prop),
    .divp_nan_state_prop(divp_nan_state_prop),
    .divp_nan_round_prop(divp_nan_round_prop),
    .divp_nan_op_predec_prop(divp_nan_op_predec_prop),
    .divider0_start(divider0_start),
    .divider1_start(divider1_start),
    .divider2_start(divider2_start),
    .divider3_start(divider3_start),

    //inputs
    .overflow(overflow_div),
    .nan_retry(nan_retry),
    .divp_busy0(divp_busy0),
    .divp_busy1(divp_busy1),
    .divp_busy2(divp_busy2),
    .divp_busy3(divp_busy3),
    //outputs
    .prop_retry_out(prop_retry_out),
    .sel_retry_out(sel_retry_out),

`ifndef ANUBIS_GLOBAL_0
    .special_select(special_select),
`endif
    // Retry re-clocking signals
    .rci13(rci),
    .rco13(rco14)
    );

  always_comb begin
`ifndef ANUBIS_LOCAL_7
    retry_div = (prop_retry_out && special_select) || (sel_retry_out && ~special_select);
`else
    retry_div = prop_retry_out || sel_retry_out;
`endif
  end

  wire [`FP_STATE_BITS-1:0]       divp_state_sel;
  wire [2-1:0]                       divp_round_sel;
  wire [`FP_PREDEC_BITS-1:0]      divp_op_predec_sel;
  wire                            divp_op_ready0;
  wire                            divp_op_valid0;
  wire                            divp_op_fault0;
  wire [`FP_MAN_BITS+1-1:0]       divp_op_man0;

  wire [`FP_PREDEC_BITS-1:0]      divp_op_predec0;
  wire [`FP_STATE_BITS-1:0]       divp_state0;
  wire [2-1:0]                       divp_round0;
  wire [`FP_EXP_BITS-1:0]         divp_op_exp0;
  wire                            divp_op_sign0;
  wire                            divp_rshift_man0;
  //------------------
  wire                            divp_op_ready1;
  wire                            divp_op_valid1;
  wire                            divp_op_fault1;
  wire [`FP_MAN_BITS+1-1:0]       divp_op_man1;

  wire [`FP_PREDEC_BITS-1:0]      divp_op_predec1;
  wire [`FP_STATE_BITS-1:0]       divp_state1;
  wire [2-1:0]                       divp_round1;
  wire [`FP_EXP_BITS-1:0]         divp_op_exp1;
  wire                            divp_op_sign1;
  wire                            divp_rshift_man1;
   //------------------
  wire                            divp_op_ready2;
  wire                            divp_op_valid2;
  wire                            divp_op_fault2;
  wire [`FP_MAN_BITS+1-1:0]       divp_op_man2;

  wire [`FP_PREDEC_BITS-1:0]      divp_op_predec2;
  wire [`FP_STATE_BITS-1:0]       divp_state2;
  wire [2-1:0]                       divp_round2;
  wire [`FP_EXP_BITS-1:0]         divp_op_exp2;
  wire                            divp_op_sign2;
  wire                            divp_rshift_man2;
  //------------------
  wire                            divp_op_ready3;
  wire                            divp_op_valid3;
  wire                            divp_op_fault3;
  wire [`FP_MAN_BITS+1-1:0]       divp_op_man3;

  wire [`FP_PREDEC_BITS-1:0]      divp_op_predec3;
  wire [`FP_STATE_BITS-1:0]       divp_state3;
  wire [2-1:0]                       divp_round3;
  wire [`FP_EXP_BITS-1:0]         divp_op_exp3;
  wire                            divp_op_sign3;
  wire                            divp_rshift_man3;

 fp_div64_to_64 divider0
  (.clk              (clk),
   .reset            (reset),
   // inputs
   .start            (divider0_start),

   .man1             (man1_div),
   .man2             (man2_div),

   .sign1            (sign1_div),
   .sign2            (sign2_div),

   .op_predec        (op_predec_div),
   .state            (state_div),
   .round            (round_div),
   .exp              (exp_div),

   .rshift_man       (rshift_man_div),

   .div_in_retry     (div_in_retry0),

   // Re-clocking signals
   .rci12            (rco14),

   // outputs
   .divp_op_ready    (divp_op_ready0),
   .divp_op_valid    (divp_op_valid0),
   .divp_op_fault    (divp_op_fault0),
   .divp_op_man      (divp_op_man0),

   .divp_op_predec   (divp_op_predec0),
   .divp_state       (divp_state0),
   .divp_round       (divp_round0),
   .divp_op_exp      (divp_op_exp0),
   .divp_op_sign     (divp_op_sign0),

   .divp_rshift_man  (divp_rshift_man0),
   .divp_busy        (divp_busy0),

   .div_out_retry    (div_out_retry0),

   // Re-clocking signals
   .rco12            (rco15)
 );

  fp_div64_to_64 divider1
  (.clk              (clk),
   .reset            (reset),
   // inputs
   .start            (divider1_start),

   .man1             (man1_div),
   .man2             (man2_div),

   .sign1            (sign1_div),
   .sign2            (sign2_div),

   .op_predec        (op_predec_div),
   .state            (state_div),
   .round            (round_div),
   .exp              (exp_div),

   .rshift_man       (rshift_man_div),

   .div_in_retry     (div_in_retry1),

   // Re-clocking signals
   .rci12             (rco15),

   // outputs
   .divp_op_ready    (divp_op_ready1),
   .divp_op_valid    (divp_op_valid1),
   .divp_op_fault    (divp_op_fault1),
   .divp_op_man      (divp_op_man1),

   .divp_op_predec   (divp_op_predec1),
   .divp_state       (divp_state1),
   .divp_round       (divp_round1),
   .divp_op_exp      (divp_op_exp1),
   .divp_op_sign     (divp_op_sign1),

   .divp_rshift_man  (divp_rshift_man1),
   .divp_busy        (divp_busy1),

   .div_out_retry    (div_out_retry1),

   // Re-clocking signals
   .rco12             (rco16)
  );

  fp_div64_to_64 divider2
  (.clk              (clk),
   .reset            (reset),
   // inputs
   .start            (divider2_start),

   .man1             (man1_div),
   .man2             (man2_div),

   .sign1            (sign1_div),
   .sign2            (sign2_div),

   .op_predec        (op_predec_div),
   .state            (state_div),
   .round            (round_div),
   .exp              (exp_div),

   .rshift_man       (rshift_man_div),

   .div_in_retry     (div_in_retry2),

   // Re-clocking signals
   .rci12             (rco16),

   // outputs
   .divp_op_ready    (divp_op_ready2),
   .divp_op_valid    (divp_op_valid2),
   .divp_op_fault    (divp_op_fault2),
   .divp_op_man      (divp_op_man2),

   .divp_op_predec   (divp_op_predec2),
   .divp_state       (divp_state2),
   .divp_round       (divp_round2),
   .divp_op_exp      (divp_op_exp2),
   .divp_op_sign     (divp_op_sign2),

   .divp_rshift_man  (divp_rshift_man2),
   .divp_busy        (divp_busy2),

   .div_out_retry    (div_out_retry2),

   // Re-clocking signals
   .rco12             (rco17)
  );

  fp_div64_to_64 divider3
  (.clk              (clk),
   .reset            (reset),
   // inputs
   .start            (divider3_start),

   .man1             (man1_div),
   .man2             (man2_div),

   .sign1            (sign1_div),
   .sign2            (sign2_div),

   .op_predec        (op_predec_div),
   .state            (state_div),
   .round            (round_div),
   .exp              (exp_div),

   .rshift_man       (rshift_man_div),

   .div_in_retry     (div_in_retry3),

   // Re-clocking signals
   .rci12            (rco17),

   // outputs
   .divp_op_ready    (divp_op_ready3),
   .divp_op_valid    (divp_op_valid3),
   .divp_op_fault    (divp_op_fault3),
   .divp_op_man      (divp_op_man3),

   .divp_op_predec   (divp_op_predec3),
   .divp_state       (divp_state3),
   .divp_round       (divp_round3),
   .divp_op_exp      (divp_op_exp3),
   .divp_op_sign     (divp_op_sign3),

   .divp_rshift_man  (divp_rshift_man3),
   .divp_busy        (divp_busy3),
   .div_out_retry    (div_out_retry3),

   // Re-clocking signals
   .rco12            (rco18)
  );

  fp_divide_result_selector_comb div_result_selector(
  
   .divp_op_ready0    (divp_op_ready0),
   .divp_op_fault0    (divp_op_fault0),
   .divp_op_man0      (divp_op_man0),
   .divp_op_predec0   (divp_op_predec0),
   .divp_state0       (divp_state0),
   .divp_round0       (divp_round0),
   .divp_op_exp0      (divp_op_exp0),
   .divp_op_sign0     (divp_op_sign0),
   .divp_rshift_man0  (divp_rshift_man0),
   .divp_op_ready1    (divp_op_ready1),
   .divp_op_fault1    (divp_op_fault1),
   .divp_op_man1      (divp_op_man1),
   .divp_op_predec1   (divp_op_predec1),
   .divp_state1       (divp_state1),
   .divp_round1       (divp_round1),
   .divp_op_exp1      (divp_op_exp1),
   .divp_op_sign1     (divp_op_sign1),
   .divp_rshift_man1  (divp_rshift_man1),
   .divp_op_ready2    (divp_op_ready2),
   .divp_op_fault2    (divp_op_fault2),
   .divp_op_man2      (divp_op_man2),
   .divp_op_predec2   (divp_op_predec2),
   .divp_state2       (divp_state2),
   .divp_round2       (divp_round2),
   .divp_op_exp2      (divp_op_exp2),
   .divp_op_sign2     (divp_op_sign2),
   .divp_rshift_man2  (divp_rshift_man2),
   .divp_op_ready3    (divp_op_ready3),
   .divp_op_fault3    (divp_op_fault3),
   .divp_op_man3      (divp_op_man3),
   .divp_op_predec3   (divp_op_predec3),
   .divp_state3       (divp_state3),
   .divp_round3       (divp_round3),
   .divp_op_exp3      (divp_op_exp3),
   .divp_op_sign3     (divp_op_sign3),
   .divp_rshift_man3  (divp_rshift_man3),
   .divp_op_ready_sel (divp_op_ready_sel),
   .divp_op_fault_sel (divp_op_fault_sel),
   .divp_op_man_sel   (divp_op_man_sel),
   .divp_op_predec_sel(divp_op_predec_sel),
   .divp_state_sel    (divp_state_sel),
   .divp_round_sel    (divp_round_sel),
   .divp_op_exp_sel   (divp_op_exp_sel),
   .divp_op_sign_sel  (divp_op_sign_sel),
   .divp_rshift_man_sel(divp_rshift_man_sel)
  );

 //Retry logic for the block

  assign nan_retry     = div_out_nan_retry || divp_op_ready_sel; //nan retries if any divider is ready
  assign res_nan_retry = res_retry_div || divp_op_ready;         //corner case when both nan and divp out stages have pending valid(s); same priority enforced

  parameter RET    = 1'b1;
  parameter NO_RET = 1'b0;

  always_comb begin
   //divider0 given highest priority to finish
    casez({div_out_retry,divp_op_ready3,divp_op_ready2,divp_op_ready1,divp_op_ready0})

      5'b1???? : begin
                div_in_retry0 = RET;
                div_in_retry1 = RET;
                div_in_retry2 = RET;
                div_in_retry3 = RET;
              end
      5'b0???1 : begin
                div_in_retry0 = NO_RET;
                div_in_retry1 = RET;
                div_in_retry2 = RET;
                div_in_retry3 = RET;
              end
      5'b0??10 : begin
                div_in_retry0 = NO_RET;
                div_in_retry1 = NO_RET;
                div_in_retry2 = RET;
                div_in_retry3 = RET;
              end
      5'b0?100 : begin
                div_in_retry0 = NO_RET;
                div_in_retry1 = NO_RET;
                div_in_retry2 = NO_RET;
                div_in_retry3 = RET;
              end
      5'b01000 : begin
                div_in_retry0 = NO_RET;
                div_in_retry1 = NO_RET;
                div_in_retry2 = NO_RET;
                div_in_retry3 = NO_RET;
              end
      default : begin
                div_in_retry0 = NO_RET;
                div_in_retry1 = NO_RET;
                div_in_retry2 = NO_RET;
                div_in_retry3 = NO_RET;
              end
    endcase
  end

  stage #(.Size(1+`FP_PREDEC_BITS+`FP_STATE_BITS+2+1+`FP_EXP_BITS+(`FP_MAN_BITS+1)+1))

  div_out_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({divp_op_fault_sel, divp_op_predec_sel, divp_state_sel,
                  divp_round_sel, divp_op_sign_sel, divp_op_exp_sel, divp_op_man_sel, divp_rshift_man_sel})
   ,.dinValid (divp_op_ready_sel)
   ,.dinRetry (div_out_retry)

   ,.q        ({divp_op_fault, divp_op_predec,  divp_state,
                  divp_round, divp_op_sign, divp_op_exp, divp_op_man, divp_rshift_man})
   ,.qValid   (divp_op_ready)
`ifndef ANUBIS_LOCAL_1
   ,.qRetry   (res_retry_div)
`else
   ,.qRetry   (~res_retry_div)
`endif

   // Re-clocking signals
   ,.rci     (rco18)
   ,.rco     (rco19)
   );

  stage #(.Size(1+1+`FP_EXP_BITS+`FP_MAN_BITS+`FP_PREDEC_BITS+`FP_STATE_BITS+2)) //Might be cleaner with one stage

  div_out_nan_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({divp_nan_fault_prop, divp_nan_sign_prop, divp_nan_exp_prop, divp_nan_man_prop,
                  divp_nan_op_predec_prop, divp_nan_state_prop, divp_nan_round_prop})
`ifndef ANUBIS_LOCAL_11
   ,.dinValid (divp_nan_ready_prop && ~divp_op_ready_sel)
`else
  ,.dinValid (divp_nan_ready_prop && ~nan_retry)
`endif
   ,.dinRetry (div_out_nan_retry)

   ,.q        ({divp_nan_fault, divp_nan_sign, divp_nan_exp, divp_nan_man,
                  divp_nan_op_predec, divp_nan_state, divp_nan_round})
   ,.qValid   (divp_nan_ready)
   ,.qRetry   (res_nan_retry)

   // Re-clocking signals
   ,.rci     (rco19)
   ,.rco     (rco)
   );

endmodule
