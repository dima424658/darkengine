/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamescrn.cpp,v 1.5 2000/02/19 13:16:17 toml Exp $
#include <gamescrn.h>
#include <scrnmode.h>
#include <family.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static sScrnMode gGameScrnMode = 
{
   kScrnModeAllValid,
   640, 480, 
   8, // will get auto-switched to 16 in hardware 
   kScrnModeFullScreen|kScrnMode2dDriver|kScrnMode3dDriver
};

//
// Set/Get screen mode 
//

const sScrnMode* GetGameScreenMode(void)
{
   return &gGameScrnMode; 
}

static tScrnConstraintFunc constrain = NULL; 

const sScrnMode* SetGameScreenMode(const sScrnMode* mode)
{
   ScrnModeCopy(&gGameScrnMode,mode,mode->valid_fields); 
   ScrnModeValidate(&gGameScrnMode); 
   if (constrain)
      constrain(&gGameScrnMode); 

   family_reload_water(); 
   return &gGameScrnMode; 
}


void ConstrainGameScreenMode(tScrnConstraintFunc func)
{
   constrain = func; 
}

