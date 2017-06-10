/****************************************************************************

    Written by Alana Muldoon - Dec 2008

    Description: Here to mimick the rest of the operations, could probably
    be removed though

****************************************************************************/

`include "dfuncs.h"
//`include "scoore.h"

`include "scoore_fpu.h"
//import scoore_fpu::*;

module fp_sqrt64_post_comb
  (
   // inputs
   input                             sqrtp_ready,
   input  [`FP_PREDEC_BITS-1:0]      sqrtp_op_predec,
   input  [`FP_STATE_BITS-1:0]       sqrtp_state,
   input  [2-1:0]                  sqrtp_round,

   input                             sqrtp_sign,
   input  [`FP_EXP_BITS-1:0]         sqrtp_exp,
   input  [`FP_MAN_BITS-1:0]         sqrtp_man,

   // outputs
   output                            sqrt_ready,
   output [`FP_PREDEC_BITS-1:0]      sqrt_op_predec,
   output [`FP_STATE_BITS-1:0]       sqrt_state,
   output [2-1:0]                  sqrt_round,

   output                            sqrt_sign,
   output [`FP_EXP_BITS-1:0]         sqrt_exp,
   output [`FP_MAN_BITS-1:0]         sqrt_man
  );



  assign                      sqrt_ready     = sqrtp_ready;
  assign                      sqrt_op_predec = sqrtp_op_predec;
  assign                      sqrt_state     = sqrtp_state;
  assign          sqrt_round     = sqrtp_round;
  assign                      sqrt_sign      = sqrtp_sign;

  assign                      sqrt_exp       = sqrtp_exp;
`ifndef ANUBIS_LOCAL_4
  assign                      sqrt_man       = sqrtp_man; //64'hFFFFAFFFFFFFFFFF;
`else
  assign                      sqrt_man       = 64'hFFFFAFFFFFFFFFFF;
`endif

endmodule
