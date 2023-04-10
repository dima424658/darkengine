/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbhtoh.cpp,v 1.105 2000/03/06 22:36:26 bfarquha Exp $
//

// THINGS I HAVE TO ADD TO COMBAT THIS WEEKEND
//x  a. detect what the weapon actually is
//x  b. make sure sidestep and forward motion actions often end with attacks
//  c. better skill dynamic range
//x  d. bias against current specific mode
//x  e. bias against current mode type
//  f. try and deal w/distance better
//x  g. support special attacks
//  h. note an opponent facing away... deal with it
// what to do with blocks?

// skill usage in the HtoH combat code
//  Sloth=IdleTime... if Null, should never idle
//  Dodginess=Chance of doing a jitter motion (sidestep, backup)
//  Aggression=Chance of doing an attack type
//  Verbosity=modifier to likelihood of speaking during combat

// @TBD (toml 03-04-99): pick through these and determine what really is needed

// #define PROFILE_ON 1


#include <lg.h>
#include <mprintf.h>
#include <cfgdbg.h>
#include <appagg.h>

#include <ctagset.h>
#include <relation.h>
#include <playrobj.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactori.h>
#include <aiactseq.h>
#include <aiapibhv.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aiapiact.h>
#include <aidebug.h>
#include <aicbhtoh.h>
#include <aipthloc.h>
#include <aibasabl.h>
#include <aiapisnd.h>
#include <aisndtyp.h>
#include <aiapiiai.h>
#include <aiprattr.h>
#include <aitagtyp.h>
#include <aipthcst.h>
#include <aigoal.h>
#include <aitrginf.h>

#include <mtagvals.h>
#include <objhp.h>
#include <objpos.h>
#include <dmgbase.h>
#include <physapi.h>
#include <objedit.h>
#include <simtime.h>

// @TODO: this is dark only, somehow...
#include <drkwswd.h>
#include <drkwbow.h>

#include <aiprcore.h>

// property stuff
#include <property.h>
#include <propface.h>
#include <propbase.h>

// for sAIFrustrated:
#include <aialert.h>

// for g_pRangedCombatProp:
#include <aiprrngd.h>

// for AIFleeIsDest
#include <aiflee.h>

// for cell zone info
#include <aipathdb.h>

#include <aiteams.h>

#include <traitman.h>

#include <random.h>

// for event callbacks/weapon watching
#include <weapcb.h>
#include <weapon.h>

// Must be last header
#include <dbmem.h>

#ifndef SHIP
#define AI_COMBAT_DEBUGGING
#endif

#ifdef AI_COMBAT_DEBUGGING
#define _HD                              "AIHTHWatch %d: "
#define _HD_P                            GetID()
#define _HTHWatchObj()                   (AIIsWatched(Cbt, GetID()))
#define _HTHWatchPrint(str)              do { if (_HTHWatchObj()) mprintf(_HD##str,_HD_P); } while (0)
#define _HTHWatchPrint1(str,v1)          do { if (_HTHWatchObj()) mprintf(_HD##str,_HD_P,v1); } while (0)
#define _HTHWatchPrint2(str,v1,v2)       do { if (_HTHWatchObj()) mprintf(_HD##str,_HD_P,v1,v2); } while (0)
#define _HTHWatchPrint3(str,v1,v2,v3)    do { if (_HTHWatchObj()) mprintf(_HD##str,_HD_P,v1,v2,v3); } while (0)
#define _HTHWatchPrint4(str,v1,v2,v3,v4) do { if (_HTHWatchObj()) mprintf(_HD##str,_HD_P,v1,v2,v3,v4); } while (0)
#define _HTHAddScreenString(str)         // AddScreenString(str)
#define _HTHModeName(mode)               g_AIAtkModeNames[ModeGetBase(mode)]
const char * g_AIAtkModeNames[] =
 { "Undecided","Clock","CounterClock","Charge","ChargeUnseen","Backoff",
   "JumpBack","Advance","AdvanceBig","Avoid","Block","DirectedBlock",
   "Dodge","Idle","NoMove","SwingQuick","SwingNormal","SwingMassive",
   "SwingSpecial","TakeDamage","OffTheList" };
#else
#define _HTHWatchPrint(str)
#define _HTHWatchPrint1(str,v1)
#define _HTHWatchPrint2(str,v1,v2)
#define _HTHWatchPrint3(str,v1,v2,v3)
#define _HTHWatchPrint4(str,v1,v2,v3,v4)
#define _HTHAddScreenString(str)
#define _HTHModeName(mode)
#endif

#define _HTHWatchInterrupts(str)       _HTHWatchPrint("_intr_ "str)

//////////////////////////////////////////////////////////////////////////////

#define YA_PI (3.14159265358979323846) // yet another pi

//////////////////////////////////////////////////////////////////////////////

static int overall_last_speech_time=0;

static mxs_vector default_audio;
static mxs_vector default_motion;

static IVectorProperty *g_pAIHtoHAudioResponse = NULL;
static sPropertyDesc HtoHAudioResp = { "HTHAudioResp", 0, NULL, 0, 0, { AI_ABILITY_CAT, "HtoHCombat: Audio Response" }, kPropertyChangeLocally };

static IVectorProperty *g_pAIHtoHMotionResponse = NULL;
static sPropertyDesc HtoHMotionResp = { "HTHMotionResp", 0, NULL, 0, 0, { AI_ABILITY_CAT, "HtoHCombat: Motion Response" }, kPropertyChangeLocally };

static IBoolProperty *g_pAIHtoHGruntAlways = NULL;
static sPropertyDesc HtoHGruntAlways = { "HTHGruntAlways", 0, NULL, 0, 0, { AI_ABILITY_CAT, "HtoHCombat: Grunt Always" }, kPropertyChangeLocally };

#ifdef AI_COMBAT_DEBUGGING
static IIntProperty *g_pAIHtoHModeOverride = NULL;
static sPropertyDesc HtoHModeOverride = { "HTHModeOverride", 0, NULL, 0, 0, { AI_DEBUG_CAT, "HtoHModeOverride" }, kPropertyChangeLocally };
#endif

static BOOL g_NoCombatRehosting=FALSE; // Set when defined config var "ai_no_combat_rehosting"

 // These are for the first byte of network messages.
static tNetMsgHandlerID g_NetMsgRehostHandlerID;
static tNetMsgHandlerID g_NetMsgLoseControlHandlerID;

// build our properties
BOOL AIInitCombatHtoHAbility(IAIManager *)
{
   g_pAIHtoHAudioResponse  = CreateVectorProperty(&HtoHAudioResp,kPropertyImplVerySparse);
   g_pAIHtoHMotionResponse = CreateVectorProperty(&HtoHMotionResp,kPropertyImplVerySparse);
   g_pAIHtoHGruntAlways    = CreateBoolProperty(&HtoHGruntAlways,kPropertyImplVerySparse);
#ifdef AI_COMBAT_DEBUGGING
   g_pAIHtoHModeOverride   = CreateIntProperty(&HtoHModeOverride,kPropertyImplVerySparse);
#endif

   // i think i cant locally static init arrays, so i do them here once
   default_audio.el[0]  = 1.5; default_audio.el[1]  = 7.0; default_audio.el[2]  = 1.5;
   default_motion.el[0] = 3.7; default_motion.el[1] = 7.0; default_motion.el[2] = 1.6;

   return TRUE;
}

// clean up
BOOL AITermCombatHtoHAbility(void)
{
   SafeRelease(g_pAIHtoHAudioResponse);
   SafeRelease(g_pAIHtoHMotionResponse);
   SafeRelease(g_pAIHtoHGruntAlways);
#ifdef AI_COMBAT_DEBUGGING
   SafeRelease(g_pAIHtoHModeOverride);
#endif
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIHtoHSubcombat
//

cAIHtoHSubcombat::cAIHtoHSubcombat()
 : m_mode(kUndecided),
   m_ModeDuration(0),
   m_lastDistSq(kFloatMax),
   m_OppAttacking(FALSE),
   m_OppBlocking(FALSE),
   m_DoDirectionBlock(-1),
   m_EventQueuePtr(0),
   m_TakeDamage(FALSE),
   m_BackoffCount(0),
   m_FrustrationTimer(20000, 45000),
   m_lastChargeTime(0),
   m_lastFleeTime(0),
   m_failedChargePathTime(0),
   m_BackoffFail(0),
   m_pModeSels(&gm_DefaultModeSelections)
{
}

///////////////////////////////////////

cAIHtoHSubcombat::cAIHtoHSubcombat(sHtoHModeSelections * pModeSels)
 : m_mode(kUndecided),
   m_ModeDuration(0),
   m_lastDistSq(kFloatMax),
   m_OppAttacking(FALSE),
   m_OppBlocking(FALSE),
   m_DoDirectionBlock(-1),
   m_EventQueuePtr(0),
   m_TakeDamage(FALSE),
   m_BackoffCount(0),
   m_FrustrationTimer(20000, 45000),
   m_lastChargeTime(0),
   m_failedChargePathTime(0),
   m_BackoffFail(0),
   m_pModeSels(pModeSels)
{
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIHtoHSubcombat::Init()
{
   cAISubcombat::Init();

   SetNotifications(kAICN_ActionProgress |
                    kAICN_Damage |
                    kAICN_Weapon |
                    kAICN_GoalChange);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIHtoHSubcombat::GetName()
{
   return "Hand-to-hand combat";
}

///////////////////////////////////////

#define kHtoHVer 3

STDMETHODIMP_(BOOL) cAIHtoHSubcombat::Save(ITagFile * pTagFile)
{
   if (cAISubcombat::Save(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatHtoH, 0, kHtoHVer, pTagFile))
      {
         AITagMove(pTagFile, &m_mode);
         AITagMove(pTagFile, &m_ModeDuration);
         AITagMove(pTagFile, &m_lastDistSq);
         AITagMove(pTagFile, &m_DoDirectionBlock);
         AITagMove(pTagFile, &m_OppAttacking);
         AITagMove(pTagFile, &m_OppBlocking);
         AITagMove(pTagFile, &m_OppUnarmed);
         AITagMove(pTagFile, &m_EventQueue);
         AITagMove(pTagFile, &m_EventQueuePtr);
         AITagMove(pTagFile, &m_TakeDamage);

         m_FrustrationTimer.Save(pTagFile);

         cStr tagStr;

         m_DamageTags.ToString(&tagStr);
         AITagMoveString(pTagFile, &tagStr);

         AICloseTagBlock(pTagFile);
      }
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIHtoHSubcombat::Load(ITagFile * pTagFile)
{
   if (cAISubcombat::Load(pTagFile))
   {
      if (AIOpenTagBlock(GetID(), kAISL_CombatHtoH, 0, kHtoHVer, pTagFile))
      {
         AITagMove(pTagFile, &m_mode);
         AITagMove(pTagFile, &m_ModeDuration);
         AITagMove(pTagFile, &m_lastDistSq);
         AITagMove(pTagFile, &m_DoDirectionBlock);
         AITagMove(pTagFile, &m_OppAttacking);
         AITagMove(pTagFile, &m_OppBlocking);
         AITagMove(pTagFile, &m_OppUnarmed);
         AITagMove(pTagFile, &m_EventQueue);
         AITagMove(pTagFile, &m_EventQueuePtr);
         AITagMove(pTagFile, &m_TakeDamage);

         m_FrustrationTimer.Load(pTagFile);

         cStr tagStr;

         AITagMoveString(pTagFile, &tagStr);
         m_DamageTags.FromString(tagStr);

         AICloseTagBlock(pTagFile);

         overall_last_speech_time=0;
      }
   }
  return TRUE;
}

////////////////////////////
// various rating interpretation/usage defines

#define RatingFltPercentage(rating) ((rating*0.2)-0.1)
#define RatingIdleTime(rating)      (100+(200*rating))

///////////////////////////////////////////////////////////////////////////////

BOOL cAIHtoHSubcombat::CheckStat(eAIRating stat, eActPriority pri, float fac)
{
   if (stat==kAIRT_Null)
      return FALSE;
   float chance=RatingFltPercentage(stat)*fac*256.0;
   switch (pri)
   {  // @TODO: this is dumb, need stats to be able to max things out much easier
      case kRnd:  chance*=((Rand()&0x7)+2)/5;
      case kLow:  chance/=1.8; break;
      case kNorm: break;
      case kHigh: chance*=1.8; break;            // just use the base
      case kMust: return TRUE;
   }
#ifdef LISTEN_LOUD
   BOOL res = (Rand()&0xff)<chance;
   mprintf("Choosing %d chance %g for stat %d pri %d fac %g\n",res,chance,stat,pri,fac);
   return res;
#else
   return (Rand()&0xff)<chance;
#endif
}

#define CheckVerbosity(pri) (CheckStat(AIGetVerbosity(GetID()),pri,0.5))
#define CheckAptitude(pri)  (CheckStat(AIGetAptitude(GetID()),pri,1.2))

///////////////////////////////////////
// check for our magic "halo code has told us to block in a direction"
STDMETHODIMP_(void) cAIHtoHSubcombat::OnGameEvent(void *magic)
{
   enum eMTagDirectionValues dir_tags[]={kMTV_high,kMTV_right,kMTV_low,kMTV_left};
   int phys_dir=(int)magic;

   if (phys_dir<(sizeof(dir_tags)/sizeof(dir_tags[0])))
   {
      m_DoDirectionBlock=dir_tags[phys_dir];
      SignalAction();
      _HTHWatchPrint1("Halo Hit, directed block to %d\n",m_DoDirectionBlock);
   }
}

DECLARE_TIMER(cAIHtoHSubcombat_OnActionProgress, Average);

STDMETHODIMP_(void) cAIHtoHSubcombat::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIHtoHSubcombat_OnActionProgress);

#ifdef NET_TODO // NEW_NETWORK_ENABLED
   if (m_pAI->IsNetworkProxy())
   {
      // We are only controlling low-level combat actions, so don't call
      // cAISubcombat::OnActionProgress which is for higher level decisions.
      if (!pAction->InProgress())
         SignalAction();
   }
   else if (m_pAIState->IsBrainsOnly())
   {
      // Another machine is deciding combat actions, but we still need to
      // decide on goal changes.
      SignalGoal();
   }
   else
#endif
      cAISubcombat::OnActionProgress(pAction);

   pAction = pAction->GetTrueAction(); // get our action in case proxied

   if (pAction->GetResult() == kAIR_NoResultSwitch)
      if ((pAction->GetType()==kAIAT_Locomote)||
          ((m_mode&kTypeInterrupt)==kTypeInterrupt))
         SignalAction();   // while Loco or interruptable, signal actions
}

// if we are idling, check the timings
STDMETHODIMP_(void) cAIHtoHSubcombat::OnBeginFrame(void)
{
   m_ModeDuration += AIGetFrameTime();  // let it go
}

STDMETHODIMP_(void) cAIHtoHSubcombat::OnDamage(const sDamageMsg *pMsg, ObjID realCulpritID)
{
   mxs_vector *audio=&default_audio, *motion=&default_motion;
   cAISubcombat::OnDamage(pMsg, realCulpritID);
   if (pMsg->kind!=kDamageMsgDamage)
      return;   // if it isnt "damage" damage, go home

   sDamage *ouch=pMsg->data.damage;

   g_pAIHtoHAudioResponse->Get(GetID(),&audio);
   g_pAIHtoHMotionResponse->Get(GetID(),&motion);

   // add random
   float dmg=ouch->amount, var;
   var=((2.0*Rand()-RAND_MAX)/(RAND_MAX+1))*audio->el[2];
   dmg+=var;
   _HTHWatchPrint4("AudioResponse raw %g use %g parms %g %g\n",(float)ouch->amount,dmg,audio->el[0],audio->el[1]);
   if (dmg>audio->el[0])
      PlayCombatSound((dmg>audio->el[1])?kAISC_CombatHitDamageHigh:kAISC_CombatHitDamageLow,TRUE);
   else
   {
      BOOL grunts=FALSE;
      g_pAIHtoHGruntAlways->Get(GetID(),&grunts);
      if (grunts)
      {
         _HTHWatchPrint("Grunt!\n");  // for now, till we have real data
         PlayCombatSound(kAISC_CombatHitNoDam,TRUE);
      }
   }
   dmg=ouch->amount;
   var=((2.0*Rand()-RAND_MAX)/(RAND_MAX+1))*motion->el[2];
   dmg+=var;
   _HTHWatchPrint4("MotionResponse raw %g use %g parms %g %g\n",(float)ouch->amount,dmg,motion->el[0],motion->el[1]);
   if (dmg>motion->el[0])
   {
      CacheVisualDamageTags(dmg>motion->el[1]);
      m_TakeDamage=TRUE;             // messages
      SignalAction();
   }
}

STDMETHODIMP_(void) cAIHtoHSubcombat::OnWeapon(eWeaponEvent ev, ObjID victim, ObjID culprit)
{
   cAISubcombat::OnWeapon(ev, victim, culprit);

   ObjID targetObj=GetTarget();

   if (!targetObj)
      return;

   eAIHtoHCombatEvent new_ev=kAIHCE_EventNull;

   if (ev&kStartEndEvents)
      if (culprit==targetObj)
      {  // my target has started or ended an action
              if (ev&(kStartWindup|kStartAttack)) new_ev=kAIHCE_OppAttackStart;
         else if (ev&kEndAttack)   new_ev=kAIHCE_OppAttackEnd;
         else if (ev&kStartBlock)  new_ev=kAIHCE_OppBlockStart;
         else if (ev&kEndBlock)    new_ev=kAIHCE_OppBlockEnd;
         else Warning(("what StartEndEvent is this? %x\n",ev));
      }
      else
      if ((culprit != GetID()) && (victim != GetID()))
         Warning(("Why am i being told %x %d %d\n",ev,victim,culprit));
   else if (ev&kBlockEvent)
      if (victim==GetID() || culprit==GetID())
         new_ev=kAIHCE_Block;
      else
         Warning(("Hey, getting a block for %d %d, im %d\n",victim,culprit,GetID()));
   else if (ev&kHitEvent)
      if (victim==GetID())
         new_ev=kAIHCE_Hit;
      else if (culprit==GetID())
         new_ev=kAIHCE_Hitting;
      else
         Warning(("Hey, getting a hit for %d %d, im %d\n",victim,culprit,GetID()));
   if (new_ev!=kAIHCE_EventNull)
   {
      InformOfEvent(new_ev);
      SignalAction();
   }
}

///////////////////////////////////////

BOOL cAIHtoHSubcombat::CheckModeInterrupt(eMode * pNewMode)
{
   // if we are in "response to ourselves" mode, do so
   if (m_DoDirectionBlock!=-1)   // we must Block
      *pNewMode=kDirectedBlock;
   else if (m_TakeDamage)
      *pNewMode=kTakeDamage;
   else
   {
#define kTooCloseSq sq(3.5)
      // See if we need a distance event
      if (m_lastDistSq > kTooCloseSq && GetTargetInfo()->distSq < kTooCloseSq)
         InformOfEvent(kAIHCE_TooClose);
      m_lastDistSq = GetTargetInfo()->distSq;
      if (m_EventQueuePtr)                               // hey, events to examine
         if (!CheckInterruptForEvent(pNewMode))          // oh, we dont care
            return FALSE;
   }

   return TRUE;
}

///////////////////////////////////////


float g_H2HNoFreshenRange = 34.0;
float g_H2HSpeed = 11.0;

DECLARE_TIMER(cAIHtoHSubcombat_SuggestActions, Average);

eMode cAIHtoHSubcombat::SelectMode()
{
   eMode newMode = kUndecided;
   if (m_mode == kIdle && m_ModeDuration < RatingIdleTime(AIGetSloth(GetID())))
   {
      _HTHWatchPrint1("still idling %d\n",m_ModeDuration);
      newMode = kIdle;  // if weve been idling under 1 second, keep going
   }
   else
   {
      switch (GetTargetInfo()->range)         // now, we need to decide what to do
      {
         case kAICR_Unseen:  newMode = kChargeUnseen; break;
         case kAICR_HugeZ:   newMode = kFrustrate; break;
         case kAICR_Huge:    newMode = kCharge; break;
         case kAICR_Far:     newMode = kAdvanceBig; break;
         case kAICR_JustFar: newMode = kAdvance; break;
         case kAICR_None:
         case kAICR_Near:
            if (m_BackoffCount < 3)
               newMode = kBackoff;
            else // this may need to become a RespTooOftenTooClose thing!!
               newMode = kSwingQuick; // or should we just do a ChooseNewAttackMode?
            break;
         default:
            {
            newMode = ChooseNewAttackMode(GetTargetInfo()->id, GetTargetInfo()->loc, GetTargetInfo()->zdist);
            }
            break;
      }
   }
   return newMode;
}



////////////////////////////////////////////////
// event tracking/parsing/updating sorts of code
////////////////////////////////////////////////

// update my internal gnosis of what the opponent is doing
void cAIHtoHSubcombat::UpdateOppState(eAIHtoHCombatEvent ev)
{
   switch (ev)   // parse what happened into quick changes in Opponent State
   {
      case kAIHCE_OppAttackStart: m_OppAttacking=TRUE; break;
      case kAIHCE_OppAttackEnd:   m_OppAttacking=FALSE; break;
      case kAIHCE_OppBlockStart:  m_OppBlocking=TRUE; break;
      case kAIHCE_OppBlockEnd:    m_OppBlocking=FALSE; break;
   }
}

// tell me a combat event has happened
void cAIHtoHSubcombat::InformOfEvent(eAIHtoHCombatEvent ev)
{
   if (m_EventQueuePtr>=AIHCE_EventQueueLen)
   {
      int i;
      for (i=0; i<m_EventQueuePtr; i++)
         UpdateOppState(m_EventQueue[i]);
      m_EventQueuePtr=0;
   }
   m_EventQueue[m_EventQueuePtr++]=ev;
}

// check to see if any events in the queue imply a new mode now
BOOL cAIHtoHSubcombat::CheckInterruptForEvent(eMode * newMode)
{
   sModeSelection *pSolution = NULL;
   eAISoundConcept newCombatSound=kAISC_TypeMax;
   int i=0;

   while (i<m_EventQueuePtr)
   {
      eAIHtoHCombatEvent ev=m_EventQueue[i++];
      UpdateOppState(ev);
#define kMaxResponseDistSq sq(10.0)
      if (m_lastDistSq>kMaxResponseDistSq)
         continue;      // we only want to react if we are close to the opponent
      switch (ev)
      {
         case kAIHCE_OppAttackStart:
            if (!IsAttacking()&&!m_OppUnarmed&&CheckAptitude(kHigh))
               pSolution=SolveResponse(m_pModeSels->responses.OpponentAttack);
            else
               _HTHWatchInterrupts("cant respond to attack, too bad\n");
            break;
         case kAIHCE_OppAttackEnd:
            if (IsAvoiding())
               _HTHWatchInterrupts("Ha-ha, missed me\n");
            break;
         case kAIHCE_OppBlockStart:
            if (IsAttacking())
            {
               if (CheckVerbosity(kNorm))
                  newCombatSound=kAISC_CombatDetBlock;
               _HTHWatchInterrupts("getting scared?\n");
            }
            break;
         case kAIHCE_TooClose:
            _HTHWatchInterrupts("back off\n");
            if (CanRespond())
               if (m_BackoffCount<2)
                  pSolution=SolveResponse(m_pModeSels->responses.TooCloseToOpponent);
               else
                  pSolution=SolveResponse(m_pModeSels->responses.RemainingTooCloseToOpponent);
            break;
         case kAIHCE_OppBlockEnd:
            if (CanRespond())
               pSolution=SolveResponse(m_pModeSels->responses.Opening);
            else
               _HTHWatchInterrupts("cant respond to block end, too bad\n");
            break;
         case kAIHCE_ImContacted:
            _HTHWatchInterrupts("Ooh! that hurt... not\n");
            break;
         case kAIHCE_TargetContacted:
            _HTHWatchInterrupts("Next time you'll feel it\n");
            break;
         case kAIHCE_Stunned:
            _HTHWatchInterrupts("Arent you tough\n");
            break;
         case kAIHCE_Block:
            if (IsAttacking())          // oops, bummer for us
            {
               if (CheckVerbosity(kHigh)) newCombatSound=kAISC_CombatBlocked;
               _HTHWatchInterrupts("You mock me\n");
            }
            else if (IsBlocking())      // take advantage
            {
               if (CheckVerbosity(kNorm)) newCombatSound=kAISC_CombatSuccBlock;
               _HTHWatchInterrupts("Ha! nice try\n");
            }
            break;
         case kAIHCE_ImDamaged:
            _HTHWatchInterrupts("Ouch");           // if really hurt, worry about it
            if (CanRespond()&&!m_OppUnarmed)
            {
               int my_hp=25, my_max=25;
               ObjGetHitPoints(GetID(),&my_hp); ObjGetMaxHitPoints(GetID(),&my_max);
               if (my_hp*3<my_max) // under 33%
                  pSolution=SolveResponse(m_pModeSels->responses.LowHitpoints);
            }
            break;
         case kAIHCE_TargetDamaged:
            _HTHWatchInterrupts("Ha! hit you");    // press the attack if opponent hurt
            if (CanRespond())
            {
               int it_hp=25, it_max=25;
               ObjGetHitPoints(GetTarget(),&it_hp); ObjGetMaxHitPoints(GetTarget(),&it_max);
               if (it_hp*2<it_max) // under 50%
                  if (CheckAptitude(kHigh))
                     pSolution=SolveResponse(m_pModeSels->responses.Opening);
            }
            break;
         case kAIHCE_Hitting:
            break;
         case kAIHCE_Hit:
            if (CheckVerbosity(kNorm))
               newCombatSound=kAISC_CombatSuccHit;
            break;
      }
   }
   m_EventQueuePtr=0;
   if (newCombatSound!=kAISC_TypeMax)
      PlayCombatSound(newCombatSound,FALSE);
   if (pSolution)
   {
      *newMode = pSolution->mode;
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////

eMode cAIHtoHSubcombat::ChooseNewAttackMode(ObjID target, const cMxsVector & targetLoc, float fZDist)
{
   eMode        newMode;
   sModeSelection *  pSolution = NULL;
   ObjID        weap_obj=GetWeaponObjID(target);
   BOOL         imBehindTarget=FALSE;     // ok, want to add a facing check sort of thing

   ObjPos    *  TargetPos = ObjPosGet(target);
   floatang ang(targetLoc.x,targetLoc.y,m_pAIState->GetLocation()->x,m_pAIState->GetLocation()->y);
   floatang plyfac(TargetPos->fac.tz*2*YA_PI/65536.0);
   float ang_diff=Delta(plyfac,ang).value;
   imBehindTarget=(ang_diff>(YA_PI/1.8));  // in the back quadrant

   //   if (imBehindTarget)
   //      mprintf("imBehindYou (%g player %g ang %g)\n",ang_diff,plyfac.value,ang.value);

   // @TODO: cant be used for ranged stuff yet, since it doesnt deal
#ifdef THIEF
   m_OppUnarmed=((weap_obj==OBJ_NULL)||(!WeaponIsSword(target,weap_obj)));
#else
   m_OppUnarmed = FALSE;
#endif

   // semi-hack. Flip "high" bit if we're above, say, 2.5 ft. @TBD: make this a config setting.
   // Reset bit after.
   if (fZDist > 2.5)
      m_mode = (eMode)(((ulong)m_mode) | kTypeTargetHigh);

   if (m_OppUnarmed || imBehindTarget)
      pSolution = SolveResponse(m_pModeSels->attacks.OpponentUnarmed);
   else if (m_OppAttacking&&CheckAptitude(kHigh))   // perhaps i should dodge or something
      pSolution = SolveResponse(m_pModeSels->attacks.OpponentAttacking);
   else if (m_OppBlocking&&CheckAptitude(kNorm))
      pSolution = SolveResponse(m_pModeSels->attacks.OpponentBlocking);
   else if (m_mode==kCharge)  // for now, want to attack if we just charged, so pretend unarmed
      pSolution = SolveResponse(m_pModeSels->attacks.OpponentUnarmed);
   else if (CanRespond())
      pSolution = SolveResponse(m_pModeSels->attacks.NormalWhileIdle);
   else
      pSolution = SolveResponse(m_pModeSels->attacks.NormalWhileActive);

   // Reset "high" bit.
   m_mode = (eMode)(((ulong)m_mode) & ~kTypeTargetHigh);

   if (pSolution)
      newMode = pSolution->mode;
   else // im here, im near the player (in the range sweet spot)
      newMode = (eMode)(Rand()%(kNumHtoHModes-1)+1);  // skip undecided, shall we

   return newMode;
}

///////////////////////////////////////

float cAIHtoHSubcombat::StatWeightMode(sModeSelection *choice)
{
   eAIRating use_rating=kAIRT_Avg;
   float modifier=1.0;
   if (choice->mode==kIdle)
      use_rating=AIGetSloth(GetID());
   else if (ModeTest(choice->mode,kTypeAttack))
   {
      use_rating=AIGetAggression(GetID());
      if (!ModeTest(m_mode,kTypeAttack))
         if (use_rating>=kAIRT_Avg)
            modifier*=1.2;  // more likely to attack after not-attack

      if (ModeTest(m_mode,kTypeTargetHigh) && ModeTest(choice->mode,kTypeTargetHigh))
         modifier *= 10; // we definitely want to go for a high swing.
   }
   else if (ModeTest(choice->mode,kTypeDefend))
      use_rating=AIGetDefensive(GetID());
   else if (ModeTest(choice->mode,kTypeJitter))
      use_rating=AIGetDodginess(GetID());

   // skip things we never do
   if (use_rating==kAIRT_Null) return 0.0;

   // work on the modifier some
   if (m_mode==choice->mode) modifier*=0.8;  // reduce chance of picking same mode twice
   if ((m_mode&kModeTypeMask)==(choice->mode&kModeTypeMask))
      modifier*=0.8;  // if the new mode is the same type as last mode, bias against
   return 2*RatingFltPercentage(use_rating)*choice->wgt*modifier;
}     // returns 0-1.0, so double, so 1.0 is "basic" value

// hmm, special dont attack unless we are close, somewhere, in this
sModeSelection *cAIHtoHSubcombat::SolveResponse(sModeSelections &selections)
{
   float total=0.0;
   int i, pick;

   sModeSelection *Choices = selections.selections;
   int nChoices = kAIHC_MaxSelections;

   for (i=0; i<nChoices; i++)
      if (Choices[i].mode == kUndefined)
      {
         nChoices = i;
         break;
      }

   if (nChoices==0)
      return NULL;

   for (i=0; i<nChoices; i++)
      total+=StatWeightMode(&Choices[i]);
   if (total<=0)
      return NULL;
   //Having rare problem wherein AI with a low possible number of choices could have a
   //total < 1, and then when you cast it to int and take the mod you crash.  Whee.
   //Putting in a failsafe if < 0.1 and then multiplying by 10 otherwise.
   if (total<0.1)
      pick = 0;
   else
      pick=Rand()%((int)(total*10.0));
   for (total=0.0, i=0; i<nChoices; i++)
   {
      total+=(10.0*StatWeightMode(&Choices[i]));
      if (pick<total)
         break;
   }
   _HTHWatchPrint4("Chose %s (%d/%d) [weighted to %g]\n",_HTHModeName(Choices[i].mode),i,nChoices,StatWeightMode(&Choices[i]));
   return &Choices[i];
}

///////////////////////////////////////

// who knows, etc, etc
#define DONT_TALK_TOO_MUCH 1600

void cAIHtoHSubcombat::PlayCombatSound(eAISoundConcept CombatSound, BOOL always)
{
   if (!always)
      if ( GetSimTime() < overall_last_speech_time + DONT_TALK_TOO_MUCH)
      {
         _HTHWatchPrint("The royal WE talked recently, staying silent\n");
         return;
      }
      else
         _HTHWatchPrint("Im allowed to talk\n");

   if (!m_pAI->AccessSoundEnactor())
      return;

   int my_hp=50, it_hp=50, my_max=50, it_max=50;
   float my_val=1, it_val=1, rating=1;
   cTag balance_tags[]={cTag("ComBal","Losing"),cTag("ComBal","Even"),cTag("ComBal","Winning")};
   cTagSet CombatStateTags;
   int pick=1; // lose even win

   // get Winning - change based on stats?
   ObjGetHitPoints(GetID(),&my_hp);        ObjGetHitPoints(GetTarget(),&it_hp);
   ObjGetMaxHitPoints(GetID(),&my_max);    ObjGetMaxHitPoints(GetTarget(),&it_max);

   if (my_max!=0) my_val=my_hp/(float)my_max;
   if (it_max!=0) it_val=it_hp/(float)it_max;
   if (it_val!=0) rating=my_val/it_val;
   if (rating<0.35) pick=0; else if (rating>2.0) pick=2;

   if (pick==0) if (my_val>0.85) pick=1;
   if (pick==2) if (it_val>0.85) pick=1;
   if (pick==2) if (my_val<0.15) pick=1;
   if (pick==0) if (it_val<0.15) pick=1;

   CombatStateTags.Add(balance_tags[pick]);

   // get Friends - for now, lets skip this

   // actually do the sound
   if (m_pAI->AccessSoundEnactor()->RequestConcept(CombatSound,&CombatStateTags))
   {
      if (CombatSound == kAISC_ReactCharge)
         SetPlayedReactCharge();
      if (!always)
         overall_last_speech_time=GetSimTime();
   }
}

///////////////////////////////////////

#define kTagLoco        "Locomote 0"
#define kTagMelee       "MeleeCombat 0"
#define kTagMeleeLoco   kTagLoco "," kTagMelee
#define kTagDirection   "Direction"
#define kTagMiddle      "Direction 0"
#define kTagLeft        "Direction 1"
#define kTagRight       "Direction 2"
#define kTagForward     "Direction 3"
#define kTagBackward    "Direction 4"
#define kTagHigh        "Direction 5"
#define kTagLow         "Direction 6"
#define kTagFront       "Direction 7"
#define kTagBack        "Direction 8"
#define kTagDirParm(d)  cTag("Direction",d)
#define kTagUrgent      "LocoUrgent 0"
#define kTagStand       "Stand 0"
#define kTagBlock       "Block 0"
#define kTagAttack      "Attack 0"
#define kTagDamaged     "ReceiveWound 0"
#define kTagSevereWound "SevereWound 0"
#define kTagSpecial     "SpecialAttack 0"
#define kTagSearch      "Search 0"
#define kTagScan        "Scan 0"
#define kTagFrustrated  "Discover 0, Thwarted 0"
// "Crumple 0, Die 0"
//

#define MakeTagString1(a)              a
#define MakeTagString2(a, b)           a "," b
#define MakeTagString3(a, b, c)        a "," b "," c
#define MakeTagString4(a, b, c, d)     a "," b "," c "," d
#define MakeTagString5(a, b, c, d, e)  a "," b "," c "," d "," e

extern int g_CollisionObj1;     // holy crap this is a hack
extern int g_CollisionObj2;     // for weapreac...

void cAIHtoHSubcombat::CacheVisualDamageTags(BOOL big_hit)
{
   m_DamageTags.FromString(MakeTagString2(kTagMelee,kTagDamaged));
   int parm_map[4]={7,1,8,2}, idx=0;
   // compute direction somehow
   int i_am_obj1=TRUE;
   if (g_CollisionObj1!=GetID())
      if (g_CollisionObj2==GetID())
         i_am_obj1=FALSE;
      else
         Warning(("AICBHtoH InDamage, but I (%s) not involved\n",ObjWarnName(GetID())));

   mxs_vector pos;
   if (!GetObjLocation((ObjID)(i_am_obj1?g_CollisionObj2:g_CollisionObj1), &pos))
   {
      Warning(("AICBHtoH: Locationless object %s damaged me %s\n",
               ObjWarnName(i_am_obj1?g_CollisionObj2:g_CollisionObj1),
               ObjWarnName(GetID())));
      _HTHWatchPrint1("damage response no direction (locationless obj %s)\n",
                      ObjWarnName(i_am_obj1?g_CollisionObj2:g_CollisionObj1));
      return;
   }

   floatang direction = m_pAIState->AngleTo(pos);
   float val = direction.value - m_pAIState->GetFacingAng().value;
   if (val<0) val+=2*YA_PI;
   //   val+=(5*YA_PI/16)+0.05; // fudge val
   val/=(YA_PI/2);
   if ((val>0)&&(val<4)) idx=(int)val;
   _HTHWatchPrint3("damage response direction leads to %g from %g -> %g\n",val,direction.value,direction.value-m_pAIState->GetFacingAng().value);
   m_DamageTags.Add(kTagDirParm(parm_map[idx]));
   if (big_hit)
      m_DamageTags.Add(cTag("SevereWound",0));  // cant seem to add this as a string
}

///////////////////////////////////////

#define kFrustration 0x110598

cAISeqAction * cAIHtoHSubcombat::CreateFrustrationAction(ObjID target, const mxs_vector & targetLoc)
{
   BOOL firstRecent = FALSE;
   if (m_FrustrationTimer.Expired() && m_pAI->AccessSoundEnactor())
   {
      m_FrustrationTimer.Reset();
      m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_OutOfReach);
      firstRecent = TRUE;
   }

   cAISeqAction *    pSeqAction = new cAISeqAction(this, kFrustration);

   cAIMotionAction * pMotionAction,
                   * pRightMotionAction,
                   * pLeftMotionAction;

   if (firstRecent || AIRandom(1, 100) < 30)
   {
      pMotionAction = CreateMotionAction();
      pMotionAction->AddTags(cTagSet(kTagFrustrated));
      pSeqAction->Add(pMotionAction);
      pMotionAction->Release();
   }

   floatang direction = m_pAIState->AngleTo(targetLoc);
   if (AIRandom(1, 100) < 10)
   {

      pMotionAction = CreateMotionAction();
      pMotionAction->AddTags(MakeTagString2(kTagStand, kTagMelee));
      pMotionAction->SetFacing(direction);
      pMotionAction->SetFocus(target);

      pLeftMotionAction = CreateMotionAction();
      pLeftMotionAction->AddTags(cTagSet(MakeTagString2(kTagMeleeLoco, kTagLeft)));
      pLeftMotionAction->SetFacing(direction);
      pLeftMotionAction->SetFocus(target);

      pRightMotionAction = CreateMotionAction();
      pRightMotionAction->AddTags(cTagSet(MakeTagString2(kTagMeleeLoco, kTagRight)));
      pRightMotionAction->SetFacing(direction);
      pRightMotionAction->SetFocus(target);

      if (AIRandom(1, 100) < 50)
      {
         pSeqAction->Add(pLeftMotionAction);
         pSeqAction->Add(pMotionAction);
         pSeqAction->Add(pRightMotionAction);
      }
      else
      {
         pSeqAction->Add(pRightMotionAction);
         pSeqAction->Add(pMotionAction);
         pSeqAction->Add(pLeftMotionAction);
      }

      pLeftMotionAction->Release();
      pRightMotionAction->Release();
      pMotionAction->Release();
   }
   else
   {
      for (int i = 0; i < 2; i++)
      {
         pMotionAction = CreateMotionAction();
         pMotionAction->AddTags(MakeTagString2(kTagStand, kTagMelee));
         pMotionAction->SetFacing(direction);
         pMotionAction->SetFocus(target);
         pSeqAction->Add(pMotionAction);
         pMotionAction->Release();
      }
   }

   return pSeqAction;
}


// We want to attack player, but we can't get at him. What do we do now? Possibly any of the following:
//
//    - Hide (?)
//    - Shake fist
//    - Run to get nearest long-range AI
//    - Flee (only if long-range weapon is brandished)
//
// @TBD: We may want to move this out into a script call, so we don't do this special "human" stuff here.
//       Or, we may want to move this out into the flee behavior, or its own thing.
//
cAIAction *cAIHtoHSubcombat::DoFrustration(const cAIActions & previous)
{
   int nCurrentTime = GetSimTime();

   // First, try to get help. Might fail if no one nearby, or someone is already getting help there.
   // Path (flee?) to object that is a long-range human combatant AI, if *we* are human, too.

#ifdef THIEF
   // If can't get help, then try to flee. Might fail to flee if no flee points around.
   // Ideally, we'd like to ask the flee ability directly if it wan't to flee.

#if 0 // wsf: do we really need this? :
   if ((nCurrentTime-m_lastFleeTime) > 5000)
#endif
      {
      m_lastFleeTime=nCurrentTime;
      // This will set us up so any frustration listeners will respond. Specifically, the flee response.
      if (IsBowArrowKnocked()) // We're wielding a bow that is equipped. Does this reall work? Does it really reset after bow is put away?
         {
         sAIFrustrated temp;
         temp.nSourceId = GetTargetInfo()->id;
         temp.nDestId = 0;
         temp.pPath = 0;
         g_pAIFrustratedProperty->Set(GetID(), &temp);
         }
      else // try to find nearby long-range humanoid combatant if we are not one, and flee to that person.
         {
// @TBD: Consider moving all this into AIFlee, via property notification or something.
#if 0 // wsf: naw. Just do it.
         if (RandFloat() < 0.5) // play the odds. Give a little time to shake fist.
#endif
            {
            int i;
            cDynZeroArray<int> UsedZones[kAIZone_Num];
            tAIPathZone cellZone[kAIZone_Num];
            IAIPath *pPath = 0;
            AutoAppIPtr(ObjectSystem);
            AutoAppIPtr(TraitManager);

            ObjID Human = pObjectSystem->GetObjectNamed("human");

            for (i = 0; i < kAIZone_Num; i++)
               UsedZones[i].SetSize(g_AIPathDB.m_ZoneDatabases[i].m_nZones);

            // We are humanoid and have no long-range combat ability
            if (!g_pRangedCombatProp->IsRelevant(GetID()) && pTraitManager->ObjHasDonor(GetID(), Human))
               {
               // Find nearest long-range human combatant
               ObjID FoundId = 0;
               ObjID FoundHelper = 0;
               sPropertyObjIter Iter;
               mxs_vector *pPos;
               mxs_vector *pOurPos = ObjPosGetLocVector(GetID());
               float fDist2;

               static const float fMaxRadiusTest2 = 70*70;
               static const float fMinRadiusTest2 = 20*20;

               // wsf: Be careful here. Don't let this get too slow.

               // @TBD: maybe make a limiter on this, so that only 'n' pathfinds are attempted,
               // then it gives up. Yoda forsees many disturbances if this is attempted, though.

               //
               // We store failed zones, so that objects in that zone don't attempt to
               // be pathfound to.
               //

               g_pRangedCombatProp->IterStart(&Iter);

               while (g_pRangedCombatProp->IterNext(&Iter, &FoundId))
                  {
                  if (OBJ_IS_CONCRETE(FoundId) &&
                      pTraitManager->ObjHasDonor(FoundId, Human) &&
                      !AIFleeIsCurrentDest(FoundId) &&
                      (AITeamCompare(GetID(), FoundId) == kAI_Teammates))
                     { // Here's one. Now, is it close enough?

                     // If there's a zone, of any flavor, that hasn't been visited yet, then let's try it.
                     for (i = 0; i < kAIZone_Num; i++)
                        cellZone[i] = g_AIPathDB.GetCellZone((eAIPathZoneType)i, FoundId);

                     for (i = 0; i < kAIZone_Num; i++)
                        if ((cellZone[i] == AI_ZONE_SOLO) || !UsedZones[i][cellZone[i]])
                           break;

                     if (i == kAIZone_Num)
                        continue;

                     pPos = ObjPosGetLocVector(FoundId);
                     // First, do braindead radius check, then if that passes, do a pathcast to see if we can
                     // get there in short order. We pick the first AI that passes the test.
                     fDist2 = mx_dist2_vec(pPos, pOurPos);
                     if ((fDist2 > fMinRadiusTest2) && (fDist2 < fMaxRadiusTest2))
                        {
                        tAIPathCellID startCell;
                        mxs_vector Pos;
                        tAIPathCellID hintCell = 0;
                        uint nSwimBits = 0;
                        IInternalAI *pAI;
                        eAIAwareLevel nAlertness = kAIAL_Lowest;

                        // If we can swim, but found AI can't, then, don't swim.
                        AutoAppIPtr(AIManager);
                        pAI = (IInternalAI *)pAIManager->GetAI(FoundId);
                        if (pAI)
                        {
                           nSwimBits = pAI->GetState()->GetStressBits() & kAIOK_Swim;
                           nAlertness = pAI->GetState()->GetAlertness();
                           SafeRelease(pAI);
                        }

                        // If found AI is already himself at alert 3 then let's skip him.
                        // Ideally, we'd like to see if found AI is already in combat via some kind of 'combat' link
                        // or 'mode'.
                        if (nAlertness == kAIAL_High)
                           continue;

                        startCell = m_pAIState->GetPathCell();

                        // This is kind of hacky. We expect, eventually, for the cAILocoAction to get a
                        // hold of this path, so we set it up in term it would want to if it were the one finding it.
                        //
                        // We share the path at all simply for performance reasons.
                        //
                        Pos = *pPos;
                        cAILocoAction::StrokeDest(&Pos, &hintCell, FoundId);

                        if ((pPath = AIPathfind(*pOurPos, startCell, Pos, 0, (m_pAIState->GetStressBits() & nSwimBits) |kAIOK_Walk|kAIOKCOND_Stressed, m_pAI->AccessPathfinder()->AccessControl())) != 0)
                           {
                           FoundHelper = FoundId;
                           break;
                           }
                        else // mark zone. not AI_ZONE_SOLO, though.
                           {
                           for (i = 0; i < kAIZone_Num; i++)
                              if (cellZone[i] != AI_ZONE_SOLO)
                                 UsedZones[i][cellZone[i]] = 1;
                           }
                        }
                     }
                  }
               g_pRangedCombatProp->IterStop(&Iter);

               if (FoundHelper) // there is a nearby long-range human combatant
                  {
                  sAIFrustrated temp;
                  temp.nSourceId = GetTargetInfo()->id;
                  temp.nDestId = FoundHelper;
                  temp.pPath = pPath;
                  g_pAIFrustratedProperty->Set(GetID(), &temp);
                  }
               }
            }
         }
//      return 0;
      }
#endif

// Always do fist shaking frustration as fallback:
   cAIAction * pAction = NULL;

   if (previous.Size() &&
       previous[0]->InProgress() &&
       previous[0]->GetType() == kAIAT_Sequence &&
       previous[0]->Access()->ownerData == kFrustration)
   {
      pAction = (cAISeqAction *)previous[0];
      pAction->AddRef();
   }
   else
   {
      pAction = CreateFrustrationAction(GetTargetInfo()->id, GetTargetInfo()->loc);
      SignalFrustration();
   }
   _HTHWatchPrint("Target not attackable. Getting frustrated\n");
   return pAction;
}



///////////////////////////////////////

HRESULT cAIHtoHSubcombat::SuggestActionsForMode(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   eAISoundConcept newCombatSound=kAISC_TypeMax;
   cAIAction * pAction = NULL;
   BOOL pathFail=FALSE;
   BOOL bDidFrustration = FALSE;

   // HACK HACK HACK
   if (m_mode==kBackoff||m_mode==kJumpback)
   {  // this is really horrible, since i shouldnt be mutating this here
      if (GetTargetInfo()->flags & kAITF_Behind)
      {
         _HTHWatchPrint("Enemy behind me, AdvanceBig instead of back/jump\n");
         m_mode=kAdvanceBig;
      }
      else
      {
         m_BackoffCount++;
         mxs_vector backoffSpot;
         float backang=m_pAIState->GetFacingAng().value;
         backoffSpot.x=-4.9*cos(backang);  // which is which   - use hip ang point dist thing
         backoffSpot.y=-4.9*sin(backang);  // what sign is what
         backoffSpot.z=0;
         mx_addeq_vec(&backoffSpot,m_pAIState->GetLocation());
         if (!m_pAI->AccessPathfinder()->Pathcast(backoffSpot))
         {  // this could get a little bit more intelligent, really, like just solveresponse something
            sModeSelection *pSolution=NULL;
            m_mode=kSwingQuick;   // so we bias away from attack
            if (m_BackoffFail>2)
               pSolution=SolveResponse(m_pModeSels->responses.Cornered);
            else
               pSolution=SolveResponse(m_pModeSels->responses.FailedBackup);
            _HTHWatchPrint1("Cant Pathcast back, switched to %s\n",_HTHModeName(pSolution->mode));
            m_mode=pSolution->mode;
            m_BackoffCount=0;
            pathFail=TRUE;
         }
      }
   }
   else
      m_BackoffCount=0;
   if (pathFail)
      m_BackoffFail++;
   else
      m_BackoffFail=0;

   // HACK HACK HACK
   if ((m_mode!=kCharge)&&(m_mode!=kChargeUnseen))
      m_lastChargeTime=0; // so when we next charge, we do

   switch (m_mode)
   {
      //
      // Motion actions
      //
      case kCircleCW:
      case kCircleCCW:
      case kBackoff:
      case kJumpback:
      case kAdvanceBig:
      case kAdvance:
      case kDodge:
      case kIdle:
      case kSwingQuick:
      case kSwingNormal:
      case kSwingMassive:
      case kSwingSpecial:
      case kBlock:
      case kDirectedBlock:
      {
         // @TBD (toml 05-25-98): after E3, clean this up so specific actions are gotten from behavior set
         cTagSet tags;
         switch (m_mode)
         {
            case kCircleCW:
               tags.FromString(MakeTagString2(kTagMeleeLoco, kTagLeft));
               break;

            case kCircleCCW:
               tags.FromString(MakeTagString2(kTagMeleeLoco, kTagRight));
               break;

            // for now, backoff=jumpback for retreat and retreatfast
            case kBackoff:                       // kAIMT_RetreatFast
            case kJumpback:                      // kAIMT_RetreatFast
               tags.FromString(MakeTagString2(kTagMeleeLoco, kTagBackward));
               break;

            case kAdvance:
               tags.FromString(MakeTagString2(kTagMeleeLoco, kTagForward));
               break;

            case kAdvanceBig:
               tags.FromString(MakeTagString3(kTagMeleeLoco, kTagForward, kTagUrgent));
               break;

            case kDodge:
               if (AIRandom(0, 1))
                  tags.FromString(MakeTagString3(kTagMeleeLoco, kTagLeft, kTagUrgent));
               else
                  tags.FromString(MakeTagString3(kTagMeleeLoco, kTagRight, kTagUrgent));
               break;

            case kIdle:
               tags.FromString(MakeTagString2(kTagStand, kTagMelee));
               SetNotifications(kAICN_BeginFrame);  // |kAICN_ActionProgress);
               break;

            case kSwingQuick:
               tags.FromString(MakeTagString3(kTagMelee, kTagAttack, kTagLow));
               if (CheckVerbosity(kNorm)) newCombatSound=kAISC_CombatAttack;
               WeaponEvent(kStartAttack, m_pAIState->GetID(), GetTargetInfo()->id, kWeaponEventSmall);
               break;

            case kSwingNormal:
               tags.FromString(MakeTagString3(kTagMelee, kTagAttack, kTagMiddle));
               if (CheckVerbosity(kHigh)) newCombatSound=kAISC_CombatAttack;
               WeaponEvent(kStartAttack, m_pAIState->GetID(), GetTargetInfo()->id, kWeaponEventMedium);
               break;

            case kSwingMassive:
               tags.FromString(MakeTagString3(kTagMelee, kTagAttack, kTagHigh));
               newCombatSound=kAISC_CombatAttack;
               WeaponEvent(kStartAttack, m_pAIState->GetID(), GetTargetInfo()->id, kWeaponEventLarge);
               break;

            case kSwingSpecial:
               tags.FromString(MakeTagString3(kTagMelee, kTagAttack, kTagSpecial));
               newCombatSound=kAISC_CombatAttack;
               WeaponEvent(kStartAttack, m_pAIState->GetID(), GetTargetInfo()->id, kWeaponEventLarge);
               break;

            case kBlock:
               tags.FromString(MakeTagString2(kTagMelee, kTagBlock));
               SetNotifications(kAICN_GameEvent);
               WeaponEvent(kStartBlock, m_pAIState->GetID(), GetTargetInfo()->id);
               break;

            case kDirectedBlock:
               if (m_DoDirectionBlock==-1)
               {
                  Warning(("In Directed Block, but Direction -1"));
                  m_DoDirectionBlock=kMTV_right;
               }
               tags.FromString(MakeTagString2(kTagMelee, kTagBlock));
               tags.Add(kTagDirParm(m_DoDirectionBlock));
               m_DoDirectionBlock=-1;
               WeaponEvent(kStartBlock, m_pAIState->GetID(), GetTargetInfo()->id);
               break;
         }

         if (tags.Size())
         {
            pAction = CreateMotionAction();
            pAction->AddTags(tags);
            // all these want to face my opponent
            floatang direction = m_pAIState->AngleTo(GetTargetInfo()->loc);
            ((cAIMotionAction *)pAction)->SetFacing(direction);
            ((cAIMotionAction *)pAction)->SetFocus(GetTargetInfo()->id);
         }
         break;
      }

#if 0 // wsf: removed, and moved to "charge" case. We still want to try pathing.
      //
      // Target is way too high for us. Get frustrated.
      // wsf: in the near future, this will mean shaking fist, fleeing, hiding, or calling long range combatant.
      //
      case kFrustrate:
      {
         pAction = DoFrustration(previous);
         bDidFrustration = TRUE;
         break;
      }
#endif

      //
      // Locomotion actions
      //
      case kChargeUnseen:
      {
         cMxsVector myLoc;
         GetObjLocation(GetID(), &myLoc);
         // @TODO: bug if zdist too big... really should know if seen...
         if (mx_dist2_vec(&myLoc,&GetTargetInfo()->loc)<2.5*2.5)
         {  // if we are already at GetTargetInfo()->loc, search instead
            if (previous.Size() &&
                previous[0]->InProgress() &&
                previous[0]->GetType() == kAIAT_Motion &&
                previous[0]->Access()->ownerData == kChargeUnseen)
            {
               pAction = (cAIMotionAction *)previous[0];
               pAction->AddRef();
            }
            else
            {
               cTagSet tags(MakeTagString2(kTagSearch, kTagScan));
               pAction = CreateMotionAction(kChargeUnseen);
               pAction->AddTags(tags);
            }
            break;
         }
         // ELSE FALL THROUGH TO KCHARGE
         _HTHWatchPrint("ChargeUnseen falling through to normal charge\n");
      }

      case kCharge:
      case kFrustrate:
      {
         #define kReactChargeDistSq sq(17.5)
         if (GetTargetInfo()->distSq > kReactChargeDistSq && CanPlayReactCharge())
            newCombatSound=kAISC_ReactCharge;
         cAILocoAction * pLocoAction = NULL;
         if (previous.Size() &&
             previous[0]->InProgress() &&
             previous[0]->GetType() == kAIAT_Locomote &&
             mx_dist2_vec(&GetTargetInfo()->loc, &((cAILocoAction *)previous[0])->GetLogicalDest()) < sq(1.0))
         {
            pLocoAction = (cAILocoAction *)previous[0];
            pLocoAction->AddRef();
         }
         else
         {
#define kDeltaDist2RePath (4.0*4.0)
#define kDeltaTimeRePath  (3000)

            // am i even allowed to do this again?
            float delta_dist2=mx_dist2_vec(&GetTargetInfo()->loc,&m_lastChargePos);
            int   delta_time=GetSimTime()-m_failedChargePathTime;

            if ((delta_dist2>kDeltaDist2RePath)||
                (delta_time>kDeltaTimeRePath))
            {
               switch (m_mode)
               {
                  case kFrustrate:
                     // Special (read HACKY) way aimultcb.cpp tells us that target is climbing, so we should just
                     // get frustrated. Else, do normal charging behavior.
                     if (GetTargetInfo()->fStrikeRange > 0.01)
                     {
                        pAction = DoFrustration(previous);
                        newCombatSound=kAISC_TypeMax; // The FrustrationAction took care of this already.
                        bDidFrustration = TRUE;
                        m_failedChargePathTime = GetSimTime();
                        break;
                     }
                  case kCharge:
                  case kChargeUnseen:
                     pLocoAction = CreateLocoAction();
                     pLocoAction->Set(GetTargetInfo()->loc, GetTargetInfo()->id, kAIS_Fast, sq(GetRanges()->Huge));
                     if (!pLocoAction->Pathfind(TRUE, GetTargetInfo()->fStrikeRange))
                     {
                        // set a "gosh, we have failed"
                        SafeRelease(pLocoAction);
                        newCombatSound=kAISC_TypeMax; // The FrustrationAction took care of this already.
                        pAction = DoFrustration(previous);
                        bDidFrustration = TRUE;
                        m_failedChargePathTime = GetSimTime();
                     }
                     else
                        _HTHWatchPrint("Succesful pathfind\n");
                     // update charge state
                     m_lastChargePos=GetTargetInfo()->loc;
                     m_lastChargeTime=GetSimTime();
                     break;
               }

            // We're going to signal Flee that we're fleeing to a friend:
            if (g_pAIFrustratedProperty->IsRelevant(GetID()))
               newCombatSound=kAISC_ReactFindFriend;
            }
            else
            {
               _HTHWatchPrint2("Punted pathfind due to time/space %g %d\n",delta_dist2,delta_time);
               if (previous.Size() && previous[0]->InProgress())
               {
                  pAction = (cAIAction *)previous[0];
                  pAction->AddRef();
               }
               else
               {
                  floatang direction = m_pAIState->AngleTo(GetTargetInfo()->loc);
                  cAIMotionAction * pMotionAction = CreateMotionAction();
                  if (m_pAI->IsNetworkProxy())  //!@HACK: This looks better in shock multiplayer.
                     pMotionAction->AddTags(MakeTagString2(kTagSearch, kTagScan));
                  else
                     pMotionAction->AddTags(MakeTagString2(kTagStand, kTagMelee));
                  pMotionAction->SetFacing(direction);
                  pMotionAction->SetFocus(GetTargetInfo()->id);
                  pAction = pMotionAction;
               }
            }
         }
         if (pLocoAction)
         {
            if (m_pAI->AccessSenses() && IsAPlayer(GetTargetInfo()->id))
            {
               float distKeepFreshSense = pLocoAction->ComputePathDist()  - g_H2HNoFreshenRange;

               if (distKeepFreshSense > 0.0)
                  m_pAI->AccessSenses()->KeepFresh(GetTargetInfo()->id, (distKeepFreshSense / g_H2HSpeed) * 1000.0);
            }
            pAction = pLocoAction;
         }
         break;
      }

      case kTakeDamage:
      {
         pAction = CreateMotionAction();
         pAction->AddTags(m_DamageTags);
         m_TakeDamage=FALSE;
         break;
      }

      //
      // Raw move actions
      //
      case kAvoid:   // run away! - WHAT IS THIS!!
      case kNoMove:
         switch (m_mode)
         {
            case kAvoid:   // run away! - WHAT IS THIS!!
   #if 0
               tacticalGoalPt = GetTargetInfo()->loc;
               desiredFacing=kAIF_SpecificDir;
               specFacing= m_pAI->ComputeAngleFromMeToPoint(GetTargetInfo()->loc);
               directionToGoal = floatang(PI) + specFacing;
               baseSpeed = 50;
               haveSuggestion=TRUE;
   #endif
               break;

            case kNoMove:
   #if 0
               directionToGoal = m_pAI->ComputeAngleFromMeToPoint(GetTargetInfo()->loc);
               baseSpeed = 0;
               desiredFacing=kAIF_SpecificDir;
               specFacing=directionToGoal;
               haveSuggestion=TRUE;
   #endif
               break;
         }
         break;


      //
      // No actions
      //

      case kUndecided:
         Warning(("Hey, we called SetMove stuff while undecided!\n"));
#if 0
         m_specificAction=FALSE;
#endif
         break;

      default:
#if 0
         m_specificAction=FALSE;
#endif
         CriticalMsg("Bad attack mode");
         break;
   }

   if (newCombatSound!=kAISC_TypeMax)
      PlayCombatSound(newCombatSound,FALSE);

   if (!pAction && !bDidFrustration)
   {
      Warning(("Hand to hand wants to \"%s\", but I'll just run at you.\n", _HTHModeName(m_mode)));
      return cAISubcombat::SuggestActions(pGoal, previous, pNew);
   }

   if (pAction)
      pNew->Append(pAction);
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cAIHtoHSubcombat::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAISubcombat::OnGoalChange(pPrevious, pGoal);
}


///////////////////////////////////////

STDMETHODIMP cAIHtoHSubcombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   AUTO_TIMER(cAIHtoHSubcombat_SuggestActions);

   eMode newMode = kUndecided;

   // Check if an event has caused an ability to want a forced interrupt
   if (!CheckModeInterrupt(&newMode))
   {
      // ... otherwise, if already executing an action, continue that action
      if (previous.Size() && previous[0]->InProgress())
         return ContinueActions(previous,pNew);
   }

   // If no interrupt...
   if (newMode == kUndecided)
   {
      // the last action completed and some subability needs to be chosen to provide a new one
      newMode = SelectMode();
   }

   AIWatchTrue(newMode == kUndecided, Cbt, GetID(), "Suggest Moves: Never Decided What to do!");
   if (newMode != m_mode)
      AIWatch2(Cbt, GetID(), "%s %s", "Going to", _HTHModeName(newMode));
   else
      AIWatch2(Cbt, GetID(), "%s %s", "Staying in", _HTHModeName(m_mode));

   // Track time of current ability control
   if (m_mode != newMode)
   {
      m_mode = newMode;
      m_ModeDuration = 0;
   }

   // @TBD (toml 02-19-99): I'm a bit unclear of the purpose of following...
   ClearNotifications(kAICN_GameEvent|kAICN_BeginFrame);

   // NOTE: this can change mode away from Backup/Jumpback if it wants
   HRESULT result = SuggestActionsForMode(pGoal, previous, pNew);

   return S_OK;
}

///////////////////////////////////////


#if 1

sHtoHModeSelections cAIHtoHSubcombat::gm_DefaultModeSelections =
{
   //////////////////////////
   //
   // Responses
   //
   {
      // OpponentAttack
      {
         0,                                      // no flags
         {
            {kSwingQuick, 3},
            {kSwingNormal, 1},
            {kBlock, 9},
            {kBackoff, 6},
            {kUndecided, 7},  // dont respond at all
         }
      },

      // LowHitpoints
      {
         0,                                      // no flags
         {
            {kSwingQuick, 1},
            {kBlock, 6},
            {kBackoff, 3},
            {kDodge, 6},
            {kUndecided, 7},  // dont respond at all
         }
      },

      // Opening
      {
         0,                                      // no flags
         {
            {kSwingQuick, 2},
            {kSwingNormal, 4},
            {kSwingMassive, 6},
            {kSwingSpecial, 1},
            {kUndecided, 8},  // dont respond at all
         }
      },

      // TooCloseToOpponent
      {
         0,                                      // no flags
         {
            {kJumpback, 1}
         }
      },

      // RemainingTooCloseToOpponent
      {
         0,                                      // no flags
         {
            {kSwingQuick, 3},
            {kCircleCCW, 1},
            {kCircleCW, 1},
            {kDodge, 1},
         }
      },

      // FailedBackup
      {
         0,                                      // no flags
         {
            {kSwingQuick, 2},
            {kSwingNormal, 3},
            {kCircleCCW, 1},
            {kCircleCW, 1},
            {kBlock, 1},
            {kAdvance, 1},
         }
      },

      // Cornered
      {
         0,                                      // no flags
         {
            {kSwingNormal, 2},
            {kSwingMassive, 2},
            {kAdvance, 3},
            {kBlock, 2},
            {kIdle, 1},
         }
      },
   },

   //////////////////////////
   //
   // Attacks
   //
   {
      // NormalWhileIdle
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    4},
            {kSwingMassive,   6},
            {kSwingSpecial,   1},
            {kBlock,          1},
            {kCircleCW,       1},
            {kCircleCCW,      1},
         }
      },

      // NormalWhileActive
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    2},
            {kSwingMassive,   2},
            {kIdle,           8},
            {kBlock,          1},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentAttacking
      {
         0,                                      // no flags
         {
            {kSwingQuick,     1},
            {kSwingNormal,    1},
            {kBlock,          8},
            {kDodge,          8},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentBlocking
      {
         0,                                      // no flags
         {
            {kSwingMassive,   2},
            {kDodge,          2},
            {kIdle,           2},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentUnarmed
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    4},
            {kSwingMassive,   8},
            {kSwingSpecial,   2},
         }
      }
   }
};


#endif
