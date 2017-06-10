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

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"


module fp_op_predec
  (
   input [`OP_TYPE_BITS-1:0]          op,
   input                              start,
   output reg [`FP_PREDEC_BITS-1:0]   op_predec
  );

  always @(*) begin
    op_predec = 'b0;
    if (start) begin
      case(op)
        `OP_C_FSTOI : op_predec[`FP_PREDEC_DST_SINT32_BIT] = 1'b1;
        `OP_C_FSTOD : op_predec[`FP_PREDEC_DST_FP64_BIT]   = 1'b1;
        `OP_C_FMOVS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FNEGS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FABSS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FSQRTS: op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FADDS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FSUBS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FMULS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FDIVS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FSMULD: op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FCMPS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FCMPES: op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;

        `OP_C_FDTOI : op_predec[`FP_PREDEC_DST_SINT32_BIT] = 1'b1;
        `OP_C_FDTOS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FMOVD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FNEGD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FABSD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FSQRTD: op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FADDD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FSUBD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FMULD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FDIVD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FCMPD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;
        `OP_C_FCMPED: op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;

        `OP_C_UMUL  : op_predec[`FP_PREDEC_DST_UINT64_BIT] = 1'b1;
        `OP_C_UDIV  : op_predec[`FP_PREDEC_DST_UINT32_BIT] = 1'b1;
        `OP_C_UDIVCC: op_predec[`FP_PREDEC_DST_UINT32_BIT] = 1'b1;
        `OP_C_UMULCC: op_predec[`FP_PREDEC_DST_UINT64_BIT] = 1'b1;

        `OP_C_FITOS : op_predec[`FP_PREDEC_DST_FP32_BIT] = 1'b1;
        `OP_C_FITOD : op_predec[`FP_PREDEC_DST_FP64_BIT] = 1'b1;

        `OP_C_CONCAT: op_predec[`FP_PREDEC_DST_UINT32_BIT] = 1'b1;
`ifndef ANUBIS_LOCAL_24
        `OP_C_MULSCC: op_predec[`FP_PREDEC_DST_UINT32_BIT] = 1'b1;
`else
        `OP_C_MULSCC: op_predec[`FP_PREDEC_DST_UINT32_BIT] = 1'b0;
`endif

        `OP_C_SMUL  : op_predec[`FP_PREDEC_DST_SINT64_BIT] = 1'b1;
        `OP_C_SDIV  : op_predec[`FP_PREDEC_DST_SINT32_BIT] = 1'b1;
        `OP_C_SDIVCC: op_predec[`FP_PREDEC_DST_SINT32_BIT] = 1'b1;
`ifndef ANUBIS_LOCAL_24
        `OP_C_SMULCC: op_predec[`FP_PREDEC_DST_SINT64_BIT] = 1'b1;
`else
        `OP_C_SMULCC: op_predec[`FP_PREDEC_DST_SINT64_BIT] = 1'b0;
`endif
        default: begin
          op_predec = 'bx;
        end
      endcase
    end
  end

endmodule
