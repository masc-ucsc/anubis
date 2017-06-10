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
      w

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//import scoore_fpu::*;
//import retry_common::*;

module fp_result_queue
  (input                      clk,
   input                      reset,

   //inputs
   //add
   input                         add_ready,
   input                         sub_rnd_flag_ready,
   input [`FP_PREDEC_BITS-1:0]   add_op_predec,
   input [`FP_STATE_BITS-1:0]    add_state,
   input [2-1:0]               add_round,

   input                         add_sign,
   input [`FP_EXP_BITS-1:0]      add_exp,
   input [`FP_MAN_BITS-1:0]      add_man,

   //mult
   input                         mult_ready,
   input                         mult_fault,
   input [`FP_PREDEC_BITS-1:0]   mult_op_predec,
   input [`FP_STATE_BITS-1:0]    mult_state,
   input [2-1:0]               mult_round,

   input                         mult_sign,
   input [`FP_EXP_BITS-1:0]      mult_exp,
   input [`FP_MAN_BITS-1:0]      mult_man,

   //div
   input                         div_ready,
   input                         div_fault,
   input [`FP_PREDEC_BITS-1:0]   div_op_predec,
   input [`FP_STATE_BITS-1:0]    div_state,
   input [2-1:0]               div_round,

   input                         div_sign,
   input [`FP_EXP_BITS-1:0]      div_exp,
   input [`FP_MAN_BITS-1:0]      div_man,

   //sqrt
   input                         sqrt_ready,
 //  input            sqrt_fault,
   input [`FP_PREDEC_BITS-1:0]   sqrt_op_predec,
   input [`FP_STATE_BITS-1:0]    sqrt_state,
   input [2-1:0]               sqrt_round,

   input                         sqrt_sign,
   input [`FP_EXP_BITS-1:0]      sqrt_exp,
   input [`FP_MAN_BITS-1:0]      sqrt_man,

   //retry
   input                         ex_retry,

   // outputs
   output [`FP_PREDEC_BITS-1:0]  ex_op_predec,
   output                        ex_sub_rnd_flag,
   output [2-1:0]              ex_round,

   output                        ex_sign,
   output [`FP_EXP_BITS-1:0]     ex_exp,
   output [`FP_MAN_BITS-1:0]     ex_man,

   output [`FP_STATE_BITS-1:0]   ex_state,
   output                        ex_start,
   output                        ex_man_zero,

   //retry
   output reg                    res_retry_add,
   output reg                    res_retry_mult,
   output reg                    res_retry_div,
   output reg                    res_retry_sqrt,

   //Retry re-clocking signals
   input  [4:0]            rci,
   output [4:0]            rco

 );

  reg                                   ex_start_next;
  reg [`FP_PREDEC_BITS-1:0]             ex_op_predec_next;
  reg [2-1:0]                           ex_round_next;
  reg [`FP_STATE_BITS-1:0]              ex_state_next;

  reg                                   ex_sign_next;
  reg [`FP_EXP_BITS-1:0]                ex_exp_next;
  reg [`FP_MAN_BITS-1:0]                ex_man_next;

  wire                                  res_retry;

  always_comb begin                             //Logic for retries for func units

    casez({res_retry,add_ready,mult_ready,sqrt_ready,div_ready})

      5'b1????: begin                           //retry from previous stage
        res_retry_add  = 1'b1;
        res_retry_mult = 1'b1;
        res_retry_sqrt = 1'b1;
        res_retry_div  = 1'b1;
      end

      5'b01???: begin
        res_retry_add  = 1'b0;                  //Add given top priority
        res_retry_mult = mult_ready;
        res_retry_sqrt = sqrt_ready;
        res_retry_div  = div_ready;
      end

      5'b001??: begin                          //mult 2nd priority
        res_retry_add  = 1'b0;
        res_retry_mult = 1'b0;
        res_retry_sqrt = sqrt_ready;
        res_retry_div  = div_ready;
      end

      5'b0001?: begin                         //sqrt 3rd
        res_retry_add  = 1'b0;
        res_retry_mult = 1'b0;
        res_retry_sqrt = 1'b0;
        res_retry_div  = div_ready;
      end

      5'b00001,                              //lastly div -> could be swapped with sqrt
      5'b00000: begin
        res_retry_add  = 1'b0;
        res_retry_mult = 1'b0;
        res_retry_sqrt = 1'b0;
        res_retry_div  = 1'b0;
      end

      default: begin  // WARNING: this is redudant, warning in dc, but it is cleaner
        res_retry_add  = 1'b0;
        res_retry_mult = 1'b0;
        res_retry_sqrt = 1'b0;
        res_retry_div  = 1'b0;
      end
    endcase
  end

  always_comb begin
    if ( add_ready ) begin
      ex_start_next     = 1'b1;

      ex_op_predec_next = add_op_predec;
      ex_state_next     = add_state;
      ex_round_next     = add_round;

      ex_sign_next      = add_sign;
`ifndef ANUBIS_LOCAL_26
      ex_exp_next       = add_exp;
`else
      ex_exp_next       = mult_exp;
`endif
      ex_man_next       = add_man;
    end else if ( mult_ready ) begin

      ex_start_next     = 1'b1;

      ex_op_predec_next = mult_op_predec;
      ex_state_next     = mult_state;
      ex_round_next     = mult_round;

      ex_sign_next      = mult_sign;
      ex_exp_next       = mult_exp;
      ex_man_next       = mult_man;
    end else if ( sqrt_ready ) begin

      ex_start_next     = 1'b1;

      ex_op_predec_next = sqrt_op_predec;
      ex_state_next     = sqrt_state;
      ex_round_next     = sqrt_round;

      ex_sign_next      = sqrt_sign;
      ex_exp_next       = sqrt_exp;
      ex_man_next       = sqrt_man;

    end else if ( div_ready ) begin

      ex_start_next     = 1'b1;

      ex_op_predec_next = div_op_predec;
      ex_state_next     = div_state;
      ex_round_next     = div_round;

      ex_sign_next      = div_sign;
      ex_exp_next       = div_exp;
      ex_man_next       = div_man;
    end else begin
      ex_start_next     = 1'b0;

      ex_op_predec_next = 'bx;
      ex_state_next     = 'bx;
      ex_round_next     = 'bx;

      ex_sign_next      = 'bx;
      ex_exp_next       = 'bx;
      ex_man_next       = 'bx;
    end
  end

 reg man_zero_next;

  always_comb begin
    if((add_ready) && (add_man == `FP_MAN_BITS'b0)) begin
     man_zero_next = 1'b1;
   end else begin
     man_zero_next = 1'b0;
   end
 end


 stage #(.Size(`FP_PREDEC_BITS+`FP_STATE_BITS+2+
                 1+`FP_EXP_BITS+`FP_MAN_BITS + 2)) s_res_q

   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({ex_op_predec_next, ex_state_next, ex_round_next,
                  ex_sign_next, ex_exp_next, ex_man_next,
                    man_zero_next, sub_rnd_flag_ready})
   ,.dinValid (ex_start_next)
   ,.dinRetry (res_retry)

   ,.q        ({ex_op_predec, ex_state, ex_round,
                  ex_sign, ex_exp, ex_man,
                    ex_man_zero, ex_sub_rnd_flag})
   ,.qValid   (ex_start)
   ,.qRetry   (ex_retry)

   // Re-clocking signals
   ,.rci      (rci)
   ,.rco      (rco)
   );
endmodule
