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

module fp_div64_post_comb
  (
   // inputs
   input  [`FP_PREDEC_BITS-1:0]      divp_op_predec,
   input  [`FP_STATE_BITS-1:0]       divp_state,
   input  [2-1:0]                  divp_round,

   input  [`FP_PREDEC_BITS-1:0]      divp_nan_op_predec,
   input  [`FP_STATE_BITS-1:0]       divp_nan_state,
   input  [2-1:0]                  divp_nan_round,
   
   input                             divp_nan_ready,
   input                             divp_nan_fault,
   input                             divp_nan_sign,
   input  [`FP_EXP_BITS-1:0]         divp_nan_exp,
   input  [`FP_MAN_BITS-1:0]         divp_nan_man,

   input                             divp_op_ready,
   input                             divp_op_fault,
   input                             divp_op_sign,
   input  [`FP_EXP_BITS-1:0]         divp_op_exp,
   input  [`FP_MAN_BITS+1-1:0]       divp_op_man,
   
   input                             divp_rshift_man,
   
   // outputs
   output                            div_ready,
   output                            div_fault,
   output [`FP_PREDEC_BITS-1:0]      div_op_predec,
   output [`FP_STATE_BITS-1:0]       div_state,
   output reg [2-1:0]                div_round,
   
   output                            div_sign,
   output [`FP_EXP_BITS-1:0]         div_exp,
   output [`FP_MAN_BITS-1:0]         div_man
  );
  
//---------
  
  reg   [`FP_EXP_BITS-1:0]       p1_divp_op_exp_sub1_next;
  reg   [`FP_EXP_BITS-1:0]       shift_amount_next;
  reg   [`FP_MAN_BITS-1:0]       p1_div_man_rshifted_amount_next;
  reg   [`FP_MAN_BITS-1:0]       p1_div_man_rshifted_opexp_next;
  reg   [`FP_MAN_BITS-1:0]       p1_div_man_rshifted_amount_bits_next;
  reg   [`FP_MAN_BITS-1:0]       p1_div_man_rshifted_opexp_bits_next;
  reg   divp_op_man_bitset;

  always_comb begin
    if (divp_nan_ready | divp_op_ready) begin
      shift_amount_next = 2-divp_op_exp;  // shift_amount_next = ((-divp_op_exp) +2)
      divp_op_man_bitset = |divp_op_man[`FP_MAN_BITS-1:1];
      {p1_div_man_rshifted_amount_next, p1_div_man_rshifted_amount_bits_next} = {divp_op_man, `FP_MAN_BITS'b0} >> shift_amount_next; // >> by shift_amount  
    end else begin
      shift_amount_next  = 'bx;
      divp_op_man_bitset = 'bx;
      {p1_div_man_rshifted_amount_next, p1_div_man_rshifted_amount_bits_next} = 'bx;
    end
  end
  
  wire  [`FP_PREDEC_BITS-1:0]      p1_divp_op_predec;
  wire  [`FP_STATE_BITS-1:0]       p1_divp_state;
  reg   [2-1:0]                    p1_divp_round;

  wire                             p1_divp_nan_ready;
  wire                             p1_divp_nan_fault;
  wire                             p1_divp_nan_sign;
  wire  [`FP_EXP_BITS-1:0]         p1_divp_nan_exp;
  wire  [`FP_MAN_BITS-1:0]         p1_divp_nan_man;

  wire                             p1_divp_op_ready;
  wire                             p1_divp_op_fault;
  wire                             p1_divp_op_sign;
  wire  [`FP_EXP_BITS-1:0]         p1_divp_op_exp;
  wire  [`FP_MAN_BITS+1-1:0]       p1_divp_op_man;
  
  wire                             p1_divp_rshift_man;

  wire  [`FP_EXP_BITS-1:0]         p1_divp_op_exp_sub1;
  wire  [`FP_EXP_BITS-1:0]         shift_amount;

  wire  [`FP_MAN_BITS-1:0]         p1_div_man_rshifted_amount;
  wire  [`FP_MAN_BITS-1:0]         p1_div_man_rshifted_opexp;
  
  wire  [`FP_MAN_BITS-1:0]         p1_div_man_rshifted_amount_bits;  // FIXME: change to reasonable size, FP_MAN_BITS?
  wire  [`FP_MAN_BITS-1:0]         p1_div_man_rshifted_opexp_bits;

  assign                           p1_divp_op_predec = (divp_op_ready)?divp_op_predec:divp_nan_op_predec;
  assign                           p1_divp_state     = (divp_op_ready)?divp_state:divp_nan_state;
  
  always_comb begin
    
    if(divp_op_ready) begin
      p1_divp_round = divp_round;
    end else begin
      p1_divp_round = divp_nan_round;
    end

  end
  
  assign                           p1_divp_nan_ready               = divp_nan_ready;
  assign                           p1_divp_nan_fault               = divp_nan_fault;
  assign                           p1_divp_nan_sign                = divp_nan_sign;
  assign                           p1_divp_nan_exp                 = divp_nan_exp;
  assign                           p1_divp_nan_man                 = divp_nan_man;
  assign                           p1_divp_op_ready                = divp_op_ready;
  assign                           p1_divp_op_fault                = divp_op_fault;
  assign                           p1_divp_op_sign                 = divp_op_sign;
  assign                           p1_divp_op_exp                  = divp_op_exp;
  assign                           p1_divp_op_man                  = divp_op_man;
  assign                           p1_divp_rshift_man              = divp_rshift_man;
  assign                           p1_divp_op_exp_sub1             = p1_divp_op_exp_sub1_next;
  assign                           shift_amount                    = shift_amount_next;
  assign                           p1_div_man_rshifted_amount      = p1_div_man_rshifted_amount_next;
  assign                           p1_div_man_rshifted_opexp       = p1_div_man_rshifted_opexp_next;
  assign                           p1_div_man_rshifted_amount_bits = p1_div_man_rshifted_amount_bits_next;
  assign                           p1_div_man_rshifted_opexp_bits  = p1_div_man_rshifted_opexp_bits_next;

  //------------

  reg                              div_ready_next;
  reg                              div_fault_next;
  reg [`FP_PREDEC_BITS-1:0]        div_op_predec_next;
  reg [`FP_STATE_BITS-1:0]         div_state_next;
  reg [2-1:0]                      div_round_next;

  reg                              div_sign_next;
  reg [`FP_EXP_BITS-1:0]           div_exp_next;
  reg [`FP_MAN_BITS-1:0]           div_man_next;
  
  always_comb begin
    div_sign_next      = 'bx;
    div_exp_next       = 'bx;
    div_man_next       = 'bx;
    div_op_predec_next = p1_divp_op_predec;
    div_state_next     = p1_divp_state;
    div_round_next     = p1_divp_round;
    if(p1_divp_op_ready)begin
      div_ready_next     = p1_divp_op_ready;
      div_fault_next     = p1_divp_op_fault;
      div_sign_next      = p1_divp_op_sign;
      
      if ( p1_divp_rshift_man ) begin
        // result is denormalized, must right shift
        div_exp_next = `FP_EXP_BITS'b0;
        div_man_next = p1_div_man_rshifted_amount;
        
        // set lsb if any of the bits shifted off were 1
        //$display(2-`FP_MAN_BITS);
        div_man_next[0] = (|p1_div_man_rshifted_amount_bits) /*| (((divp_op_exp < (2 - `FP_MAN_BITS)) & (p1_divp_op_man[`FP_MAN_BITS])*/ | (divp_op_man_bitset) | (p1_divp_op_man[0]);
      end else if ( p1_divp_op_man[`FP_MAN_BITS] ) begin  // `FP_MAN_BITS is MSB
          div_exp_next = p1_divp_op_exp;
          
          div_man_next = p1_divp_op_man[`FP_MAN_BITS:1];
          div_man_next[0] = |p1_divp_op_man[1:0];
      end else begin
        if ( p1_divp_op_exp == 0 ) begin
          div_exp_next = `FP_EXP_BITS'b0;  // could assign p1_divp_op_exp
          
          div_man_next = {1'b0, p1_divp_op_man[`FP_MAN_BITS:2]};
          div_man_next[0] = |p1_divp_op_man[2:0];
        end else if ( p1_divp_op_exp == 1 ) begin
          // becomes denormalized?
          div_exp_next = `FP_EXP_BITS'b0;  // could assign p1_divp_op_exp-1
          
          div_man_next = p1_divp_op_man[`FP_MAN_BITS:1];
          div_man_next[0] = |p1_divp_op_man[1:0];
        end else begin
          div_exp_next = p1_divp_op_exp-1;
          div_man_next = p1_divp_op_man[`FP_MAN_BITS-1:0];
        end
       end 
     end else begin
      div_ready_next     = p1_divp_nan_ready;
      div_fault_next     = p1_divp_nan_fault;
      div_sign_next      = p1_divp_nan_sign;
      div_man_next       = p1_divp_nan_man;
      div_exp_next       = p1_divp_nan_exp;
     end
  end  
  
  assign div_ready     = div_ready_next;
  assign div_fault     = div_fault_next;
  assign div_op_predec = div_op_predec_next;
  assign div_state     = div_state_next;
  always_comb begin
    div_round     = div_round_next;
  end
  assign div_sign      = div_sign_next;
  assign div_exp       = div_exp_next;
  assign div_man       = div_man_next;

endmodule
