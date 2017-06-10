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

`define NUM_DIVIDERS 4
`define COUNTER_BITS 2

module fp_divider_selector_comb
  (
   //inputs
   input                enable_div,
   input                nan_select,
   input                nan_prop_retry,
   input                divp_busy0,
   input                divp_busy1,
   input                divp_busy2,
   input                divp_busy3,

   //outputs
   output reg           divider0_start,
   output reg           divider1_start,
   output reg           divider2_start,
   output reg           divider3_start,
   output reg           sel_retry_out
  );

  reg [`COUNTER_BITS-1:0]    counter_next;
  wire [`COUNTER_BITS-1:0]   counter;
  reg                        start;

  always_comb begin
`ifndef ANUBIS_LOCAL_8
    start = enable_div & ~nan_select; // & ~nan_prop_retry; //valid only when previous ~previous_stage->retry
`else
    start = enable_div & ~nan_select & ~nan_prop_retry; //valid only when previous ~previous_stage->retry
`endif
  end

  always_comb begin
    sel_retry_out = divp_busy0 & divp_busy1 & divp_busy2 & divp_busy3;

    //if(start) begin
      if(~divp_busy0) begin
        divider0_start = start;
        divider1_start = 1'b0;
        divider2_start = 1'b0;
        divider3_start = 1'b0;
        //sel_retry_out  = 1'b0;
      end else if(~divp_busy1) begin
        divider0_start = 1'b0;
        divider1_start = start;
        divider2_start = 1'b0;
        divider3_start = 1'b0;
        //sel_retry_out  = 1'b0;
      end else if(~divp_busy2) begin
        divider0_start = 1'b0;
        divider1_start = 1'b0;
        divider2_start = start;
        divider3_start = 1'b0;
        //sel_retry_out  = 1'b0;
      end else if(~divp_busy3) begin
        divider0_start = 1'b0;
        divider1_start = 1'b0;
        divider2_start = 1'b0;
        divider3_start = start;
        //sel_retry_out  = 1'b0;
      end else begin
        divider0_start = 1'b0;
        divider1_start = 1'b0;
        divider2_start = 1'b0;
        divider3_start = 1'b0;
        //sel_retry_out  = 1'b1;
      end
    /*end else begin
      divider0_start = 1'b0;
      divider1_start = 1'b0;
      divider2_start = 1'b0;
      divider3_start = 1'b0;
      //sel_retry_out  = 1'b0;
    end */
  end
endmodule
