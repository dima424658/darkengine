/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psndinfo.h,v 1.18 2000/01/29 13:41:45 adurant Exp $
//
// Sound information class header
//
#pragma once

#ifndef __SNDINFO_H
#define __SNDINFO_H

#include <objtype.h>
#include <matrix.h>
#include <appsfx.h>

#include <dlist.h>

////////////////////////////////////////////////////////////////////////////////

typedef int ObjID;

typedef class cSoundInfo cSoundInfo;

typedef cDList<cSoundInfo, 0>     cSoundInfoList;
typedef cDListNode<cSoundInfo, 0> cSoundInfoListNode;

////////////////////////////////////////////////////////////////////////////////

enum eSoundInfoFlags
{
   kSIF_None           = 0x0000,
   kSIF_HasBeenUpdated = 0x0001,   // Has been repropagated and updated
   kSIF_OnObject       = 0x0002,   // Attached to an object
   kSIF_Deferred       = 0x0004,   // A deferred sound
   kSIF_Intercept      = 0x0008,   // Being bumped, don't tell schema
   kSIF_Static         = 0x0010,   // Non-directional sound
   kSIF_NetSent        = 0x0020,   // Has this sound been sent over the network?
   kSIF_Networked      = 0x0040,   // This is a sound that we've been told to play
   kSIF_ForcePlay      = 0x0080,   // This sound was forced to play
};

struct sSoundInfo
{
   ulong      flags;
   int        Volume;         // Volume of virtual sound 
   mxs_vector Position;       // Position of virtual sound
   mxs_real   Distance;       // munged distance to actual sound (includes blocking factor)

   mxs_vector SrcPoint;       // The location of sound source

   ObjID      SrcObject;      // Object causing the sound (-1 if not applicable)
   ObjID      Object;         // Object hearing the sound
   ObjID      ActiveObject;   // Object placed at the virtual sound

   void      *Data;           // Data propagated with the sound

   float      AttenFactor;    // Attenuation factor

   int        FramesUntilUpdate; // # of frames until sound is re-propagated (only if it's active)

   char       SampleName[16]; // Name of playing sample

   sfx_parm   AppsfxParms;    // Parameters to appsfx for this sound

   int        Handle;         // Sound propagation handle
   int        SFXHandle;      // Handle of playing sample

   ObjID      SchemaID;       // ObjID of schema

   mxs_real   BlockingFactor; // amount of obstruction along sound path
   mxs_real   RealDistance;   // real distance to actual sound (not including blocking factor)
};

class cSoundInfo : public cSoundInfoListNode, public sSoundInfo
{
public:
   cSoundInfo(const cSoundInfo& info)
      : sSoundInfo(info)
   {
   }

   cSoundInfo()
   {
      flags = 0;

      Volume = -1;

      mx_zero_vec(&Position);
      mx_zero_vec(&SrcPoint);

      Distance = 0.0;

      SrcObject = OBJ_NULL;
      Object = OBJ_NULL;
      ActiveObject = OBJ_NULL;

      Data = NULL;

      AttenFactor = 1.0;
      FramesUntilUpdate = 0;
      SampleName[0] = '\0';

      Handle = -1;
      SFXHandle = SFX_NO_HND;

      SchemaID = OBJ_NULL;
   } 

};

////////////////////////////////////////////////////////////////////////////////

class cPlayerSoundList : public cSoundInfoList
{
public:

   // Callback and info for schema system
   void (*SchemaCallback)(int hSound, void *user_data);
   void (*SchemaLoopCallback)(int hSound, void *user_data);
   void *SchemaUserData;
};

////////////////////////////////////////////////////////////////////////////////

#endif




