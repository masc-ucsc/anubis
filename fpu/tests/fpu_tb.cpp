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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "milieu.h"
#include "softfloat.h"

#include "veriuser.h"

#include "svdpi.h"

#define OP_C_FSTOI    b000001
#define OP_C_FSTOD    b000010
#define OP_C_FMOVS    b000011
#define OP_C_FNEGS    b000100
#define OP_C_FABSS    b000101
#define OP_C_FSQRTS   0x06 // b000110
#define OP_C_FADDS    0x07
#define OP_C_FSUBS    0x08//b001000
#define OP_C_FMULS    0x09
#define OP_C_FDIVS    0x0A
#define OP_C_FSMULD   b001011
#define OP_C_FCMPS    b001100
#define OP_C_FCMPES   0x0D//b001101

// 64bit FP inputs
#define OP_C_FDTOI    b010001
#define OP_C_FDTOS    b010010
#define OP_C_FMOVD    b010011
#define OP_C_FNEGD    b010100
#define OP_C_FABSD    b010101
#define OP_C_FSQRTD   0x16 // b010110
#define OP_C_FADDD    0x17
#define OP_C_FSUBD    0x18 //b011000
#define OP_C_FMULD    0x19 // b011001
#define OP_C_FDIVD    0x1A
//`define OP_C_FDMULD 6'b011011 same as FMULD
#define OP_C_FCMPD    b011100
#define OP_C_FCMPED   0x1D//b011101

// 32bit UINT inputs
#define OP_C_UMUL     0x20 // b100000
#define OP_C_UDIV     0x21 //b100001
#define OP_C_UDIVCC   0x22// b100010
#define OP_C_UMULCC   0x23//b100011

#define OP_C_FITOS    b100100
#define OP_C_FITOD    b100101

#define OP_C_CONCAT   b100111
#define OP_C_MULSCC   b101000 // Weirop

// 32bit UINT inputs
#define OP_C_SMUL     0x30 // b110000
#define OP_C_SDIV     0x31 // b110001
#define OP_C_SDIVCC   0x32//b110010
#define OP_C_SMULCC   0x33//b110011



#ifdef MODELSIM
int fpu_check();
int fpu_set();
int fpu_init();
void inc_stat1();
void inc_stat2();
int params_check();
s_tfcell veriusertfs[] = {
  {usertask, 0, params_check, 0, fpu_init , 0, "$fpu_init"},
  {usertask, 0, params_check, 0, fpu_check, 0, "$fpu_check"},
  {usertask, 0, params_check, 0, fpu_set  , 0, "$fpu_set"},
  {usertask, 0, params_check, 0, inc_stat1, 0, "$inc_stat1"},
  {usertask, 0, params_check, 0, inc_stat2, 0, "$inc_stat2"},
  {0} /* last entry must be 0 */
};
#else
extern "C" int fpu_check();
extern "C" int fpu_set();
extern "C" int fpu_init();
extern "C" int inc_stat1();
extern "C" int inc_stat2();
extern "C" int params_check();
#endif

enum TestType {
  float64_add_Test  ,
  float64_sub_Test  ,
  float64_mul_Test  ,
  float64_div_Test  ,
  float64_sqrt_Test ,
  float32_add_Test  ,
  float32_sub_Test  ,
  float32_mul_Test  ,
  float32_div_Test  ,
  float32_sqrt_Test ,

  umul_Test        ,
  udiv_Test        ,
  smul_Test        ,
  sdiv_Test        ,
 // sets the condition codes 
  umulcc_Test      ,
  smulcc_Test      ,
  udivcc_Test      ,
  sdivcc_Test      ,  

  rank_Test        ,
  
  float64_bench_Test,
  float32_bench_Test,

  random_Test,
};

bool  retry_flg = false;

bool  idle_state = false;

int max_ret = 0;

double time_unit = 100;

enum TestType do_test = float64_add_Test;

bool reclock_enable = false;


unsigned long long join_low_high(int low, int high) {

  unsigned int *tmp_low  = (unsigned int *)&low;
  unsigned int *tmp_high = (unsigned int *)&high;
  
  unsigned long long val = *tmp_high; 
  val<<=32;
  val |=*tmp_low;

  return val;
}

unsigned long long my_gettime() {

  int low;
  int high;

  low = tf_getlongtime(&high);

  return join_low_high(low, high);
}

unsigned long long my_getlongp(int pos) {

  int low;
  int high;

  low = tf_getlongp(&high, pos);

  return join_low_high(low, high);
}

void my_putlongp(int pos, unsigned long long val) {

  int tmp_low;
  int tmp_high;
  tmp_low = val;
  val>>=32;
  tmp_high= val;

  tf_putlongp(pos, tmp_low, tmp_high);
}

int get_idle_time() {

  int idle_time;
  int r = random() % 10;
  
  if (r <5)
    idle_time = 1;
  else if (r<7)
    idle_time = random() % 7;
  else
    idle_time = random() % 2;
  
  return idle_time;
  //return 0; //hardcoding idle time to 0
}

struct InstMix{

    float percentOfFAdd;
    float percentOfFMul;
    float percentOfFDiv;
    float percentOfIMul;
    float percentOfIDiv;
    float numberOfFAdd; 
    float numberOfFMul; 
    float numberOfFDiv; 
    float numberOfIMul; 
    float numberOfIDiv; 
    float totalOp;
    float SampleMax;
  }Tbench[] = {
       {52.70,40.22,0.41,6.58,0.06,0,0,0,0,0,1,10000},
       {72.14,26.52,1.32,0.00,0.00,0,0,0,0,0,1,10000},
       {85.31,14.61,0.00,0.06,0.00,0,0,0,0,0,1,10000},
       {40.95,57.64,1.37,0.02,0.00,0,0,0,0,0,1,10000}
  };// 0 - wup 1 - swim 2-mgrid 3-applu 

bool new_rand = false;
int benchNumber = 0;

int rand_next_bench() // 0 - wup 1 - swim
{

  static int floor = 0, ceiling = 5, range = (ceiling - floor);
  static int rnd = 0 , tmp ;
  do {  
       new_rand = false;
       rnd   = floor + int((range * random()) / (RAND_MAX + 1.0));
      if((rnd == 0)&&((Tbench[benchNumber].numberOfFAdd/Tbench[benchNumber].SampleMax)*100)  > (Tbench[benchNumber].percentOfFAdd)){
        new_rand = true;
      } if((rnd == 1)&&((Tbench[benchNumber].numberOfFMul/Tbench[benchNumber].SampleMax)*100)  > (Tbench[benchNumber].percentOfFMul)){
        new_rand = true;
      } if((rnd == 2)&&((Tbench[benchNumber].numberOfFDiv/Tbench[benchNumber].SampleMax)*100)  > (Tbench[benchNumber].percentOfFDiv)){
        new_rand = true;
      } if((rnd == 3)&&((Tbench[benchNumber].numberOfIMul/Tbench[benchNumber].SampleMax)*100)  > (Tbench[benchNumber].percentOfIMul)){
        new_rand = true;
      } if((rnd == 4)&&((Tbench[benchNumber].numberOfIDiv/Tbench[benchNumber].SampleMax)*100)  > (Tbench[benchNumber].percentOfIDiv)){
        new_rand = true;
      }   
        if(Tbench[benchNumber].totalOp > (Tbench[benchNumber].SampleMax-(0.001 * Tbench[benchNumber].SampleMax))){// 0.001 = threshold for contention
          new_rand = false;
          Tbench[benchNumber].totalOp = 1;
          //io_printf("\npercent of fadd = %f",((Tbench[benchNumber].numberOfFAdd/Tbench[benchNumber].SampleMax)*100));
          //io_printf("\npercent of fmul = %f",((Tbench[benchNumber].numberOfFMul/Tbench[benchNumber].SampleMax)*100));
          //io_printf("\npercent of fdiv = %f",((Tbench[benchNumber].numberOfFDiv/Tbench[benchNumber].SampleMax)*100));
          //io_printf("\npercent of imul = %f",((Tbench[benchNumber].numberOfIMul/Tbench[benchNumber].SampleMax)*100));
          //io_printf("\npercent of idiv = %f",((Tbench[benchNumber].numberOfIDiv/Tbench[benchNumber].SampleMax)*100));
          Tbench[benchNumber].numberOfFAdd = 0;
          Tbench[benchNumber].numberOfFMul = 0;
          Tbench[benchNumber].numberOfFDiv = 0;
          Tbench[benchNumber].numberOfIMul = 0;
          Tbench[benchNumber].numberOfIDiv = 0;
        }
    }while(new_rand);

switch(rnd){
      case 0 : Tbench[benchNumber].numberOfFAdd++;
              tmp = 0; 
               break;
      case 1 : Tbench[benchNumber].numberOfFMul++;
              tmp = 2;
               break;
      case 2 : Tbench[benchNumber].numberOfFDiv++;
              tmp = 3;
               break;
      case 3 : Tbench[benchNumber].numberOfIMul++;
              tmp = 10;
               break;
      case 4 : Tbench[benchNumber].numberOfIDiv++;
              tmp = 11;
               break;
      default: io_printf("Error in bench Test");
              break;
      }
Tbench[benchNumber].totalOp++;

return tmp;
}

TestType get_rnd_test() {
  return static_cast<TestType>(rand() % rank_Test);
}


#define SIMPLE_N_TEST 1024
struct Result_Entry {
  unsigned long long when;
  unsigned long long val;
  enum TestType op;
  unsigned int in_use;
};

int simple_result_head = 0;
int simple_result_tail = 0;
struct Result_Entry simple_result[SIMPLE_N_TEST]; 

void float64_add_next(float64 &src1, float64 &src2, float64 &res);
void float64_sub_next(float64 &src1, float64 &src2, float64 &res);
void float64_mul_next(float64 &src1, float64 &src2, float64 &res);
void float64_div_next(float64 &src1, float64 &src2, float64 &res);
void float64_sqrt_next(float64 &src2, float64 &res);

void float32_add_next(float32 &src1, float32 &src2, float32 &res);
void float32_sub_next(float32 &src1, float32 &src2, float32 &res);
void float32_mul_next(float32 &src1, float32 &src2, float32 &res);
void float32_div_next(float32 &src1, float32 &src2, float32 &res);
void float32_sqrt_next(float32 &src2, float32 &res);

static int num_tests_pending=0;

#define NUM_TESTS (327680)

void do_reset() {
  simple_result_head = 0;
  simple_result_tail = 0;
  for(int i=0;i<SIMPLE_N_TEST;i++)
    simple_result[i].in_use = 0;

  num_tests_pending = 0;
}

bool random_test_flag = false;
bool debug_print = false;     
bool benchTest = false;

int fpu_init() {

   const char *opt = mc_scan_plusargs("debug=");
  
   if (opt){
     if (strcasecmp(opt,"yes") == 0 || strcasecmp(opt,"true") == 0) {
        debug_print = true;
        io_printf("setting debug option\n"); 
      }else{ 
       debug_print = false;               
        io_printf("unsetting debug option\n"); 
     }
   }

  opt = mc_scan_plusargs("bench=");
  
  if(opt){
    if(strcasecmp(opt,"wup") == 0) {
      benchTest   = true;
      benchNumber = 0;
      }else if(strcasecmp(opt,"swim") == 0) {
      benchTest   = true;
      benchNumber = 1;
      }else if(strcasecmp(opt,"mgrid") == 0) {
      benchTest   = true;
      benchNumber = 2;
      }else if(strcasecmp(opt,"applu") == 0) {
      benchTest   = true;
      benchNumber = 3;
      }else{
      benchTest = false;
      io_printf("Incorrect option, aborting TB\n\n +bench= options\n\n wup,\n swim,\n mgrid,\n applu\n\n");
      tf_dofinish();
      }
  }

  reclock_enable = false;
	opt = mc_scan_plusargs("reclock=");
	if (opt) {
		if( atoi(opt) !=0 )
      reclock_enable = true;
  }

  io_printf("reclock set to %s\n",reclock_enable?"True":"False");

  opt = mc_scan_plusargs("test=");

  if (opt) {
    if (strcasecmp(opt,"float64_add") == 0) {
      do_test = float64_add_Test;
      io_printf("64 add\n");
    }else if (strcasecmp(opt,"float64_sub") == 0) {
      do_test = float64_sub_Test;
      io_printf("64 sub\n");
    }else if (strcasecmp(opt,"float64_mul") == 0) {
      do_test = float64_mul_Test;
      io_printf("64 mul\n");
    }else if (strcasecmp(opt,"float64_div") == 0) {
      do_test = float64_div_Test;
      io_printf("64 div\n");
    }else if (strcasecmp(opt,"float64_sqrt") == 0) {
      do_test = float64_sqrt_Test;
    }else if (strcasecmp(opt,"float32_add") == 0) {
      do_test = float32_add_Test;
    }else if (strcasecmp(opt,"float32_sub") == 0) {
      do_test = float32_sub_Test;
    }else if (strcasecmp(opt,"float32_mul") == 0) {
      do_test = float32_mul_Test;
    }else if (strcasecmp(opt,"float32_div") == 0) {
      do_test = float32_div_Test;
    }else if (strcasecmp(opt,"float32_sqrt") == 0) {
      do_test = float32_sqrt_Test;
    }else if (strcasecmp(opt,"float32_bench") == 0) {
      do_test = float32_bench_Test;
    }else if (strcasecmp(opt,"float64_bench") == 0) {
      do_test = float64_bench_Test;
    }else if (strcasecmp(opt,"rank") == 0) {
      do_test = rank_Test;
      retry_flg = true;
    }else if (strcasecmp(opt,"umul") == 0) {
      do_test = umul_Test;
    }else if (strcasecmp(opt,"umulcc") == 0) {
      do_test = umulcc_Test;
    }else if (strcasecmp(opt,"udiv") == 0) {
      do_test = udiv_Test;
    }else if (strcasecmp(opt,"udivcc") == 0) {
      do_test = udivcc_Test;
    }else if (strcasecmp(opt,"smul") == 0) {
      do_test = smul_Test;
    }else if (strcasecmp(opt,"smulcc") == 0) {
      do_test = smulcc_Test;
    }else if (strcasecmp(opt,"sdiv") == 0) {
      do_test = sdiv_Test;
    }else if (strcasecmp(opt,"sdivcc") == 0) {
      do_test = sdivcc_Test;
    }else if (strcasecmp(opt,"random") == 0){
      do_test = random_Test;
      random_test_flag = true;
    }else{
      io_printf("ERROR: Unknown %s test option\n",opt);
      io_printf("options:\n");
      io_printf("\tfloat64_add    : 64bit addition\n");
      io_printf("\tfloat64_sub    : 64bit subtraction\n");
      io_printf("\tfloat64_mul    : 64bit multiplication\n");
      io_printf("\tfloat64_div    : 64bit division\n");
      io_printf("\tfloat64_sqrt	  : 64bit squareroot\n");
      io_printf("\tfloat32_add    : 32bit addition\n");
      io_printf("\tfloat32_sub    : 32bit subtraction\n");
      io_printf("\tfloat32_mul    : 32bit multiplication\n");
      io_printf("\tfloat32_div    : 32bit division\n");
      io_printf("\tfloat32_sqrt   : 32bit squareroot\n");      io_printf("\n");
      io_printf("\tfloat64_bench  : 64bit mix\n");
      io_printf("\tfloat32_bench  : 32bit mix\n");
      io_printf("\trandom         : Default Option tests all operations randomly\n");// pranav : added random function to the list operations
      io_printf("\n");
      io_printf("\tumul           : 32bit unsigned multiplication\n");
      io_printf("\tumulcc         : 32bit unsigned multiplication, sets icc\n");
      io_printf("\tudiv           : 32bit unsigned division\n");
      io_printf("\tudivcc         : 32bit unsigned division, sets icc\n");
      io_printf("\tsmul           : 32bit signed multiplication\n");
      io_printf("\tsmulcc         : 32bit signed multiplication, sets icc\n");
      io_printf("\tsdiv           : 32bit signed division\n");
      io_printf("\tsdivcc         : 32bit signed division, sets icc\n");
      io_printf("\tFiTOs          : 32bit integer to single\n");
      io_printf("\tFiTOd          : 64bit integer to double\n");
      io_printf("\tFsTOi          : 32bit single  to single\n");
      io_printf("\tFdTOi          : 64bit double  to single\n");
      tf_dofinish();
    }
    io_printf("Performing %s test\n",opt);
  }else{                          //pranav : added default test => random_Test which calls a test from (enum) TestType
   // if(!benchTest)
    random_test_flag = true;
  }

  opt = mc_scan_plusargs("clk=");
  if (opt) {
    time_unit = atof(opt);
    io_printf("Time unit is %g\n", time_unit);
  }
  

  opt = mc_scan_plusargs("retry=");
  
  if(opt){
    if(strcasecmp(opt,"off") == 0) {
      retry_flg = true;
    }
  }
 
  int reset = 1;
  tf_putp(1, reset);
  do_reset();

  return 0;
}

void sf_float64_2op_set(unsigned int &start,
			unsigned int &state,
			unsigned int &round,
			unsigned int &op,
			unsigned long long &src1,
			unsigned long long &src2) {

  enum TestType          do_op;
  if (do_test == rank_Test) {
    static int alternate= 0;
    alternate++;
    if ((alternate&1)!=0)
      return;

    int prop = random() % 10;
    if ( prop < 5) {
      // 50% adds
      do_op = float64_add_Test; // TODO: 45-45-10 32/64/fix
    }else if (prop <8) {
      // ~30% mult
      do_op = float32_mul_Test; // TODO: 35-35-30 32/64/fix
    }else if (prop <9){
      // ~10% div  (changed from 20%)
      do_op = float32_div_Test; // TODO: 35-35-30 32/64/fix
    }else {
      // ~10% sqrt
      do_op = float32_sqrt_Test;
    }
    static int iddle_time = 0;
    if (iddle_time >=0) {
      iddle_time--;
      return;
    }
    int r = random() % 10;
    if (r <5)
      iddle_time = 1;
    else if (r<7)
      iddle_time = random() % 15;
    else
      iddle_time = random() % 2;
	}else{
    do_op = do_test;
  }
    
  static int conta = 0;
  conta++;
  if(conta>=(2*NUM_TESTS)) {
    io_printf("ERROR: Did not finish all the tests (pending = %d)\n", num_tests_pending);
    tf_dofinish();
  }

  if(conta>=NUM_TESTS)
    return; // No more tasks

#if 0
  // Useful for debugging, to send an operation at a time
  if (num_tests_pending > 0) {
    return;
  }
#endif
  num_tests_pending++;
  start = 1;

 
 switch(random() % 4) {  
  case 0:
    round = 0; // 0 nearest
    float_rounding_mode = float_round_nearest_even;
    break;
  case 1:
    round = 1; // 1 zero
    float_rounding_mode = float_round_to_zero;
    break;
  case 2:
    round = 2; // 2 up
    float_rounding_mode = float_round_up;
    break;
  case 3:
    round = 3; // 3 down
    float_rounding_mode = float_round_down;
  }


  float64 f64_src1_f; 
  float64 f64_src2_f; 
  float64 f64_res_f;

  float32 f32_src1_f; 
  float32 f32_src2_f; 
  float32 f32_res_f;

  unsigned long long ll_val1;
  unsigned long long ll_val2;
  unsigned long long ll_res;  

  switch(do_op) {
  case float64_add_Test:
    op    = OP_C_FADDD; 	
    float64_add_next(f64_src1_f, f64_src2_f, f64_res_f);
    ll_val1 = *(unsigned long long *)&f64_src1_f;
    ll_val2 = *(unsigned long long *)&f64_src2_f;
    ll_res  = *(unsigned long long *)&f64_res_f;
   //io_printf("Pre-comp %lld + %lld = %lld \n",ll_val1,ll_val2,ll_res);
    break;
  case float64_sub_Test:
    op    = OP_C_FSUBD; 	
    float64_sub_next(f64_src1_f, f64_src2_f, f64_res_f);
    ll_val1 = *(unsigned long long *)&f64_src1_f;
    ll_val2 = *(unsigned long long *)&f64_src2_f;
    ll_res  = *(unsigned long long *)&f64_res_f;
   //io_printf("Pre-comp %lld + %lld = %lld \n",ll_val1,ll_val2,ll_res);
    break;
  case float64_mul_Test:
    op    = OP_C_FMULD;
    float64_mul_next(f64_src1_f, f64_src2_f, f64_res_f);
    ll_val1 = *(unsigned long long *)&f64_src1_f;
    ll_val2 = *(unsigned long long *)&f64_src2_f;
    ll_res  = *(unsigned long long *)&f64_res_f;
   // io_printf("Pre-comp %d *  %d = %d \n",ll_val1,ll_val2,ll_res);
    break;
  case float64_div_Test:
    op    = OP_C_FDIVD;
    float64_div_next(f64_src1_f, f64_src2_f, f64_res_f);
    ll_val1 = *(unsigned long long *)&f64_src1_f;
    ll_val2 = *(unsigned long long *)&f64_src2_f;
    ll_res  = *(unsigned long long *)&f64_res_f;
   // io_printf("Pre-comp %d /  %d = %d \n",ll_val1,ll_val2,ll_res);
    break;
  case float64_sqrt_Test:
  	op		= OP_C_FSQRTD;
  	float64_sqrt_next(f64_src2_f, f64_res_f);
  	f64_src1_f = 0;
    ll_val1 = *(unsigned long long *)&f64_src1_f;
  	ll_val2 = *(unsigned long long *)&f64_src2_f;
  	ll_res  = *(unsigned long long *)&f64_res_f;
  // io_printf("Pre-comp sqrt %lld = %lld \n",ll_val2,ll_res);
    break;
  case float32_add_Test:
    op    = OP_C_FADDS;
    float32_add_next(f32_src1_f, f32_src2_f, f32_res_f);
    ll_val1 = *(unsigned long *)&f32_src1_f;
    ll_val2 = *(unsigned long *)&f32_src2_f;
    ll_res  = *(unsigned long *)&f32_res_f;
    break;
  case float32_sub_Test:
    op    = OP_C_FSUBS;
    float32_sub_next(f32_src1_f, f32_src2_f, f32_res_f);
    ll_val1 = *(unsigned long *)&f32_src1_f;
    ll_val2 = *(unsigned long *)&f32_src2_f;
    ll_res  = *(unsigned long *)&f32_res_f;
    break;
  case float32_mul_Test:
    op    = OP_C_FMULS;
    float32_mul_next(f32_src1_f, f32_src2_f, f32_res_f);
    ll_val1 = *(unsigned long *)&f32_src1_f;
    ll_val2 = *(unsigned long *)&f32_src2_f;
    ll_res  = *(unsigned long *)&f32_res_f;
    break;
  case float32_div_Test:
    op    = OP_C_FDIVS;
    float32_div_next(f32_src1_f, f32_src2_f, f32_res_f);
    ll_val1 = *(unsigned long *)&f32_src1_f;
    ll_val2 = *(unsigned long *)&f32_src2_f;
    ll_res  = *(unsigned long *)&f32_res_f;
    break;
  case float32_sqrt_Test:
    op		= OP_C_FSQRTS;
  	float32_sqrt_next(f32_src2_f, f32_res_f);
  	f32_src1_f = 0;
  	ll_val2 = *(unsigned long long *)&f32_src1_f;
  	ll_val2 = *(unsigned long long *)&f32_src2_f;
  	ll_res  = *(unsigned long long *)&f32_res_f;
    break;
  default:
    io_printf("ERROR:floatset unknown test [%d]\n",do_op);
    tf_dofinish();
    exit(0);
  }
   
  src1  = ll_val1;
  src2  = ll_val2;
  unsigned long long res  = ll_res;
  
  simple_result_head = (simple_result_head+1) % SIMPLE_N_TEST;
  state = simple_result_head;
  
  if (simple_result[state].in_use) {
    io_printf("ERROR: Still did not finish state[%0x]=0x%x, but it is too long ago\n"
              ,simple_result_head, simple_result[simple_result_head].val);
    tf_dofinish();
  }

  simple_result[state].when       = my_gettime();
  simple_result[state].val        = ll_res;
  simple_result[state].op         = do_op;
  simple_result[state].in_use     = 1;

  double *src1d = (double *)&src1;
  double *src2d = (double *)&src2;
  double *resd = (double *)&res;
  float *src1f = (float *)&src1;
  float *src2f = (float *)&src2;
  float *resf = (float *)&res;
  
  if (start && debug_print) {
    switch (do_op) {
      case float64_add_Test:
	      io_printf("Checking float64_add_Test 64: state[%0x] round[%x] op[%x] src1=%g[0x%llx] src2=%g[0x%llx] res=%g[0x%llx]\n",state, round, op, *src1d, ll_val1, *src2d, ll_val2, *resd, ll_res);
      	break;
      case float64_sub_Test:
	      io_printf("Checking float64_sub_Test 64: state[%0x] round[%x] op[%x] src1=%g[0x%llx] src2=%g[0x%llx] res=%g[0x%llx]\n",state, round, op, *src1d, ll_val1, *src2d, ll_val2, *resd, ll_res);
      	break;
      case float64_mul_Test:
	      io_printf("Checking float64_mul_Test 64: state[%0x] round[%x] op[%x] src1=%g[0x%llx] src2=%g[0x%llx] res=%g[0x%llx]\n",state, round, op, *src1d, ll_val1, *src2d, ll_val2, *resd, ll_res);
        break;	
      case float64_div_Test:
      	io_printf("Checking float64_div_Test 64: state[%0x] round[%x] op[%x] src1=%g[0x%llx] src2=%g[0x%llx] res=%g[0x%llx]\n",state, round, op, *src1d, ll_val1, *src2d, ll_val2, *resd, ll_res);
        break;
      case float64_sqrt_Test:
        io_printf("Checking 64 sqrt: state[%0x] round[%x] op[%x] src1=%g[0x%llx] src2=%g[0x%llx] res=%g[0x%llx]\n"
	         ,state, round, op, *src1d, ll_val1, *src2d, ll_val2, *resd, ll_res);
        break;
      case float32_add_Test:
        io_printf("Checking add 32: state[%0x] round[%x] op[%x] src1=%g[0x%x] src2=%g[0x%x] res=%g[0x%x]\n"
	         ,state, round, op, *src1f, (unsigned long)ll_val1, *src2f, (unsigned long)ll_val2, *resf, (unsigned long)ll_res);
        break;  
      case float32_sub_Test:
        io_printf("Checking sub 32: state[%0x] round[%x] op[%x] src1=%g[0x%x] src2=%g[0x%x] res=%g[0x%x]\n"
	         ,state, round, op, *src1f, (unsigned long)ll_val1, *src2f, (unsigned long)ll_val2, *resf, (unsigned long)ll_res);
        break;  
      case float32_mul_Test:
        io_printf("Checking mul 32: state[%0x] round[%x] op[%x] src1=%g[0x%x] src2=%g[0x%x] res=%g[0x%x]\n"
	         ,state, round, op, *src1f, (unsigned long)ll_val1, *src2f, (unsigned long)ll_val2, *resf, (unsigned long)ll_res);
        break;  
      case float32_div_Test:
        io_printf("Checking div 32: state[%0x] round[%x] op[%x] src1=%g[0x%x] src2=%g[0x%x] res=%g[0x%x]\n"
	         ,state, round, op, *src1f, (unsigned long)ll_val1, *src2f, (unsigned long)ll_val2, *resf, (unsigned long)ll_res);
        break;  
      case float32_sqrt_Test:
        io_printf("Checking sqrt 32: state[%0x] round[%x] op[%x] src1=%g[0x%x] src2=%g[0x%x] res=%g[0x%x]\n"
	         ,state, round, op, *src1f, (unsigned long)ll_val1, *src2f, (unsigned long)ll_val2, *resf, (unsigned long)ll_res);
        break;
      default:
        io_printf("ERROR: float2op: unknown test [%d]\n",do_op);
        tf_dofinish();
        exit(0);
    }
  }

}

// FIXME: should reference these in fputest.cpp as above... but how with an enum?
enum {
    numInputs_int32 = 32
};

static const int32 inputs_int32[ numInputs_int32 ] = {
    0xFFFFBB79, 0x405CF80F, 0x00000000, 0xFFFFFD04,
    0xFFF20002, 0x9C8EF795, 0xF00011FF, 0x900006CA,
    0xD0009BFE, 0x6F4862E3, 0x9FFFEFFE, 0xFFFFFFB7,
    0x7BFF7FFF, 0xA000F37A, 0x6011DFFE, 0xE0000006,
    0xFFF02006, 0x7FFFF7D1, 0xC0200003, 0xDE8DF765,
    0x80003E02, 0x900019E8, 0x8008FFFE, 0x5FFFFB5C,
    0xFFDF7FFE, 0xB7C42FBF, 0x9FFFE3FF, 0xC40B9F13,
    0x5FFFFFF8, 0xA001BF56, 0x800017F6, 0x700A908A
};

static const int64 inputs_int64[ numInputs_int32 ] = {
    LIT64( 0x0000000000000000 ),
    LIT64( 0xB7E0000480000000 ),
    LIT64( 0xF3FD2546120B7935 ),
    LIT64( 0x3FF0000000000000 ),
    LIT64( 0xCE07F766F09588D6 ),
    LIT64( 0x8100000000000000 ),
    LIT64( 0x3FCE000400000000 ),
    LIT64( 0x8313B60F0032BED8 ),
    LIT64( 0xC1EFFFFFC0002000 ),
    LIT64( 0x3FB3C75D224F2B0F ),
    LIT64( 0x7FD00000004000FF ),
    LIT64( 0xA12FFF8000001FFF ),
    LIT64( 0x3EE0000000FE0000 ),
    LIT64( 0x0010000080000004 ),
    LIT64( 0x41CFFFFE00000020 ),
    LIT64( 0x40303FFFFFFFFFFD ),
    LIT64( 0x3FD000003FEFFFFF ),
    LIT64( 0xBFD0000010000000 ),
    LIT64( 0xB7FC6B5C16CA55CF ),
    LIT64( 0x413EEB940B9D1301 ),
    LIT64( 0xC7E00200001FFFFF ),
    LIT64( 0x47F00021FFFFFFFE ),
    LIT64( 0xBFFFFFFFF80000FF ),
    LIT64( 0xC07FFFFFE00FFFFF ),
    LIT64( 0x001497A63740C5E8 ),
    LIT64( 0xC4BFFFE0001FFFFF ),
    LIT64( 0x96FFDFFEFFFFFFFF ),
    LIT64( 0x403FC000000001FE ),
    LIT64( 0xFFD00000000001F6 ),
    LIT64( 0x0640400002000000 ),
    LIT64( 0x479CEE1E4F789FE0 ),
    LIT64( 0xC237FFFFFFFFFDFE )
};

int64 int32_umul( int32 a, int32 b ) {
  // FIXME: is there any better way to do this?
  // (temp is only here for debug printing)
  uint64 temp;
  uint64 result = 0;
  temp = 0x00000000FFFFFFFF & ((uint64)((uint16)(a & 0x0000FFFF) * (uint16)(b & 0x0000FFFF)));
  result += temp;
  //io_printf("temp =   0x%016llx\nresult = 0x%016llx\n", temp, result);
  temp = 0x0000FFFFFFFF0000 & ((uint64)((uint16)((a & 0xFFFF0000) >> 16) * (uint16)(b & 0x0000FFFF))) << 16;
  result += temp;
  //io_printf("temp =   0x%016llx\nresult = 0x%016llx\n", temp, result);
  temp = 0x0000FFFFFFFF0000 & ((uint64)((uint16)(a & 0x0000FFFF) * (uint16)((b & 0xFFFF0000) >> 16))) << 16;
  result += temp;
  //io_printf("temp =   0x%016llx\nresult = 0x%016llx\n", temp, result);
  temp = 0xFFFFFFFF00000000 & ((uint64)((uint16)((a & 0xFFFF0000) >> 16) * (uint16)((b & 0xFFFF0000) >> 16))) << 32;
  result += temp;
  //io_printf("temp =   0x%016llx\nresult = 0x%016llx\n", temp, result);
  return result;
}

void fixed_mul_next(int32 &src1, int32 &src2, int64 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  src1 = inputs_int32[ inputNumA ];
  src2 = inputs_int32[ inputNumB ];
    
  inputNumA = ( inputNumA + 1 ) & ( numInputs_int32 - 1 );
  if ( inputNumA == 0 ) ++inputNumB;
  inputNumB = ( inputNumB + 1 ) & ( numInputs_int32 - 1 );

  res = int32_umul( src1, src2 );
}


// My fix 
// Added this for signed multiplication
//

void fixed_smul_next(int32 &src1, int32 &src2, int64 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;
  int sign1, sign2; 
  int32 temp1, temp2;

  src1 = inputs_int32[ inputNumA ];
  src2 = inputs_int32[ inputNumB ];

  inputNumA = ( inputNumA + 1 ) & ( numInputs_int32 - 1 );
  if ( inputNumA == 0 ) ++inputNumB;
  inputNumB = ( inputNumB + 1 ) & ( numInputs_int32 - 1 );

  sign1 = (src1 >> 31) & 0x1;
  sign2 = (src2 >> 31) & 0x1;
  temp1 = src1 & 0x7FFFFFFF;
  temp2 = src2 & 0x7FFFFFFF;
  if (sign1) temp1 = ~src1 + 1;
  if (sign2) temp2 = ~src2 + 1;

  res = int32_umul( temp1, temp2 );  //calling the multiplier function
  if (sign1 ^ sign2)  res = ~res + 1;
    
}
//
//End my Fix

//=======
//My Fix
//Modify the function to calculate the unsigned devision of two numbers
int32 int32_udiv( int64 a, int32 b ) {

  uint32 divisor;
  uint64 divident;
  uint32 temp32;
  uint64 temp64, divisor64;
  int32 res;
  int16 neg_flag;
  int16 n_divisor, n_divident;

  divisor  = b;
  divident = a;
  res = 0;
  n_divisor = 0;                          //number of leading zeros in divisor
  n_divident = 0;                         //number of leading zeros  in divident


  temp32 = divisor;
  while ((temp32 >> 31) == 0) {
    divisor = divisor << 1;
    n_divisor++;
    temp32 = divisor;
  }

  if (divident == 0x0) return(0);          //if divident=0, return 0
  temp64 = divident;
  while ((temp64 >> 63) == 0) {
    divident = divident << 1;
    n_divident++;
    temp64 = divident;
  }

  if (n_divident > n_divisor+32) return(0);        //if divisor > divident return 0
  if (n_divisor > n_divident) return(0xFFFFFFFF);  //if divident >> divisor return MAX

  divisor64 = divisor;
  divisor64 = divisor64 << 32;

  for (int i=0; i < n_divisor + 33 - n_divident; i++){
     neg_flag =1;
     if (!(divident < divisor64)) {
       neg_flag = 0;
       temp64 = divident - divisor64;
     }
     if (neg_flag==0) {                       //if divident > divisor
       res = (res << 1) + 1;                  //add 1 to the right side of result
       divident = temp64;                     //subtract divisor from divident
     }
     else {                                   //if divident < devisor
       res = (res << 1);                      // add 0 to the right side of result
     }
     divisor64 = divisor64 >> 1;              // shift devisor 1 bit to the right for next step
  }
  return res;
}

void fixed_div_next(int64 &src1, int32 &src2, int32 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  src1 = inputs_int64[ inputNumA ];
  src2 = inputs_int32[ inputNumB ];
  if (src2 == 0x0) {
    res= 0xFFFFFFFF;	                 //if divisor=0, return MAX
    if (src1 == 0x0)   res = 0x0;
    return;
  }

  //Make sure src1/src2 has only 32 bits, not more!!
  while ((unsigned long)(src1 >> 32) >= (unsigned long)src2) {
    src1 = (src1 >> 1) & 0x7FFFFFFFFFFFFFFF;
  }

  inputNumA = ( inputNumA + 1 ) & ( numInputs_int32 - 1 );
  if ( inputNumA == 0 ) ++inputNumB;
  inputNumB = ( inputNumB + 1 ) & ( numInputs_int32 - 1 );

  res = int32_udiv( src1, src2 );        //calling the unsigned division function

}

void fixed_sdiv_next(int64 &src1, int32 &src2, int32 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;
  int sign1, sign2; 
  int32 temp2;
  int64 temp1;

  src1 = inputs_int64[ inputNumA ];
  src2 = inputs_int32[ inputNumB ];

  inputNumA = ( inputNumA + 1 ) & ( numInputs_int32 - 1 );
  if ( inputNumA == 0 ) ++inputNumB;
  inputNumB = ( inputNumB + 1 ) & ( numInputs_int32 - 1 );

  sign1 = (src1 >> 63) & 0x1;                    //Getting the signs of inputs
  sign2 = (src2 >> 31) & 0x1;
  temp1 = src1 & 0x7FFFFFFFFFFFFFFF;     //separating the sign from the number
  temp2 = src2 & 0x7FFFFFFF;
  if (sign1) temp1 = ~src1 + 1;          //Do a 2's complement if necessary
  if (sign2) temp2 = ~src2 + 1;

  if (temp2 == 0x0) {
    res= 0x7FFFFFFF;	                  //if divisor=0, return MAX
    if (sign1 ^ sign2)  res = 0x80000000; //if negative return MIN
    if (temp1 == 0x0)   res = 0x0;
    return;
  }

  //Make sure src1/src2 has only 32 bits (31 bits + 1 sign bit), not more!!
  while ((temp1 >> 31) >= temp2) {
    temp1 = (temp1 >> 1) & 0x7FFFFFFFFFFFFFFF;
  }
  src1 = temp1;
  if (sign1) src1 = ~temp1 + 1;

  res = int32_udiv( temp1, temp2 );    //calling the unsigned division function
  if (sign1 ^ sign2)  res = ~res + 1;  //Adding the negative sign if necessary
}


void sf_int_2op_set(unsigned int &start,
			unsigned int &state,
			unsigned int &round,
			unsigned int &op,
			unsigned long long &src1,
			unsigned long long &src2) {

  enum TestType  do_op;
  do_op = do_test;

  static int conta = 0;
  conta++;
  if(conta>=(2*NUM_TESTS)) {
    io_printf("ERROR: Did not finish all the tests (pending = %d)\n", num_tests_pending);
    tf_dofinish();
  }

  if(conta>=NUM_TESTS)
    return; // No more tasks

  num_tests_pending++;
  start = 1;
  
  int64 i64_src1_f; 
  int64 i64_res_f;

  int32 i32_src1_f; 
  int32 i32_src2_f; 
  int32 i32_res_f;

  unsigned long long ll_val1;
  unsigned long long ll_val2;
  unsigned long long ll_res;  

  switch(do_op) {
  case umul_Test:
    op    = OP_C_UMUL;
    fixed_mul_next(i32_src1_f, i32_src2_f, i64_res_f);
    ll_val1 = *(unsigned long *)&i32_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long long*)&i64_res_f;
    break;
  //My fix for signed numbers
  case smul_Test:
    op    = OP_C_SMUL;
    fixed_smul_next(i32_src1_f, i32_src2_f, i64_res_f);
    ll_val1 = *(unsigned long *)&i32_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long long*)&i64_res_f;
  //End my Fix
    break;
  //My Fix: Add udiv case
  case udiv_Test:
    op    = OP_C_UDIV;
    fixed_div_next(i64_src1_f, i32_src2_f, i32_res_f);
    ll_val1 = *(unsigned long long *)&i64_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i32_res_f;
    break;
  //My Fix: Add sdiv case
  case sdiv_Test:
    op    = OP_C_SDIV;
    fixed_sdiv_next(i64_src1_f, i32_src2_f, i32_res_f);
    ll_val1 = *(unsigned long long *)&i64_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i32_res_f;
    break;
  
  
  case umulcc_Test:
    op    = OP_C_UMULCC;
    fixed_mul_next(i32_src1_f, i32_src2_f, i64_res_f);
    ll_val1 = *(unsigned long long *)&i32_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i64_res_f;
    break;
  case smulcc_Test:
    op    = OP_C_SMULCC;
    fixed_smul_next(i32_src1_f, i32_src2_f, i64_res_f);
    ll_val1 = *(unsigned long long *)&i32_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i64_res_f;
    break;
  case udivcc_Test:
    op    = OP_C_UDIVCC;
    fixed_div_next(i64_src1_f, i32_src2_f, i32_res_f);
    ll_val1 = *(unsigned long long *)&i64_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i32_res_f;
    break;
  case sdivcc_Test:
    op   =  OP_C_SDIVCC;
    fixed_sdiv_next(i64_src1_f, i32_src2_f, i32_res_f);
    ll_val1 = *(unsigned long long *)&i64_src1_f;
    ll_val2 = *(unsigned long *)&i32_src2_f;
    ll_res  = *(unsigned long *)&i32_res_f;
    break;
  
  
  default:
    io_printf("ERROR:int2op unknown test [%d]\n",do_op);
    tf_dofinish();
    exit(0);
  }
  
  src1  = ll_val1;
  src2  = ll_val2;
  
  simple_result_head = (simple_result_head+1) % SIMPLE_N_TEST;
  state = simple_result_head;

  if (simple_result[state].in_use) {
    io_printf("ERROR: Still did not finish state[%0x]=0x%016llx op=%d, but it is too long ago\n"
              ,simple_result_head
							,simple_result[simple_result_head].val
							,simple_result[simple_result_head].op
							);
   tf_dofinish();
  }

  simple_result[state].when       = my_gettime();
  simple_result[state].val        = ll_res;
  simple_result[state].op         = do_op;
  simple_result[state].in_use     = 1;
  
  if (start && debug_print) {
    switch (do_op) {
      case umul_Test:
      io_printf("Checking umul: state[%0x] round[%x] op[%x] src1=%u[0x%08x] src2=%u[0x%08x]\n"
	      ,state, round, op, (unsigned long)src1, (unsigned long)src1, (unsigned long)src2, (unsigned long)src2);
        break;
      case smul_Test:
        io_printf("Checking smul: state[%0x] round[%x] op[%x] src1=%d[0x%08x] src2=%d[0x%08x]\n"
	        ,state, round, op, src1, src1, src2, src2);
        break;
      //My Fix: Add udiv case
      case udiv_Test:
        io_printf("Checking udiv: state[%0x] round[%x] op[%x] src1=%lld[0x%016llx] src2=%u[0x%08x]\n"
	         ,state, round, op, (unsigned long)src1, (unsigned long)src1, (unsigned long)src2, (unsigned long)src2);
        break;
      //My Fix: Add sdiv case
      case sdiv_Test:
       io_printf("Checking sdiv: state[%0x] round[%x] op[%x] src1=%lld[0x%016llx] src2=%d[0x%08x]\n"
	       ,state, round, op, src1, src1, src2, src2);
        break;
      
      case umulcc_Test:
      io_printf("Checking umulcc: state[%0x] round[%x] op[%x] src1=%u[0x%08x] src2=%u[0x%08x]\n"
	      ,state, round, op, (unsigned long)src1, (unsigned long)src1, (unsigned long)src2, (unsigned long)src2);
        break;
      case smulcc_Test:
        io_printf("Checking smulcc: state[%0x] round[%x] op[%x] src1=%d[0x%08x] src2=%d[0x%08x]\n"
	        ,state, round, op, src1, src1, src2, src2);
        break;
      //My Fix: Add udiv case
      case udivcc_Test:
        io_printf("Checking udivcc: state[%0x] round[%x] op[%x] src1=%lld[0x%016llx] src2=%u[0x%08x]\n"
	         ,state, round, op, (unsigned long)src1, (unsigned long)src1, (unsigned long)src2, (unsigned long)src2);
        break;
      //My Fix: Add sdiv case
      case sdivcc_Test:
       io_printf("Checking sdivcc: state[%0x] round[%x] op[%x] src1=%lld[0x%016llx] src2=%d[0x%08x]\n"
	       ,state, round, op, src1, src1, src2, src2);
        break;
          
      default:
        io_printf("ERROR: unknown test [%d]\n",do_op);
        tf_dofinish();
        exit(0);
    }
  }
}

void check_delay(int pos, unsigned int fpu_ready) {

  if (!simple_result[pos].in_use)
    return;

  if (fpu_ready)
    return;

  double max_delay;
  if (simple_result[pos].op == float64_add_Test ||
      simple_result[pos].op == float32_add_Test) {
    //max_delay = 6;
    max_delay = 10;
  }else if (simple_result[pos].op == float64_mul_Test ||
	          simple_result[pos].op == float32_mul_Test ||
            simple_result[pos].op == umul_Test ||
            simple_result[pos].op == smul_Test) {
    //max_delay = 8; // 8 cycles max for multiply
    max_delay = 12; // 8 cycles max for multiply
  }else{
    //max_delay = 80; // 80 cycles max for division or SQRT or whatever
    max_delay = 300; // 80 cycles max for division or SQRT or whatever
  }
  max_delay = max_delay*2+20; // Random Delay average slowdown
  max_delay *=time_unit; // 10 clks #5 for clock
  if ( (simple_result[pos].when + max_delay + 1) < my_gettime()) {
    io_printf("ERROR: fpu_state[%0x] should have finished before %lld (now %lld)\n"
	      , pos, simple_result[pos].when+ max_delay, my_gettime());
    tf_dofinish();
  }
}

void sf_float64_2op_check(unsigned int start,
			  unsigned long long fpu_result,
			  unsigned int fpu_state,
			  unsigned int fpu_ready,
        unsigned int in_retry,
        unsigned int out_retry
        ) {

  check_delay(simple_result_head, fpu_ready);

  static int conta=0;
  if (fpu_ready)
    conta++;

  if(conta>=(NUM_TESTS/2-1) && num_tests_pending==0) {
    for(int i=0;i<SIMPLE_N_TEST;i++) {
      if (simple_result[i].in_use) {
        io_printf("ERROR: fpu_state[%0x] never finished\n", fpu_state);
        tf_dofinish();
        return;
      }
    }
    io_printf("Longest successive in retries by TB: %d clks \n",max_ret);
    io_printf("PASS: simple test passed %d checks. Congratulations!\n", conta);
    io_printf("IPC %g!\n", (((double)100*conta))/my_gettime());

    tf_dofinish();
  }
 
  if (!fpu_ready)
    return;

  if (!simple_result[fpu_state].in_use) {
    io_printf("ERROR: fpu_state[%0x] finished (1), but nothing is pending??op =%d in_use = %d do_op = %d\n", fpu_state,simple_result[fpu_state].op,simple_result[fpu_state].in_use,do_test);
    tf_dofinish();
    return;
  }

  // switched to float/double primatives from float32/float64 structs for printing
  float a32 = *(float *)&simple_result[fpu_state].val;
  float b32 = *(float *)&fpu_result;

  double a64 = *(double *)&simple_result[fpu_state].val;
  double b64 = *((double *)(&fpu_result));

  if (fpu_state >= SIMPLE_N_TEST) {
    io_printf("ERROR: invalid fpu_state[%0x] = %llx 32[%g] 64[%g]\n", fpu_state, fpu_result, b32, b64);
    tf_dofinish();
  }
  
  switch ( simple_result[fpu_state].op ) {
    case float64_add_Test:
	  
      if (fpu_result != simple_result[fpu_state].val ) {
        io_printf("ERROR: add 64 check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG

        io_printf("SUCCESS: float64_add_Test expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
      }
      break;
    case float64_sub_Test:
	    if (fpu_result != simple_result[fpu_state].val ) {
        io_printf("ERROR: sub 64 check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
       // tf_dofinish();
      } else if(debug_print){  // DEBUG

        io_printf("SUCCESS: float64_sub_Test expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
      }
      break;
    case float64_mul_Test:
      if (fpu_result != simple_result[fpu_state].val ) {
        io_printf("ERROR: float64_mul check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float64_mul_Test expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
      }
      break;
    case float64_div_Test:
        if (fpu_result != simple_result[fpu_state].val ) {
        io_printf("ERROR: float64_div check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float64_div_Test expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
      }
      break;
    case float64_sqrt_Test:
       if (fpu_result != simple_result[fpu_state].val ) {
       io_printf("ERROR: float64_sqrt check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	        , b64, fpu_result
	        , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float64_sqrt_Test expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
      }
      break;
    case float32_add_Test:
	     if (fpu_result != (simple_result[fpu_state].val &  0x00000000FFFFFFFF )) {
        io_printf("ERROR: float32 add check failed expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	         , a32, ( simple_result[fpu_state].val & 	0x00000000FFFFFFFF) 
           , b32, fpu_result
	         , fpu_state);
        tf_dofinish();

      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float32_add_Test expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF) 
	         , b32, fpu_result
	         , fpu_state);
     }
    break;
    case float32_sub_Test:
	     if (fpu_result != (simple_result[fpu_state].val &  0x00000000FFFFFFFF )) {
        io_printf("ERROR: float32_sub check failed expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	         , a32, ( simple_result[fpu_state].val & 	0x00000000FFFFFFFF) 
           , b32, fpu_result
	         , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float32_sub_Test expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF) 
	         , b32, fpu_result
	         , fpu_state);
     }
    break;
    
    case float32_mul_Test:
    
      if (fpu_result != (simple_result[fpu_state].val &  0x00000000FFFFFFFF )) {
        io_printf("ERROR: float32_mul check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a32, ( simple_result[fpu_state].val & 	0x00000000FFFFFFFF) 
           , b32, fpu_result
	         , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float32_mul_Test expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF) 
	         , b32, fpu_result
	         , fpu_state);
     }
    break;
    case float32_div_Test:
         
    
      if (fpu_result != (simple_result[fpu_state].val &  0x00000000FFFFFFFF )) {
        io_printf("ERROR: float32_div check failed expected 64[%g] [0x%016llx], received 64[%g] [0x%016llx] state[%0x]\n"
	         , a32, ( simple_result[fpu_state].val & 	0x00000000FFFFFFFF) 
           , b32, fpu_result
	         , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: float32_div_Test expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF) 
	         , b32, fpu_result
	         , fpu_state);
     }
   break;
   
    case float32_sqrt_Test:
      if (fpu_result != (simple_result[fpu_state].val & 0x00000000FFFFFFFF)) {
        io_printf("ERROR: float32_sqrt check failed expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
       io_printf("SUCCESS: float32_sqrt_Test expected 32[%g] [0x%08llx], received 32[%g] [0x%08llx] state[%0x]\n"
             , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
       	     , b32, fpu_result
	           , fpu_state);
      }
      break;
    default:
      ;
  }

  simple_result[fpu_state].in_use = 0;
  while(!simple_result[simple_result_tail].in_use && simple_result_tail != simple_result_head) {
    // results can finish out of order. skip already finished
    simple_result_tail = (simple_result_tail+1) % SIMPLE_N_TEST;
  }
    

  num_tests_pending--;
  if (num_tests_pending)
    check_delay(simple_result_tail, fpu_ready);

}

void sf_int_2op_check(unsigned int start,
			  unsigned long long fpu_result,
			  unsigned int fpu_state,
			  unsigned int fpu_ready,
        unsigned int in_retry,
        unsigned int out_retry
        ) {

  check_delay(simple_result_head, fpu_ready);

  static int conta=0;
  if (fpu_ready)
    conta++;

  if(conta>=(NUM_TESTS/2-1) && num_tests_pending==0) {
    for(int i=0;i<SIMPLE_N_TEST;i++) {
      if (simple_result[i].in_use) {
        io_printf("ERROR: fpu_state[%0x] never finished\n", fpu_state);
        tf_dofinish();
        return;
      }
    }
    io_printf("Longest successive in retries by TB: %d clks \n",max_ret);
    io_printf("PASS: simple test passed %d checks. Congratulations!\n", conta);
    io_printf("IPC %g!\n", (((double)100*conta))/my_gettime());
    
    tf_dofinish();
  }
 
  if (!fpu_ready)
    return;

  if (!simple_result[fpu_state].in_use) {
    io_printf("ERROR: fpu_state[%0x] finished (2), but nothing is pending?? op = %d in_use = %d \n", fpu_state,simple_result[fpu_state].op,simple_result[fpu_state].in_use);
    tf_dofinish();
    return;
  }

  unsigned int a32 = (unsigned int)simple_result[fpu_state].val;
  unsigned int b32 = (unsigned int)fpu_result;

  unsigned long long a64 = simple_result[fpu_state].val;
  unsigned long long b64 = fpu_result;

  if (fpu_state >= SIMPLE_N_TEST) {
    io_printf("ERROR: invalid fpu_state[%0x] = %llx 32[%g] 64[%g]\n", fpu_state, fpu_result, b32, b64);
    tf_dofinish();
  }
  
  //io_printf("done[0x%llx] state[%0x]\n", fpu_result , fpu_state);
  
  switch ( simple_result[fpu_state].op ) {
    case umul_Test:
      if (fpu_result != simple_result[fpu_state].val ) {
       io_printf("ERROR: umul check failed expected 64[%u] [0x%016llx], received 64[%u] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: umul_Test expected 64[%u] [0x%016llx], received 64[%u] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	          , b64, fpu_result
          , fpu_state);
      }
      break;
    case smul_Test:
      if (fpu_result != simple_result[fpu_state].val ) {
          io_printf("ERROR: smul check failed expected 64[%lld] [0x%016llx], received 64[%lld] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS:smul_Test expected 64[%lld] [0x%016llx], received 64[%lld] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
      }
      break;
    case udiv_Test:
      if (fpu_result != (simple_result[fpu_state].val & 0x00000000FFFFFFFF)) {
        io_printf("ERROR: udiv check failed expected 32[%lld] [0x%08llx], received 32[%lld] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS:udiv_Test expected 32[%lld] [0x%08llx], received 32[%lld] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
      }
      break;
    case sdiv_Test:
      if (fpu_result != (simple_result[fpu_state].val & 0x00000000FFFFFFFF)) {
        io_printf("ERROR: sdiv check failed expected 32[%d] [0x%08llx], received 32[%d] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	         , fpu_state);
       tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: sdiv expected 32[%d] [0x%08llx], received 32[%d] [0x%08llx] state[%0x]\n"
	        , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
      }
      
     break;
    
    
    case umulcc_Test:
      if (fpu_result != simple_result[fpu_state].val ) {
       io_printf("ERROR: umulcc check failed expected 64[%u] [0x%016llx], received 64[%u] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	         , b64, fpu_result
	         , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: umulcc_Test expected 64[%u] [0x%016llx], received 64[%u] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	          , b64, fpu_result
          , fpu_state);
      }
      break;
    case smulcc_Test:
      if (fpu_result != simple_result[fpu_state].val ) {
          io_printf("ERROR: smulcc check failed expected 64[%lld] [0x%016llx], received 64[%lld] [0x%016llx] state[%0x]\n"
	          , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS:smulcc_Test expected 64[%lld] [0x%016llx], received 64[%lld] [0x%016llx] state[%0x]\n"
	         , a64, simple_result[fpu_state].val
	          , b64, fpu_result
	          , fpu_state);
        io_printf("smulcc use value: %d,", simple_result[fpu_state].in_use);
      }
      break;
    case udivcc_Test:
      if (fpu_result != (simple_result[fpu_state].val & 0x00000000FFFFFFFF)) {
        io_printf("ERROR: udivcc check failed expected 32[%lld] [0x%08llx], received 32[%lld] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
        tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS:udivcc_Test expected 32[%lld] [0x%08llx], received 32[%lld] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
      }
      break;
    case sdivcc_Test:
      if (fpu_result != (simple_result[fpu_state].val & 0x00000000FFFFFFFF)) {
        io_printf("ERROR: sdivcc check failed expected 32[%d] [0x%08llx], received 32[%d] [0x%08llx] state[%0x]\n"
	          , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	         , fpu_state);
       tf_dofinish();
      } else if(debug_print){  // DEBUG
        io_printf("SUCCESS: sdivcc expected 32[%d] [0x%08llx], received 32[%d] [0x%08llx] state[%0x]\n"
	        , a32, (simple_result[fpu_state].val & 0x00000000FFFFFFFF)
	          , b32, fpu_result
	          , fpu_state);
      }
    break;
    default:
      break;
  }

  simple_result[fpu_state].in_use = 0;
  
  while(!simple_result[simple_result_tail].in_use && simple_result_tail != simple_result_head) {
    // results can finish out of order. skip already finished
    simple_result_tail = (simple_result_tail+1) % SIMPLE_N_TEST;
  }

  num_tests_pending--;
  if (num_tests_pending)
    check_delay(simple_result_tail, fpu_ready);

}

//states for FSM
#define IDLE  0
#define START 1
#define HOLD 2
static unsigned int dut_state = IDLE; 
static unsigned int  dut_state_next = IDLE;

int fpu_check() {

  // system signals
  // unsigned int clk     = tf_getp(1);
  unsigned int reset   = tf_getp(2);

  // called whenever in reset
  if(reset) {
    do_reset();
    tf_putp(12, rand()&1); // Either zero or one, but no X to in_retry
    return 0;
  }

  // inputs
  unsigned int start   = tf_getp(3);
  unsigned int state   = tf_getp(4);
  //   unsigned int round   = tf_getp(5);
  //   unsigned int op      = tf_getp(6);
  //   unsigned long long src1   = my_getlongp(7);
  //   unsigned long long src2   = my_getlongp(8);
  // outputs
  unsigned long long fpu_result = my_getlongp(9);
  unsigned int fpu_state        = tf_getp(10);
  unsigned int fpu_ready        = tf_getp(11);
  
  unsigned int out_retry        = tf_getp(13);           //out_retry is asserted by the fpu's denorm stage .. similar to fpu_busy
  unsigned int in_retry         = tf_getp(12);

  if(out_retry){
    dut_state = HOLD;   // DUT forced to stay on HOLD if out_retry is detected at check.. better impl?
  }  
  
  
  if(!in_retry){          // TB cannot check for outputs if it sends out a retry`

    if (simple_result[fpu_state].op < umul_Test ) {
      sf_float64_2op_check(start, fpu_result, fpu_state, fpu_ready, out_retry, in_retry);
    } else {
      sf_int_2op_check(start, fpu_result, fpu_state, fpu_ready, out_retry, in_retry);
    }
  
  }

  return 0;
}

int fpu_set() {
  // system signals
  //unsigned int clk     = tf_getp(1);
  unsigned int reset   = tf_getp(2);
  static int busy_clks = 0;
  unsigned int out_retry = tf_getp(13);          
  // inputs
  static unsigned int start;
  static unsigned int state;
  static unsigned int round;
  static unsigned int op;
  static unsigned long long src1;
  static unsigned long long src2;
  static unsigned int in_retry;
  // called whenever in reset
  static int next_reset = 0;
  static int idle_time;


  static bool initialized = false;
  
  next_reset--;
  if (next_reset <= 0) {
    next_reset = (random() % 4096)+102400; // reset every 5K cycles
    reset = 1;
    tf_putp(2, reset);
    tf_putp(12, rand()&1); // Either zero or one, but no X to in_retry
  }
  
  if(reset) {
    static int conta = (random() & 15) + 20; 
    dut_state = 0;
    dut_state_next = 0;

    
    if (conta <= 0) {
      conta = 20+(random() % 31);

      reset = 0;
      tf_putp(2, reset);
      tf_putp(12, rand()&1); // Either zero or one, but no X to in_retry
      start = 0;
      tf_putp(3, start);
    }
    conta--;
      
    do_reset();
    initialized = false;
    return 0;
  }

  if (!initialized) {
    static int conta = 0;

    static int last_clk = 0;
    int cmd;

    if (conta <= 300) {
      cmd  = 5; // Reset
    }else if (conta <= 600) {
      if (reclock_enable) {
        cmd  = 1; // Reclock_Enable = 1 
        //io_printf("1.cmd = %d\n",cmd);
      }else{
        cmd  = 0; // No reclock by default
        //io_printf("2.cmd = %d\n",cmd);
      }
    }else{
      cmd  = 1; //0; // Reclock_NOP = 0
    }

    tf_putp(14,cmd);
    tf_putp(15,last_clk?1:0);

    if ((conta & 1)) {
      last_clk = !last_clk;
    }
    conta++;
    if (conta >900) {
      // 100 cycles == 50 stages supported, 900 = 150 3 phase stages
      initialized = true;
      conta = 0;
    }

    dut_state = IDLE;
    idle_time++;

  }else{
    tf_putp(14,0); // rci.cmd
    tf_putp(15,0); // rci.clk
  }
  
  // state machine for "stage" like interface
  switch(dut_state){

      case IDLE:  start          = 0;
                  state          = 0;
                  round          = 0;
                  op             = 0;
                  src1           = 0;
                  src2           = 0;
                  if (do_test == rank_Test)
                    idle_time      = 1;
                  else
                    idle_time      = get_idle_time();

                  idle_time--; 

                  if(idle_time <= 0)
                    dut_state_next = START;
                  else
                    dut_state_next = IDLE;
                  

                  break;

      case START: if(benchTest){
                    int testSelect = rand_next_bench();
                    do_test = (TestType)testSelect;
                   }else if(random_test_flag){       
                    do_test = get_rnd_test();
                   }  
                  //io_printf("\ndo_test: %d\n", do_test); 
                  
                  if ( do_test < umul_Test ) {                                    
                    sf_float64_2op_set(start, state, round, op, src1, src2); 
                  } else {
                    sf_int_2op_set(start, state, round, op, src1, src2);    
                  }

                  if(out_retry){
                    dut_state_next = HOLD;
                  }else{  
                    dut_state_next = IDLE;
                  }

                  break;
      case HOLD:  //KEEP static values

                  if(out_retry){
                    dut_state_next = HOLD;
                  }else{
                    dut_state_next = IDLE;
                  }

                  break;
  }

  static uint64_t stats[random_Test] = {0,};
  static uint64_t total_stats=0;
  if (start) {
    stats[do_test]++;
    total_stats++;
#if 0
    if ((total_stats & 127) == 0) {
      io_printf("stats:");
      for(int i=0;i<random_Test;i++) {
        io_printf(" %lld", stats[i]);
      }
      io_printf("\n");
    }
#endif
  }

  //TB : in retry generation from TB +retry=off switch to test without retries
  if(retry_flg){
    in_retry = 0;
  }else{  
   in_retry      = ((rand()%3) == 1) ;
   static int iter = 0 ; //longest retry asserted by TB required for (sanity check)   
   if(in_retry == 1) iter++;
   
   if(in_retry == 0){
     if(iter>max_ret){
       max_ret = iter;
     }
   iter = 0;
   }
   
   if(iter > 20){       // MAX continuos in_retry high
    in_retry = 0;
    }
  }
  // Check for busy for too long
  if(out_retry) {
    busy_clks++;
    if (busy_clks>500) {
      io_printf("ERROR: FPU busy for %d cycles\n", busy_clks);
      tf_dofinish();
    }
  }else{
    busy_clks = 0;
  }

  tf_putp(3, start);
  tf_putp(4, state);
  tf_putp(5, round);
  tf_putp(6, op);
  my_putlongp(7, src1);
  my_putlongp(8, src2);
  tf_putp(12, in_retry);
  
  dut_state = dut_state_next;  
  
  return 0;
}

int inc_stat1(){
  return 0;
}

int inc_stat2(){
  return 0;
}
        
int params_check() {

  return 0;
}
