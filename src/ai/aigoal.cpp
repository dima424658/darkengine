/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aigoal.cpp,v 1.11 1999/11/29 12:28:24 BFarquha Exp $
//
//
//

#include <lg.h>
#include <str.h>

#include <aiapiabl.h>
#include <aiapiiai.h>
#include <aigoal.h>
#include <aitagtyp.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Goal Type Enum to String
//

static const char * g_ppszGoalTypes[] =
{
   "Idle Near Object/Location",                  // kAIGT_Idle
   "Goto Object or Location",                    // kAIGT_Goto
   "Follow Object",                              // kAIGT_Follow
   "Investigate Object/Location",                // kAIGT_Investigate
   "Conversation",                               // kAIGT_Converse
   "Custom Goal",                                // kAIGT_Custom
   "Defend Object or Location",                  // kAIGT_Defend
   "Attack Object",                              // kAIGT_Attack
   "Flee From Object or Location",               // kAIGT_Flee
   "Die",                                        // kAIGT_Die
#ifndef SHIP
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
   "Invalid goal!",
#endif
};

///////////////////////////////////////

const char * AIGetGoalTypeName(eAIGoalType type)
{
   AssertMsg(type < kAIGT_Num, "Invalid goal type");
   return g_ppszGoalTypes[type];
}

///////////////////////////////////////////////////////////////////////////////

cAIGoal * AICreateGoalFromType(eAIGoalType type, IAIAbility * pOwner)
{
   cAIGoal * pGoal = NULL;
   switch (type)
   {
      case kAIGT_Idle:        pGoal = new cAIIdleGoal(pOwner);         break;
      case kAIGT_Goto:        pGoal = new cAIGotoGoal(pOwner);         break;
      case kAIGT_Follow:      pGoal = new cAIFollowGoal(pOwner);       break;
      case kAIGT_Investigate: pGoal = new cAIInvestigateGoal(pOwner);  break;
      case kAIGT_Custom:      pGoal = new cAICustomGoal(pOwner);       break;
      case kAIGT_Defend:      pGoal = new cAIDefendGoal(pOwner);       break;
      case kAIGT_Attack:      pGoal = new cAIAttackGoal(pOwner);       break;
      case kAIGT_Flee:        pGoal = new cAIFleeGoal(pOwner);         break;
      case kAIGT_Die:         pGoal = new cAIDieGoal(pOwner);          break;
      case kAIGT_Converse:    pGoal = new cAIConverseGoal(pOwner);     break;
      default:
         CriticalMsg1("Cannot resolve unknown goal %d", type);
   }
   return pGoal;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGoal
//

cAIGoal::~cAIGoal()
{
   // good place for a breakpoint when looking for reference count problems...
   if (ownerData && (flags & kAIGF_FreeData))
      delete ((void *)ownerData);
}


IAIAbility *cAIGoal::GetAbility()
{
   return pOwner;
}


///////////////////////////////////////

BOOL cAIGoal::LocFromObj()
{
   return GetObjLocation(object, &location);
}

///////////////////////////////////////

void cAIGoal::Describe(cStr * pStr) const
{
   if (location != kInvalidLoc)
      pStr->FmtStr("'%s' [%d; (%f, %f, %f) ] at priority '%s'",
                  AIGetGoalTypeName(type),
                  object,
                  location.x,
                  location.y,
                  location.z,
                  AIGetPriorityName(priority));
   else if (object != OBJ_NULL)
      pStr->FmtStr("'%s' [%d] at priority '%s'",
                  AIGetGoalTypeName(type),
                  object,
                  AIGetPriorityName(priority));
   else
      pStr->FmtStr("'%s' at priority '%s'",
                  AIGetGoalTypeName(type),
                  AIGetPriorityName(priority));
}

///////////////////////////////////////

void cAIGoal::Save(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &location);
   AITagMove(pTagFile, &object);
   AITagMove(pTagFile, &priority);
   AITagMove(pTagFile, &expiration);
   AITagMove(pTagFile, &flags);
   AITagMove(pTagFile, &pctComplete);
   AITagMove(pTagFile, &result);
   AITagMove(pTagFile, &ownerData); // clients should resave this if fixup needed

   switch (type)
   {
      case kAIGT_Goto:
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->speed);
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->accuracySq);
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->accuracyZ);
         break;
      case kAIGT_Investigate:
         AITagMove(pTagFile, &((cAIInvestigateGoal *)this)->urgency);
         break;
   }
}

///////////////////////////////////////

void cAIGoal::Load(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &location);
   AITagMove(pTagFile, &object);
   AITagMove(pTagFile, &priority);
   AITagMove(pTagFile, &expiration);
   AITagMove(pTagFile, &flags);
   AITagMove(pTagFile, &pctComplete);
   AITagMove(pTagFile, &result);
   AITagMove(pTagFile, &ownerData); // clients should reload this if fixup needed

   switch (type)
   {
      case kAIGT_Goto:
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->speed);
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->accuracySq);
         AITagMove(pTagFile, &((cAIGotoGoal *)this)->accuracyZ);
         break;
      case kAIGT_Investigate:
         AITagMove(pTagFile, &((cAIInvestigateGoal *)this)->urgency);
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGotoGoal
//

BOOL cAIGotoGoal::Reached() const
{
   const cAIState * pAIState = pOwner->AccessOuterAI()->GetState();

   // Accuracy sphere
   if (accuracyZ == kFloatMax)
   {
      return (pAIState->DistSq(location) < accuracySq);
   }
   // Accuracy cylinder
   else if (pAIState->DiffZ(location) < accuracyZ + pOwner->AccessOuterAI()->GetGroundOffset() / 2) // @tbd: getgroundoffset() is property lookup -- move to sAIState
   {
      return (pAIState->DistXYSq(location) < accuracySq);
   }

   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
