/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psndinst.h,v 1.22 2000/01/29 13:41:46 adurant Exp $
//
// Sound propagation instance header
//
#pragma once

#ifndef __PSNDINST_H
#define __PSNDINST_H

#include <matrixs.h>

#include <dynarray.h>
#include <hashpp.h>

#include <appsfx.h>
#include <roomsys.h>
#include <roompa.h>

#include <psndinfo.h>

F_DECLARE_INTERFACE(IAIManager);

////////////////////////////////////////

// Max active sounds.
const int kMaxActiveSounds = 32;

////////////////////////////////////////////////////////////////////////////////

EXTERN void EndSampleCallback(int hSound, void *ptr);
EXTERN void LoopSampleCallback(int hSound, void *ptr);

////////////////////////////////////////////////////////////////////////////////

typedef cHashTableFunctions<ObjID> ObjIDHashFunctions;

typedef cHashTable<ObjID, cSoundInfoList *, ObjIDHashFunctions> cSoundInfoTable;
typedef cHashTable<ObjID, mxs_real, ObjIDHashFunctions>         cSoundPropTable;

typedef cDynArray<ObjID> cSoundObjectList;

typedef cDynArray<const cRoomPortal *> cPortalStack;

typedef struct 
{
   mxs_vector pos;
   BOOL valid;
} sAnchorPt;

////////////////////
enum PrioritizationResult
{
   PRIORITY_RES_OK,
   PRIORITY_RES_FAIL,
   PRIORITY_RES_DEFERRED
};

enum eDeferredHaltType
{
   kDH_Kill,
   kDH_Silence,
};

typedef struct
{
   int sfx_handle;
   int type;
} sDeferredHalt;


////////////////////////////////////////////////////////////////////////////////

class cPropSndInst : public cRoomPAFuncs
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cPropSndInst();
   ~cPropSndInst();

   void Init(int handle, ObjID objID, const mxs_vector &loc, ObjID schemaID, const char *sampleName, 
             int flags, float atten_factor, sfx_parm *parms, void *data);
   void Cleanup();

   void ApplySoundInfo();
   void ClearSoundInfo();

   cPlayerSoundList *GeneratePlayerSoundList(int handle);

   void AddDeferredHalt(int sfx_handle, int type);
   void ApplyDeferredHaltList();

   void AddActiveSound(cSoundInfo *pSoundInfo);
   void EndActiveSound(cSoundInfo *pSoundInfo);

   void BumpActiveSound(cPlayerSoundList *pPlayerList);

   void SetupPlayerSoundListCallbacks (cPlayerSoundList* pCurPlayerList);
   BOOL PlaySoundList (cPlayerSoundList* pCurPlayerList);

   #ifndef SHIP
   void SetDrawSoundPath(BOOL state) { m_DrawSoundPath = state; };

   void DrawActiveSounds();
   void SpewActiveSounds();
   #endif

protected:

   ////////////////////////////

   mxs_real    m_MaxDistance;    // Maximum distance the sound can be heard
   mxs_real    m_ScaleDistance;  // Forces max distance for sound

   ObjID       m_SrcObject;      // ID of object making sound (-1 if not applicable)
   mxs_vector  m_SrcPoint;       // Coordinates of sound source 

   const char *m_SampleName;     // Name of sample being propagated
   sfx_parm   *m_pParms;         // Parameters from schema system

   ObjID       m_SchemaID;       // ObjID of the schema

   void       *m_Data;           // Any data that wants to be propagated too

   int         m_Handle;         // Sound propagation handle

   int         m_Flags;          // Sound info flags 

   ////////////////////////////

   cSoundInfoTable  m_InfoTable;      // Hash by objid to list sounds heard this frame
   cSoundObjectList m_InfoList;       // List of objects that have heard a sound this frame

   cSoundInfo      *m_ActiveSound[kMaxActiveSounds];
   ObjID            m_ActiveObject[kMaxActiveSounds];
   int              m_nActiveSoundObjects;

   cDynArray<sDeferredHalt> m_deferredHaltList;

   short m_roomNoBlock1;
   short m_roomNoBlock2;

   float m_attenFactor;

#ifndef SHIP
   BOOL m_DrawSoundPath;

   BOOL m_SoundSpew;
   BOOL m_SoundFullSpew;
   BOOL m_SoundGhostSpew;
   BOOL m_SoundFullAISpew;
   BOOL m_SoundPathSpew;
   BOOL m_SoundHearSpew;
#endif

   void FindSoundPath(const mxs_vector &src_pt, const mxs_vector &hear_pt, const cBFRoomInfo *room_table, int end_room, 
                      mxs_real *dist, mxs_vector *position, mxs_real *realDist, mxs_real *blockingFactor);
   void BackCastPath(sAnchorPt *anchor_list, cRoomPortal **portal_list, int i, const mxs_vector &L, const mxs_vector &src_pt); 

private:

   BOOL MergeSounds(cSoundInfoList *pSoundInfoList);

   BOOL ActiveSound(cSoundInfo *pSoundInfo); 

   void ResolveActiveSounds(ObjID hearingObj);
   void UpdateActiveSounds();

   void ResolveDeferredSounds(ObjID hearingObj);
   void UpdateDeferredSounds();

   IAIManager * m_pAIManager;
};

////////////////////////////////////////////////////////////////////////////////

class cPropSndInstHigh : public cPropSndInst
{
public:

   /////////////////
   //
   // Callbacks
   //
   BOOL     EnterCallback(const cRoom *room, const cRoomPortal *enterPortal, const mxs_vector &enterPt, mxs_real dist);
   void     ExitCallback(const cRoom *room);
   mxs_real PortalsCallback(const cRoomPortal *enterPortal, const cRoomPortal *exitPortal, mxs_real dist);
};

////////////////////////////////////////////////////////////////////////////////

#endif








