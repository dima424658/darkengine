/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmrk.h,v 1.4 1999/03/02 17:42:01 TOML Exp $
//
// AI Action - marker movement
//

#ifndef __AIACTMRK_H
#define __AIACTMRK_H

#include <property.h>
#include <relation.h>

#include <aiapiiai.h>
#include <aibasact.h>
#include <aiactloc.h>

F_DECLARE_INTERFACE(IIntProperty);

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitMarkerAction();
BOOL AITermMarkerAction();

///////////////////////////////////////////////////////////////////////////////

enum eAIMarkerType_
{
   eRetreatPoint,
   eVantagePoint,
   eMeleePoint,
};

typedef int eAIMarkerType;

// class me  
struct sMarkerParams
{
   float toAIBias;
   float toTargetBias;
   float LOSBias;
   float markerValueBias;

   float idealDistance;
   float maxDistance;
   float minDistance;

   ObjID target;

   eAISpeed speed;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveToMarkerAction
//
// Find and move to a marker of the specified type
//

class cAIMoveToMarkerAction : public cAIAction
{
public:
   cAIMoveToMarkerAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIMoveToMarkerAction();

   // Set up action parameters
   BOOL Set(eAIMarkerType markerType, sMarkerParams &markerParams);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   IIntProperty *m_pCurProperty;
   IRelation    *m_pCurRelation;

   cAILocoAction *m_pLocoAction;

   eAISpeed m_speed;
};

inline cAIMoveToMarkerAction::cAIMoveToMarkerAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_MoveToMarker, pOwner, data),
   m_pCurProperty(NULL),
   m_pCurRelation(NULL),
   m_pLocoAction(NULL),
   m_speed(kAIS_Stopped)
{
}

#pragma pack()

#endif /* !__AIACTMRK */

