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

 All the defines used on the scoore FPU project

****************************************************************************/

// The FPU input/output operands use 32 bits, but the FPU can operate with 64
// bits (128bits in the future?). Internally, all the operations are 64 bits, a
// first stage on the FP unit converts from 32 to 64 bits.

`define FP_TYPE_I_BITS  32
`define FP_TYPE_S_BITS  32
`define FP_TYPE_D_BITS  64
`define FP_TYPE_Q_BITS 128

// FPU internal Floating point representation
`define FP_MAN_BITS    64 // 64bits for int (56 for FP: 52 double FP + 3 rounding + 1 hidden bit)
`define FP_EXP_BITS    11
`define FP_RND_BITS     3


`define FP_HIDDEN_FMT  63:63

`define FP_SMAN_BITS   23
`define FP_SMAN_FMT    62:40
`define FP_SRND_FMT    39:37

`define FP_DMAN_BITS   52
`define FP_DMAN_FMT    62:11
`define FP_DRND_FMT    10:08

// IEEE FP representation
`define FP_IEEE_SMAN_FMT  22:0
`define FP_IEEE_SEXP_FMT  30:23
`define FP_IEEE_SSIGN_FMT 31:31

`define FP_IEEE_DMAN_FMT  51:0
`define FP_IEEE_DEXP_FMT  62:52
`define FP_IEEE_DSIGN_FMT 63:63

`define FP_QMAN_BITS  112 // Not implemented
`define FP_QEXP_BITS   15 // Not implemented


`define FP_PREDEC_DST_SINT32_BIT  0
`define FP_PREDEC_DST_UINT32_BIT  1
`define FP_PREDEC_DST_SINT64_BIT  2
`define FP_PREDEC_DST_UINT64_BIT  3
`define FP_PREDEC_DST_FP32_BIT    4
`define FP_PREDEC_DST_FP64_BIT    5

`define FP_PREDEC_BITS 6
//pranav : new def added after prting fpu out of scoore folder
`define FP_STATE_BITS       10
`define OP_TYPE_BITS        6

// 32bit FP inputs
`define OP_C_FSTOI    6'b000001
`define OP_C_FSTOD    6'b000010
`define OP_C_FMOVS    6'b000011
`define OP_C_FNEGS    6'b000100
`define OP_C_FABSS    6'b000101
`define OP_C_FSQRTS   6'b000110
`define OP_C_FADDS    6'b000111
`define OP_C_FSUBS    6'b001000
`define OP_C_FMULS    6'b001001
`define OP_C_FDIVS    6'b001010
`define OP_C_FSMULD   6'b001011
`define OP_C_FCMPS    6'b001100
`define OP_C_FCMPES   6'b001101

// 64bit FP inputs
`define OP_C_FDTOI    6'b010001
`define OP_C_FDTOS    6'b010010
`define OP_C_FMOVD    6'b010011
`define OP_C_FNEGD    6'b010100
`define OP_C_FABSD    6'b010101
`define OP_C_FSQRTD   6'b010110
`define OP_C_FADDD    6'b010111
`define OP_C_FSUBD    6'b011000
`define OP_C_FMULD    6'b011001
`define OP_C_FDIVD    6'b011010
//`define OP_C_FDMULD 6'b011011 same as FMULD
`define OP_C_FCMPD    6'b011100
`define OP_C_FCMPED   6'b011101

// 32bit UINT inputs (src1 for division is 64bits INT)
`define OP_C_UMUL     6'b100000
`define OP_C_UDIV     6'b100001
`define OP_C_UDIVCC   6'b100010
`define OP_C_UMULCC   6'b100011

`define OP_C_FITOS    6'b100100
`define OP_C_FITOD    6'b100101

`define OP_C_CONCAT   6'b100111
`define OP_C_MULSCC   6'b101000 // Weird op

// 32bit SINT inputs
`define OP_C_SMUL     6'b110000
`define OP_C_SDIV     6'b110001
`define OP_C_SDIVCC   6'b110010
`define OP_C_SMULCC   6'b110011



`define OP_C_TYPE_SRC_FMT 5:4
`define OP_C_32UINT_SRC 2'b10
`define OP_C_32SINT_SRC 2'b11
`define OP_C_32FP_SRC   2'b00
`define OP_C_64FP_SRC   2'b01

`define FP_ROUND_NEAREST 2'b00
`define FP_ROUND_ZERO    2'b01
`define FP_ROUND_PLUSINF 2'b10
`define FP_ROUND_MININF  2'b11

