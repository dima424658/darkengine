/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
/*
 * $Source: r:/t2repos/thief2/src/render/scrnman.h,v $
 * $Revision: 1.16 $
 * $Author: adurant $
 * $Date: 2000/01/31 09:53:18 $
 *
 * Screen Manager
 *
*/

#ifndef __SCRNMAN_H
#define __SCRNMAN_H

#include <dev2d.h>
#include <rect.h>

// The big purpose of this module is to hide whether you
// are page flipping or offscreen buffered, and to hide
// much 8 vs 16 bit stuff like blackening screens


typedef enum {
   SCR_320x200x8        = GRM_320x200x8,
   SCR_320x400x8        = GRM_320x400x8,
   SCR_512x384x8        = GRM_512x384x8,
   SCR_640x400x8        = GRM_640x400x8, 
   SCR_640x400x16       = GRM_640x400x16,
   SCR_640x480x8        = GRM_640x480x8,
   SCR_800x600x8        = GRM_800x600x8,
   SCR_1024x768x8       = GRM_1024x768x8,
   SCR_1280x1024x8      = GRM_1280x1024x8, 


   SCR_320x200          = SCR_320x200x8,
   SCR_320x400          = SCR_320x400x8,
   SCR_512x384          = SCR_512x384x8,
   SCR_640x400          = SCR_640x400x8, 
   SCR_640x480          = SCR_640x480x8,
   SCR_800x600          = SCR_800x600x8,
   SCR_1024x768         = SCR_1024x768x8,
   SCR_1280x1024        = SCR_1280x1024x8,

   SCR_NOMODE           = -1,
} ScrnMode;

#define ScrnModeToGrMode(x) (x)

// find a screen mode given parameters, returns SCR_NOMODE if none found
EXTERN ScrnMode ScrnFindModeFlags(short w, short h, ubyte bitdepth, ulong flags);
#define ScrnFindMode(w,h,d) ScrnFindModeFlags(w,h,d,0)

enum eScrnFlags
{
   kScrnPreservePal = (1 << 9), // preserve the palette 
};

// set a new display mode.
EXTERN int ScrnSetDisplay(int kind, int flags, GUID *pDDrawGuid);

// set a new screen mode.  Flags are gr_set_mode flags plus eScrnFlags
// palette or not
// returns TRUE for success
EXTERN BOOL ScrnSetRes(ScrnMode mode,ulong flags);

EXTERN void ScrnUnsetRes(void); 

// returns SCR_NOMODE if not yet set
EXTERN ScrnMode ScrnGetRes(void);

// Startup/Term 3d accel, happens within a mode change 
EXTERN BOOL ScrnInit3d(void); // return TRUE for success.
EXTERN void ScrnTerm3d(void);



// background drawer callback function, takes data,
// takes effect next start frame.  Should call it outside
// of a frame.  Only one can be installed at a time
// no chaining.  Maybe later if you ever need it
//
typedef void (*ScrnCallBack)(int data);

EXTERN void ScrnInstallBackground(ScrnCallBack cb,int data);

// clear the screen to black and blacken the palette
// note that this works great, 8 or 16 bit as a prelude
// to loading a new screen
EXTERN void ScrnBlacken(void);

// don't change the palette, just clear
EXTERN void ScrnClear(void); 

// call the background callback if present
EXTERN void ScrnStartFrame(void);

// blit or page flip, transparently, woo woo.
EXTERN void ScrnEndFrame(void);

// force a blit or page flip right now dammit
EXTERN void ScrnForceUpdate(void);
EXTERN void ScrnForceUpdateRect(Rect *r);

// set the current canvas to be the visible screen
// the idea is you're not page flipping or blitting
// in fact, if you do an end_frame in this mode, you get
// an error
EXTERN void ScrnSetVisible(void);

// set the current canvas to be the offscreen canvas(default)
EXTERN void ScrnSetOffscreen(void);

// get a pointer to the draw (i.e., "offscreen") canvas
EXTERN grs_canvas *ScrnGetDrawCanvas(void);

// load and use this palette
EXTERN void ScrnNewPalette(uchar *,int start,int size);

// make the current palette black and use it
EXTERN void ScrnNewBlackPalette(void);

// load but do not set this palette
EXTERN void ScrnLoadPalette(uchar *pal,int start,int size);

// set the currently loaded palette
EXTERN void ScrnUsePalette(void);

// return pointer to loaded palette.  Beware, don't mess with it.
EXTERN const uchar *ScrnGetPalette(void);

// return a pointer to the loaded inverse palette.  
EXTERN const uchar *ScrnGetInvPalette(void); 

// return its size
EXTERN int ScrnInvPaletteSize(void);

// Lock the draw canvas
EXTERN void ScrnLockDrawCanvas(void);

// Unlock the draw canvas
EXTERN void ScrnUnlockDrawCanvas(void);

#endif // __SCRNMAN_H

