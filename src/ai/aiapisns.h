/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapisns.h,v 1.13 1998/11/03 16:24:27 TOML Exp $
//
// Specification of the AI sense interface
//

#ifndef __AIAPISNS_H
#define __AIAPISNS_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <dynarray.h>
#include <aiapicmp.h>

#pragma once
#pragma pack(4)

struct sAIAwareness;
struct sSoundInfo;

typedef cDynArray<const sAIAwareness *> cAIAwareArray;

///////////////////////////////////////////////////////////////////////////////
//
// Enums and constants
//

///////////////////////////////////////
//
// Flags for IAISenses::Update()
//

enum eAISensesUpdateFlags
{
   // Force the sensory data to correspond 100% with current reality, otherwise
   // cached data may cause a perceptual lag
   kAISU_Force = 0x01
};


///////////////////////////////////////
//
// Flags for IAISenses::GetHighAwareness()
//

enum eAISensesGetHighFlags
{
   kAISGH_OpponentsOnly = 0x01,
   kAISGH_FirstHand     = 0x02,
   kAISGH_Alerting      = 0x04,
};

///////////////////////////////////////////////////////////////////////////////
//
// Update the visibility propery for an object
//

void AIUpdateVisibility(ObjID objId);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAISenses
//

EXTERN BOOL g_fAIAwareOfPlayer;

///////////////////////////////////////

DECLARE_INTERFACE_(IAISenses, IAIComponent)
{
   //
   // Perform sensing
   //
   STDMETHOD (Update)() PURE;

   //
   // Perform sensing of one
   //
   STDMETHOD (Update)(ObjID objId, unsigned flags = 0) PURE;

   //
   //
   //
   STDMETHOD (ClearSense)(ObjID objId) PURE;

   //
   // Get the object the sensory system is most aware of. 
   // Do not store returned pointer.
   //
   STDMETHOD_(const sAIAwareness *, GetHighAwareness)(unsigned flags = kAISGH_OpponentsOnly, unsigned maxLastContact = INT_MAX) PURE;

   //
   // Get sense level of an object.
   //
   // Never returns NULL. Returned structure should never be considered
   // valid across calls to this function or across frames.
   //
   STDMETHOD_(const sAIAwareness *, GetAwareness)(ObjID objId) PURE;

   //
   // Get all awareness. Appropriate for iteration.
   //
   STDMETHOD_(int, GetAllAwareness)(cAIAwareArray * pResult) PURE;
   
   //
   // Can the AI hear a given sound. Returns the "percent heard"
   //
   STDMETHOD_(int, Hears)(const sSoundInfo * pSoundInfo, const sAISoundType * pType) PURE;

   //
   // Keep an awareness link fresh longer than normal
   //
   // When executing long plans, call this to avoid forgetting 
   // what you were doing
   //
   STDMETHOD_(void, KeepFresh)(ObjID obj, unsigned duration) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPISNS_H */
