/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psndapi.h,v 1.14 2000/01/29 13:41:44 adurant Exp $
//
// Sound propagation api header
//
#pragma once

#ifndef __PSNDAPI_H
#define __PSNDAPI_H

#include <objtype.h>
#include <appsfx.h>

////////////////////////////////////////////////////////////////////////////////

EXTERN void SoundPropagationInit();
EXTERN void SoundPropagationTerm();
EXTERN void SoundPropagationReset();


enum eGenerateSoundFlags
{
   kGSF_None       = 0x0000,
   kGSF_WantHandle = 0x0001,
   kGSF_Networked  = 0x0002,
   kGSF_NetSent    = 0x0004,
   kGSF_ForcePlay  = 0x0008,
};

// Returned from GenerateSound functions if we don't actually make a sound:
#define kPSndFail  (-1)

EXTERN int  GenerateSoundObj(ObjID objID, ObjID schemaID, const char *sampleName, 
                             float atten_factor, sfx_parm *parms, int flags, void *data);
EXTERN int  GenerateSoundVec(mxs_vector *vec, ObjID src_obj, ObjID schemaID, const char *sampleName, 
                             float atten_factor, sfx_parm *parms, int flags, void *data);
EXTERN int  GenerateSound(const char *sampleName, sfx_parm *parms);

EXTERN void SoundHalt(int handle);
EXTERN void SoundHaltObj(ObjID objID);

EXTERN void ApplyPropSounds();
EXTERN void ClearPropSounds();
EXTERN void CleanupSoundObjects();

EXTERN BOOL IsSoundListener(ObjID objID);

#ifndef SHIP
EXTERN void DrawSounds();
EXTERN void SpewSounds();

EXTERN BOOL g_show_sounds;
#endif

////////////////////////////////////////////////////////////////////////////////

#endif
