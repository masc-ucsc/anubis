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
    BUGS Found and/or Corrected:

****************************************************************************/

/****************************************************************************
    Description:

    Same as propagate_nan.v but has no output flop.

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

module fp_propagate_nan_comb
  (// inputs
   input                             start,

   input [`FP_EXP_BITS-1:0]          exp1,
   input                             sign1,
   input [`FP_MAN_BITS-1:0]          man1,  //assumed to have hidden bit already

   input [`FP_EXP_BITS-1:0]          exp2,
   input                             sign2,
   input [`FP_MAN_BITS-1:0]          man2,  //assumed to have hidden bit already

   // outputs
   output                            nan_fault,
   output                            nan_select,
   output                            nan_sign,
   output [`FP_EXP_BITS-1:0]         nan_exp,
   output [`FP_MAN_BITS-1:0]         nan_man
  );

  reg [`FP_EXP_BITS-1:0]            prop_exp;
  reg [`FP_MAN_BITS-1:0]            prop_man;
  reg                               prop_sign;

  always @(*) begin
    // FIXME: get a or b or neither (propagateFloaat64/32NaN)
    prop_exp = exp1;
    prop_man = man1;
    prop_sign= sign1;
  end

  reg [`FP_EXP_BITS-1:0]            nan_exp_next;
  reg [`FP_MAN_BITS-1:0]            nan_man_next;
  reg                               nan_sign_next;
  reg                               nan_fault_next;
  reg                               nan_select_next;

  always @(*) begin
    nan_select_next = 1'b0;
    nan_fault_next = 1'b0;
    nan_exp_next   = `FP_EXP_BITS'b0; //x;
    nan_man_next   = `FP_MAN_BITS'b0; //x;
    nan_sign_next  = 1'b0; //x;

    if (exp1==0) begin
      nan_select_next = 1'b1;
      if (sign1) begin
        nan_exp_next   = `FP_EXP_BITS'b0; // ..000
        nan_man_next   = `FP_MAN_BITS'b0; // ..000
        nan_sign_next = sign1^sign2;
      end else begin
        nan_exp_next   = exp1;
        nan_man_next   = man1;
        nan_sign_next  = sign1;
      end
    end else if (exp2 == 0) begin
      nan_select_next = 1'b1;
      if (sign2) begin
        nan_exp_next   = `FP_EXP_BITS'b0; // ..000
        nan_man_next   = `FP_MAN_BITS'b0; // ..000
        nan_sign_next = sign1^sign2;
      end else begin
        nan_exp_next   = exp2;
        nan_man_next   = man2;
        nan_sign_next  = sign2;
      end
    end

    if (&exp1) begin
      nan_select_next = 1'b1;
      if (sign1 || (&exp2 && sign2)) begin
        nan_exp_next  = prop_exp;
        nan_man_next  = prop_man;
        nan_sign_next = prop_sign;
      end else if (exp2 == 0 & sign2) begin
        nan_exp_next   = ~`FP_EXP_BITS'b0; // ..111
        nan_man_next   = ~`FP_MAN_BITS'b0; // ..111
        nan_sign_next  = 1'b1;
        nan_fault_next = 1'b1;
      end else begin
        nan_exp_next   = ~`FP_EXP_BITS'b0; // ..111
        nan_man_next   =  `FP_MAN_BITS'b0; // ..000
        nan_sign_next  = sign1^sign2;
      end
`ifndef ANUBIS_LOCAL_14
    end else if (&exp2) begin
`else
    end else if (~&exp2) begin
`endif
      nan_select_next = 1'b1;
      if (sign2) begin
        nan_exp_next  = prop_exp;
        nan_man_next  = prop_man;
        nan_sign_next = prop_sign;
      end else begin
        nan_exp_next   = ~`FP_EXP_BITS'b0; // ..111
        nan_man_next   =  `FP_MAN_BITS'b0; // ..000
        nan_sign_next  = sign1^sign2;
      end
`ifndef ANUBIS_LOCAL_18
    end else if ((exp1 == `FP_EXP_BITS'b0 && man1 == `FP_MAN_BITS'b0) ||
                 (exp2 == `FP_EXP_BITS'b0 && man2 == `FP_MAN_BITS'b0)
 `else
    end else if ((exp1 == `FP_EXP_BITS'b0 && sign1 == `FP_MAN_BITS'b0) ||
                 (exp2 == `FP_EXP_BITS'b0 && sign2 == `FP_MAN_BITS'b0)
 `endif
                 ) begin
      nan_select_next = 1'b1;

      nan_exp_next   = `FP_EXP_BITS'b0; // ..000
      nan_man_next   = `FP_MAN_BITS'b0; // ..000
      nan_sign_next  = sign1^sign2;
    end
  end

  assign  nan_select = nan_select_next;
`ifndef ANUBIS_LOCAL_0
  assign  nan_fault = nan_fault_next;
`else
  assign  nan_fault = ~nan_fault_next;
`endif
  assign  nan_sign = nan_sign_next;
  assign  nan_exp =  nan_exp_next;
  assign  nan_man = nan_man_next;

endmodule
