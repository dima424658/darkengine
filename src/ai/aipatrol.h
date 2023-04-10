/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipatrol.h,v 1.9 1998/10/14 22:53:40 TOML Exp $
//
//
//

#ifndef __AIPATROL_H
#define __AIPATROL_H

#include <proptype.h>
#include <ainoncbt.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitPatrolAbility(IAIManager *);
BOOL AITermPatrolAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPatrol
//

class cAIPatrol : public cAINonCombatAbility
{
public:

   cAIPatrol();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnSimStart)();
   STDMETHOD_(void, OnUpdateProperties)(eAIPropUpdateKind kind);
   STDMETHOD_(void, OnGoalProgress)(const cAIGoal *);
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnDefend)(const sAIDefendPoint *);

   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);


protected:
   void SetShouldPatrolFromProperty();

   void Start();
   void Stop();
   BOOL ShouldPatrol();
   void SetShouldPatrol(BOOL patrolling);
   
   ObjID GetCurrentPatrolObj();
   ObjID TargetNextPatrolObj();
   
   enum
   {
      kShouldPatrol = kBaseFirstAvailFlag,

      kPatrolFirstAvailFlag =  (kShouldPatrol << 1)
   };
   
   cAITimer m_CheckPropTimer; // A hack to handle AIs editing each others properties
   
};

///////////////////////////////////////

inline BOOL cAIPatrol::ShouldPatrol()
{
   return (m_flags & kShouldPatrol);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPATROL_H */

