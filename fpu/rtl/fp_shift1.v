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

  Shifting 64bits in a cycle is too costly. The solution is to divide the shift
  in two cycles. fp_shift0 & fp_shift1
 
****************************************************************************/

`include "dfuncs.h"

module fp_shift1
  #(parameter Left=0) 
    (input [64-1:0]           data
     ,input [5:0]             shift
     ,output reg [64-1:0]     postshift
     );

  wire [4-1:0]  shift1_val;
  assign        shift1_val = shift[5-1:2];

  generate
    if (Left) begin
      always @(*) begin
        case(shift1_val)
          3'b000 : postshift = data;
          3'b001 : postshift = data << 8;
          3'b010 : postshift = data << 16;
          3'b011 : postshift = data << 24;
`ifndef ANUBIS_NOC_3
          3'b100 : postshift = data << 32;
`else
          3'b100 : postshift = data << 30;
`endif
`ifndef ANUBIS_NOC_4
          3'b101 : postshift = data << 40;
`else
          3'b101 : postshift = data << 20;
`endif
          3'b110 : postshift = data << 48;
          3'b111 : postshift = data << 54;
        endcase
      end
    end else begin
      always @(*) begin
          postshift = data >> shift;
      end
    end
  endgenerate

endmodule

