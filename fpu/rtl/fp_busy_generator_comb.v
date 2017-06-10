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
Could share same resources as op_predec
FIXME  : ADD MISC CASES for retry
****************************************************************************/

`include "dfuncs.h"

`include "scoore_fpu.h"
module fp_busy_generator_comb
  (input                              start,
   input [`OP_TYPE_BITS-1:0]          op,
   input                              denorm_retry_add,
   input                              denorm_retry_mult,
   input                              denorm_retry_div,
   input                              denorm_retry_sqrt,

   // outputs
   output reg                         out_retry
   );

  logic                          do_retry;

  always_comb begin
    out_retry = do_retry;
  end
  always_comb begin
    if (start) begin
      case(op)
        `OP_C_UMUL, 
        `OP_C_UMULCC, 
        `OP_C_SMUL, 
        `OP_C_SMULCC,
        `OP_C_FMULS,
        `OP_C_FMULD : begin 
           do_retry = denorm_retry_mult;
         end

         `OP_C_UDIV, 
         `OP_C_SDIV, 
         `OP_C_UDIVCC, 
         `OP_C_SDIVCC, 
         `OP_C_FDIVS,
         `OP_C_FDIVD : begin
           do_retry = denorm_retry_div;
         end

         `OP_C_FADDS,
         `OP_C_FSUBS,
         `OP_C_FADDD,
         `OP_C_FSUBD : begin
           do_retry = denorm_retry_add;
         end      

         `OP_C_FSQRTS,
         `OP_C_FSQRTD : begin 
           do_retry = denorm_retry_sqrt;
         end

         default : begin
           do_retry = 'b0;
         end
       endcase
    end else begin
      do_retry = 0;
    end
  end

endmodule

