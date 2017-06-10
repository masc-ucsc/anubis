
/*============================================================================

This C source file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY THE AUTHOR, JOHN HAUSER, (possibly via similar legal
warning) AGAINST ALL LOSSES, COSTS, OR OTHER PROBLEMS INCURRED BY THEIR
CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "milieu.h"
#include "softfloat.h"


enum {
    minIterations = 1000
};

static void fail( const char *message, ... )
{
    va_list varArgs;

    fputs( "timesoftfloat: ", stderr );
    va_start( varArgs, message );
    vfprintf( stderr, message, varArgs );
    va_end( varArgs );
    fputs( ".\n", stderr );
    exit( EXIT_FAILURE );

}

static char *functionName;
static char *roundingPrecisionName, *roundingModeName, *tininessModeName;

static void reportTime( int32 count, long clocks )
{

    printf(
        "%8.1f kops/s: %s",
        ( count / ( ( (float) clocks ) / CLOCKS_PER_SEC ) ) / 1000,
        functionName
    );
    if ( roundingModeName ) {
        if ( roundingPrecisionName ) {
            fputs( ", precision ", stdout );
            fputs( roundingPrecisionName, stdout );
        }
        fputs( ", rounding ", stdout );
        fputs( roundingModeName, stdout );
        if ( tininessModeName ) {
            fputs( ", tininess ", stdout );
            fputs( tininessModeName, stdout );
            fputs( " rounding", stdout );
        }
    }
    fputc( '\n', stdout );

}

enum {
    numInputs_int32 = 32
};

static const int32 inputs_int32[ numInputs_int32 ] = {
    0xFFFFBB79, 0x405CF80F, 0x00000000, 0xFFFFFD04,
    0xFFF20002, 0x0C8EF795, 0xF00011FF, 0x000006CA,
    0x00009BFE, 0xFF4862E3, 0x9FFFEFFE, 0xFFFFFFB7,
    0x0BFF7FFF, 0x0000F37A, 0x0011DFFE, 0x00000006,
    0xFFF02006, 0xFFFFF7D1, 0x10200003, 0xDE8DF765,
    0x00003E02, 0x000019E8, 0x0008FFFE, 0xFFFFFB5C,
    0xFFDF7FFE, 0x07C42FBF, 0x0FFFE3FF, 0x040B9F13,
    0xBFFFFFF8, 0x0001BF56, 0x000017F6, 0x000A908A
};

static void time_a_int32_to_float32( ) {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    int32 src1 = inputs_int32[ inputNum ];
    float32 res = int32_to_float32( src1 );

    fprintf(stderr,"int32_to_float32(%d) -> %f\n",src1, float32_to_float64(res));

    inputNum = ( inputNum + 1 ) & ( numInputs_int32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );

}

static void time_a_int32_to_float64() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    int32 src1 = inputs_int32[ inputNum ];
    float64 res = int32_to_float64( src1 );

    fprintf(stderr,"int32_to_float64(%d) -> %f\n",src1, res);

    inputNum = ( inputNum + 1 ) & ( numInputs_int32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}



enum {
    numInputs_int64 = 32
};

static const int64 inputs_int64[ numInputs_int64 ] = {
    LIT64( 0xFBFFC3FFFFFFFFFF ),
    LIT64( 0x0000000003C589BC ),
    LIT64( 0x00000000400013FE ),
    LIT64( 0x0000000000186171 ),
    LIT64( 0xFFFFFFFFFFFEFBFA ),
    LIT64( 0xFFFFFD79E6DFFC73 ),
    LIT64( 0x0000000010001DFF ),
    LIT64( 0xDD1A0F0C78513710 ),
    LIT64( 0xFFFF83FFFFFEFFFE ),
    LIT64( 0x00756EBD1AD0C1C7 ),
    LIT64( 0x0003FDFFFFFFFFBE ),
    LIT64( 0x0007D0FB2C2CA951 ),
    LIT64( 0x0007FC0007FFFFFE ),
    LIT64( 0x0000001F942B18BB ),
    LIT64( 0x0000080101FFFFFE ),
    LIT64( 0xFFFFFFFFFFFF0978 ),
    LIT64( 0x000000000008BFFF ),
    LIT64( 0x0000000006F5AF08 ),
    LIT64( 0xFFDEFF7FFFFFFFFE ),
    LIT64( 0x0000000000000003 ),
    LIT64( 0x3FFFFFFFFF80007D ),
    LIT64( 0x0000000000000078 ),
    LIT64( 0xFFF80000007FDFFD ),
    LIT64( 0x1BBC775B78016AB0 ),
    LIT64( 0xFFF9001FFFFFFFFE ),
    LIT64( 0xFFFD4767AB98E43F ),
    LIT64( 0xFFFFFEFFFE00001E ),
    LIT64( 0xFFFFFFFFFFF04EFD ),
    LIT64( 0x07FFFFFFFFFFF7FF ),
    LIT64( 0xFFFC9EAA38F89050 ),
    LIT64( 0x00000020FBFFFFFE ),
    LIT64( 0x0000099AE6455357 )
};

static void time_a_int64_to_float32() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    int64 src1 = inputs_int64[ inputNum ];
    float32 res = int64_to_float32( src1 );

    fprintf(stderr,"int64_to_float32(%lld) -> %f\n", src1, float32_to_float64(res));

    inputNum = ( inputNum + 1 ) & ( numInputs_int32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_a_int64_to_float64() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    int64 src1 = inputs_int64[ inputNum ];
    float64 res = int64_to_float64( src1 );

    fprintf(stderr,"int64_to_float64(%lld) -> %f\n", src1, res);

    inputNum = ( inputNum + 1 ) & ( numInputs_int32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}


enum {
    numInputs_float32 = 32
};

static const float32 inputs_float32[ numInputs_float32 ] = {
    0x4EFA0000, 0xC1D0B328, 0x80000000, 0x3E69A31E,
    0xAF803EFF, 0x3F800000, 0x17BF8000, 0xE74A301A,
    0x4E010003, 0x7EE3C75D, 0xBD803FE0, 0xBFFEFF00,
    0x7981F800, 0x431FFFFC, 0xC100C000, 0x3D87EFFF,
    0x4103FEFE, 0xBC000007, 0xBF01F7FF, 0x4E6C6B5C,
    0xC187FFFE, 0xC58B9F13, 0x4F88007F, 0xDF004007,
    0xB7FFD7FE, 0x7E8001FB, 0x46EFFBFF, 0x31C10000,
    0xDB428661, 0x33F89B1F, 0xA3BFEFFF, 0x537BFFBE
};

static void time_a_float32_to_int32() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    float32 src1 = inputs_float32[ inputNum ];
    int32   res  = float32_to_int32( src1 );

    fprintf(stderr,"float32_to_int32(%f) -> %d\n", float32_to_float64(src1), res);

    inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_a_float32_to_int32_round_to_zero() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    float32 src1 = inputs_float32[ inputNum ];
    int32   res  = float32_to_int32_round_to_zero( src1 );

    fprintf(stderr,"float32_to_int32_round_to_zero(%f) -> %d\n", float32_to_float64(src1), res);

    inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}


static void time_a_float32_to_int64() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    float32 src1 = inputs_float32[ inputNum ];
    int64   res  = float32_to_int64( src1 );

    fprintf(stderr,"float32_to_int64(%f) -> %lldd\n", float32_to_float64(src1), res);

    inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_a_float32_to_int64_round_to_zero() {
  clock_t startClock, endClock;
  int32 i;
  int8 inputNum;
  
  inputNum = 0;
  startClock = clock();
  for ( i =minIterations ; i; --i ) {
    float32 src1 = inputs_float32[ inputNum ];
    int64   res  = float32_to_int64_round_to_zero( src1 );

    fprintf(stderr,"float32_to_int64_round_to_zero(%f) -> %lldd\n", float32_to_float64(src1), res);

    inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_a_float32_z_float64( float64 function( float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float32[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float32[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_az_float32( float32 function( float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float32[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float32[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_ab_float32_z_flag( flag function( float32, float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

void float32_add_next(float32 &src1, float32 &src2, float32 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;
   
   //src1 = 0x4EFA0000;
   //src2 = 0xCEFA0000;
  //      do{
    src1 = inputs_float32[ inputNumA ];
    src2 = inputs_float32[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );

    //      }while(float64_lt_quiet(0,src1) || float64_lt_quiet(0,src2));

  res  = float32_add(src1, src2);
}

void float32_sub_next(float32 &src1, float32 &src2, float32 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

   //     do{
   src1 = inputs_float32[ inputNumA ];
   src2 = inputs_float32[ inputNumB ];
   // src1 = inputs_float32[ 8 ];
   // src2 = inputs_float32[ 8 ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );

     //     }while(float64_lt_quiet(0,src1) || float64_lt_quiet(0,src2));

  res  = float32_sub(src1,  src2);
}
static void time_a_float32_add() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float32 src1; 
    float32 src2; 
    float32 res;
    float32_add_next(src1, src2, res);
#ifdef DEBUG
    fprintf(stderr,"float32_add(%f,%f) -> %f\n", src1, src2, res);
#endif
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_sub_float32( float32 function( float32, float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_div_float32( float32 function( float32, float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_rem_float32( float32 function( float32, float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float32[ inputNumA ], inputs_float32[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static const float32 inputs_float32_pos[ numInputs_float32 ] = {
    0x4EFA0000, 0x41D0B328, 0x00000000, 0x3E69A31E,
    0x2F803EFF, 0x3F800000, 0x17BF8000, 0x674A301A,
    0x4E010003, 0x7EE3C75D, 0x3D803FE0, 0x3FFEFF00,
    0x7981F800, 0x431FFFFC, 0x4100C000, 0x3D87EFFF,
    0x4103FEFE, 0x3C000007, 0x3F01F7FF, 0x4E6C6B5C,
    0x4187FFFE, 0x458B9F13, 0x4F88007F, 0x5F004007,
    0x37FFD7FE, 0x7E8001FB, 0x46EFFBFF, 0x31C10000,
    0x5B428661, 0x33F89B1F, 0x23BFEFFF, 0x537BFFBE
};

static void time_az_float32_pos( float32 function( float32 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float32_pos[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float32_pos[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float32 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

enum {
    numInputs_float64 = 32
};

static const float64 inputs_float64[ numInputs_float64 ] = {

//	LIT64( 0x0000000000000000 ),   // exist to test special cases
//	LIT64( 0x7FF0000000000000 ),
//	LIT64( 0xFFF0000000000000 ),
//	LIT64( 0x7FF0000800000000 ),
//	LIT64( 0xFFF0020200002000 ),	
    LIT64( 0x422FFFC008000000 ),
    LIT64( 0xB7E0000480000000 ),
    LIT64( 0xF3FD2546120B7935 ),
    LIT64( 0x3FF0000000000000 ),
    LIT64( 0xCE07F766F09588D6 ),
    LIT64( 0x8000000000000000 ),
    LIT64( 0x3FCE000400000000 ),
    LIT64( 0x8313B60F0032BED8 ),
    LIT64( 0xC1EFFFFFC0002000 ),
    LIT64( 0x3FB3C75D224F2B0F ),
    LIT64( 0x7FD00000004000FF ),
    LIT64( 0xA12FFF8000001FFF ),
    LIT64( 0x3EE0000000FE0000 ),
//  LIT64( 0x0010000080000004 ), TODO
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

static void time_a_float64_z_int32( int32 function( float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float64[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float64[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_a_float64_z_int64( int64 function( float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float64[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float64[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_a_float64_z_float32( float32 function( float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float64[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float64[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}



static void time_az_float64( float64 function( float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float64[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float64[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static void time_ab_float64_z_flag( flag function( float64, float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float64[ inputNumA ], inputs_float64[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float64[ inputNumA ], inputs_float64[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

void float64_add_next(float64 &src1, float64 &src2, float64 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  //      do{
    src1 = inputs_float64[ inputNumA ];
    src2 = inputs_float64[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );

    //      }while(float64_lt_quiet(0,src1) || float64_lt_quiet(0,src2));

  res  = float64_add(src1, src2);
}

void float64_sub_next(float64 &src1, float64 &src2, float64 &res) {

  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

    
  //      do{
   // src1 = 0x8313B60F0032BED8 ;
    //src2 = 0x8313B60F0032BED8 ;
    src1 = inputs_float64[ inputNumA ];
    src2 = inputs_float64[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );

    //      }while(float64_lt_quiet(0,src1) || float64_lt_quiet(0,src2));

  res  = float64_sub(src1, src2);

}
static void time_a_float64_add() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float64 src1; 
    float64 src2; 
    float64 res;
    float64_add_next(src1, src2, res);
#ifdef DEBUG
    fprintf(stderr,"float64_add(%f,%f) -> %f\n", src1, src2, res);
#endif
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

void float64_mul_next(float64 &src1, float64 &src2, float64 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  do{
    src1 = inputs_float64[ inputNumA ];
    src2 = inputs_float64[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );

    float_exception_flags = 0;
    res  = float64_mul(src1, src2);

  }while(float_exception_flags!=0);
  //	 res == LIT64( 0x7ff0000000000000 )  // FIXME: handle inf

}

static void time_a_float64_mul() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float64 src1; 
    float64 src2; 
    float64 res;
    float64_mul_next(src1, src2, res);
    fprintf(stderr,"float64_mul(%f,%f) -> %f\n", src1, src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

void float64_div_next(float64 &src1, float64 &src2, float64 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  do{
    src1 = inputs_float64[ inputNumA ];
    src2 = inputs_float64[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );

    float_exception_flags = 0;
    res  = float64_div(src1, src2);

  }while(float_exception_flags==float_flag_divbyzero || float_exception_flags == float_flag_invalid);
  //	 res == LIT64( 0x7ff0000000000000 )  // FIXME: handle inf

}

static void time_a_float64_div() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float64 src1; 
    float64 src2; 
    float64 res;
    float64_div_next(src1, src2, res);
    fprintf(stderr,"float64_div(%f,%f) -> %f\n", src1, src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}



void float64_sqrt_next(float64 &src2, float64 &res) {
  
  static int8 inputNumB = 0;

//  do{
    src2 = inputs_float64[ inputNumB ];
    
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );

    float_exception_flags = 0;
    res  = float64_sqrt(src2);

  //}while(float_exception_flags == float_flag_invalid);

}

static void time_a_float64_sqrt() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float64 src2; 
    float64 res;
    float64_sqrt_next(src2, res);
    fprintf(stderr,"float64_sqrt(%f) -> %f\n", src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}


void float32_mul_next(float32 &src1, float32 &src2, float32 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  do{
    src1 = inputs_float32[ inputNumA ];
    src2 = inputs_float32[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );

    float_exception_flags = 0;
    res  = float32_mul(src1, src2);

  }while(float_exception_flags==float_flag_divbyzero || float_exception_flags == float_flag_invalid);
  //	 res == LIT64( 0x7ff0000000000000 )  // FIXME: handle inf

}

static void time_a_float32_mul() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float32 src1; 
    float32 src2; 
    float32 res;
    float32_mul_next(src1, src2, res);
    fprintf(stderr,"float32_mul(%f,%f) -> %f\n", src1, src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

void float32_div_next(float32 &src1, float32 &src2, float32 &res) {
  
  static int8 inputNumA = 0;
  static int8 inputNumB = 0;

  do{
    src1 = inputs_float32[ inputNumA ];
    src2 = inputs_float32[ inputNumB ];
    
    inputNumA = ( inputNumA + 1 ) & ( numInputs_float32 - 1 );
    if ( inputNumA == 0 ) ++inputNumB;
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );

    float_exception_flags = 0;
    res  = float32_div(src1, src2);

  }while(float_exception_flags==float_flag_divbyzero || float_exception_flags == float_flag_invalid);

}

static void time_a_float32_div() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float32 src1; 
    float32 src2; 
    float32 res;
    float32_div_next(src1, src2, res);
    fprintf(stderr,"float32_div(%f,%f) -> %f\n", src1, src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

void float32_sqrt_next(float32 &src2, float32 &res) {
  
  static int8 inputNumB = 0;

  do{
    src2 = inputs_float32[ inputNumB ];
    
    inputNumB = ( inputNumB + 1 ) & ( numInputs_float32 - 1 );

    float_exception_flags = 0;
    res  = float32_sqrt(src2);

  }while(float_exception_flags == float_flag_invalid);

}

static void time_a_float32_sqrt() {

  clock_t startClock, endClock;
  int32 i;
  startClock = clock();
  for ( i = minIterations; i; --i ) {
    float32 src2; 
    float32 res;
    float32_sqrt_next(src2, res);
    fprintf(stderr,"float32_sqrt(%f) -> %f\n", src2, res);
  }
  endClock = clock();
  reportTime( minIterations, endClock - startClock );
}

static void time_abz_float64( float64 function( float64, float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNumA, inputNumB;

    count = 0;
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function(
                inputs_float64[ inputNumA ], inputs_float64[ inputNumB ] );
            inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
            if ( inputNumA == 0 ) ++inputNumB;
            inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNumA = 0;
    inputNumB = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
            function(
                inputs_float64[ inputNumA ], inputs_float64[ inputNumB ] );
        inputNumA = ( inputNumA + 1 ) & ( numInputs_float64 - 1 );
        if ( inputNumA == 0 ) ++inputNumB;
        inputNumB = ( inputNumB + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}

static const float64 inputs_float64_pos[ numInputs_float64 ] = {
    LIT64( 0x422FFFC008000000 ),
    LIT64( 0x37E0000480000000 ),
    LIT64( 0x73FD2546120B7935 ),
    LIT64( 0x3FF0000000000000 ),
    LIT64( 0x4E07F766F09588D6 ),
    LIT64( 0x0000000000000000 ),
    LIT64( 0x3FCE000400000000 ),
    LIT64( 0x0313B60F0032BED8 ),
    LIT64( 0x41EFFFFFC0002000 ),
    LIT64( 0x3FB3C75D224F2B0F ),
    LIT64( 0x7FD00000004000FF ),
    LIT64( 0x212FFF8000001FFF ),
    LIT64( 0x3EE0000000FE0000 ),
 // LIT64( 0x0110000080000004 ),
    LIT64( 0x41CFFFFE00000020 ),
    LIT64( 0x40303FFFFFFFFFFD ),
    LIT64( 0x3FD000003FEFFFFF ),
    LIT64( 0x3FD0000010000000 ),
    LIT64( 0x37FC6B5C16CA55CF ),
    LIT64( 0x413EEB940B9D1301 ),
    LIT64( 0x47E00200001FFFFF ),
    LIT64( 0x47F00021FFFFFFFE ),
    LIT64( 0x3FFFFFFFF80000FF ),
    LIT64( 0x407FFFFFE00FFFFF ),
    LIT64( 0x001497A63740C5E8 ),
    LIT64( 0x44BFFFE0001FFFFF ),
    LIT64( 0x16FFDFFEFFFFFFFF ),
    LIT64( 0x403FC000000001FE ),
    LIT64( 0x7FD00000000001F6 ),
    LIT64( 0x0640400002000000 ),
    LIT64( 0x479CEE1E4F789FE0 ),
    LIT64( 0x4237FFFFFFFFFDFE )
};

static void time_az_float64_pos( float64 function( float64 ) )
{
    clock_t startClock, endClock;
    int32 count, i;
    int8 inputNum;

    count = 0;
    inputNum = 0;
    startClock = clock();
    do {
        for ( i = minIterations; i; --i ) {
            function( inputs_float64_pos[ inputNum ] );
            inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
        }
        count += minIterations;
    } while ( clock() - startClock < CLOCKS_PER_SEC );
    inputNum = 0;
    startClock = clock();
    for ( i = count; i; --i ) {
        function( inputs_float64_pos[ inputNum ] );
        inputNum = ( inputNum + 1 ) & ( numInputs_float64 - 1 );
    }
    endClock = clock();
    reportTime( count, endClock - startClock );

}



enum {
    INT32_TO_FLOAT32 = 1,
    INT32_TO_FLOAT64,
    INT64_TO_FLOAT32,
    INT64_TO_FLOAT64,
    FLOAT32_TO_INT32,
    FLOAT32_TO_INT32_ROUND_TO_ZERO,
    FLOAT32_TO_INT64,
    FLOAT32_TO_INT64_ROUND_TO_ZERO,
    FLOAT32_TO_FLOAT64,
    FLOAT32_ROUND_TO_INT,
    FLOAT32_ADD,
    FLOAT32_SUB,
    FLOAT32_MUL,
    FLOAT32_DIV,
    FLOAT32_REM,
    FLOAT32_SQRT,
    FLOAT32_EQ,
    FLOAT32_LE,
    FLOAT32_LT,
    FLOAT32_EQ_SIGNALING,
    FLOAT32_LE_QUIET,
    FLOAT32_LT_QUIET,
    FLOAT64_TO_INT32,
    FLOAT64_TO_INT32_ROUND_TO_ZERO,
    FLOAT64_TO_INT64,
    FLOAT64_TO_INT64_ROUND_TO_ZERO,
    FLOAT64_TO_FLOAT32,
    FLOAT64_ROUND_TO_INT,
    FLOAT64_ADD,
    FLOAT64_SUB,
    FLOAT64_MUL,
    FLOAT64_DIV,
    FLOAT64_REM,
    FLOAT64_SQRT,
    FLOAT64_EQ,
    FLOAT64_LE,
    FLOAT64_LT,
    FLOAT64_EQ_SIGNALING,
    FLOAT64_LE_QUIET,
    FLOAT64_LT_QUIET,
    NUM_FUNCTIONS
};

static struct {
    char *name;
    int8 numInputs;
    flag roundingPrecision, roundingMode;
    flag tininessMode, tininessModeAtReducedPrecision;
} functions[ NUM_FUNCTIONS ] = {
    { 0, 0, 0, 0, 0, 0 },
    { "int32_to_float32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "int32_to_float64",                1, FALSE, FALSE, FALSE, FALSE },
    { "int64_to_float32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "int64_to_float64",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_to_int32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_to_int32_round_to_zero",  1, FALSE, FALSE, FALSE, FALSE },
    { "float32_to_int64",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_to_int64_round_to_zero",  1, FALSE, FALSE, FALSE, FALSE },
    { "float32_to_float64",              1, FALSE, FALSE, FALSE, FALSE },
    { "float32_round_to_int",            1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_add",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_sub",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_mul",                     2, FALSE, TRUE,  TRUE,  FALSE },
    { "float32_div",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_rem",                     2, FALSE, FALSE, FALSE, FALSE },
    { "float32_sqrt",                    1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_eq",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_le",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_lt",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_eq_signaling",            2, FALSE, FALSE, FALSE, FALSE },
    { "float32_le_quiet",                2, FALSE, FALSE, FALSE, FALSE },
    { "float32_lt_quiet",                2, FALSE, FALSE, FALSE, FALSE },
    { "float64_to_int32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float64_to_int32_round_to_zero",  1, FALSE, FALSE, FALSE, FALSE },
    { "float64_to_int64",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float64_to_int64_round_to_zero",  1, FALSE, FALSE, FALSE, FALSE },
    { "float64_to_float32",              1, FALSE, TRUE,  TRUE,  FALSE },
    { "float64_round_to_int",            1, FALSE, TRUE,  FALSE, FALSE },
    { "float64_add",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float64_sub",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float64_mul",                     2, FALSE, TRUE,  TRUE,  FALSE },
    { "float64_div",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float64_rem",                     2, FALSE, FALSE, FALSE, FALSE },
    { "float64_sqrt",                    1, FALSE, TRUE,  FALSE, FALSE },
    { "float64_eq",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float64_le",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float64_lt",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float64_eq_signaling",            2, FALSE, FALSE, FALSE, FALSE },
    { "float64_le_quiet",                2, FALSE, FALSE, FALSE, FALSE },
    { "float64_lt_quiet",                2, FALSE, FALSE, FALSE, FALSE },
};

enum {
    ROUND_NEAREST_EVEN = 1,
    ROUND_TO_ZERO,
    ROUND_DOWN,
    ROUND_UP,
    NUM_ROUNDINGMODES
};
enum {
    TININESS_BEFORE_ROUNDING = 1,
    TININESS_AFTER_ROUNDING,
    NUM_TININESSMODES
};

static void
 timeFunctionVariety(
     uint8 functionCode,
     int8 roundingPrecision,
     int8 roundingMode,
     int8 tininessMode
 )
{
    uint8 roundingCode;
    int8 tininessCode;

    functionName = functions[ functionCode ].name;
    if ( roundingPrecision == 32 ) {
        roundingPrecisionName = "32";
    }
    else if ( roundingPrecision == 64 ) {
        roundingPrecisionName = "64";
    }
    else if ( roundingPrecision == 80 ) {
        roundingPrecisionName = "80";
    }
    else {
        roundingPrecisionName = 0;
    }
    switch ( roundingMode ) {
     case 0:
        roundingModeName = 0;
        roundingCode = float_round_nearest_even;
        break;
     case ROUND_NEAREST_EVEN:
        roundingModeName = "nearest_even";
        roundingCode = float_round_nearest_even;
        break;
     case ROUND_TO_ZERO:
        roundingModeName = "to_zero";
        roundingCode = float_round_to_zero;
        break;
     case ROUND_DOWN:
        roundingModeName = "down";
        roundingCode = float_round_down;
        break;
     case ROUND_UP:
        roundingModeName = "up";
        roundingCode = float_round_up;
        break;
    }
    float_rounding_mode = roundingCode;
    switch ( tininessMode ) {
     case 0:
        tininessModeName = 0;
        tininessCode = float_tininess_after_rounding;
        break;
     case TININESS_BEFORE_ROUNDING:
        tininessModeName = "before";
        tininessCode = float_tininess_before_rounding;
        break;
     case TININESS_AFTER_ROUNDING:
        tininessModeName = "after";
        tininessCode = float_tininess_after_rounding;
        break;
    }
    float_detect_tininess = tininessCode;
    switch ( functionCode ) {
     case INT32_TO_FLOAT32:
       time_a_int32_to_float32();
       break;
     case INT32_TO_FLOAT64:
        time_a_int32_to_float64();
        break;
     case INT64_TO_FLOAT32:
        time_a_int64_to_float32();
        break;
     case INT64_TO_FLOAT64:
        time_a_int64_to_float64();
        break;
     case FLOAT32_TO_INT32:
       time_a_float32_to_int32();
        break;
     case FLOAT32_TO_INT32_ROUND_TO_ZERO:
       time_a_float32_to_int32_round_to_zero();
       break;
    case FLOAT32_TO_INT64:
      time_a_float32_to_int64();
      break;
     case FLOAT32_TO_INT64_ROUND_TO_ZERO:
       time_a_float32_to_int64_round_to_zero();
       break;
    case FLOAT32_TO_FLOAT64:
      time_a_float32_z_float64( float32_to_float64 );
      break;
    case FLOAT32_ROUND_TO_INT:
        time_az_float32( float32_round_to_int );
        break;
     case FLOAT32_SUB:
        time_sub_float32( float32_sub );
        break;
     case FLOAT32_REM:
        time_rem_float32( float32_rem );
        break;
     case FLOAT32_SQRT:
        time_az_float32_pos( float32_sqrt );
        break;
     case FLOAT32_EQ:
        time_ab_float32_z_flag( float32_eq );
        break;
     case FLOAT32_LE:
        time_ab_float32_z_flag( float32_le );
        break;
     case FLOAT32_LT:
        time_ab_float32_z_flag( float32_lt );
        break;
     case FLOAT32_EQ_SIGNALING:
        time_ab_float32_z_flag( float32_eq_signaling );
        break;
     case FLOAT32_LE_QUIET:
        time_ab_float32_z_flag( float32_le_quiet );
        break;
     case FLOAT32_LT_QUIET:
        time_ab_float32_z_flag( float32_lt_quiet );
        break;
     case FLOAT64_TO_INT32:
        time_a_float64_z_int32( float64_to_int32 );
        break;
     case FLOAT64_TO_INT32_ROUND_TO_ZERO:
        time_a_float64_z_int32( float64_to_int32_round_to_zero );
        break;
     case FLOAT64_TO_INT64:
        time_a_float64_z_int64( float64_to_int64 );
        break;
     case FLOAT64_TO_INT64_ROUND_TO_ZERO:
        time_a_float64_z_int64( float64_to_int64_round_to_zero );
        break;
     case FLOAT64_TO_FLOAT32:
        time_a_float64_z_float32( float64_to_float32 );
        break;
     case FLOAT64_ROUND_TO_INT:
        time_az_float64( float64_round_to_int );
        break;
	
	// BEGIN: Added to SCOORE/FPU
    case FLOAT64_ADD:
      time_a_float64_add();
      break;
    case FLOAT64_MUL:
      time_a_float64_mul( );
      break;
    case FLOAT64_DIV:
      time_a_float64_div( );
      break;
    case FLOAT32_ADD:
      time_a_float32_add();
      break;
    case FLOAT32_MUL:
      time_a_float32_mul( );
      break;
    case FLOAT32_DIV:
      time_a_float32_div( );
      break;
      
       // END: 
     case FLOAT64_SUB:
        time_abz_float64( float64_sub );
        break;
     case FLOAT64_REM:
        time_abz_float64( float64_rem );
        break;
     case FLOAT64_SQRT:
        time_az_float64_pos( float64_sqrt );
        break;
     case FLOAT64_EQ:
        time_ab_float64_z_flag( float64_eq );
        break;
     case FLOAT64_LE:
        time_ab_float64_z_flag( float64_le );
        break;
     case FLOAT64_LT:
        time_ab_float64_z_flag( float64_lt );
        break;
     case FLOAT64_EQ_SIGNALING:
        time_ab_float64_z_flag( float64_eq_signaling );
        break;
     case FLOAT64_LE_QUIET:
        time_ab_float64_z_flag( float64_le_quiet );
        break;
     case FLOAT64_LT_QUIET:
        time_ab_float64_z_flag( float64_lt_quiet );
        break;
    }

}

static void
 timeFunction(
     uint8 functionCode,
     int8 roundingPrecisionIn,
     int8 roundingModeIn,
     int8 tininessModeIn
 )
{
    int8 roundingPrecision, roundingMode, tininessMode;

    roundingPrecision = 32;
    for (;;) {
        if ( ! functions[ functionCode ].roundingPrecision ) {
            roundingPrecision = 0;
        }
        else if ( roundingPrecisionIn ) {
            roundingPrecision = roundingPrecisionIn;
        }
        for ( roundingMode = 1;
              roundingMode < NUM_ROUNDINGMODES;
              ++roundingMode
            ) {
            if ( ! functions[ functionCode ].roundingMode ) {
                roundingMode = 0;
            }
            else if ( roundingModeIn ) {
                roundingMode = roundingModeIn;
            }
            for ( tininessMode = 1;
                  tininessMode < NUM_TININESSMODES;
                  ++tininessMode
                ) {
                if (    ( roundingPrecision == 32 )
                     || ( roundingPrecision == 64 ) ) {
                    if ( ! functions[ functionCode ]
                               .tininessModeAtReducedPrecision
                       ) {
                        tininessMode = 0;
                    }
                    else if ( tininessModeIn ) {
                        tininessMode = tininessModeIn;
                    }
                }
                else {
                    if ( ! functions[ functionCode ].tininessMode ) {
                        tininessMode = 0;
                    }
                    else if ( tininessModeIn ) {
                        tininessMode = tininessModeIn;
                    }
                }
                timeFunctionVariety(
                    functionCode, roundingPrecision, roundingMode, tininessMode
                );
                if ( tininessModeIn || ! tininessMode ) break;
            }
            if ( roundingModeIn || ! roundingMode ) break;
        }
        if ( roundingPrecisionIn || ! roundingPrecision ) break;
        if ( roundingPrecision == 80 ) {
            break;
        }
        else if ( roundingPrecision == 64 ) {
            roundingPrecision = 80;
        }
        else if ( roundingPrecision == 32 ) {
            roundingPrecision = 64;
        }
    }

}

void usage() {

  fprintf(stderr,"timesoftfloat [<option>...] <function>\n");
  fprintf(stderr,"  <option>:  (* is default)\n");
  fprintf(stderr,"    -help            --Write this message and exit.\n");
  fprintf(stderr,"    -nearesteven     --Only time rounding to nearest/even.\n");
  fprintf(stderr,"    -tozero          --Only time rounding to zero.\n");
  fprintf(stderr,"    -down            --Only time rounding down.\n");
  fprintf(stderr,"    -up              --Only time rounding up.\n");
  fprintf(stderr,"    -tininessbefore  --Only time underflow tininess before rounding.\n");
  fprintf(stderr,"    -tininessafter   --Only time underflow tininess after rounding.\n");
  fprintf(stderr,"  <function>:\n");
  fprintf(stderr,"    int32_to_<float>                 <float>_add   <float>_eq\n");
  fprintf(stderr,"    <float>_to_int32                 <float>_sub   <float>_le\n");
  fprintf(stderr,"    <float>_to_int32_round_to_zero   <float>_mul   <float>_lt\n");
  fprintf(stderr,"    int64_to_<float>                 <float>_div   <float>_eq_signaling\n");
  fprintf(stderr,"    <float>_to_int64                 <float>_rem   <float>_le_quiet\n");
  fprintf(stderr,"    <float>_to_int64_round_to_zero                 <float>_lt_quiet\n");
  fprintf(stderr,"    <float>_to_<float>\n");
  fprintf(stderr,"    <float>_round_to_int\n");
  fprintf(stderr,"    <float>_sqrt\n");
  fprintf(stderr,"    -all1            --All 1-operand functions.\n");
  fprintf(stderr,"    -all2            --All 2-operand functions.\n");
  fprintf(stderr,"    -all             --All functions.\n");
  fprintf(stderr,"  <float>:\n");
  fprintf(stderr,"    float32          --Single precision.\n");
  fprintf(stderr,"    float64          --Double precision.\n");

}

int fputest(int argc, char **argv) {

  char *argPtr;
  flag functionArgument;
  uint8 functionCode;
  int8 operands, roundingPrecision, roundingMode, tininessMode;

  if ( argc <= 1 ) {
    usage();
    return 0;
  }

  functionArgument = FALSE;
  functionCode = 0;
  operands = 0;
  roundingPrecision = 0;
  roundingMode = 0;
  tininessMode = 0;
  --argc;
  ++argv;
  while ( argc && ( argPtr = argv[ 0 ] ) ) {
    if ( argPtr[ 0 ] == '-' ) ++argPtr;
    if ( strcmp( argPtr, "help" ) == 0 ) {
      usage();
      return 0;
    } else if (    ( strcmp( argPtr, "nearesteven" ) == 0 )
		   || ( strcmp( argPtr, "nearest_even" ) == 0 ) ) {
      roundingMode = ROUND_NEAREST_EVEN;
    } else if (    ( strcmp( argPtr, "tozero" ) == 0 )
		   || ( strcmp( argPtr, "to_zero" ) == 0 ) ) {
      roundingMode = ROUND_TO_ZERO;
    } else if ( strcmp( argPtr, "down" ) == 0 ) {
      roundingMode = ROUND_DOWN;
    } else if ( strcmp( argPtr, "up" ) == 0 ) {
      roundingMode = ROUND_UP;
    } else if ( strcmp( argPtr, "tininessbefore" ) == 0 ) {
      tininessMode = TININESS_BEFORE_ROUNDING;
    } else if ( strcmp( argPtr, "tininessafter" ) == 0 ) {
      tininessMode = TININESS_AFTER_ROUNDING;
    } else if ( strcmp( argPtr, "all1" ) == 0 ) {
      functionArgument = TRUE;
      functionCode = 0;
      operands = 1;
    } else if ( strcmp( argPtr, "all2" ) == 0 ) {
      functionArgument = TRUE;
      functionCode = 0;
      operands = 2;
    } else if ( strcmp( argPtr, "all" ) == 0 ) {
      functionArgument = TRUE;
      functionCode = 0;
      operands = 0;
    } else {
      for ( functionCode = 1;
	    functionCode < NUM_FUNCTIONS;
	    ++functionCode 
	    ) {
	if ( strcmp( argPtr, functions[ functionCode ].name ) == 0 ) {
	  break;
	}
      }
      if ( functionCode == NUM_FUNCTIONS ) {
	fail( "Invalid option or function `%s'", argv[ 0 ] );
      }
      functionArgument = TRUE;
    }
    --argc;
    ++argv;
  }
  if ( ! functionArgument ) 
    fail( "Function argument required" );

  if ( functionCode ) {
    timeFunction(functionCode, roundingPrecision, roundingMode, tininessMode );
  } else if ( operands == 1 ) {
    for ( functionCode = 1; functionCode < NUM_FUNCTIONS; ++functionCode
	  ) {
      if ( functions[ functionCode ].numInputs == 1 ) {
	timeFunction(functionCode, roundingPrecision, roundingMode, tininessMode);
      }
    }
  } else if ( operands == 2 ) {
    for ( functionCode = 1; functionCode < NUM_FUNCTIONS; ++functionCode
	  ) {
      if ( functions[ functionCode ].numInputs == 2 ) {
	timeFunction(functionCode, roundingPrecision, roundingMode, tininessMode);
      }
    }
  } else {
    for ( functionCode = 1; functionCode < NUM_FUNCTIONS; ++functionCode
	  ) {
      timeFunction(functionCode, roundingPrecision, roundingMode, tininessMode );
    }
  }

  return 0;
}

#ifdef NATIVE
int main(int argc, char **argv) {

  return fputest(argc, argv);
}
#endif
