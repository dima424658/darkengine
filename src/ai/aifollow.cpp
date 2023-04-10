/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aifollow.cpp,v 1.8 2000/02/28 11:30:42 toml Exp $
//
// @TBD (toml 08-07-98): rather than timer, should listen to link
//

// #define PROFILE_ON 1

#include <link.h>
#include <linkbase.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiactfol.h>
#include <aifollow.h>
#include <aigoal.h>
#include <aitagtyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

static IRelation * g_pAIFollowLinks;

///////////////////////////////////////////////////////////////////////////////
//
// Follow links
//

#define FOLLOW_VECTOR_FIELDS(i) \
   { "Vector " #i ": Angle",    kFieldTypeFloat, FieldLocation(sAIFollowLink, vectors[i-1].angle)                                                                  }, \
   { "               Distance", kFieldTypeInt,   FieldLocation(sAIFollowLink, vectors[i-1].distance)                                                               }

static sFieldDesc _g_AIFollowLinkFieldDesc[] =
{
   FOLLOW_VECTOR_FIELDS(1),
   FOLLOW_VECTOR_FIELDS(2),
   FOLLOW_VECTOR_FIELDS(3)
};

static sStructDesc _g_AIFollowLinkStructDesc = StructDescBuild(sAIFollowLink, kStructFlagNone, _g_AIFollowLinkFieldDesc);

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitFollowAbility(IAIManager *)
{
   sRelationDesc     FollowLinkDesc     = { "AIFollowObj", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc FollowLinkDataDesc = { "sAIFollowLink", sizeof(sAIFollowLink), kRelationDataAutoCreate };

   StructDescRegister(&_g_AIFollowLinkStructDesc);

   g_pAIFollowLinks = CreateStandardRelation(&FollowLinkDesc, &FollowLinkDataDesc, kQCaseSetSourceKnown);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermFollowAbility()
{
   SafeRelease(g_pAIFollowLinks);

   return TRUE;
}

///////////////////////////////////////

static const sAIFollowLink * AIGetFollowLink(ObjID obj)
{
   LinkID linkId = g_pAIFollowLinks->GetSingleLink(obj, LINKOBJ_WILDCARD);

   if (linkId)
   {
      sLink link;
      sAIFollowLink * pResult;

      pResult = (sAIFollowLink * )g_pAIFollowLinks->GetData(linkId);
      g_pAIFollowLinks->Get(linkId, &link);
      pResult->target = link.dest;

      return pResult;
   }
   return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFollow -- follow component
//

cAIFollow::cAIFollow()
 : m_Timer(kAIT_1Sec)
{
}

///////////////////////////////////////

cAIFollow::~cAIFollow()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIFollow::GetName()
{
   return "Follow ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFollow::Init()
{
   SetNotifications(kAICN_ActionProgress);
   m_Timer.Reset();
}

///////////////////////////////////////

DECLARE_TIMER(cAIFollow_OnActionProgress, Average);

STDMETHODIMP_(void) cAIFollow::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIFollow_OnActionProgress);

   // If we're in a good position to interrupt...
   if (pAction->GetResult() > kAIR_NoResult)
   {
      if (!m_Timer.Expired())
         return;

      SignalGoal();
   }
}

///////////////////////////////////////

STDMETHODIMP cAIFollow::SuggestGoal(cAIGoal * pCurrentGoal, cAIGoal ** ppGoal)
{
   m_Timer.Reset();

   const sAIFollowLink * pFollow = AIGetFollowLink(GetID());

   if (!pFollow)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   if (pCurrentGoal && memcmp(pFollow, &m_Current, sizeof(sAIFollowLink)) == 0)
   {
      pCurrentGoal->AddRef();
      *ppGoal = pCurrentGoal;

      if (!InControl())
         SignalAction();

      return S_OK;
   }


   *ppGoal = new cAIFollowGoal(this);
   if (!pFollow->priority)
      (*ppGoal)->priority = kAIP_Normal;
   else
      (*ppGoal)->priority = pFollow->priority;

   (*ppGoal)->object = pFollow->target;

   m_Current = *pFollow;

   SignalAction();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIFollow::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIFollowAction * pAction = CreateFollowAction();

//  DANGER!  This causes us to set a target that is unitialized memory on
//  load, since until we run UpdateGoal, we haven't set m_Current.target.
//  Since the goal always gets the target anyway, we should read the object
//  from the goal.  AMSD 2/15/00
//   pAction->SetObject(m_Current.target);

   //check if m_current seems out of date
   if ((pGoal->pOwner == this) && (pGoal->object!=m_Current.target))
   {
      const sAIFollowLink * pFollow = AIGetFollowLink(GetID());
      if (pFollow)
         m_Current = *pFollow;  //grab and update;
   }

   pAction->SetObject(m_Current.target); //it's our next best guess.
  
   for (int i = 0; i < kAIF_MaxVectors; i++)
   {
      if (m_Current.vectors[i].distance)
         pAction->AddVector(DEGREES(m_Current.vectors[i].angle), m_Current.vectors[i].distance);
   }

   pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
