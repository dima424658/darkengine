/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnmode.h,v 1.5 1998/11/05 17:09:46 mahk Exp $
#pragma once  
#ifndef __SCRNMODE_H
#define __SCRNMODE_H

struct sScrnMode 
{
   int valid_fields;  // which fields are valid
   int w; // width
   int h; // height
   int bitdepth;  
   ulong flags; 
}; 

enum eScrnModeValidFields
{
   kScrnModeDimsValid         = 1 << 0,
   kScrnModeBitDepthValid     = 1 << 1,
   kScrnModeFlagsValid        = 1 << 2,

   kScrnModeAllValid = (kScrnModeDimsValid|kScrnModeBitDepthValid|kScrnModeFlagsValid)
}; 

enum eScrnModeFlags
{
   kScrnModeFullScreen        = 1 << 0,   // fullscreen mode
   kScrnModeWindowed          = 1 << 1,   // windowed mode
   kScrnMode3dDriver          = 1 << 2,   // Use 3d driver
   kScrnMode2dDriver          = 1 << 3,   // Use 2d driver (DDraw)
}; 


typedef struct sScrnMode sScrnMode; 

//
// Set the screen mode
//
// Specify minimum parameters, preferred (but optional) parameters, and  
// maximum parameters.  (min <= x <= max) 
//
// In the case of flags, if the flags of "min" are valid, then the mode WILL use them.
// if the flags of "max" are valid, then zero max in max will be prohibited.
//

EXTERN BOOL ScrnModeSet(const sScrnMode* min, const sScrnMode* prefer, const sScrnMode* max);

//
// Just set the screen mode, don't be clever.
// All fields must be valid, returns success
//
EXTERN BOOL ScrnSetModeRaw(const sScrnMode* mode) ;

//
// Describe the current screen mode
//
EXTERN void ScrnModeGet(sScrnMode* current); 

//
// Copy the specified fields from src to targ, leave others unchanged 
//
EXTERN void ScrnModeCopy(sScrnMode* targ, const sScrnMode* src, int fields); 

//
// Set the default screen mode, whose parameters are used to fill 
// incomplete mode suggestions. 
//
EXTERN void ScrnModeSetDefault(const sScrnMode* def); 
EXTERN const sScrnMode* ScrnModeGetDefault(void); 

//
// Get a scrnmode from the config system
//
EXTERN sScrnMode* ScrnModeGetConfig(struct sScrnMode* targ,const char* prefix); 

//
// Write it back
//
EXTERN void ScrnModeSetConfig(const struct sScrnMode* targ, const char* prefix); 

//
// Mutate a screen mode so that it's not absurd.  
// Includes a check for hardware  
//
EXTERN void ScrnModeValidate(struct sScrnMode* targ); 


#endif // __SCRNMODE_H






