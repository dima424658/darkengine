/*
 * fancy joystick hack
 */
#include <stdlib.h>

#include <stdio.h>

#include <joystick.h>

#ifndef _WIN32
#include <basjoy.h>
#else
#include <joywin.h>
#endif

#include <fanjoy.h>

extern uchar (*read_buttons_fun)( void );

uchar fancy_flpro( short *raw )
{  // move buttons to coolie, argh
   uchar bv;

   if (raw!=NULL) return 0;

   bv=(*read_buttons_fun)();
   switch (bv)
   {
   case 3:  bv=JOY_HAT_W; break;
   case 7:  bv=JOY_HAT_S; break;
   case 11: bv=JOY_HAT_E; break;
   case 15: bv=JOY_HAT_N; break;
   }

   return bv;
}

//short thrustmaster_middles[5]={30,300,570,840,1110};
// actually edges now, not middles
short thrustmaster_middles[]={165,435,705,975};

uchar fancy_thrust( short *raw )
{  // move throttle to coolie, punt throttle
   short tmp[4];
   int fv;
//   int i;
   uchar bv, nv;

   if (raw!=NULL)
   { raw[3]=0; return 0; }

   bv=(*read_buttons_fun)();
#ifndef _WIN32
   // @TBD (toml 02-06-96): why basic_read_pots_raw() instead of (*read_pots_raw)()?
   basic_read_pots_raw(tmp);
#else
   win32joy_read_pots_raw(tmp);
#endif
   fv=tmp[3];

   if (fv < thrustmaster_middles[0]) nv = JOY_HAT_N;
   else if (fv < thrustmaster_middles[1] ) nv = JOY_HAT_E;
   else if (fv < thrustmaster_middles[2] ) nv = JOY_HAT_S;
   else if (fv < thrustmaster_middles[3] ) nv = JOY_HAT_W;
   else nv = 0;

   return bv|nv;
}
