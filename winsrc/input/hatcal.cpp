// $Header: x:/prj/tech/libsrc/input/RCS/hatcal.cpp 1.5 1996/10/10 16:50:06 TOML Exp $

#include <joystick.h>
#include <fanjoy.h>

//
// The Watcom C++ parser is verbose in warning of possible integral
// truncation, even in cases where for the given native word size
// there is no problem (i.e., assigning longs to  ints).  Here, we
// quiet the warnings, although it wouldn't be bad for someone to
// evaluate them. (toml 09-14-96)
//
#ifdef __WATCOMC__
#pragma warning 389 9
#endif

#define ABS(a) ((a)<0?-(a):(a))

#define NUM_CATS (5)
int vals[NUM_CATS];
int standard[NUM_CATS] = { 0, 250, 500, 750, 1000 };

#define abs(a) ((a)<0?-(a):(a))

static int filter_range=1000;
#define FILTER_THRESHOLD (filter_range/5)
#define FILTER_DELAY (15)
static int filter_read( void )
{
   joy_read_pots_raw( NULL );

   return joy_raw[3];
}

// Given new max/min, set guess for all values
static void guess_based_on_range( void )
{
   int i;
   // Set the range properly for the filter
   filter_range = vals[NUM_CATS-1]-vals[0];

   // Guess values between min and max
   for (i=1; i<NUM_CATS-1; ++i)
      vals[i] = vals[0] +
         ((long)(standard[i]-standard[0])*(long)(filter_range)) /
         (long)(standard[NUM_CATS-1]-standard[0]);

   // want to detect if not valid hat, check range
   for (i=0; i<4; ++i)
      thrustmaster_middles[i] = (vals[i]+vals[i+1])/2;
}

void hatcal_calib( void )
{
   int v;
   v = filter_read();

   if (v<vals[0])
   {
      vals[0] = v;
      guess_based_on_range();
   }
   if (v>vals[NUM_CATS-1])
   {
      vals[NUM_CATS-1] = v;
      guess_based_on_range();
   }

}

void hatcal_init( void )
{
   // prime the filter and get an initial reading
   vals[0] = vals[NUM_CATS-1] = filter_read();

   // Guess_Based_On_Range.
   guess_based_on_range();
}
