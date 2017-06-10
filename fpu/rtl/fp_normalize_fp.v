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

 int_man requires a shift RIGHT (up to 63bits int_exp).

 fp_man requires rounding and at most one bit shift RIGHT (only if overflow).

 FIXME: substract can clear lots of bits. So a lead0 stage is required


****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

//import scoore_fpu::*;

module fp_normalize_fp
  (
   input [`FP_PREDEC_BITS-1:0]       op_predec,
   input                             sub_rnd_flag,
   input [2-1:0]                   round,
   input                             start,

   input                             sign,

   input [`FP_EXP_BITS-1:0]          fp_exp,
   input [`FP_MAN_BITS-1:0]          fp_man,
   input                             man_zero,
   // outputs
   output reg [`FP_TYPE_D_BITS-1:0]  fp_result_next,
   output reg                        fp_overflow_next
  );


  reg [40-1:0] round_increment;
  always_comb begin
    case({op_predec[`FP_PREDEC_DST_FP64_BIT],op_predec[`FP_PREDEC_DST_FP32_BIT],round})
      // 64 bits
      {2'b10,`FP_ROUND_NEAREST}: round_increment = 40'h0000000400;
      {2'b10,`FP_ROUND_ZERO}   : round_increment = 40'h0000000000;
      {2'b10,`FP_ROUND_PLUSINF}: round_increment = sign ? 40'h0000000000 : 40'h00000007FF;
      {2'b10,`FP_ROUND_MININF} : round_increment = sign ? 40'h00000007FF : 40'h0000000000;

      // 32 bits
      {2'b01,`FP_ROUND_NEAREST}: round_increment = 40'h8000000000;
      {2'b01,`FP_ROUND_ZERO}   : round_increment = 40'h0000000000;
      {2'b01,`FP_ROUND_PLUSINF}: round_increment = sign ? 40'h0000000000 : 40'hFFFFFFFFFF;
      {2'b01,`FP_ROUND_MININF} : round_increment = sign ? 40'hFFFFFFFFFF : 40'h0000000000;
      default: round_increment = 'b0;
    endcase
  end

  /*reg [`FP_MAN_BITS-1:0]    fp_right_jam_man;
  always @(*) begin
    if (fp_exp == `FP_EXP_BITS'b0) begin
      fp_right_jam_man = fp_man>>1;
    end else if (fp_exp < 64) begin
      fp_right_jam_man    = (fp_man>>fp_exp);
      fp_right_jam_man[0] = ((fp_right_jam_man<<fp_exp) != fp_man);
    end else if (fp_exp[`FP_EXP_BITS-1]) begin
      fp_right_jam_man = fp_man>>1;
      fp_right_jam_man[0] = (| fp_man);
    end else begin
      fp_right_jam_man = fp_man;
    end
  end*/

  reg [`FP_MAN_BITS+1-1:0]  fp_round_man;
  reg [`FP_EXP_BITS-1:0]    fp_exp_next;
  reg [11-1:0]              dp_round_bits;
  reg [41-1:0]              sp_round_bits;
  reg [`FP_DMAN_BITS-1:0]   fp_dp_man;
  reg [`FP_SMAN_BITS-1:0]   fp_sp_man;
  logic tmp;

  always_comb begin
    fp_round_man   = fp_man + round_increment;
    fp_exp_next    = fp_exp+fp_round_man[`FP_MAN_BITS];

    fp_dp_man      = fp_round_man[`FP_MAN_BITS-2:11];
    dp_round_bits  = fp_man[10:0];
    // FIXME: dp never hits this case--is it possible to hit?
    tmp = start && ((fp_exp == 0) && fp_round_man[`FP_MAN_BITS-1]);
    if ( tmp ) begin
      //$display("(fp_exp == 0) && fp_round_man[`FP_MAN_BITS-1] == 1");
      if ( (round == `FP_ROUND_PLUSINF && sign == 1'b0) || (round == `FP_ROUND_MININF && sign == 1'b1) ) begin  // FIXME: move this case so round_increment covers it
        fp_sp_man    = fp_round_man[`FP_MAN_BITS-1:41]+1;
      end else begin
        fp_sp_man    = fp_round_man[`FP_MAN_BITS-1:41];
      end
      sp_round_bits   = fp_man[40:0];
    end else begin
      /*if ( (fp_exp == 0) && (fp_round_man[`FP_MAN_BITS-1] == 0) ) begin
        $display("(fp_exp == 0) && (fp_round_man[`FP_MAN_BITS-1] == 0)");
      end else begin
        $display("fp_exp = %h", fp_exp);
      end*/
      fp_sp_man    = fp_round_man[`FP_MAN_BITS-2:40];
      sp_round_bits   = {fp_man[39:0], 1'b0};
    end

    // only appears in add64.  why?
    if (round == `FP_ROUND_NEAREST && dp_round_bits == 11'h400) begin
      /*if ( fp_dp_man[0] == 1 ) begin
        $display("clearing LSB of fp_dp_man!!! (state = %d)", state);
      end*/
      fp_dp_man[0] = 1'b0;
    end

    // only appears in add32, mul32.  why?
    if (round == `FP_ROUND_NEAREST && sp_round_bits == 41'h10000000000) begin
      /*if ( fp_sp_man[0] == 1 ) begin
        $display("clearing LSB of fp_sp_man!!! (state = %d)", state);
      end*/
      fp_sp_man[0] = 1'b0;
    end

 end

 wire sign_change;

 assign sign_change = (round == `FP_ROUND_MININF) && (man_zero) && (sub_rnd_flag);

  always_comb begin
    fp_result_next = `FP_TYPE_D_BITS'b0;

    case({op_predec[`FP_PREDEC_DST_FP64_BIT],op_predec[`FP_PREDEC_DST_FP32_BIT],sign_change})
      // 64bit output
      3'b100: begin
        fp_result_next[`FP_IEEE_DSIGN_FMT] = sign;
        fp_result_next[`FP_IEEE_DMAN_FMT]  = fp_dp_man;
        fp_result_next[`FP_IEEE_DEXP_FMT]  = fp_exp_next;
      end
      // 32bit output
      3'b010: begin
        fp_result_next[`FP_IEEE_SSIGN_FMT] = sign;
        fp_result_next[`FP_IEEE_SMAN_FMT]  = fp_sp_man;
        fp_result_next[`FP_IEEE_SEXP_FMT]  = fp_exp_next;
      end
      // 64bit output
      3'b101: begin
        fp_result_next[`FP_IEEE_DSIGN_FMT] = 1'b1;
        fp_result_next[`FP_IEEE_DMAN_FMT]  = fp_dp_man;
        fp_result_next[`FP_IEEE_DEXP_FMT]  = fp_exp_next;
      end
      // 32bit output
      3'b011: begin
        fp_result_next[`FP_IEEE_SSIGN_FMT] = 1'b1;
        fp_result_next[`FP_IEEE_SMAN_FMT]  = fp_sp_man;
        fp_result_next[`FP_IEEE_SEXP_FMT]  = fp_exp_next;
      end

      // nothing output
      default: begin
        // clock gate the output
        fp_result_next = `FP_TYPE_D_BITS'bx;
      end
    endcase
  end


  always @(*) begin
    fp_overflow_next = 1'b0;

    // FIXME: not considering single precision overflow?
    if (fp_exp > `FP_EXP_BITS'h7FD ||
        (fp_exp == `FP_EXP_BITS'h7FD && fp_round_man[`FP_MAN_BITS-1])
        ) begin
      // overflow
      fp_overflow_next = 1'b1;
    end

    // FIXME: handle underflow flag
    //if (fp_exp[`FP_EXP_BITS-1] && fp_exp >= `FP_EXP_BITS'h3FD) begin
    //end

  end
endmodule
