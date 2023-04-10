/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapiinf.h,v 1.3 1999/11/29 12:27:44 BFarquha Exp $
//
//
//

#ifndef __AIAPIINF_H
#define __AIAPIINF_H

#include <comtools.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IInternalAI);
F_DECLARE_INTERFACE(IAIInform);
typedef unsigned eAISoundConcept;

///////////////////////////////////////////////////////////////////////////////

struct sAIInform
{
   sAIInform() : concept(0), bUseLocOnly(FALSE) {}
   sAIInform(IInternalAI * pAI, eAISoundConcept concept, ObjID source);

   eAISoundConcept concept;
   ObjID           ofWhom;
   eAIAlertLevel   alertValue;
   mxs_vector      loc;
   unsigned        locTime;
   unsigned        maxPass;
   unsigned        transferDelay;
   unsigned        expiration;
   BOOL            bUseLocOnly;
   mxs_vector OriginLoc;
};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIInform
//

IAIInform * AIGetInform(IInternalAI * pAI);

///////////////////////////////////////

DECLARE_INTERFACE_(IAIInform, IUnknown)
{
   //
   //
   //
   STDMETHOD_(BOOL, WantsInforms)() PURE;

   //
   //
   //
   STDMETHOD (Post)(const sAIInform *) PURE;

   //
   //
   //
   STDMETHOD (Receive)(const sAIInform *) PURE;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIINF_H */
