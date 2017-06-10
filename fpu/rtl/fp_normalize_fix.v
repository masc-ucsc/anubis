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

 requires a shift RIGHT (up to 63bits int_exp).
 
****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

module fp_normalize_fix
  (
   input [`FP_PREDEC_BITS-1:0]       op_predec,

   input                             sign,

   input [`FP_EXP_BITS-1:0]          fix_exp,
   input [`FP_MAN_BITS-1:0]          fix_man,

   // outputs
   output reg [`FP_TYPE_D_BITS-1:0]  fix_result_next
  );

  reg [`FP_MAN_BITS-1:0] man;
  always @(*) begin
    man = fix_man >> (fix_exp & 6'b111111);
  end
  
  always @(*) begin
    case({op_predec[`FP_PREDEC_DST_SINT32_BIT],
          op_predec[`FP_PREDEC_DST_UINT32_BIT],
          op_predec[`FP_PREDEC_DST_SINT64_BIT],
          op_predec[`FP_PREDEC_DST_UINT64_BIT]})
      // SINT32
      4'b1000: begin
        fix_result_next[30:0]  = man[`FP_MAN_BITS-1:`FP_MAN_BITS-31];
        fix_result_next[31]    = sign;
        fix_result_next[63:32] = 32'b0;
      end
      // UINT32
      4'b0100: begin
        fix_result_next[31:0]  = man[`FP_MAN_BITS-1:`FP_MAN_BITS-32];
        fix_result_next[63:32] = 32'b0;
      end
      // SINT64
      4'b0010: begin
        fix_result_next[62:0]  = man[`FP_MAN_BITS-1:1];
        fix_result_next[63]    = sign;
      end
      // UINT64
      4'b0001: begin
        fix_result_next[63:0]  = man;
      end
      default: begin
        fix_result_next = `FP_TYPE_D_BITS'bx;
      end
    endcase
  end
  
endmodule
