/*
 * $Header: x:/prj/tech/winsrc/input/RCS/joyold.cpp 1.1 1997/10/06 20:46:19 JON Exp $
*/

#include <stdio.h>
#include <string.h>

#include <mprintf.h>

#include <recapi.h>

#include <defehand.h>
#include <inpcompo.h>

#include <joystick.h>

#ifndef _WIN32
#include <basjoy.h>
#include <ngpjoy.h>
#include <biosjoy.h>
#else
#include <fanjoy.h>
#include <joywin.h>
#endif

static const char pszJoyReadButtonsTag[] = "JoyReadButtons";
static const char pszJoyReadPotsRawTag[] = "JoyReadPotsRaw";
static const char pszJoyReadPotsTag[]    = "JoyReadPots";

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


#define DEAD_LIMIT 10 // pots with values lower than this are dead

bool joy_try_bios=0;
bool joy_dead[4];
bool joy_mask[4] = {1,1,1,1};
ulong joy_type=JOY_NONE;

// Throttle is not autocenter
static bool autocenter[4]={1,1,1,0};

static void joy_cook( char *cooked, short *raw );

void  (*read_pots_raw)( short *buf ) = 0;
uchar (*fancy_stick_hack)( short *buf ) = 0;
uchar (*read_buttons_fun)( void ) = 0;

#ifndef _WIN32

void basic_use( void )
{
   read_pots_raw = basic_read_pots_raw;
   read_buttons_fun = basic_read_buttons;
}

void ngp_use( void )
{
   read_pots_raw = ngp_read_pots_raw;
   read_buttons_fun = ngp_read_buttons;
}

void bios_use( void )
{
   read_pots_raw = &bios_read_pots_raw;
   read_buttons_fun = &bios_read_buttons;
}

#else

void win32joy_use( void )
{
   read_pots_raw = &win32joy_read_pots_raw;
   read_buttons_fun = &win32joy_read_buttons;
}

#endif

void null_read_pots_raw( short *raw ) { memset( raw, 0, 4*sizeof(*raw) ); }
uchar null_read_buttons( void ) { return 0; }

void nothing_use( void )
{
   read_pots_raw = &null_read_pots_raw;
   read_buttons_fun = &null_read_buttons;
}

short joyd_range[4][2];
short joyd_center[4];

#define START_RADIUS 80
#define NONAUTOCENTER_LOW_GUESS 500
#define NONAUTOCENTER_HIGH_GUESS 900

void joy_guess_range( int pot )
{
   int radius;

   if (autocenter[pot])
   {
      radius = joyd_center[pot];
      // usually the top bottoms out or tops out
      if (2*radius > 2500) radius = 2500 - joyd_center[pot];
      // bring it in a bit
      radius = 8*radius/10;
      joyd_range[pot][0]=joyd_center[pot]-radius;
      // 30 is universal min
      if (joyd_range[pot][0]<30) joyd_range[pot][0] = 30;
      joyd_range[pot][1]=joyd_center[pot]+radius;
   }
   else
   {
      joyd_range[pot][0]=NONAUTOCENTER_LOW_GUESS;
      joyd_range[pot][1]=NONAUTOCENTER_HIGH_GUESS;
   }
}

void joy_guess_ranges( void )
{
   int i;

#ifndef _WIN32
   for (i=0; i<4; ++i)
      joy_guess_range( i );

#else
// Under windows, we're centered at 128,
// and have 9 bits of range, so we want 
// to set these absolute ranges
   for (i=0; i<4; ++i)
   {
      joyd_range[i][0] = 128-256;
      joyd_range[i][1] = 128+256;
   }   
#endif
}

#if 0
// SPUFF is a guess at the joystick movement radius
#define SPUFF 400
static void joy_guess_range( void )
{
   int i;
   for (i=0; i<4; ++i)
   {
      joyd_range[i][0]=joyd_center[i]-SPUFF;
      joyd_range[i][1]=joyd_center[i]+SPUFF;
   }
}
#endif

#define MAX_RAW 7000

// essentially scratch pad memory
short joy_raw[4];
void joy_read_pots_raw( short *raw )
{
   static short last_raw[4];

   memcpy(last_raw,joy_raw,4*2);
   if (joy_type == JOY_NONE)
   {
       if (raw)
          memset( raw, 0, sizeof(*raw)*4 );
   }
   else
   {
      if (read_pots_raw)
      {
         (*read_pots_raw)( joy_raw );
         // Take a recording here, since this is what
         // really goes and gets the input
         RecStreamAddOrExtract(g_pInputRecorder, joy_raw, sizeof(joy_raw), pszJoyReadPotsRawTag);

         if ((joy_raw[0]>=MAX_RAW)||(joy_raw[1]>=MAX_RAW)) {
            memcpy(joy_raw,last_raw,4*2);
         }
      }
      if (raw) memcpy( raw, joy_raw, 4*2 );
   }
}

void joy_center( void )
{
   joy_read_pots_raw( joyd_center );
}

void joy_read_pots( char *buf )
{
   memset( buf, 0, 4 );
   joy_read_pots_raw( NULL );
   if (fancy_stick_hack) (*fancy_stick_hack)(joy_raw);
   joy_cook( buf, joy_raw );

   // Since fancy stick hack is not recorded, record here as well
   RecStreamAddOrExtract(g_pInputRecorder, buf, 4*sizeof(char), pszJoyReadPotsTag);
}

uchar joy_read_buttons( void )
{
   uchar buts;

   if (joy_type == JOY_NONE)
      buts=0;
   else
      if (read_buttons_fun)
      {
         if (fancy_stick_hack)
            buts = (*fancy_stick_hack)(NULL);
         else
            buts = (*read_buttons_fun)();
      }
      else
         buts = 0;

   // take a recording here to get the buttons               
   RecStreamAddOrExtract(g_pInputRecorder, &buts, sizeof(buts), pszJoyReadButtonsTag);

   return buts;
}

uchar joy_readmethod = JOY_READMETHOD_NONE;

short joy_init( int jt )
{
   short pots=0;

   // First go and set up for
   // recording
   GetInputComponents();
   ConnectInput();

   fancy_stick_hack=0;
   read_pots_raw=0;
   read_buttons_fun=0;

   joy_type = jt;

   if ((joy_type&JOY_TYPE_MASK)==JOY_NONE)
      return 0;

#ifndef _WIN32
   if ((joy_type&JOY_NO_NGP)==0)
   {
      pots=ngp_init();
      joy_type&=~JOY_NO_NGP;
      ngp_use();
      joy_readmethod = JOY_READMETHOD_NGP;
   }

   if (pots == 0)
   {
      pots = basic_init();
      basic_use();
      joy_readmethod = JOY_READMETHOD_BASIC;
   }

   if (pots == 0 && joy_try_bios)
   {
      pots = bios_init();
      bios_use(); 
      joy_readmethod = JOY_READMETHOD_BIOS;
   }

#else
   // Basic Win32 support

   if (pots == 0)
   {
      pots = win32joy_init();
      win32joy_use();
      // @TBD (toml 02-06-96): Must pick or create correct thing here
      joy_readmethod = JOY_READMETHOD_BIOS;
   }
#endif

   if (pots == 0)
   {
      nothing_use();
      joy_readmethod = JOY_READMETHOD_NONE;
   }

   if (pots!=0)
   {
      switch (joy_type&JOY_TYPE_MASK)
      {
       case JOY_NORM:   break;
#ifndef _WIN32
       case JOY_FLPRO:  fancy_stick_hack=fancy_flpro; break;
       case JOY_THRUST: fancy_stick_hack=fancy_thrust; break;
#else
      case JOY_FLPRO:
      case JOY_THRUST:
      fancy_stick_hack=fancy_win32;
#endif
      }
   }


   joy_center();

   joy_guess_ranges();

   {
      short buf[4]; int i;
      memset( buf, 0, 8 );
      (*read_pots_raw)( buf );
      if (fancy_stick_hack) (*fancy_stick_hack)( buf );

#ifndef _WIN32
      for (i=0; i<4; ++i) {
         joy_dead[i] = (buf[i]<DEAD_LIMIT) || (!joy_mask[i]);
      }
#else
      // Check the caps bits instead, much better
      win32joy_set_dead(joy_dead);
      for (i=0; i<4; ++i) {
         joy_dead[i] = joy_dead[i] || (!joy_mask[i]);
      }
#endif


   }

   return pots;
}

static bool pot_calibrated[4];
void joy_calibrate_start( void )
{
   int i;

   memset( pot_calibrated, 0, sizeof(pot_calibrated) );

   // guess center
   joy_center();

   // cause if you're going to cal, this is in range, guaranteed
   for (i=0; i<4; ++i) {
      joyd_range[i][0] = joyd_center[i];
      joyd_range[i][1] = joyd_center[i];
   }
}

#define AVG_CALIB
#define BASE_SET_SIZE 3
//void joy_calibrate( uchar *butval, char cooked[4], short raw[4], short rawmax[4][2], bool rangeset )
void joy_calibrate( int pot )
{
#ifdef AVG_CALIB
   short base_set[BASE_SET_SIZE][4];
   int i;
#endif
   short raw[4];
#ifdef AVG_CALIB
   for (i=0; i<BASE_SET_SIZE; i++)
	   joy_read_pots_raw( base_set[i] );
   raw[pot]=(base_set[0][pot]+base_set[1][pot]+base_set[2][pot])/3;
#else
   joy_read_pots_raw( raw );
#endif

   if (raw[pot]<joyd_range[pot][0]) joyd_range[pot][0]=raw[pot];
   if (raw[pot]>joyd_range[pot][1]) joyd_range[pot][1]=raw[pot];
   pot_calibrated[pot] = 1;
}

#define NOISE_THRESHOLD 100
#define ABS(a) ((a)<0?-(a):(a))
void joy_calibrate_end( void )
{
   int i;

   // Re-guess the ranges for anthing that wasnt calibrated
   for (i=0; i<4; ++i)
      if (!pot_calibrated[i])
         joy_guess_range( i );
}

/* history for idiotic filter */
static char cooked_hist[2][4]={
   0,0,0,0,
   0,0,0,0
};
static int cooked_hist_index=0;

#define CLIP(a) (a)=(a)>127?127:(a)<-128?-128:(a)

int joy_dead_zone_percent=7;

static void joy_cook( char *cooked, short *raw )
{
   short cook, i;

   cooked_hist_index=cooked_hist_index^1;

   for (i=0; i<4; i++)
   {
      if ( (raw[i]==0) || joy_dead[i])
         cook = 0;
      else
      {
         if (autocenter[i])
         {
            if (raw[i] < joyd_center[i])
            {
               int radius;
               int dead = ((joyd_range[i][0]-joyd_center[i])*joy_dead_zone_percent)/100;
               int center = joyd_center[i] + dead;
               if (raw[i]>center) raw[i]=center;
               radius = (joyd_range[i][0]-center);
               cook = ((raw[i]-center)*-128) / (radius?radius:1);
            }
            else
            {
               int radius;
               int dead = ((joyd_range[i][1]-joyd_center[i])*joy_dead_zone_percent)/100;
               int center = joyd_center[i] + dead;
               if (raw[i]<center) raw[i]=center;
               radius = (joyd_range[i][1]-center);
               cook = ((raw[i]-center)* 127) / (radius?radius:1);
            }
         }
         else
         {
            cook = ((raw[i]-joyd_range[i][0])*255) / (joyd_range[i][1]-joyd_range[i][0]);
            cook -= 128;
         }
      }
      CLIP(cook);

      /* idiotic filter */
      if (cooked_hist[0][i]==cooked_hist[1][i])
         cooked[i]=cooked_hist[0][i];
      else
         cooked[i]=cook;
      cooked_hist[cooked_hist_index][i]=cook;
   }
}
