//==============================================================================
//      File:           $URL$
//      Version:        $Revision$
//      Author:         Jose Renau  (http://masc.cse.ucsc.edu/)
//                      Elnaz Ebrahimi
//      Copyright:      Copyright 2011 UC Santa Cruz
//==============================================================================

//==============================================================================
//      Section:        License
//==============================================================================
//      Copyright (c) 2011, Regents of the University of California
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


module stage_flop_retry
  #(parameter Size=1)
    (input             clk
     ,input            reset

     ,input   [Size-1:0]  din
     ,input            dinValid
     ,output reg       dinRetry

     ,output reg [Size-1:0]  q
     ,input            qRetry
     ,output reg       qValid

     // Re-clocking signals
     ,input  [4:0]  rci  
     ,output [4:0]  rco 
     );

  // {{{1 Private variable priv_*i, failure, and shadowq declaration
  // Output private signals
  reg [Size-1:0] shadowq;
  reg c1;
  reg c2;
  reg shadowValid;

  reg          priv_qValid;
  always @(*) begin
    qValid = priv_qValid;
  end
  reg          priv_dinRetry;
  always @(*) begin
    dinRetry = priv_dinRetry;
  end

  // Inputs private signals
  reg          priv_qRetry;
  always @(*) begin
    priv_qRetry = qRetry;
  end
  reg          priv_dinValid;
  always @(*) begin
    priv_dinValid = dinValid;
  end
  // 1}}}

  // {{{1 cond1 and cond2
  always @(*) begin
    c1 = (priv_qValid & priv_qRetry); // resend (even with failure, we can have a resend) 
    c2 = priv_dinValid | shadowValid; // pending
  end
  // 1}}}

  // {{{1 shadowValid
  always @(posedge clk) begin
    if (reset) begin
      shadowValid <= 'b0;
    end else begin
      shadowValid <= (c1 & c2);
    end 
  end 
  // 1}}}

  // {{{1 shadowq
  reg s_enable;
  always @(*) begin
    s_enable = !shadowValid;
  end

  always@ (posedge clk) begin
    if (s_enable) begin
      shadowq <= din;
    end
  end 
  // 1}}}

   // {{{1 q
  reg q_enable;
  always @(negedge clk) begin
    q_enable <= !c1; 
  end

  always @ (negedge clk) begin
      if (q_enable) begin
        q <= shadowq;
      end 
  end 
  // 1}}}

  // {{{1 priv_qValid (qValid internal value)
  reg priv_qValidla2;
  always @(posedge clk) begin
    if (reset) begin
      priv_qValidla2 <='b0;
    end else begin
      priv_qValidla2 <= (c1 | c2);
    end 
  end 


  always @(*) begin
    priv_qValid = priv_qValidla2;
  end
  // 1}}}

  // {{{1 priv_dinRetry (dinRetry internal value)

  always @(*) begin
   priv_dinRetry = shadowValid | reset;
  end
  // 1}}}


endmodule 
