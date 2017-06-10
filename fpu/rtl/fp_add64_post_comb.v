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

  Half a cycle post-add operations (pure combinational, no clock).
 
****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//import scoore_fpu::*;

module fp_add64_post_comb
  (
   // inputs
   input                             addp_ready,
   input  [`FP_PREDEC_BITS-1:0]      addp_op_predec,
   input  [`FP_STATE_BITS-1:0]       addp_state,
   input  [2-1:0]                  addp_round,
   
   input                             addp_sign,
   input  [`FP_EXP_BITS-1:0]         addp_exp,
   input  [`FP_MAN_BITS-1:0]         addp_man,

   // outputs
   output                            add_ready,
   output [`FP_PREDEC_BITS-1:0]      add_op_predec,
   output [`FP_STATE_BITS-1:0]       add_state,
   output reg [2-1:0]                add_round,
   
   output                            add_sign,
   output [`FP_EXP_BITS-1:0]         add_exp,
   output [`FP_MAN_BITS-1:0]         add_man
  );

  wire [5:0]                  add_man_lead0;
  wire [`FP_MAN_BITS-1:0]     add_man_normalized;
  fp_lead0 #(.Bits(`FP_MAN_BITS)
             ,.Fast(1)
             )
    add_norm
      (.data       (addp_man),
       .lead0      (add_man_lead0),
       .normalized (add_man_normalized)
       );

  assign                      add_ready     = addp_ready;
  assign                      add_op_predec = addp_op_predec;
  assign                      add_state     = addp_state;
  always_comb begin
    add_round     = addp_round;
  end
  assign                      add_sign      = addp_sign;
  
  assign                      add_exp       = addp_exp-add_man_lead0;
  assign                      add_man       = add_man_normalized;
                      
endmodule
                                   
