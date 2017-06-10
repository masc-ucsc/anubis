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

  COMBINATIONAL block: Looks for the first possition with a 1.
 
****************************************************************************/

`include "dfuncs.h"

`define BITS 3
`define FAST 0

module fp_lead0
  #(parameter Bits=64, Fast=0) 
    (input [Bits-1:0]          data
     ,output reg [5:0]         lead0
     ,output reg [Bits-1:0]    normalized
     );

  reg [63:0] datai;

  always @(*) begin
    datai = 64'b1;
    datai[63:64-Bits] = data;
  end
  
  reg [5:0]         lead0b;
  always @(*) begin
    casez(datai[29:0])
      30'b1?????????????????????????????: begin lead0b = 6'd34; end
      30'b01????????????????????????????: begin lead0b = 6'd35; end
      30'b001???????????????????????????: begin lead0b = 6'd36; end
      30'b0001??????????????????????????: begin lead0b = 6'd37; end
      30'b00001?????????????????????????: begin lead0b = 6'd38; end
      30'b000001????????????????????????: begin lead0b = 6'd39; end
      30'b0000001???????????????????????: begin lead0b = 6'd40; end
      30'b00000001??????????????????????: begin lead0b = 6'd41; end
      30'b000000001?????????????????????: begin lead0b = 6'd42; end
      30'b0000000001????????????????????: begin lead0b = 6'd43; end
      30'b00000000001???????????????????: begin lead0b = 6'd44; end
      30'b000000000001??????????????????: begin lead0b = 6'd45; end
      30'b0000000000001?????????????????: begin lead0b = 6'd46; end
      30'b00000000000001????????????????: begin lead0b = 6'd47; end
      30'b000000000000001???????????????: begin lead0b = 6'd48; end
      30'b0000000000000001??????????????: begin lead0b = 6'd49; end
      30'b00000000000000001?????????????: begin lead0b = 6'd50; end
      30'b000000000000000001????????????: begin lead0b = 6'd51; end
      30'b0000000000000000001???????????: begin lead0b = 6'd52; end
      30'b00000000000000000001??????????: begin lead0b = 6'd53; end
      30'b000000000000000000001?????????: begin lead0b = 6'd54; end
      30'b0000000000000000000001????????: begin lead0b = 6'd55; end
      30'b00000000000000000000001???????: begin lead0b = 6'd56; end
      30'b000000000000000000000001??????: begin lead0b = 6'd57; end
      30'b0000000000000000000000001?????: begin lead0b = 6'd58; end
      30'b00000000000000000000000001????: begin lead0b = 6'd59; end
      30'b000000000000000000000000001???: begin lead0b = 6'd60; end
      30'b0000000000000000000000000001??: begin lead0b = 6'd61; end
      30'b00000000000000000000000000001?: begin lead0b = 6'd62; end
      30'b000000000000000000000000000000: begin lead0b = 6'd0; end
      30'b000000000000000000000000000001: begin lead0b = 6'd63; end
    endcase
  end

  always @(*) begin
    casez(datai[63:30])
      34'b1?????????????????????????????????: begin lead0 = 6'd0; end
      34'b01????????????????????????????????: begin lead0 = 6'd1; end
      34'b001???????????????????????????????: begin lead0 = 6'd2; end
      34'b0001??????????????????????????????: begin lead0 = 6'd3; end
      34'b00001?????????????????????????????: begin lead0 = 6'd4; end
      34'b000001????????????????????????????: begin lead0 = 6'd5; end
      34'b0000001???????????????????????????: begin lead0 = 6'd6; end
      34'b00000001??????????????????????????: begin lead0 = 6'd7; end
      34'b000000001?????????????????????????: begin lead0 = 6'd8; end
      34'b0000000001????????????????????????: begin lead0 = 6'd9; end
      34'b00000000001???????????????????????: begin lead0 = 6'd10; end
      34'b000000000001??????????????????????: begin lead0 = 6'd11; end
      34'b0000000000001?????????????????????: begin lead0 = 6'd12; end
      34'b00000000000001????????????????????: begin lead0 = 6'd13; end
      34'b000000000000001???????????????????: begin lead0 = 6'd14; end
      34'b0000000000000001??????????????????: begin lead0 = 6'd15; end
      34'b00000000000000001?????????????????: begin lead0 = 6'd16; end
      34'b000000000000000001????????????????: begin lead0 = 6'd17; end
      34'b0000000000000000001???????????????: begin lead0 = 6'd18; end
      34'b00000000000000000001??????????????: begin lead0 = 6'd19; end
      34'b000000000000000000001?????????????: begin lead0 = 6'd20; end
      34'b0000000000000000000001????????????: begin lead0 = 6'd21; end
      34'b00000000000000000000001???????????: begin lead0 = 6'd22; end
      34'b000000000000000000000001??????????: begin lead0 = 6'd23; end
      34'b0000000000000000000000001?????????: begin lead0 = 6'd24; end
      34'b00000000000000000000000001????????: begin lead0 = 6'd25; end
      34'b000000000000000000000000001???????: begin lead0 = 6'd26; end
      34'b0000000000000000000000000001??????: begin lead0 = 6'd27; end
      34'b00000000000000000000000000001?????: begin lead0 = 6'd28; end
      34'b000000000000000000000000000001????: begin lead0 = 6'd29; end
      34'b0000000000000000000000000000001???: begin lead0 = 6'd30; end
      34'b00000000000000000000000000000001??: begin lead0 = 6'd31; end
      34'b000000000000000000000000000000001?: begin lead0 = 6'd32; end
      34'b0000000000000000000000000000000001: begin lead0 = 6'd33; end
      34'b0000000000000000000000000000000000: begin lead0 = lead0b; end
    endcase
  end
  
  reg [Bits-1:0]          data2;

  generate
    if (Fast) begin
      // fast but area and power hungry (x2 times more area, just 1/2 cycle). Only
      // worth it if the lead0 becomes a critical path (fp_normalize)
      reg [Bits-1:0]    data2b;
      always @(*) begin
        casez(datai[29:0])
          30'b1?????????????????????????????: begin data2b = datai<<34; end
          30'b01????????????????????????????: begin data2b = datai<<35; end
          30'b001???????????????????????????: begin data2b = datai<<36; end
          30'b0001??????????????????????????: begin data2b = datai<<37; end
          30'b00001?????????????????????????: begin data2b = datai<<38; end
          30'b000001????????????????????????: begin data2b = datai<<39; end
          30'b0000001???????????????????????: begin data2b = datai<<40; end
          30'b00000001??????????????????????: begin data2b = datai<<41; end
          30'b000000001?????????????????????: begin data2b = datai<<42; end
          30'b0000000001????????????????????: begin data2b = datai<<43; end
          30'b00000000001???????????????????: begin data2b = datai<<44; end
          30'b000000000001??????????????????: begin data2b = datai<<45; end
          30'b0000000000001?????????????????: begin data2b = datai<<46; end
          30'b00000000000001????????????????: begin data2b = datai<<47; end
          30'b000000000000001???????????????: begin data2b = datai<<48; end
          30'b0000000000000001??????????????: begin data2b = datai<<49; end
          30'b00000000000000001?????????????: begin data2b = datai<<50; end
          30'b000000000000000001????????????: begin data2b = datai<<51; end
          30'b0000000000000000001???????????: begin data2b = datai<<52; end
          30'b00000000000000000001??????????: begin data2b = datai<<53; end
          30'b000000000000000000001?????????: begin data2b = datai<<54; end
          30'b0000000000000000000001????????: begin data2b = datai<<55; end
          30'b00000000000000000000001???????: begin data2b = datai<<56; end
          30'b000000000000000000000001??????: begin data2b = datai<<57; end
          30'b0000000000000000000000001?????: begin data2b = datai<<58; end
          30'b00000000000000000000000001????: begin data2b = datai<<59; end
          30'b000000000000000000000000001???: begin data2b = datai<<60; end
          30'b0000000000000000000000000001??: begin data2b = datai<<61; end
          30'b00000000000000000000000000001?: begin data2b = datai<<62; end
          30'b00000000000000000000000000000?: begin data2b = datai<<63; end
        endcase
      end
  
      always @(*) begin
        casez(datai[63:30])
          34'b1?????????????????????????????????: begin data2 = datai;    end
          34'b01????????????????????????????????: begin data2 = datai<<1; end
          34'b001???????????????????????????????: begin data2 = datai<<2; end
          34'b0001??????????????????????????????: begin data2 = datai<<3; end
          34'b00001?????????????????????????????: begin data2 = datai<<4; end
          34'b000001????????????????????????????: begin data2 = datai<<5; end
          34'b0000001???????????????????????????: begin data2 = datai<<6; end
          34'b00000001??????????????????????????: begin data2 = datai<<7; end
          34'b000000001?????????????????????????: begin data2 = datai<<8; end
          34'b0000000001????????????????????????: begin data2 = datai<<9; end
          34'b00000000001???????????????????????: begin data2 = datai<<10; end
          34'b000000000001??????????????????????: begin data2 = datai<<11; end
          34'b0000000000001?????????????????????: begin data2 = datai<<12; end
          34'b00000000000001????????????????????: begin data2 = datai<<13; end
          34'b000000000000001???????????????????: begin data2 = datai<<14; end
          34'b0000000000000001??????????????????: begin data2 = datai<<15; end
          34'b00000000000000001?????????????????: begin data2 = datai<<16; end
          34'b000000000000000001????????????????: begin data2 = datai<<17; end
          34'b0000000000000000001???????????????: begin data2 = datai<<18; end
          34'b00000000000000000001??????????????: begin data2 = datai<<19; end
          34'b000000000000000000001?????????????: begin data2 = datai<<20; end
          34'b0000000000000000000001????????????: begin data2 = datai<<21; end
          34'b00000000000000000000001???????????: begin data2 = datai<<22; end
          34'b000000000000000000000001??????????: begin data2 = datai<<23; end
          34'b0000000000000000000000001?????????: begin data2 = datai<<24; end
          34'b00000000000000000000000001????????: begin data2 = datai<<25; end
          34'b000000000000000000000000001???????: begin data2 = datai<<26; end
          34'b0000000000000000000000000001??????: begin data2 = datai<<27; end
          34'b00000000000000000000000000001?????: begin data2 = datai<<28; end
          34'b000000000000000000000000000001????: begin data2 = datai<<29; end
          34'b0000000000000000000000000000001???: begin data2 = datai<<30; end
          34'b00000000000000000000000000000001??: begin data2 = datai<<31; end
          34'b000000000000000000000000000000001?: begin data2 = datai<<32; end
          34'b0000000000000000000000000000000001: begin data2 = datai<<33; end
          34'b0000000000000000000000000000000000: begin data2 = data2b; end
        endcase
        normalized = data2;
      end
    end else begin
      always @(*) begin
        data2 = datai << lead0;
`ifndef ANUBIS_NOC_0
        normalized = data2;
`else
        normalized = ~data2;
`endif
      end
    end
  endgenerate
    
endmodule
