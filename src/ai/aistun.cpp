/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aistun.cpp,v 1.11 2000/02/11 18:28:01 bfarquha Exp $
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>
#include <rand.h>
#include <simtime.h>

#include <appagg.h>

#include <mtagvals.h>

#include <aiactmot.h>
#include <aiapibhv.h>
#include <aibasabl.h>
#include <aigoal.h>
#include <aiutils.h>
#include <aiactloc.h>
#include <aistun.h>
#include <aistnapi.h>
#include <aitagtyp.h>
#include <aisndtyp.h>
#include <aiprops.h>
#include <aiaware.h>

// property stuff
#include <property.h>
#include <propface.h>
#include <propbase.h>

// scripting
#include <scrptbas.h>
#include <scrptapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
// init/term

static IVectorProperty *g_pAISupriseProp = NULL;
static sPropertyDesc    SupriseDesc = { "AISuprise", 0, NULL, 0, 0, { AI_CORE_CAT, "Suprise [0, 1, Rad]" }, kPropertyChangeLocally };
static mxs_vector       defSupriseVec;

// so that no two AIs ever get suprised right after eachother
#define kMinReSuprise   (8000)
static int              lastSupriseTime=0;
#define ResetSuprise()  (lastSupriseTime=-kMinReSuprise)

void AIInitStunAbility(void)
{
   g_pAISupriseProp = CreateVectorProperty(&SupriseDesc,kPropertyImplSparse);
   defSupriseVec.el[0]=80; defSupriseVec.el[1]=45; defSupriseVec.el[2]=7;
   ResetSuprise();
}

void AITermStunAbility(void)
{
   SafeRelease(g_pAISupriseProp);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIStun
//

STDMETHODIMP_(const char *) cAIStun::GetName()
{
   return "Stun ability";
}

///////////////////////////////////////

cDynArray<cAIStun *> cAIStun::gm_StunAbilities;

// constructor and destructor
cAIStun::cAIStun()
   : m_StunMs(0), m_Stunned(FALSE), m_NewStun(FALSE)
{
   gm_StunAbilities.Append(this);
}

cAIStun::~cAIStun()
{
   for (int i = 0; i < gm_StunAbilities.Size(); i++)
      if (gm_StunAbilities[i] == this)
      {
         gm_StunAbilities.FastDeleteItem(i);
         break;
      }
}

STDMETHODIMP_(void) cAIStun::Init()
{
   SetNotifications(kAICN_Alertness);
}

///////////////////////////////////////

cAIStun * cAIStun::AccessAIStun(ObjID id)
{
   for (int i = 0; i < gm_StunAbilities.Size(); i++)
      if (gm_StunAbilities[i]->GetID() == id)
         return gm_StunAbilities[i];
   return NULL;
}

///////////////////////////////////////

#define kStunSaveVer 2

STDMETHODIMP_(BOOL) cAIStun::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Stun, 0, kStunSaveVer, pTagFile))
   {
      BaseAbilitySave(pTagFile);

      AITagMove(pTagFile, &m_Stunned);
      AITagMove(pTagFile, &m_StunMs);
      AITagMove(pTagFile, &m_SingleStun);
      AITagMove(pTagFile, &m_AtLoop);
      AITagMove(pTagFile, &m_NewStun);
      m_Timer.Save(pTagFile);

      cStr tagStr;

      m_StunBegin.ToString(&tagStr);
      AITagMoveString(pTagFile, &tagStr);

      m_StunLoop.ToString(&tagStr);
      AITagMoveString(pTagFile, &tagStr);

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIStun::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Stun, 0, kStunSaveVer, pTagFile))
   {
      BaseAbilityLoad(pTagFile);

      AITagMove(pTagFile, &m_Stunned);
      AITagMove(pTagFile, &m_StunMs);
      AITagMove(pTagFile, &m_SingleStun);
      AITagMove(pTagFile, &m_AtLoop);
      AITagMove(pTagFile, &m_NewStun);
      m_Timer.Load(pTagFile);

      cStr tagStr;

      AITagMoveString(pTagFile, &tagStr);
      m_StunBegin.FromString(tagStr);

      AITagMoveString(pTagFile, &tagStr);
      m_StunLoop.FromString(tagStr);

      AICloseTagBlock(pTagFile);
   }
   ResetSuprise();   // this is somewhat of a hack
   return TRUE;
}

///////////////////////////////////////

#define PI_AGAIN 3.14159265358979323846

STDMETHODIMP_(void) cAIStun::OnAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   if (source==OBJ_NULL) return;
   // currently is happening on combat damage... dont want that, i think
   mxs_vector *pSupriseVec=&defSupriseVec;
   //   mprintf("Alert from %d to %d\n",previous,current);
   if (GetSimTime()>lastSupriseTime+kMinReSuprise)
   {
      if ((current==3)&&(previous<2))
      {
         g_pAISupriseProp->Get(GetID(),&pSupriseVec);
         if ((pSupriseVec->el[0]!=0)||(pSupriseVec->el[1]!=0))
         {
            const sAIAwareness *pAwareness = m_pAI->GetAwareness(source);
            if (pAwareness->ValidLastPos())
               if (mx_dist2_vec(&pAwareness->lastPos,m_pAIState->GetLocation())<pSupriseVec->el[2]*pSupriseVec->el[2])
               {  // within the legal radius
                  float chance=RandRange(0,100);
                  if (pSupriseVec->el[previous]>chance)
                  {
                     floatang ang(m_pAIState->GetLocation()->x,m_pAIState->GetLocation()->y,
                                  pAwareness->lastPos.x,pAwareness->lastPos.y);
                     float ang_diff=Delta(m_pAIState->GetFacingAng(),ang).value;

                     // mprintf("Have %g for angle diff (%g %g)\n",
                     //       ang_diff,ang.value,m_pAIState->GetFacingAng().value);

                     if (ang_diff>(PI_AGAIN/1.7)) // a little over 90 to force a wheel
                     {
                        // this doesnt seem to work
                        // SetStunned("stand, wheel",NULL,0);
                        SetStunned("ReceiveWound 0, Direction 3, MeleeCombat 0",NULL,0);
                        // mprintf("behind me\n");
                     }
                     else
                        SetStunned("discover, recoil",NULL,0);
                     lastSupriseTime=GetSimTime();

                     //  mprintf("Suprised %d\n",GetSimTime());
                  }
               }
         }
      }
   }
#if 0
   else
      mprintf("Stun Freq disallow now %d last %d\n",GetSimTime(),lastSupriseTime);
#endif
}

///////////////////////////////////////

DECLARE_TIMER(cAIStun_OnActionProgress, Average);

// see if it is time for a new action
STDMETHODIMP_(void) cAIStun::OnActionProgress(IAIAction * pAction)
{  // If we're in a good position to interrupt...
   AUTO_TIMER(cAIStun_OnActionProgress);

   if (IsOwn(pAction))
      if (pAction->GetResult() > kAIR_NoResult)
         if ((m_SingleStun && !pAction->InProgress()) || m_Timer.Expired())
         {   // im done
            m_Stunned=FALSE;
            SignalGoal();
         }
         else if (!pAction->InProgress())
         {
            if (!m_AtLoop)
               m_AtLoop=TRUE;  // if not already looping, start looping
            SignalAction();    // if we completed our goal, signal for more
         }
}

///////////////////////////////////////

STDMETHODIMP cAIStun::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!m_Stunned)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }
   *ppGoal = new cAIFleeGoal(this);    // ??????
   (*ppGoal)->priority = kAIP_VeryHigh;
   (*ppGoal)->object = GetID();
   (*ppGoal)->LocFromObj();
   SignalAction();  // this wasnt here before?
   SetNotifications(kAICN_ActionProgress|kAICN_GoalChange);
   return S_OK;
}

///////////////////////////////////////
//
// Send messages to scripts on stun/unstun
//

STDMETHODIMP_(void) cAIStun::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAIAbility::OnGoalChange(pPrevious, pGoal);
   AutoAppIPtr(ScriptMan);
   if (IsLosingControl(pPrevious, pGoal))
   {
      sScrMsg* pMsg = new sScrMsg(m_pAIState->GetID(), "Unstun");
      pMsg->flags |= kSMF_MsgPostToOwner;
      pScriptMan->PostMessage(pMsg);
      ClearNotifications(kAICN_ActionProgress|kAICN_GoalChange);
   }
   else if (IsGainingControl(pPrevious, pGoal))
   {
      sScrMsg* pMsg = new sScrMsg(m_pAIState->GetID(), "Stun");
      pMsg->flags |= kSMF_MsgPostToOwner;
      pScriptMan->PostMessage(pMsg);
   }
}

///////////////////////////////////////

STDMETHODIMP cAIStun::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   if (!m_Stunned)
   {
      Warning(("Why are we in Stunned controller???\n"));
      return S_FALSE;
   }

   cAIMotionAction * pAction = CreateMotionAction();
   if (m_AtLoop)
      pAction->AddTags(m_StunLoop);
   else
      pAction->AddTags(m_StunBegin);
   pNew->Append(pAction);
   if (m_NewStun)
   {
      if (m_StunMs)
         m_Timer.Set(AICustomTime(m_StunMs));
      else
         m_Timer.Set(AICustomTime(kAIT_Infinite));
      m_NewStun=FALSE;
   }
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

// actual outside things to do
// ways to set that you are stunned

BOOL cAIStun::SetStunned(cTagSet *beginTags,    cTagSet *loopTags,    int ms)
{
   m_StunLoop=*loopTags;
   if (beginTags)
   {
      m_StunBegin=*beginTags;
      m_AtLoop=FALSE;
   }
   else
      m_AtLoop=TRUE;
   m_StunMs=ms;
   m_Stunned=TRUE;
   m_SingleStun=(ms==0);
   m_NewStun=TRUE;
   SignalGoal();
   return TRUE;
}

BOOL cAIStun::SetStunned(const char    *beginTagsStr, const char    *loopTagsStr, int ms)
{  // this is just gonna convert the strings to tags
   cTagSet beginTags, loopTags;
   loopTags.FromString(loopTagsStr);
   if (beginTagsStr)
   {
      beginTags.FromString(beginTagsStr);
      return SetStunned(&beginTags,&loopTags,ms);
   }
   else
      return SetStunned(NULL,&loopTags,ms);
}

/////////////////////////////////////////////////

BOOL cAIStun::UnsetStunned(void)
{
   if (m_Stunned)
   {
      m_Stunned=FALSE;
      SignalGoal();
      return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////

EXTERN BOOL DoAISetStun(ObjID obj, const char *begin, const char *loop, int ms)
{
   cAIStun *pAIStun = cAIStun::AccessAIStun(obj);
   if (pAIStun)
      pAIStun->SetStunned(begin, loop, ms);
   else
      return FALSE;
   return TRUE;
}

/////////////////////////////////////////////////

EXTERN BOOL DoAIUnsetStun(ObjID obj)
{
   cAIStun *pAIStun = cAIStun::AccessAIStun(obj);
   if (pAIStun)
      return pAIStun->UnsetStunned();
   else
      return FALSE;
   return TRUE;
}

/////////////////////////////////////////////////

EXTERN BOOL AIGetStun(ObjID obj)
{
   cAIStun *pAIStun = cAIStun::AccessAIStun(obj);
   if (pAIStun)
      return pAIStun->GetStunned();
   return FALSE;
}
