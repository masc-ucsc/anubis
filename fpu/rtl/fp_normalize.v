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
    Description

 fix_man requires a shift RIGHT (up to 63bits fix_exp).

 fp_man requires rounding and at most one bit shift RIGHT (only if overflow).

****************************************************************************/
`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

//import scoore_fpu::*;
//import retry_common::*;

module fp_normalize
  (input                             clk,
   input                             reset,

   // inputs
   input                             start,
   input [`FP_STATE_BITS-1:0]        state,

   input [`FP_PREDEC_BITS-1:0]       op_predec,
   input                             sub_rnd_flag,
   input [2-1:0]                    round,

   input                             sign,
   input [`FP_EXP_BITS-1:0]          exp,
   input [`FP_MAN_BITS-1:0]          man,
   input                             man_zero,

   input                             norm_retry,

   // outputs
   output [`FP_TYPE_D_BITS-1:0]      norm_result,
   output [`FP_STATE_BITS-1:0]       norm_state,
   output                            norm_ready,
   output [3-1:0]                    norm_icc,

   output                            ex_retry,

   //Retry Re-clocking signals
   input  [4:0]                rci,
   output [4:0]                rco

 );

  // psl default clock = (posedge clk);

  wire [`FP_TYPE_D_BITS-1:0]        fp_result_next;
  wire                              fp_overflow_next;
  fp_normalize_fp
    fp
      (
       // inputs
       .op_predec       (op_predec),
       .sub_rnd_flag    (sub_rnd_flag),
       .round           (round),
`ifndef ANUBIS_LOCAL_23
       .start           (start),
`else
       .start           (~start),
`endif
       .sign            (sign),
       .fp_exp          (exp),
       .fp_man          (man),
       .man_zero        (man_zero),
       // output
       .fp_result_next  (fp_result_next),
       .fp_overflow_next(fp_overflow_next) // FIXME: still not used
       );

  reg  [ 3-1:0 ]                    norm_icc_next;
  reg [`FP_TYPE_D_BITS-1:0]         result_next;

  always_comb begin
    norm_icc_next = 'b0; //'bx;

    if(start & (op_predec[`FP_PREDEC_DST_FP32_BIT] | op_predec[`FP_PREDEC_DST_FP64_BIT])) begin
      result_next = fp_result_next;
    end else begin
      result_next = man;
      // My Fix: If it's 32 bit division result, just get the 1st 32 bits
      // My Fix: If it's sined integer and negative, 2's complement the result
      if(op_predec[`FP_PREDEC_DST_UINT32_BIT]) begin
        result_next = (man >> 32);
      ////
`ifndef ANUBIS_LOCAL_23
        if (man[63:32] == 32'hFFFFFFFF)begin //FIXME: overflow condition
`else
        if (man[31:00] == 32'hFFFFFFFF)begin //FIXME: overflow condition
`endif
          norm_icc_next[2] = 1;
        end else begin
          norm_icc_next[2] = 0;
        end
      ///

      end
      if(op_predec[`FP_PREDEC_DST_SINT32_BIT]) begin
        if (sign) begin
          result_next = ((~(man >> 32) + 1) << 32) >> 32;
        end else begin
          result_next = (man >> 32) & 32'h7FFFFFFF;
        end
        // If overflow, return MAX
        if (man == 64'hFFFFFFFFFFFFFFFF) begin
          norm_icc_next[2] = 1;
`ifndef ANUBIS_LOCAL_23
          if (sign) begin
`else
          if (~sign) begin
`endif
            result_next = 32'h80000000;
          end else begin
            result_next = 32'h7FFFFFFF;
          end
        end
        ////
        //
          else begin
`ifndef ANUBIS_LOCAL_5
            norm_icc_next[2] = 0;
`else
            norm_icc_next[2] = 1;
`endif

        end

        ////
      end
      if(op_predec[`FP_PREDEC_DST_SINT64_BIT]) begin
        if (sign) begin
          result_next = ~man + 1;
        end
      end
    end
end

//ICC to be added

stage #(.Size(`FP_TYPE_D_BITS +`FP_STATE_BITS + 3)) s_norm
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({result_next, state, norm_icc_next})
   ,.dinValid (start)
   ,.dinRetry (ex_retry)

`ifndef ANUBIS_LOCAL_2
   ,.q        ({norm_result, norm_state, norm_icc})
`else
   ,.q        ({norm_state, norm_result, norm_icc})
`endif
   ,.qValid   (norm_ready)
   ,.qRetry   (norm_retry)

    // Re-clocking signals
    ,.rci     (rci)
    ,.rco     (rco)
   );

endmodule
