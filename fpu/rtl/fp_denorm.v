//==============================================================================
//      File:           $URL$
//      Version:        $Revision$
//      Author:         Jose Renau  (http://masc.cse.ucsc.edu/)
//                      John Burr 
//                      - Pranav (stage impl)
//
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
 
 fp_denorm requires two steps (single cycle): select inputs, operate over the
 inputs

 A higher frequency may partition this in two stages (fp_deform_fix should be
 partitioned too)

 Note: restores hidden bit to mantissa field.
 
****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"

//import scoore_fpu::*;
//import retry_common::*;

module fp_denorm
  ( input                              clk,
    input                              reset,
    
    input                              start,
    input [`FP_STATE_BITS-1:0]         state,
    
    input [`OP_TYPE_BITS-1:0]          op,
    input [2-1:0]                    round,
    
    input [`FP_TYPE_D_BITS-1:0]        src1,
    input [`FP_TYPE_D_BITS-1:0]        src2,

    input                              retry_add,
    input                              retry_mult,
    input                              retry_div,
    input                              retry_sqrt,
    input                              retry_misc,

    
    // outputs
    output [`FP_PREDEC_BITS-1:0]       denorm_op_predec_add,
    output [2-1:0]                   denorm_round_add,
    output [`FP_EXP_BITS-1:0]          denorm_exp_add,
    output                             denorm_sign1_add,
    output [`FP_MAN_BITS-1:0]          denorm_man1_add,
    output                             denorm_sign2_add,
    output [`FP_MAN_BITS-1:0]          denorm_man2_add,
    output [`FP_STATE_BITS-1:0]        denorm_state_add,               //need individual outputs for all func units
    
    output [`FP_PREDEC_BITS-1:0]       denorm_op_predec_mult,
    output [2-1:0]                   denorm_round_mult,
    output [`FP_EXP_BITS-1:0]          denorm_exp_mult,
    output [`FP_EXP_BITS-1:0]          denorm_exp1_mult,
    output                             denorm_sign1_mult,
    output [`FP_MAN_BITS-1:0]          denorm_man1_mult,
    output [`FP_EXP_BITS-1:0]          denorm_exp2_mult,
    output                             denorm_sign2_mult,
    output [`FP_MAN_BITS-1:0]          denorm_man2_mult,
    output [`FP_STATE_BITS-1:0]        denorm_state_mult,               //need individual outputs for all func units
    
    
    output [`FP_PREDEC_BITS-1:0]       denorm_op_predec_div,
    output [2-1:0]                   denorm_round_div,
    output [`FP_EXP_BITS-1:0]          denorm_exp_div,
    output [`FP_EXP_BITS-1:0]          denorm_exp1_div,
    output                             denorm_sign1_div,
    output [`FP_MAN_BITS-1:0]          denorm_man1_div,
    output [`FP_EXP_BITS-1:0]          denorm_exp2_div,
    output                             denorm_sign2_div,
    output [`FP_MAN_BITS-1:0]          denorm_man2_div,
    output [`FP_STATE_BITS-1:0]        denorm_state_div,               //need individual outputs for all func units
    
    output [`FP_PREDEC_BITS-1:0]       denorm_op_predec_sqrt,
    output [2-1:0]                   denorm_round_sqrt,
    output [`FP_EXP_BITS-1:0]          denorm_exp_sqrt,
    output                             denorm_sign2_sqrt,
    output [`FP_MAN_BITS-1:0]          denorm_man2_sqrt,
    output [`FP_STATE_BITS-1:0]        denorm_state_sqrt,               //need individual outputs for all func units
    output                             denorm_sqrt_shift,

    output [`FP_PREDEC_BITS-1:0]       denorm_op_predec_misc,
    output [2-1:0]                   denorm_round_misc,
    output [`FP_EXP_BITS-1:0]          denorm_exp_misc,
    output [`FP_EXP_BITS-1:0]          denorm_exp1_misc,
    output                             denorm_sign1_misc,
    output [`FP_MAN_BITS-1:0]          denorm_man1_misc,
    output [`FP_EXP_BITS-1:0]          denorm_exp2_misc,
    output                             denorm_sign2_misc,
    output [`FP_MAN_BITS-1:0]          denorm_man2_misc,
    output [`FP_STATE_BITS-1:0]        denorm_state_misc,               //need individual outputs for all func units
    
    output                             denorm_enable_add,
    output                             denorm_sub_rnd_flag,
    output                             denorm_enable_mult,
    output                             denorm_enable_div,
    output                             denorm_enable_misc,
    output                             denorm_enable_sqrt,
    
    output                             denorm_overflow_mult,
    output                             denorm_rshift_man_mult,
    output                             denorm_overflow_div,
    output                             denorm_rshift_man_div,
    
    output [6-1:0]                     denorm_exp1_m_exp2_add,
    

    output logic                       out_retry,

    //Retry Re-clocking signals
    input  [4:0]                 rci,
    output [4:0]                 rco 
   );

   reg                             out_retry_add;
   reg                             out_retry_mult;
   reg                             out_retry_div;
   reg                             out_retry_sqrt;

  reg                                  in_start;
  reg [`FP_STATE_BITS-1:0]             in_state;
  reg [`OP_TYPE_BITS-1:0]              in_op;
  reg [2-1:0]                          in_round;
  reg [`FP_MAN_BITS-1:0]               in_man1;
  reg [`FP_EXP_BITS-1:0]               in_exp1;
  reg                                  in_sign1;
  reg [`FP_MAN_BITS-1:0]               in_man2;
  reg [`FP_EXP_BITS-1:0]               in_exp2;
  reg                                  in_sign2;


  wire [4:0] rco1; 
  wire [4:0] rco2; 
  wire [4:0] rco3; 

  always_comb begin
    in_state = state;
    in_op    = op;
    in_round = round;
    
    in_sign1  = 'bx;
    in_exp1   = `FP_EXP_BITS'b0;
    in_man1   = `FP_MAN_BITS'b0;
        
    in_sign2  = 'bx;
    in_exp2   = `FP_EXP_BITS'b0;
    in_man2   = `FP_MAN_BITS'b0;
    if (start) begin
      case(op[`OP_C_TYPE_SRC_FMT])
        // 32bit FP
        `OP_C_32FP_SRC: begin
          in_start = 'b1;
          
          in_sign1 = src1[`FP_IEEE_SSIGN_FMT];
          in_exp1  = src1[`FP_IEEE_SEXP_FMT];
          in_man1[`FP_SMAN_FMT] = src1[`FP_IEEE_SMAN_FMT];
          in_man1[`FP_SRND_FMT] = 3'b000;
  
          in_sign2 = src2[`FP_IEEE_SSIGN_FMT];
          in_exp2  = src2[`FP_IEEE_SEXP_FMT];
        //in_man2  = src2[`FP_IEEE_SMAN_FMT];  // any reason this could be here?
          in_man2[`FP_SMAN_FMT] = src2[`FP_IEEE_SMAN_FMT];
          in_man2[`FP_SRND_FMT] = 3'b000;
  
          // only hidden bit if value is different from 0
          in_man1[`FP_HIDDEN_FMT] = |src1[`FP_IEEE_SMAN_FMT] | (|in_exp1);
          in_man2[`FP_HIDDEN_FMT] = |src2[`FP_IEEE_SMAN_FMT] | (|in_exp2);
        end
        // 64bit FP
        `OP_C_64FP_SRC: begin
          in_start = 1;
          
          in_sign1 = src1[`FP_IEEE_DSIGN_FMT];
          in_exp1  = src1[`FP_IEEE_DEXP_FMT];
          in_man1[`FP_DMAN_FMT] = src1[`FP_IEEE_DMAN_FMT];
          in_man1[`FP_DRND_FMT] = 3'b000;
  
          in_sign2 = src2[`FP_IEEE_DSIGN_FMT];
          in_exp2  = src2[`FP_IEEE_DEXP_FMT];
          in_man2[`FP_DMAN_FMT] = src2[`FP_IEEE_DMAN_FMT];
          in_man2[`FP_DRND_FMT] = 3'b000;
  
          in_man1[`FP_HIDDEN_FMT] = |src1[`FP_IEEE_DMAN_FMT] | (|in_exp1);
          in_man2[`FP_HIDDEN_FMT] = |src2[`FP_IEEE_DMAN_FMT] | (|in_exp2);
        end
        // 32bit UNIT
        //`OP_C_32SINT_SRC,
         // Fix for Signed Integer
         //
         //
        `OP_C_32SINT_SRC: begin
          in_start = 1;
          
          if ( ^op[1:0] ) begin /* div */
            in_exp1 = 'h3ff;
            in_exp2 = 'h3ff;        
            // 64bits
            in_sign1 = src1[(2*`FP_TYPE_I_BITS)-1];
            // My Fix: since it's a signed number, need to do 2's complement if negative
            if ( in_sign1) begin
              in_man1[`FP_MAN_BITS-1:0] = ~src1[(2*`FP_TYPE_I_BITS)-1:0] + 1;
            end else begin
              in_man1[`FP_MAN_BITS-1:0] = src1[(2*`FP_TYPE_I_BITS)-1:0];
            end
          end else begin
            // 32bits
            in_sign1 = src1[`FP_TYPE_I_BITS-1];
            // My Fix: since it's a signed number, need to do 2's complement if negetive
            if ( in_sign1) begin
              in_man1[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = ~src1[`FP_TYPE_I_BITS-1:0] + 1; 
            end else begin
              in_man1[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = src1[`FP_TYPE_I_BITS-1:0]; 
            end
          end
          
          in_sign2 = src2[`FP_TYPE_I_BITS-1];
          // My Fix: since it's a signed number, need to do 2's complement if negetive
          if ( in_sign2) begin
            in_man2[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = ~src2[`FP_TYPE_I_BITS-1:0] + 1;   
          end else begin
            in_man2[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = src2[`FP_TYPE_I_BITS-1:0];   
          end
        end
        //End Fix for Signed Integer
  
        `OP_C_32UINT_SRC: begin
          in_start = 1;
  
          if ( ^op[1:0] ) begin /* div */
            in_exp1 = 'h3ff;
            in_exp2 = 'h3ff;        
            // 64bits
            in_sign1 = src1[(2*`FP_TYPE_I_BITS)-1];
            in_man1[`FP_MAN_BITS-1:0] = src1[(2*`FP_TYPE_I_BITS)-1:0];
  
          end else begin
            // 32bits
            in_sign1 = src1[`FP_TYPE_I_BITS-1];
            in_man1[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = src1[`FP_TYPE_I_BITS-1:0];
          end
          
          in_sign2 = in_man2[`FP_MAN_BITS-1];
          in_man2[`FP_MAN_BITS-1:`FP_MAN_BITS/2] = src2[`FP_TYPE_I_BITS-1:0];
          
  
          /*if (start) begin
            $display("denorm: OP_C_32UINT_SRC:\n\tin_man1 = %h\n\tin_man2 = %h", in_man1, in_man2);
          end*/
        end
        // Default
        default: begin
          in_start = 0;
        end
      endcase
    end else begin
      in_start = 0;
    end
  end
  
  wire                       dn_start;
  wire [`FP_STATE_BITS-1:0]  dn_state;
  wire [2-1:0]                  dn_round;
  wire [`FP_MAN_BITS-1:0]    dn_man1;
  wire [`FP_EXP_BITS-1:0]    dn_exp1;
  wire                       dn_sign1;
  wire [`FP_MAN_BITS-1:0]    dn_man2;
  wire [`FP_EXP_BITS-1:0]    dn_exp2;
  wire                       dn_sign2;
  
  assign                     dn_start  = in_start;
  assign                     dn_state  = in_state;
  assign                     dn_round  = in_round;
  
  assign                     dn_sign1  = in_sign1;
  assign                     dn_exp1   = in_exp1;
  assign                     dn_man1   = in_man1;
  
  assign                     dn_sign2  = in_sign2;
  assign                     dn_exp2   = in_exp2;
  assign                     dn_man2   = in_man2;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // STEP 2: Operate over the inputs
  
  //------------------------
  // Breakdown input in exp,mantisa,sig
  reg [2-1:0]                denorm_round_next;
  
  reg                        denorm_sign1_next;
  reg [`FP_MAN_BITS-1:0]     denorm_man1_next;
  
  reg                        denorm_sign2_next;
  reg [`FP_MAN_BITS-1:0]     denorm_man2_next;
  
  reg [`FP_EXP_BITS-1:0]     denorm_exp_next;
  reg [`FP_STATE_BITS-1:0]   denorm_state_next;
  reg                        denorm_enable_add_next;
  reg                        denorm_sub_rnd_flag_next;
  reg                        denorm_enable_mult_next;
  reg                        denorm_enable_div_next;
  reg                        denorm_enable_misc_next;
  reg                        denorm_enable_sqrt_next;
  
  reg [6-1:0]                denorm_exp1_m_exp2_next;
  
  reg                        dn_exp1_lt_exp2;
  always_comb begin
    dn_exp1_lt_exp2 = dn_exp1 < dn_exp2;
  end
  
  reg [`FP_EXP_BITS+1-1:0]   tmp_exp;
  reg                        denorm_rshift_man_next;
  reg                        denorm_overflow_next;
  reg                        denorm_sqrt_shift_next;
  
  always_comb begin
    out_retry = (denorm_enable_mult_next &&  out_retry_mult) ||
                (denorm_enable_div_next  &&  out_retry_div ) ||
                (denorm_enable_add_next  &&  out_retry_add ) ||
                (denorm_enable_sqrt_next &&  out_retry_sqrt);
                // (denorm_enable_misc_next &&  out_retry_misc); FIXME: Pending
  end

  always_comb begin
    tmp_exp           = 'bx;

    denorm_round_next = dn_round;

    denorm_state_next = dn_state;
    denorm_man1_next  = dn_man1;
    denorm_man2_next  = dn_man2;
    
    denorm_exp1_m_exp2_next = 'bx;
    denorm_rshift_man_next  = 'b0;
    denorm_overflow_next    = 'b0;
    denorm_sqrt_shift_next  = 'bx;

    case (op)
      //----------------------
      // 32-bit MULT (always 64bit internally)
      `OP_C_FSMULD, // untested
      // cmps are like a substract
      `OP_C_FCMPES, // FIXME: cmps are a substract
      `OP_C_FMULS : begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;

        // getting underflow, but isn't overflow possible too?
        {denorm_rshift_man_next, tmp_exp} = {2'b0, dn_exp1} + {2'b0, dn_exp2} - 13'h007f;  //FIXME: use smaller adder?
        denorm_exp_next = tmp_exp;
        
        if ( (~denorm_rshift_man_next & tmp_exp[8]) | (denorm_exp_next==`FP_EXP_BITS'h0ff) ) begin  // any way to not hardcode 8?
          denorm_overflow_next = 1'b1;
        end
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = dn_start;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      //----------------------
      // 64-bit MULT (always 64bit internally)
      `OP_C_UMUL, // Tested by Elnaz
      `OP_C_UMULCC, // untested
      `OP_C_SMUL, // Tested by Elnaz
      `OP_C_SMULCC : begin // untested
      
      // FIXME: SMUL should work with just a minor change: sign extend 32 bit inputs to 64 bits internally
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;
        
        denorm_exp_next = `FP_EXP_BITS'b0;
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = dn_start;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      // 64-bit MULT (always 64bit internally)
      // cmps are like a substract
      `OP_C_FCMPD, `OP_C_FCMPED, // untested
      // FIXME: cmps are a substract
      `OP_C_FMULD : begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;

        // isn't it possible to overflow or underflow here?
        tmp_exp = dn_exp1 + dn_exp2 - 11'h3ff;
        denorm_exp_next   = tmp_exp;
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = dn_start;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      //----------------------
      // 32bit DIV (always 64bit internally)
      `OP_C_FDIVS: begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;

        {denorm_rshift_man_next , tmp_exp} = {2'b0, dn_exp1} - {2'b0, dn_exp2} + 13'h007f;
        denorm_exp_next   = tmp_exp;
        
        /*if ( ~denorm_rshift_man_next & tmp_exp[`FP_EXP_BITS+1-1] ) begin
          denorm_overflow_next = 1'b1;
        end*/
        if ( (~denorm_rshift_man_next & tmp_exp[8]) | (denorm_exp_next==`FP_EXP_BITS'h0ff) ) begin  // any way to not hardcode 8?
          denorm_overflow_next = 1'b1;
        end
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = dn_start;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      //----------------------
      // 64bit DIV (always 64bit internally)
      `OP_C_UDIV, // Tested by Elnaz
      `OP_C_SDIV, // Tested by Elnaz
      `OP_C_UDIVCC, // untested
      `OP_C_SDIVCC, // untested
      `OP_C_FDIVD : begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;

        {denorm_rshift_man_next , tmp_exp} = {2'b0, dn_exp1} - {2'b0, dn_exp2} + 13'h03ff;
        denorm_exp_next   = tmp_exp;
        
        if ( ~denorm_rshift_man_next & tmp_exp[`FP_EXP_BITS+1-1] ) begin
          denorm_overflow_next = 1'b1;
        end
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = dn_start;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      //----------------------
      // 32bit ADD
      `OP_C_FADDS: begin        
        // select the larger exponent and shift the smaller mantissa by the difference
 
        if (dn_exp1_lt_exp2) begin
          denorm_exp1_m_exp2_next = dn_exp2 - dn_exp1;
          if (dn_exp2 - dn_exp1 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp2;
          
          // swap man1 and man2
          denorm_man1_next  = dn_man2;
          denorm_sign1_next = dn_sign2;

          denorm_man2_next  = dn_man1;
          denorm_sign2_next = dn_sign1;
          
        end else begin
          denorm_exp1_m_exp2_next = dn_exp1 - dn_exp2;
          if (dn_exp1 - dn_exp2 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp1;
          
          denorm_man1_next  = dn_man1;
          denorm_sign1_next = dn_sign1;

          denorm_man2_next  = dn_man2;
          denorm_sign2_next = dn_sign2;
  
        end
        
        if(denorm_sign1_next || denorm_sign2_next) begin
        denorm_sub_rnd_flag_next  = dn_start;
        end else begin
        denorm_sub_rnd_flag_next  = 0;
        end

        denorm_enable_add_next  = dn_start;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      
      //32 bit sub .... 
      
      `OP_C_FSUBS: begin        
        // select the larger exponent and shift the smaller mantissa by the difference
 
        if (dn_exp1_lt_exp2) begin
          denorm_exp1_m_exp2_next = dn_exp2 - dn_exp1;
          if (dn_exp2 - dn_exp1 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp2;
          
          // swap man1 and man2
          denorm_man1_next  = dn_man2;
          denorm_sign1_next = ~dn_sign2;

          denorm_man2_next  = dn_man1;
          denorm_sign2_next = dn_sign1;
          
        end else begin
          denorm_exp1_m_exp2_next = dn_exp1 - dn_exp2;
          if (dn_exp1 - dn_exp2 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
                  
          denorm_exp_next   = dn_exp1;
          
          denorm_man1_next  = dn_man1;
          denorm_sign1_next = dn_sign1;

          denorm_man2_next  = dn_man2;
          denorm_sign2_next = ~dn_sign2;
  
        end
        
        if(denorm_sign1_next || denorm_sign2_next) begin
          denorm_sub_rnd_flag_next  = dn_start;
        end else begin
          denorm_sub_rnd_flag_next  = 0;
        end

        denorm_enable_add_next  = dn_start;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      `OP_C_FSTOI, `OP_C_FSTOD, `OP_C_FITOS, `OP_C_FITOD,
      `OP_C_FMOVS, `OP_C_FNEGS, 
      `OP_C_CONCAT : begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;
  
        denorm_exp_next   = dn_exp1 - 11'h3f;
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = dn_start;
        denorm_enable_sqrt_next = 0;
      end
       
      //----------------------
      // 64bit ADD
      `OP_C_FADDD: begin        
        // select the larger exponent and shift the smaller mantissa by the difference

        if (dn_exp1_lt_exp2) begin
          denorm_exp1_m_exp2_next = dn_exp2 - dn_exp1;
          if (dn_exp2 - dn_exp1 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp2;
          
          // swap man1 and man2
          denorm_man1_next  = dn_man2;
          denorm_sign1_next = dn_sign2;

          denorm_man2_next  = dn_man1;
          denorm_sign2_next = dn_sign1;
          
        
        end else begin
          denorm_exp1_m_exp2_next = dn_exp1 - dn_exp2;
          if (dn_exp1 - dn_exp2 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp1;
          
          denorm_man1_next  = dn_man1;
          denorm_sign1_next = dn_sign1;

          denorm_man2_next  = dn_man2;
          denorm_sign2_next = dn_sign2;
        
        
         end
        
        if(denorm_sign1_next || denorm_sign2_next) begin
        denorm_sub_rnd_flag_next  = dn_start;
        end else begin
        denorm_sub_rnd_flag_next  = 0;
        end

        denorm_enable_add_next  = dn_start;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      
      //64 bit sub
      
      `OP_C_FSUBD: begin        
        // select the larger exponent and shift the smaller mantissa by the difference

        if (dn_exp1_lt_exp2) begin
          denorm_exp1_m_exp2_next = dn_exp2 - dn_exp1;
          if (dn_exp2 - dn_exp1 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp2;
          
          // swap man1 and man2
          denorm_man1_next  = dn_man2;
          denorm_sign1_next = ~dn_sign2;

          denorm_man2_next  = dn_man1;
          denorm_sign2_next = dn_sign1;
                 
        end else begin
          denorm_exp1_m_exp2_next = dn_exp1 - dn_exp2;
          if (dn_exp1 - dn_exp2 > 63)
            denorm_exp1_m_exp2_next = 63; // max shift
          denorm_exp_next   = dn_exp1;
          
          denorm_man1_next  = dn_man1;
          denorm_sign1_next = dn_sign1;

          denorm_man2_next  = dn_man2;
          denorm_sign2_next = ~dn_sign2;
        
        
        end
        if(denorm_sign1_next || denorm_sign2_next) begin
        denorm_sub_rnd_flag_next  = dn_start;
        end else begin
        denorm_sub_rnd_flag_next  = 0;
        end

        denorm_enable_add_next  = dn_start;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = 0;
      end
      `OP_C_FDTOI, `OP_C_FDTOS,
      `OP_C_FMOVD, `OP_C_FNEGD: begin
        denorm_sign1_next = dn_sign1;
        denorm_sign2_next = dn_sign2;
  
        denorm_exp_next   = dn_exp1 - 11'h3ff;
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = dn_start;
        denorm_enable_sqrt_next = 0;
      end
      //----------------------
      // 32bit SQRT (always 64bit internally)
      `OP_C_FSQRTS: begin // not tested
        denorm_sign1_next = 0;
        denorm_sign2_next = dn_sign2;
        
    //  {denorm_exp_next,denorm_sqrt_shift_next} =
    //  ({1'b0, dn_exp2-12'd127}+13'd254)^13'd1;
        
        denorm_sqrt_shift_next = dn_exp2[0];
        
        if (dn_exp2 == `FP_EXP_BITS'd0 || dn_exp2 == `FP_EXP_BITS'd255) begin
          denorm_exp_next = dn_exp2;
        end else begin
          denorm_exp_next = ((dn_exp2-12'd127)>>1)+12'd127;
        end
        
        denorm_enable_add_next  = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next = 0;
        denorm_enable_div_next  = 0;
        denorm_enable_misc_next = 0;
        denorm_enable_sqrt_next = dn_start;
      end
      //----------------------
      // 64bit SQRT (always 64bit internally)
      `OP_C_FSQRTD: begin  // not tested
        denorm_sign1_next = 0;
        denorm_sign2_next = dn_sign2;
        
    //  {denorm_exp_next,denorm_sqrt_shift_next} =
    //  ({1'b0, dn_exp2-12'd1023}+13'd2046)^13'd1;
        
        denorm_sqrt_shift_next = dn_exp2[0];
        if (dn_exp2 == `FP_EXP_BITS'd0 || dn_exp2 == `FP_EXP_BITS'h7FF) begin
          denorm_exp_next = dn_exp2;
        end else begin
          denorm_exp_next = ((dn_exp2-12'd1023)>>1)+12'd1023;
        end 
        
        denorm_enable_add_next    = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next   = 0;
        denorm_enable_div_next    = 0;
        denorm_enable_misc_next   = 0;
        denorm_enable_sqrt_next   = dn_start;
      end
      //----------------------
      // Not supported operation
      default: begin
        denorm_sign1_next = 1'bx;
        denorm_sign2_next = 1'bx;
        
        denorm_man1_next  = `FP_MAN_BITS'bx;
        denorm_man2_next  = `FP_MAN_BITS'bx;

        denorm_exp_next  = 11'bx;

        denorm_enable_add_next    = 0;
        denorm_sub_rnd_flag_next  = 0;
        denorm_enable_mult_next   = 0;
        denorm_enable_div_next    = 0;
        denorm_enable_misc_next   = 0;
        denorm_enable_sqrt_next   = 0;
      end
    endcase

  end // end always

  wire [`FP_PREDEC_BITS-1:0]       denorm_op_predec_next;
  fp_op_predec
    predec
      (.op        (op),
       .start     (start),
       .op_predec (denorm_op_predec_next)
       );
 
  stage #(.Size(`FP_PREDEC_BITS+2
                    +2*(`FP_MAN_BITS)+2+`FP_EXP_BITS
                      +`FP_STATE_BITS+7))        
                        
  denorm_add_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({denorm_op_predec_next, denorm_round_next, 
                  denorm_sign1_next, denorm_man1_next, denorm_sign2_next, denorm_man2_next, denorm_exp_next,
                    denorm_state_next, denorm_exp1_m_exp2_next, denorm_sub_rnd_flag_next})
   ,.dinValid (denorm_enable_add_next) // && ~out_retry_add)
   ,.dinRetry (out_retry_add)

   ,.q        ({denorm_op_predec_add, denorm_round_add, 
                  denorm_sign1_add, denorm_man1_add, denorm_sign2_add, denorm_man2_add, denorm_exp_add,
                    denorm_state_add, denorm_exp1_m_exp2_add, denorm_sub_rnd_flag})
   ,.qValid   (denorm_enable_add)
   ,.qRetry   (retry_add)

    // Re-clocking signals
    ,.rci     (rci) 
    ,.rco     (rco1) 
   );
 
  stage #(.Size(`FP_PREDEC_BITS+2
                    +2*(`FP_MAN_BITS+1)+`FP_EXP_BITS
                      +`FP_STATE_BITS+`FP_EXP_BITS+`FP_EXP_BITS+1+1)) 
                        
  denorm_mult_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({denorm_op_predec_next, denorm_round_next, 
                  denorm_sign1_next, denorm_man1_next, denorm_sign2_next, denorm_man2_next, denorm_exp_next,
                    denorm_state_next, dn_exp1, dn_exp2, denorm_overflow_next, denorm_rshift_man_next})
   ,.dinValid (denorm_enable_mult_next) // && ~out_retry_mult)
   ,.dinRetry (out_retry_mult)

   ,.q        ({denorm_op_predec_mult, denorm_round_mult, 
                  denorm_sign1_mult, denorm_man1_mult, denorm_sign2_mult, denorm_man2_mult, denorm_exp_mult,
                    denorm_state_mult, denorm_exp1_mult, denorm_exp2_mult, denorm_overflow_mult, denorm_rshift_man_mult})
   ,.qValid   (denorm_enable_mult)
   ,.qRetry   (retry_mult)

    // Re-clocking signals
    ,.rci     (rco1) 
    ,.rco     (rco2) 
   );
   
  stage #(.Size(`FP_PREDEC_BITS+2
                    +2*(`FP_MAN_BITS+1)+`FP_EXP_BITS
                      +`FP_STATE_BITS+`FP_EXP_BITS+`FP_EXP_BITS
                        +1+1)) 
                        
  denorm_div_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({denorm_op_predec_next, denorm_round_next, 
                  denorm_sign1_next, denorm_man1_next, denorm_sign2_next, denorm_man2_next, denorm_exp_next,
                    denorm_state_next, dn_exp1, dn_exp2, 
                      denorm_overflow_next, denorm_rshift_man_next})

   ,.dinValid (denorm_enable_div_next) // & ~out_retry_div)
   ,.dinRetry (out_retry_div)

   ,.q        ({denorm_op_predec_div, denorm_round_div, 
                  denorm_sign1_div, denorm_man1_div, denorm_sign2_div, denorm_man2_div, denorm_exp_div,
                    denorm_state_div, denorm_exp1_div, denorm_exp2_div,
                      denorm_overflow_div, denorm_rshift_man_div})
   ,.qValid   (denorm_enable_div)
   ,.qRetry   (retry_div)

    // Re-clocking signals
    ,.rci     (rco2) 
    ,.rco     (rco3) 
   );

  stage #(.Size(`FP_PREDEC_BITS+2 
                   +(`FP_MAN_BITS+1)+`FP_EXP_BITS
                     +`FP_STATE_BITS+1)) 
                        
  denorm_sqrt_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({denorm_op_predec_next, denorm_round_next, 
                  denorm_sign2_next, denorm_man2_next, denorm_exp_next,
                    denorm_state_next, denorm_sqrt_shift_next})
   ,.dinValid (denorm_enable_sqrt_next) // &~out_retry_sqrt)
   ,.dinRetry (out_retry_sqrt)

   ,.q        ({denorm_op_predec_sqrt, denorm_round_sqrt, 
                  denorm_sign2_sqrt, denorm_man2_sqrt, denorm_exp_sqrt,
                    denorm_state_sqrt, denorm_sqrt_shift})
   ,.qValid   (denorm_enable_sqrt)
   ,.qRetry   (retry_sqrt)

    // Re-clocking signals
    ,.rci     (rco3) 
    ,.rco     (rco) 
   );
 /*
   
  stage #(.Size(`FP_PREDEC_BITS+2
                    +2*(`FP_MAN_BITS+1)+`FP_EXP_BITS
                      +`FP_STATE_BITS+`FP_EXP_BITS+`FP_EXP_BITS
                        +7)) 
                        
  denorm_misc_s
   (.clk      (clk)
   ,.reset    (reset)

   ,.din      ({denorm_op_predec_next, denorm_round_next, 
                  denorm_sign1_next, denorm_man1_next, denorm_sign2_next, denorm_man2_next, denorm_exp_next,
                    denorm_state_next, dn_exp1, dn_exp2,
                      denorm_exp1_m_exp2_next, denorm_sqrt_shift_next, denorm_sub_rnd_flag_next})
   ,.dinValid (denorm_enable_misc_next)
   ,.dinRetry (out_retry_misc)

   ,.q        ({denorm_op_predec, denorm_round, 
                  denorm_sign1, denorm_man1, denorm_sign2, denorm_man2, denorm_exp,
                    denorm_state, denorm_exp1, denorm_exp2,
                      denorm_exp1_m_exp2, denorm_sqrt_shift,denorm_sub_rnd_flag})
   ,.qValid   (denorm_enable_misc)
   ,.qRetry   (denorm_retry_misc)
   );
*/
 endmodule
