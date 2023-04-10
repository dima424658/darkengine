/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psnd.h,v 1.22 1999/08/05 18:20:32 Justin Exp $
//
// Sound propagation header
//

#pragma once

#ifndef __PSND_H
#define __PSND_H

#include <matrixs.h>
#include <hashpp.h>

#include <objtype.h>

#include <roomsys.h>
#include <roompa.h>

#include <psndapi.h>
#include <psndinst.h>
#include <psndinfo.h>

#include <mprintf.h>

///////////////////////////////////////////////////////////////////////////////

#define EFF_PRI_MIN 0
#define EFF_PRI_MAX 65535

const int kMinUpdateFrequency = 16;
const int kMaxUpdateFrequency = 1;
const int kDeferredUpdateFrequency = 16;

////////////////////////////////////////////////////////////////////////////////

typedef class cPropSndInst     cPropSndInst;
typedef class cPropSndInstHigh cPropSndInstHigh;
typedef class cPropSndInstLow  cPropSndInstLow;

typedef cHashTableFunctions<long> LongHashFunctions;
typedef cHashTable<long, mxs_real, LongHashFunctions> cSoundBlockingTable;

////////////////////////////////////////////////////////////////////////////////

class cPropSnd 
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cPropSnd();
   ~cPropSnd();

   /////////////////
   //
   // Functions
   //
   int  GenerateSound(ObjID objID, ObjID schemaID, const char *sampleName, float atten_factor, 
                      sfx_parm *parms, int flags, void *data = NULL);
   int  GenerateSound(mxs_vector &vec, ObjID src_obj, ObjID schemaID, const char *sampleName, float atten_factor,
                      sfx_parm *parms, int flags, void *data = NULL);
   int  GenerateSound(const char *sampleName, sfx_parm *parms);


   int  GenerateSoundHandle(int handle, ObjID objID, ObjID schemaID, const char *sampleName, float atten_factor, 
                            sfx_parm *parms, int flags, void *data = NULL);
   int  GenerateSoundHandle(int handle, mxs_vector &vec, ObjID src_obj, ObjID schemaID, const char *sampleName, 
                            float atten_factor, sfx_parm *parms, int flags, void *data = NULL);

   // Stops the sound, all sounds, and checks for any playing sounds
   void SoundHalt(int handle);
   void SoundHaltObj(ObjID objID);
   void SoundHaltAll();
   BOOL AreSounds();

   // Informs each non-player object about sounds this frame
   void ApplySoundInfo();
   void ClearSoundInfo();

   // Cleans up game-mode created objects
   void Cleanup();

   // Informs the active sound system that the sound has ended
   void EndActiveSound(cSoundInfo *pSoundInfo);

   // Valid handle checking
   int               GetSoundHandle();
   void              RegisterSound(int handle, cPlayerSoundList *pSoundList);
   int               FindSound(cPlayerSoundList *pSoundList);
   cPlayerSoundList *GetSound(int handle);
   void              RemoveSound(int handle);

   // Sound blocking functions
   void     ClearBlockingFactors();
   void     SetBlockingFactor(short roomID1, short roomID2, mxs_real factor, BOOL blockAdjacent = TRUE);
   mxs_real GetBlockingFactor(short roomID1, short roomID2);
   void     RemoveBlockingFactor(short roomID1, short roomID2, BOOL blockAdjacent = TRUE);

   PrioritizationResult Prioritize(const cPlayerSoundList* pPlayerList);

   // Networking functions
   BOOL ShouldPlaySound(const sfx_parm *pParms, int flags, BOOL onObject, ObjID srcObject) const;
   BOOL ShouldNetworkSound(const sfx_parm *pParms, int flags, BOOL onObject, ObjID srcObject) const;

#ifndef SHIP
   void SetDrawSoundPath(BOOL state);

   void DrawActiveSounds();
   void SpewActiveSounds();
#endif

private:
   // Prioritization functions.
   BOOL TypeCapReached(uchar type);
   int  EffectivePriority(const cPlayerSoundList *pPlayerList);
   cPlayerSoundList* LowestPrioritySoundOfType(uchar type, int *itsEffectivePriority);
   cPlayerSoundList* LowestPrioritySound(int *itsEffectivePriority);

   void BumpActiveSound(cPlayerSoundList *pPlayerList);

   // Sound blocking
   void BlockAdjacentRooms(short roomID1, short roomID2, mxs_real factor);

   cPlayerSoundList *GeneratePlayerSoundList(int handle);

   cPlayerSoundList *m_SoundList[kMaxActiveSounds];

   cSoundBlockingTable m_SoundBlockingTable;

   cPropSndInstHigh  m_SoundInstHigh;
 
   cPropSndInst   *m_SoundInstance;
   cRoomPropAgent  m_PropAgent;

   BOOL  m_NoSound;

};

////////////////////////////////////////

inline void cPropSnd::ApplySoundInfo()
{
   Assert_(m_SoundInstance);
   m_SoundInstance->ApplySoundInfo();
}

////////////////////////////////////////

inline void cPropSnd::ClearSoundInfo()
{
   Assert_(m_SoundInstance);
   m_SoundInstance->ClearSoundInfo();
}

////////////////////////////////////////

inline void cPropSnd::Cleanup()
{
   Assert_(m_SoundInstance);
   m_SoundInstance->Cleanup();
}

////////////////////////////////////////////////////////////////////////////////

inline void cPropSnd::RegisterSound(int handle, cPlayerSoundList *pSoundList)
{
   Assert_((handle >= 0) && (handle < kMaxActiveSounds));
   Assert_(m_SoundList[handle] == NULL);

   m_SoundList[handle] = pSoundList;
}

////////////////////////////////////////

inline cPlayerSoundList *cPropSnd::GetSound(int handle)
{
   Assert_((handle >= 0) && (handle < kMaxActiveSounds));

   return m_SoundList[handle];
}

////////////////////////////////////////

inline void cPropSnd::RemoveSound(int handle)
{
   Assert_((handle >= 0) && (handle < kMaxActiveSounds));

   m_SoundList[handle] = NULL;
}   

////////////////////////////////////////////////////////////////////////////////

inline void cPropSnd::EndActiveSound(cSoundInfo *pSoundInfo)
{
   m_SoundInstance->EndActiveSound(pSoundInfo);
}

////////////////////////////////////////////////////////////////////////////////

inline cPlayerSoundList *cPropSnd::GeneratePlayerSoundList(int handle)
{
   return m_SoundInstance->GeneratePlayerSoundList(handle);
}
////////////////////////////////////////////////////////////////////////////////

inline void cPropSnd::ClearBlockingFactors()
{
   m_SoundBlockingTable.Clear();
}

////////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
inline void cPropSnd::SetDrawSoundPath(BOOL state)
{
   Assert_(m_SoundInstance);
   m_SoundInstance->SetDrawSoundPath(state);
}

inline void cPropSnd::DrawActiveSounds()
{
   Assert_(m_SoundInstance);
   m_SoundInstance->DrawActiveSounds();
}

#endif

////////////////////////////////////////////////////////////////////////////////

EXTERN cPropSnd *g_pPropSnd;
EXTERN int       g_SndWatchHandle;


////////////////////////////////////////////////////////////////////////////////

#endif


