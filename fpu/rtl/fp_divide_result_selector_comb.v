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

  Half a cycle post-add operations (not synchronous).


  pranav : case statement converted to priority if else for case with Xs

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

//import scoore_fpu::*;

module fp_divide_result_selector_comb
  (// inputs
   input                            divp_op_ready0,
   input                            divp_op_fault0,
   input [`FP_MAN_BITS+1-1:0]       divp_op_man0,

   input [`FP_PREDEC_BITS-1:0]      divp_op_predec0,
   input [`FP_STATE_BITS-1:0]       divp_state0,
   input [2-1:0]                  divp_round0,
   input [`FP_EXP_BITS-1:0]         divp_op_exp0,
   input                            divp_op_sign0,
   input                            divp_rshift_man0,
   //--------------------------------------------
   input                            divp_op_ready1,
   input                            divp_op_fault1,
   input [`FP_MAN_BITS+1-1:0]       divp_op_man1,

   input [`FP_PREDEC_BITS-1:0]      divp_op_predec1,
   input [`FP_STATE_BITS-1:0]       divp_state1,
   input [2-1:0]                  divp_round1,
   input [`FP_EXP_BITS-1:0]         divp_op_exp1,
   input                            divp_op_sign1,
   input                            divp_rshift_man1,
   //--------------------------------------------
   input                            divp_op_ready2,
   input                            divp_op_fault2,
   input [`FP_MAN_BITS+1-1:0]       divp_op_man2,

   input [`FP_PREDEC_BITS-1:0]      divp_op_predec2,
   input [`FP_STATE_BITS-1:0]       divp_state2,
   input [2-1:0]                  divp_round2,
   input [`FP_EXP_BITS-1:0]         divp_op_exp2,
   input                            divp_op_sign2,
   input                            divp_rshift_man2,
   //------------------------------------------------
   input                            divp_op_ready3,
   input                            divp_op_fault3,
   input [`FP_MAN_BITS+1-1:0]       divp_op_man3,

   input [`FP_PREDEC_BITS-1:0]      divp_op_predec3,
   input [`FP_STATE_BITS-1:0]       divp_state3,
   input [2-1:0]                  divp_round3,
   input [`FP_EXP_BITS-1:0]         divp_op_exp3,
   input                            divp_op_sign3,
   input                            divp_rshift_man3,

   // outputs
   output reg                       divp_op_ready_sel,
   output reg                       divp_op_fault_sel,
   output reg [`FP_MAN_BITS+1-1:0]  divp_op_man_sel,

   output reg [`FP_PREDEC_BITS-1:0] divp_op_predec_sel,
   output reg [`FP_STATE_BITS-1:0]  divp_state_sel,
   output reg [2-1:0]               divp_round_sel,
   output reg [`FP_EXP_BITS-1:0]    divp_op_exp_sel,
   output reg                       divp_op_sign_sel,

   output reg                       divp_rshift_man_sel
  );

  always_comb begin

    if(divp_op_ready0) begin
      divp_op_ready_sel   = divp_op_ready0;
      divp_op_fault_sel   = divp_op_fault0;
      divp_op_man_sel     = divp_op_man0;

      divp_op_predec_sel  = divp_op_predec0;
      divp_state_sel      = divp_state0;
      divp_round_sel      = divp_round0;
      divp_op_exp_sel     = divp_op_exp0;
      divp_op_sign_sel    = divp_op_sign0;

      divp_rshift_man_sel = divp_rshift_man0;


    end else if (divp_op_ready1) begin
      divp_op_ready_sel   = divp_op_ready1;
      divp_op_fault_sel   = divp_op_fault1;
      divp_op_man_sel     = divp_op_man1;

      divp_op_predec_sel  = divp_op_predec1;
      divp_state_sel      = divp_state1;
      divp_round_sel      = divp_round1;
      divp_op_exp_sel     = divp_op_exp1;
      divp_op_sign_sel    = divp_op_sign1;

      divp_rshift_man_sel = divp_rshift_man1;

    end else if (divp_op_ready2) begin
      divp_op_ready_sel   = divp_op_ready2;
      divp_op_fault_sel   = divp_op_fault2;
      divp_op_man_sel     = divp_op_man2;

      divp_op_predec_sel  = divp_op_predec2;
      divp_state_sel      = divp_state2;
      divp_round_sel      = divp_round2;
      divp_op_exp_sel     = divp_op_exp2;
      divp_op_sign_sel    = divp_op_sign2;

      divp_rshift_man_sel = divp_rshift_man2;

    end else if (divp_op_ready3) begin
      divp_op_ready_sel   = divp_op_ready3;
      divp_op_fault_sel   = divp_op_fault3;
`ifndef ANUBIS_LOCAL_3
      divp_op_man_sel     = divp_op_man3;
`else
      divp_op_man_sel     = divp_op_man2;
`endif

      divp_op_predec_sel  = divp_op_predec3;
      divp_state_sel      = divp_state3;
      divp_round_sel      = divp_round3;
      divp_op_exp_sel     = divp_op_exp3;
      divp_op_sign_sel    = divp_op_sign3;

      divp_rshift_man_sel = divp_rshift_man3;

    end else begin
`ifndef ANUBIS_LOCAL_6
      divp_op_ready_sel   = 1'b0;
`else
      divp_op_ready_sel   = 1'b1;
`endif
      divp_op_fault_sel   = 1'bx;
      divp_op_man_sel     = `FP_MAN_BITS'bx;

      divp_op_predec_sel  = `FP_PREDEC_BITS'bx;
      divp_state_sel      = `FP_STATE_BITS'bx;
      divp_round_sel      = 'bx;
      divp_op_exp_sel     = `FP_EXP_BITS'bx;
      divp_op_sign_sel    = 1'bx;

      divp_rshift_man_sel = 1'bx;

    end
  end

endmodule

