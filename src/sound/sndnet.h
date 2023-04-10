/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndnet.h,v 1.5 2000/01/31 10:02:36 adurant Exp $
#pragma once

#ifndef _SNDNET_H
#define _SNDNET_H

#include <lg.h>
#include <matrix.h>
#include <objtype.h>
#include <appsfx.h>

EXTERN void SoundNetInit();
EXTERN void SoundNetTerm();

// Routines for broadcasting various kinds of sounds.
// Sound attached to an object:
EXTERN void SoundNetGenerateSoundObj(int handle,
                                     ObjID objID, 
                                     ObjID schemaID, 
                                     const char *sampleName, 
                                     float atten_factor, 
                                     sfx_parm *parms);

// Sound at a specified location:
EXTERN void SoundNetGenerateSoundVec(int handle, 
                                     mxs_vector *vec,
                                     ObjID schemaID, 
                                     const char *sampleName, 
                                     float atten_factor, 
                                     sfx_parm *parms);

// Ambient sounds:
EXTERN void SoundNetGenerateSound(int handle, 
                                  const char *sampleName,
                                  sfx_parm *parms);

// Stop a sound:
EXTERN void SoundNetHalt(int handle);

#endif // !_SNDNET_H
