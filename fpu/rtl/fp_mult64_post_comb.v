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

module fp_mult64_post_comb
  (
   // inputs
   input  [`FP_PREDEC_BITS-1:0]      multp_op_predec,
   input  [`FP_STATE_BITS-1:0]       multp_state,
   input  [2-1:0]                  multp_round,

   input                             multp_nan_ready,
   input                             multp_nan_fault,
   input                             multp_nan_sign,
   input  [`FP_EXP_BITS-1:0]         multp_nan_exp,
   input  [`FP_MAN_BITS-1:0]         multp_nan_man,

   input                             multp_op_ready,
   input                             multp_op_fault,
   input                             multp_op_sign,
   input  [`FP_EXP_BITS-1:0]         multp_op_exp,
   input  [`FP_MAN_BITS+1-1:0]       multp_op_man,

   input                             multp_rshift_man,

   // outputs
   output reg                        mult_ready,
   output reg                        mult_fault,
   output reg [`FP_PREDEC_BITS-1:0]  mult_op_predec,
   output reg [`FP_STATE_BITS-1:0]   mult_state,
   output reg [2-1:0]                mult_round,

   output reg                        mult_sign,
   output reg [`FP_EXP_BITS-1:0]     mult_exp,
   output reg [`FP_MAN_BITS-1:0]     mult_man
  );

  /*always @(*) begin
    if ( multp_nan_ready | multp_rshift_man ) begin
      $display( "***multp_nan_ready =  %d\n***multp_rshift_man = %d", multp_nan_ready, multp_rshift_man);
    end
  end*/

  //reg   [`FP_EXP_BITS-1:0]       multp_op_exp_sub1;
  reg   [`FP_EXP_BITS-1:0]       shift_amount;
  reg   [`FP_MAN_BITS-1:0]       mult_man_rshifted_amount;
  //reg   [`FP_MAN_BITS-1:0]       mult_man_rshifted_opexp;
  reg   [`FP_MAN_BITS-1:0]      mult_man_rshifted_bits;
  always_comb begin
    shift_amount = -multp_op_exp;

    // right shift based on exponent addition in denorm (that caused underflow)
    {mult_man_rshifted_amount, mult_man_rshifted_bits} = {multp_op_man[`FP_MAN_BITS+1-1:1], `FP_MAN_BITS'b0} >> shift_amount; // >> by shift_amount + 1
  end

  always_comb begin
    mult_op_predec = multp_op_predec;
    mult_state     = multp_state;
    mult_round     = multp_round;
    mult_ready     = multp_op_ready;
    if (multp_op_predec[`FP_PREDEC_DST_UINT64_BIT] |
        multp_op_predec[`FP_PREDEC_DST_SINT64_BIT] |
        multp_op_predec[`FP_PREDEC_DST_UINT32_BIT] |
        multp_op_predec[`FP_PREDEC_DST_SINT32_BIT]) begin
      //mult_ready     = multp_op_ready;
      mult_fault     = multp_op_fault;
      mult_sign      = multp_op_sign;

      mult_exp = `FP_EXP_BITS'b0;
      mult_man = multp_op_man[`FP_MAN_BITS:1];
    end else if (multp_nan_ready && multp_op_ready) begin
      //mult_ready     = multp_nan_ready;
      mult_fault     = multp_nan_fault;
      mult_sign      = multp_nan_sign;
      mult_man       = multp_nan_man;
      mult_exp       = multp_nan_exp;
    end else begin
     //mult_ready     = multp_op_ready;
      mult_fault     = multp_op_fault;
      mult_sign      = multp_op_sign;

      if ( multp_rshift_man ) begin
        // result is denormalized, must right shift
        mult_exp = `FP_EXP_BITS'b0;
        mult_man = mult_man_rshifted_amount;

        // set lsb if any of the bits shifted off were 1
`ifndef ANUBIS_LOCAL_17
        mult_man[0] = |mult_man_rshifted_bits;
`else
        mult_man[0] = (( {mult_man, 1'b0} << shift_amount ) != multp_op_man);  // << by shift_amount + 1
`endif
      end else if (multp_op_man[`FP_MAN_BITS]) begin
        mult_man = multp_op_man[`FP_MAN_BITS:1];
        mult_exp = multp_op_exp+1;
      end else begin
        mult_man = multp_op_man[`FP_MAN_BITS-1:0];
        mult_exp = multp_op_exp;
      end
    end

    /*if ( mult_ready ) begin
      $display("mult64_post:\n\tmult_man = %h\t(state = %d)",mult_man, mult_state);
    end*/
  end


endmodule
