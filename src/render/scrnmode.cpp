/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnmode.cpp,v 1.20 1999/10/14 15:07:08 BFarquha Exp $

#include <stdlib.h>
#include <appagg.h>
#include <comtools.h>
#include <scrnmode.h>
#include <dispapi.h>

#include <scrnman.h>
#include <scrnguid.h>

#include <2d.h>

#include <config.h>

extern "C"
{
#include <lgd3d.h>
#include <g2.h>
}

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


//
// Describe the screen mode as an sScrnMode
//

void ScrnModeGet(sScrnMode* current)
{
   current->valid_fields = 0;
   AutoAppIPtr_(DisplayDevice,pDispDev);

   sGrModeInfoEx info;
   pDispDev->GetMode(&info);

   if (info.flags & kGrModeNotSet)
      return;

   // Set the dims
   current->w = info.width;
   current->h = info.height;
   current->valid_fields |= kScrnModeDimsValid;

   // Set the bit depth
   current->bitdepth = info.depth;
   current->valid_fields |= kScrnModeBitDepthValid;

   // figure out flags
   current->flags = 0;

   // Windowed
   if (info.flags & kGrModeIsWindowed)
      current->flags |= kScrnModeWindowed;
   else
      current->flags |= kScrnModeFullScreen;

   eDisplayDeviceKind kind;
   int flags;
   GUID* pDD = NULL;
   pDispDev->GetKind2(&kind,&flags,&pDD);

   // 3d
   if (flags & kDispAttempt3D)
      current->flags |= kScrnMode3dDriver;
   // 2d
   if (kind == kDispFullScreen)
      current->flags |= kScrnMode2dDriver;

   current->valid_fields |= kScrnModeFlagsValid;
}

//
// Default scrn mode
//


static sScrnMode def_scrnmode =
{
   kScrnModeAllValid,
   640, 480,
   16,
   kScrnModeFullScreen|kScrnMode2dDriver
};

EXTERN void ScrnModeSetDefault(const sScrnMode* new_def)
{
   ScrnModeCopy(&def_scrnmode,new_def,kScrnModeAllValid);
}

const sScrnMode* ScrnModeGetDefault(void)
{
   return &def_scrnmode;
}


//
// Set the mode
//
static sScrnMode none = { 0 };  // mode with no valid fields


#define VALID(pmode,flagroot) ((pmode)->valid_fields & kScrnMode##flagroot##Valid)
#define MIN_3D_BITDEPTH 16

// squeeze the target within constraints
void constrain(sScrnMode* target, const sScrnMode* mmin, const sScrnMode* mmax);


BOOL ScrnModeSet(const sScrnMode* mmin, const sScrnMode* prefer, const sScrnMode* mmax)
{
   // Get rid of null pointers, for convenience
   if (!mmin) mmin = &none;
   if (!prefer) prefer = &none;
   if (!mmax) mmax = &none;

   // Check invariants
   // DIMS
   AssertMsg(!(VALID(mmin,Dims) && VALID(mmax,Dims)
             && (mmin->w > mmax->w
                 || mmin->h > mmax->h)),
             "Impossible screen dims constraint!");
   // Depth
   AssertMsg(!(VALID(mmin,BitDepth) && VALID(mmax,BitDepth)
             && mmin->bitdepth > mmax->bitdepth),
             "Impossible screen depth constraint!");

   // Flags
   AssertMsg(!(VALID(mmin,Flags) && VALID(mmax,Flags)
             && (mmin->flags & ~mmax->flags)),
             "Impossible screen flags constraint!");


   // Build target sScrnMode
   sScrnMode current;
   ScrnModeGet(&current); // defaults
   sScrnMode target = *prefer;

   //
   // fill invalid fields of target from current, then default
   //
   ScrnModeCopy(&target,&current,~target.valid_fields);
   ScrnModeCopy(&target,&def_scrnmode,~target.valid_fields);


   // apply constraints
   constrain(&target,mmin,mmax);
   constrain(&current,mmin,mmax);

   //
   // Apply special constraints
   //

   // If you don't have either windowed or fullscreen, get them from the current
   if (VALID(&target,Flags) && VALID(&current,Flags) && !(target.flags & (kScrnModeWindowed|kScrnModeFullScreen)))
   {
      target.flags |= current.flags & (kScrnModeWindowed|kScrnModeFullScreen);
      // check against maximim
      Assert_(!(VALID(mmax,Flags) && (target.flags & ~mmax->flags)));
   }

   ScrnModeValidate(&target);

   // check against maximim
   if (VALID(&target,Flags) && (target.flags & kScrnMode3dDriver))
   {
      Assert_(!(VALID(mmax,BitDepth) && mmax->bitdepth < MIN_3D_BITDEPTH));
      Assert_(!(VALID(mmax,Flags) && (target.flags & ~mmax->flags)));
   }

   // check against minimum
   Assert_(!(VALID(mmin,Flags) && (~target.flags & mmin->flags)));


   return ScrnSetModeRaw(&target);
}

//
// Apply constraints to a mode description
//

static void constrain(sScrnMode* target, const sScrnMode* mmin, const sScrnMode* mmax)
{
   if (VALID(target,Dims))
   {
      if (VALID(mmin,Dims))
      {
         if (mmin->w > target->w) target->w = mmin->w;
         if (mmin->h > target->h) target->h = mmin->h;
      }

      if (VALID(mmax,Dims))
      {
         if (mmax->w < target->w) target->w = mmax->w;
         if (mmax->h < target->h) target->h = mmax->h;
      }
   }

   if (VALID(target,BitDepth))
   {
      if (VALID(mmin,BitDepth) && mmin->bitdepth > target->bitdepth)
         target->bitdepth = mmin->bitdepth;

      if (VALID(mmax,BitDepth) && mmax->bitdepth < target->bitdepth)
         target->bitdepth = mmax->bitdepth;
   }

   if (VALID(target,Flags))
   {
      if (VALID(mmin,Flags))
         target->flags |= mmin->flags;

      if (VALID(mmax,Flags))
         target->flags &= mmax->flags;
   }
}


EXTERN BOOL g_no_dx6;
EXTERN BOOL g_lgd3d;
EXTERN int g_lgd3d_device_index;

void dump_screen_mode(const sScrnMode* mode);

//
// Raw screen mode setter
//
BOOL ScrnSetModeRaw(const sScrnMode* mode)
{
    // Check validity
    AssertMsg((mode->valid_fields & kScrnModeAllValid) == kScrnModeAllValid,"ScrnSetModeRaw: Not all fields are valid");

    g_lgd3d = FALSE;

#ifdef PLAYTEST
    if (config_is_defined("screen_mode_spew"))
    {
        mprintf("Target Screen Mode: ");
        dump_screen_mode(mode);
    };
#endif //PLAYTEST

    // First, figure out our display
    GUID *pDD;
    int flags = 0;
    int kind = (mode->flags & kScrnMode2dDriver) ? kDispFullScreen : kDispDebug;
    int modeflags = 0;

    if (kind == kDispFullScreen && (mode->flags & kScrnMode3dDriver))
    {

        //zb:
#ifndef SHIP
        //temp


        if( config_is_defined("local_debugging") )
        {


            int num_devices = g_no_dx6 ? 0 : lgd3d_enumerate_devices();

            if( (num_devices > 0) && (lgd3d_get_device_info(0)->flags & LGD3DF_CAN_DO_WINDOWED) )
            {
                kind                 = kDispWindowed;
                flags                = kDispAttemptFlippable | kDispAttempt3D;
                g_lgd3d_device_index = 0;
                g_lgd3d              = TRUE;
                pDD                  = NULL;
            }
            else
            {
                Warning(("Can't use hardware debugging in window, trying software.\nComment out \"local_debugging\" in your user.cfg\n"));
                kind = kDispDebug;

                //copied from below:
                // we're not doing hardware; check if we can preserve our current ddraw...
                if (ScrnFindModeFlags(mode->w,mode->h,mode->bitdepth,modeflags) != SCR_NOMODE)
                    // OK, preserve current ddraw!
                    pDD = (GUID*)&SCRNMAN_PRESERVE_DDRAW;  // Direct draw guid
                else
                    // Nope, go for primary!
                    pDD = NULL;
            }
        }
        else
        {
#endif
            // @TODO: modularize this out when it comes time for OpenGL or other 3d APIs
            int num_devices = g_no_dx6 ? 0 : lgd3d_enumerate_devices();
            if (num_devices > 0)
            {
                int idx = num_devices - 1;
                if (config_get_int("d3d_driver_index",&idx))
                {
                    if (idx < 0) idx += num_devices;
                    if (idx < 0 || idx >= num_devices)
                    {
                        idx = num_devices -1;
                        Warning(("D3D driver index %d is out of range, using %d",idx));
                    }
                }

                flags = kDispAttemptFlippable|kDispAttempt3D;
                pDD = lgd3d_get_device_info(idx)->p_ddraw_guid;

#ifndef SHIP
                if (config_is_defined("d3d_driver_index"))
                    mprintf("Using D3D device: %s\n", lgd3d_get_device_info(idx)->p_ddraw_desc);
#endif

                g_lgd3d_device_index = idx;
                g_lgd3d = TRUE;
            }
            else
            {
                Warning(("Can't use hardware; no device available.\n"));
                pDD = NULL;
            }
#ifndef SHIP
        }
#endif
    } else {
        // we're not doing hardware; check if we can preserve our current ddraw...
        if (ScrnFindModeFlags(mode->w,mode->h,mode->bitdepth,modeflags) != SCR_NOMODE)
            // OK, preserve current ddraw!
            pDD = (GUID*)&SCRNMAN_PRESERVE_DDRAW;  // Direct draw guid
        else
            // Nope, go for primary!
            pDD = NULL;
    }

#ifndef SHIP
    if ((!g_lgd3d) && (kind == kDispFullScreen))
        if (config_is_defined("fake_fullscreen")) {
            kind = kDispDebug;
            flags |= kDispStrictMonitors;
        } else if (config_is_defined("page_flip")) {
            flags |= kDispAttemptFlippable;
        }
#endif

        BOOL match = (ScrnSetDisplay(kind,flags,pDD) == 0);

        //
        // Test against current mode
        //
        sScrnMode cur;
        ScrnModeGet(&cur);

        match = match && VALID(&cur,Dims) && mode->w == cur.w && mode->h == cur.h;
        match = match && VALID(&cur,BitDepth) && mode->bitdepth == cur.bitdepth;

        //if (match)
            //return TRUE;

        ScrnClear();

        //
        // Now find and set the mode
        //

        ScrnMode modenum = ScrnFindModeFlags(mode->w,mode->h,mode->bitdepth,modeflags);

        if (modenum == SCR_NOMODE)
            return FALSE;

        if (ScrnSetRes(modenum,kScrnPreservePal|MODE_CLEAR_BIT) == FALSE)
            return FALSE;

        return TRUE;
};


void ScrnModeCopy(sScrnMode* targ, const sScrnMode* src, int fields)
{
   sScrnMode dummy;
   dummy.valid_fields = src->valid_fields & fields;

   if (VALID(&dummy,Dims))
   {
      targ->w = src->w;
      targ->h = src->h;
      targ->valid_fields |= kScrnModeDimsValid;
   }

   if (VALID(&dummy,BitDepth))
   {
      targ->bitdepth = src->bitdepth;
      targ->valid_fields |= kScrnModeBitDepthValid;
   }

   if (VALID(&dummy,Flags))
   {
      targ->flags = src->flags;
      targ->valid_fields |= kScrnModeFlagsValid;
   }

}

////////////////////////////////////////

sScrnMode* ScrnModeGetConfig(sScrnMode* targ, const char* prefix)
{
   char var[40] = "\0";
   int coords[2];
   int cnt = 2;

   // Look for dims
   sprintf(var,"%sscreen_size",prefix);
   if (config_get_value(var,CONFIG_INT_TYPE,coords,&cnt)
       && cnt == 2)
   {
      targ->valid_fields |= kScrnModeDimsValid;
      targ->w = coords[0];
      targ->h = coords[1];
   }

   // get depth
   sprintf(var,"%sscreen_depth",prefix);
   if (config_get_int(var,&targ->bitdepth))
      targ->valid_fields |= kScrnModeBitDepthValid;

   if (!(targ->valid_fields & kScrnModeFlagsValid))
       targ->flags = 0;
   // get flags
   sprintf(var,"%sscreen_flags",prefix);
   if (config_get_int(var,&targ->flags))
      targ->valid_fields |= kScrnModeFlagsValid;

   BOOL flag = 0;

   // get separate fullscreen bit
   sprintf(var,"%sfull_screen",prefix);
   if (config_get_int(var,&flag))
   {
      targ->valid_fields |= kScrnModeFlagsValid;

      if (flag)
         targ->flags |= kScrnModeFullScreen|kScrnMode2dDriver;
      else
         targ->flags &= ~(kScrnModeFullScreen|kScrnMode2dDriver);
   }

   // get separate 3d bit
   sprintf(var,"%shardware",prefix);
   if (config_get_int(var,&flag))
   {
      targ->valid_fields |= kScrnModeFlagsValid;

      if (flag)
         targ->flags |= kScrnModeFullScreen|kScrnMode2dDriver|kScrnMode3dDriver;
      else
         targ->flags &= ~(kScrnMode3dDriver);
   }

   return targ;
}

////////////////////////////////////////

void ScrnModeSetConfig(const sScrnMode* src, const char* prefix)
{
   char var[40];
   if (VALID(src,Dims))
   {
      int coords[2] = { src->w, src->h };
      sprintf(var,"%sscreen_size",prefix);
      config_set_value(var,CONFIG_INT_TYPE,coords,2);
   }

   if (VALID(src,BitDepth))
   {
      sprintf(var,"%sscreen_depth",prefix);
      config_set_int(var,src->bitdepth);
   }

   if (VALID(src,Flags))
   {
      sprintf(var,"%sscreen_flags",prefix);
      config_set_int(var,src->flags);
      sprintf(var,"%sfull_screen",prefix);
      config_set_int(var,(src->flags & kScrnModeFullScreen)!= 0);
      sprintf(var,"%shardware",prefix);
      config_set_int(var,(src->flags & kScrnMode3dDriver)!= 0);
   }
}


void ScrnModeValidate(sScrnMode* targ)
{

   // strip 3d driver if you have none
   if (VALID(targ,Flags) && (targ->flags & kScrnMode3dDriver))
   {
      if (config_is_defined("disallow_hardware") || lgd3d_enumerate_devices() == 0)
         targ->flags &= ~kScrnMode3dDriver;
   }

   // If you want 3d, you need 16 bit and 2d and fullscreen
   if (VALID(targ,Flags) && (targ->flags & kScrnMode3dDriver))
   {
      targ->flags |= kScrnMode2dDriver|kScrnModeFullScreen;
      if (VALID(targ,BitDepth))
      {
         if (targ->bitdepth < MIN_3D_BITDEPTH)
            targ->bitdepth = MIN_3D_BITDEPTH;
      }
      else
      {
         targ->bitdepth = MIN_3D_BITDEPTH;
         targ->valid_fields |= kScrnModeBitDepthValid;
      }

   }

   // If you want fullscreen, you need 2d driver
   // In this case, we unset fullscreen if you don't have it
   if (VALID(targ,Flags)
       && (targ->flags & (kScrnModeFullScreen|kScrnMode2dDriver)) == kScrnModeFullScreen)
   {
      targ->flags &= ~kScrnModeFullScreen;
   }
}

#ifdef PLAYTEST

static const char* flag_strings[] =
{
   "FullScreen",
   "Windowed",
   "3d",
   "DirectX",
};

#define NUM_FLAGS (sizeof(flag_strings)/sizeof(flag_strings[0]))

static void dump_screen_mode(const sScrnMode* mode)
{
   if (VALID(mode,Dims))
      mprintf("%dx%d ",mode->w,mode->h);
   if (VALID(mode,BitDepth))
      mprintf("(%d bit color) ",mode->bitdepth);
   if (VALID(mode,Flags))
   {
      mprintf("Flags: ");
      BOOL first = TRUE;
      for (int i = 0; i < NUM_FLAGS; i++)
      {
         if (mode->flags & (1 << i))
         {
            if (!first)
               mprintf(", ");
            mprintf(flag_strings[i]);
            first = FALSE;
         }
      }
   }
   mprintf("\n");
}
#endif  // PLAYTEST


