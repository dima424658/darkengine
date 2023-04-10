/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aifollow.h,v 1.2 1998/08/09 10:55:33 TOML Exp $
//
//
//

#ifndef __AIFOLLOW_H
#define __AIFOLLOW_H

#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitFollowAbility(IAIManager *);
BOOL AITermFollowAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFollow
//

struct sAIFollowVector
{
   float angle;
   int   distance;
};

///////////////////

#define kAIF_MaxVectors 4

struct sAIFollowLink
{
   // Refreshed fields
   ObjID           target;
   
   // Designer fields
   eAIPriority     priority;
   sAIFollowVector vectors[kAIF_MaxVectors];
   
};

///////////////////////////////////////

class cAIFollow : public cAIAbility
{
public:
   cAIFollow();
   ~cAIFollow();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   // Link check frequency
   cAITimer      m_Timer;
   
   sAIFollowLink m_Current;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIFOLLOW_H */
