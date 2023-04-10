/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aigoal.h,v 1.13 1999/11/29 12:28:25 BFarquha Exp $
//
// Definitions of AI Goals. Some goals are represented entirely using the
// generic goal structure. Others, like "goto", use a derived specialization.
//
// Goals are the highest level of AI processing. They generally change
// "rarely":
//    Goal --> Actions --> Move Suggestions --> Locomotions
//                     \-> Raw Motions
//                     \-> Speech
//                     \-> Custom
//

#ifndef __AIGOAL_H
#define __AIGOAL_H

#include <comtools.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(ITagFile);
F_DECLARE_INTERFACE(IAIAbility);
class cAnsiStr;
typedef cAnsiStr cStr;
class cAIGoal;

///////////////////////////////////////////////////////////////////////////////
//
// Mode suggestions
//
// These "uber goals" are used to manage very high level modes.
//

///////////////////////////////////////
//
// STRUCT: sAIModeSuggestion
//

struct sAIModeSuggestion
{
   eAIMode      mode;
   eAIPriority  priority;
};

///////////////////////////////////////////////////////////////////////////////
//
// Constants and enums
//

///////////////////////////////////////
//
// Goal flags
//

enum eAIGoalFlags
{
   // Active: Set for the goal that is the currently active one for the associated AI
   kAIGF_Active = 0x01,

   // NoRetain: Set to indicate that the goal should not be assumed as the
   //           ability goal after the current decision process
   kAIGF_NoRetain = 0x02,

   // FreeData: Set to free the owner data if the goal is deleted. Note that
   //           automatically freed data should not have any destructors, or
   //           contain any members with destructors.
   kAIGF_FreeData = 0x04,

};

///////////////////////////////////////
//
// Qualitative description of a goal
//

enum eAIGoalType
{
   kAIGT_Idle,
   kAIGT_Goto,
   kAIGT_Follow,
   kAIGT_Investigate,
   kAIGT_Converse,
   kAIGT_Custom,
   kAIGT_Defend,
   kAIGT_Attack,
   kAIGT_Flee,
   kAIGT_Die,

   kAIGT_Num,
   kAIGT_IntMax = 0xffffffff // force it use an int
};

const char * AIGetGoalTypeName(eAIGoalType);

///////////////////////////////////////

inline int AIGetGoalTypePriority(eAIGoalType type)
{
   return (int)type;
}

///////////////////////////////////////////////////////////////////////////////
//
// Helper for things like save/load
//

cAIGoal * AICreateGoalFromType(eAIGoalType type, IAIAbility * pOwner);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGoal
//
// Generally describes an AI goal
//

class cAIGoal : public cCTUnaggregated<IUnknown, &IID_IUnknown, kCTU_Default>
{
protected:
   cAIGoal(IAIAbility * pOwner, eAIGoalType goalType, DWORD data = 0);
   virtual ~cAIGoal();

public:
   // Update the location from the object
   BOOL LocFromObj();

   // Return pOwner, no AddRef.
   virtual IAIAbility *GetAbility();

   // Describe the goal, in human terms
   virtual void Describe(cStr * pStr) const;

   // Save/load into client block -- "type" is client responsibility
   virtual void Save(ITagFile *);
   virtual void Load(ITagFile *);

   // Notationally convenient check for progress, matching IAIAction model
   BOOL InProgress() const;

   // Get the type of goal
   eAIGoalType GetType() const;

private:
   // The kind of goal
   eAIGoalType  type;

public:
   // The goal location, if any
   cMxsVector   location;

   // The goal object, if any
   ObjID        object;

   // The priority of the goal
   eAIPriority  priority;

   // The time the goal expires, or zero
   unsigned     expiration;

   // Flags
   unsigned     flags;

   // Representation of goal progress, if possible
   int          pctComplete;

   // Goal resolution
   eAIResult    result;

   // IAIAbility
   IAIAbility * pOwner;
   DWORD        ownerData;
};

///////////////////////////////////////
//
// cAIGoal, inline functions
//

inline cAIGoal::cAIGoal(IAIAbility * pOwner, eAIGoalType goalType, DWORD data)
 : type       (goalType),
   location   (kInvalidLoc),
   object     (OBJ_NULL),
   priority   (kAIP_Normal),
   expiration (0),
   flags      (0),
   result     (kAIR_NoResult),
   pOwner     (pOwner),
   ownerData  (data)
{
}

///////////////////////////////////////

inline BOOL cAIGoal::InProgress() const
{
   return AIInProgress(result);
}

///////////////////////////////////////

inline eAIGoalType cAIGoal::GetType() const
{
   return type;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGotoGoal
//

#define kAIGG_DefaultAccuracy  (sq(3.0))
#define kAIGG_DefaultAccuracyZ (6.0833)
#define kAIGG_NoZAccuracy      kFloatMax

class cAIGotoGoal : public cAIGoal
{
public:
   cAIGotoGoal(IAIAbility * pOwner, DWORD data = 0);

   BOOL Reached() const;

public:
   // How fast should we move
   eAISpeed     speed;

   // How accurate must we be. If accuracyZ is kFloatMax, accuracy should
   // be measured as a sphere. Otherwise, a cylinder.
   float        accuracySq;
   float        accuracyZ;
};

///////////////////////////////////////
//
// cAIGotoGoal, inline functions
//

inline cAIGotoGoal::cAIGotoGoal(IAIAbility * pOwner, DWORD data)
 : cAIGoal   (pOwner, kAIGT_Goto, data),
   speed     (kAIS_Normal),
   accuracySq(kAIGG_DefaultAccuracy),
   accuracyZ (kAIGG_DefaultAccuracyZ)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIIdleGoal
//

class cAIIdleGoal : public cAIGoal
{
public:
   cAIIdleGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Idle, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFollowGoal
//

class cAIFollowGoal : public cAIGoal
{
public:
   cAIFollowGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Follow, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIInvestigateGoal
//

class cAIInvestigateGoal : public cAIGoal
{
public:
   cAIInvestigateGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Investigate, data),
      urgency(kAIP_Normal)
   {
   }

   eAIPriority urgency;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICustomGoal
//

class cAICustomGoal : public cAIGoal
{
public:
   cAICustomGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Custom, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDefendGoal
//

class cAIDefendGoal : public cAIGoal
{
public:
   cAIDefendGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Defend, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAttackGoal
//

class cAIAttackGoal : public cAIGoal
{
public:
   cAIAttackGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Attack, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFleeGoal
//

class cAIFleeGoal : public cAIGoal
{
public:
   cAIFleeGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Flee, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDieGoal
//

class cAIDieGoal : public cAIGoal
{
public:
   cAIDieGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Die, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIConverseGoal
//

class cAIConverseGoal : public cAIGoal
{
public:
   cAIConverseGoal(IAIAbility * pOwner, DWORD data = 0)
    : cAIGoal(pOwner, kAIGT_Custom, data)
   {
   }
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIGOAL_H */
