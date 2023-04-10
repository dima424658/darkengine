/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactps.cpp,v 1.26 2000/02/28 11:29:40 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <mprintf.h>

#include <appagg.h>
#include <frobctrl.h>
#include <iobjsys.h>
#include <linkman.h>
#include <lnkquery.h>
#include <multparm.h>
#include <objlpars.h>
#include <relation.h>
#include <scrptapi.h>
#include <traitman.h>
#include <contain.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactori.h>
#include <aiactps.h>
#include <aiactwt.h>
#include <aiapiiai.h>
#include <aiapisnd.h>
#include <aidebug.h>
#include <aiinvtyp.h>
#include <ainonhst.h>
#include <aiprabil.h>
#include <aisignal.h>
#include <aitagtyp.h>
#include <aiapibhv.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static void AddMetaPropertyToMany(ObjID metaProp, const cDynArray<ObjID> & targets)
{
   AutoAppIPtr(TraitManager);
   if (!pTraitManager->IsMetaProperty(metaProp))
      return;
   for (int i = 0; i < targets.Size(); i++)
      pTraitManager->AddObjMetaProperty(targets[i], metaProp);
}

///////////////////////////////////////

static void RemoveMetaPropertyFromMany(ObjID metaProp, const cDynArray<ObjID> & targets)
{
   AutoAppIPtr(TraitManager);
   if (!pTraitManager->IsMetaProperty(metaProp))
      return;
   for (int i = 0; i < targets.Size(); i++)
   {
#ifdef DEBUG
      if (!pTraitManager->ObjHasDonorIntrinsically(targets[i], metaProp))
         Warning(("Removing metaproperty (%d) from an object (%d) that doesn't have that metaproperty\n", metaProp, targets[i]));
#endif
      pTraitManager->RemoveObjMetaProperty(targets[i], metaProp);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void CreateLinks(IRelation * pRelation, ObjID source, const cDynArray<ObjID> & destinations, void * pData = NULL)
{
   int i;
   if (!pData)
   {
      if (source != LINKOBJ_WILDCARD)
      {
         for (i = 0; i < destinations.Size(); i++)
         {
            if (destinations[i] != LINKOBJ_WILDCARD)
               pRelation->Add(source, destinations[i]);
         }
      }
   }
   else
   {
      if (source != LINKOBJ_WILDCARD)
      {
         for (i = 0; i < destinations.Size(); i++)
         {
            if (destinations[i] != LINKOBJ_WILDCARD)
               pRelation->AddFull(source, destinations[i], pData);
         }
      }
   }
}

///////////////////////////////////////

static void CreateLinks(IRelation * pRelation, const cDynArray<ObjID> & sources, ObjID destination, void * pData = NULL)
{
   int i;
   if (!pData)
   {
      if (destination != LINKOBJ_WILDCARD)
      {
         if (sources[i] != LINKOBJ_WILDCARD)
         {
            for (i = 0; i < sources.Size(); i++)
               pRelation->Add(sources[i], destination);
         }
      }
   }
   else
   {
      if (destination != LINKOBJ_WILDCARD)
      {
         if (sources[i] != LINKOBJ_WILDCARD)
         {
            for (i = 0; i < sources.Size(); i++)
               pRelation->AddFull(sources[i], destination, pData);
         }
      }
   }
}

///////////////////////////////////////

static void CreateLinks(IRelation * pRelation, const cDynArray<ObjID> & sources, const cDynArray<ObjID> & destinations, void * pData = NULL)
{
   for (int i = 0; i < sources.Size(); i++)
      CreateLinks(pRelation, sources[i], destinations, pData);
}

///////////////////////////////////////

static void DestroyLinks(IRelation * pRelation, ObjID source, const cDynArray<ObjID> & destinations)
{
   ILinkQuery * pQuery;
   for (int i = 0; i < destinations.Size(); i++)
   {
      pQuery = pRelation->Query(source, destinations[i]);

      for (; !pQuery->Done(); pQuery->Next())
         pRelation->Remove(pQuery->ID());

      pQuery->Release();
   }
}

///////////////////////////////////////

static void DestroyLinks(IRelation * pRelation, const cDynArray<ObjID> & sources, ObjID destination)
{
   ILinkQuery * pQuery;
   for (int i = 0; i < sources.Size(); i++)
   {
      pQuery = pRelation->Query(sources[i], destination);

      for (; !pQuery->Done(); pQuery->Next())
         pRelation->Remove(pQuery->ID());

      pQuery->Release();
   }
}

///////////////////////////////////////

static void DestroyLinks(IRelation * pRelation, const cDynArray<ObjID> & sources, const cDynArray<ObjID> & destinations)
{
   ILinkQuery * pQuery;
   for (int i = 0; i < sources.Size(); i++)
      for (int j = 0; j < destinations.Size(); j++)
      {
         pQuery = pRelation->Query(sources[i], destinations[j]);

         for (; !pQuery->Done(); pQuery->Next())
            pRelation->Remove(pQuery->ID());

         pQuery->Release();
      }
}

///////////////////////////////////////////////////////////////////////////////

static ObjID ObjFromString(const char * pszObject)
{
   ObjID result = atoi(pszObject);
   if (!result)
   {
      AutoAppIPtr(ObjectSystem);
      result = pObjectSystem->GetObjectNamed(pszObject);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////

const char * g_ppszAIPsdScrActions[kAIPS_Num] =
{
   "Nothing()",
   "Script message(Message,MsgData1,MsgData2)",
   "Play sound/motion(Sound Concept,Sound Tags,Motion Tags)",
   "Alert (unimplemented)",
   "Become hostile()",
   "Enable investigate()",
   "Goto object(Object,Speed,Motion Tags)",
   "Frob object(Target Object,With Object)",
   "Wait(Time(ms),Motion Tags)",
   "Mprint(text)",
   "Add/Remove Meta-property(Add/Remove,Mprop name,Target Objects)",
   "Add link(Flavor,Destination(s),Source(s))",
   "Remove link(Flavor,Destination(s),Source(s))",
   "Face(Angle,Object(-1 for angle relative to self))",
   "Signal(Signal,Target(s),Max Distance)",
   "Dest. Script Message(Message,MsgData1,MsgData2)"
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPsdScrAction
//

cAIPsdScrAction::~cAIPsdScrAction()
{
   SafeRelease(m_pCurrent);
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIPsdScrAction::Update()
{
   // If we've already resolved, we're done...
   if (!InProgress())
      return result;

   // If we're not doing anything, there's nothing to update
   if (!m_pCurrent)
   {
      result = kAIR_NoResultSwitch;
   }
   // Otherwise, check progress...
   else
   {
      result = m_pCurrent->Update();
      if (!AIInProgress(result))
      {
         SafeRelease(m_pCurrent);
         result = (Advance()) ? kAIR_NoResultSwitch : kAIR_Success;
      }
   }

   return result;
}

///////////////////////////////////////

DECLARE_TIMER(cAIPsdScrAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIPsdScrAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIPsdScrAction_Enact);

   if (!Started())
   {
      SetStarted(TRUE);
      if (!StartActions())
      {
         result = kAIR_Success;
         return result;
      }
   }
   else if (!CanEnact())
      return result;


   if (m_pCurrent)
      result = m_pCurrent->Enact(deltaTime);
   else
   {
      // Run all instantaneous actions
      while ((m_pCurrent = HandleCurrent()) == NULL)
      {
         if (!Advance())
         {
            result = kAIR_Success;
            return result;
         }
      }

      // Getting here, we have a non-instantaneous action to deal with
      Assert_(m_pCurrent);
      result = m_pCurrent->Enact(deltaTime);
   }

   if (!AIInProgress(result) && !IsDone())
      result = kAIR_NoResultSwitch;

   return result;
}

///////////////////////////////////////
//
// Either compile to an action or simply execute if its instantaneous
//

cAIAction * cAIPsdScrAction::HandleCurrent()
{
   const sAIPsdScrAct & act = m_actions[m_iCurrent];
   cStr                 tempStr;
   cObjListParser       objListParser;

   AIWatch4(Flow,
            m_pAIState->GetID(),
            "Pseudo-script: %s (\"%s\", \"%s\", \"%s\")",
            g_ppszAIPsdScrActions[act.type],
            (act.GetArg(0)) ? act.GetArg(0) : "",
            (act.GetArg(1)) ? act.GetArg(1) : "",
            (act.GetArg(2)) ? act.GetArg(2) : "");

   switch (act.type)
   {
      case kAIPS_Nothing:
         break;

      case kAIPS_Play:
         if (!m_pAI->AccessSoundEnactor())
            break;
         if (act.GetArg(0))
         {
            cTagSet tags(act.GetArg(1));
            m_pAI->AccessSoundEnactor()->RequestConcept(act.GetArg(0), (act.GetArg(1)) ? &tags : NULL);
         }
         if (act.GetArg(2))
         {
            cAIMotionAction * pMotionAction = CreateMotionAction();
            pMotionAction->AddTags(act.GetArg(2));
            return pMotionAction;
         }
         break;

      case kAIPS_BecomeHostile:
         g_pAINonHostilityProperty->Set(m_pAIState->GetID(), kAINH_Never);
         break;

      case kAIPS_EnableInvestigate:
         g_pAIInvestKindProperty->Set(m_pAIState->GetID(), kAIIK_Always);
         break;

      case kAIPS_Script:
      case kAIPS_DestScript:
         if (act.GetArg(0))
         {
            cMultiParm arg1, arg2, arg3;

            if (act.GetArg(1))
               arg1 = act.GetArg(1);
            if (act.GetArg(2))
               arg2 = act.GetArg(2);
            if (act.GetArg(3))
               arg3 = act.GetArg(3);

            AutoAppIPtr(ScriptMan);
            if (!!pScriptMan)
            {
               cMultiParm reply = pScriptMan->SendMessage2(m_pAIState->GetID(),
                                                           (act.type == kAIPS_Script) ? m_pAIState->GetID() : m_dest,
                                                           act.GetArg(0),
                                                           arg1,
                                                           arg2,
                                                           arg3);
               if (reply.GetType() != kMT_Undef)
               {
                  AssertMsg(reply.GetType() == kMT_Int, "AI psuedo-script message handlers must reply with an integer or bool");
                  if(!((int)reply))
                  {
                     m_iCurrent = m_actions.Size();
                  }
               }
            }
         }
         break;

      case kAIPS_Alert:
         Warning(("Ai pseudo-script action \"%s\" is unimplemented\n", g_ppszAIPsdScrActions[act.type]));
         break;

      case kAIPS_Goto:
      {
         ObjID targObj = OBJ_NULL;
         if (act.GetArg(0))
         {
            cDynArray<ObjID> targets;
            float            curDist, targDist = 999999.0;

            objListParser.Parse(act.GetArg(0), &targets);

            for (int i = 0; i < targets.Size(); i++)
            {
               curDist = m_pAIState->DistSq(*GetObjLocation(targets[i]));
               if (curDist < targDist)
               {
                  targObj = targets[i];
                  targDist = curDist;
               }
            }
         }
         else
            targObj = m_dest;

         if (targObj)
         {
            eAISpeed speed = kAIS_Normal;
            if (act.GetArg(1))
            {
               tempStr = act.GetArg(1);
               tempStr.Trim();
               speed = AIGetSpeedFromName(tempStr);
            }

            cAILocoAction * pLocoAction = CreateLocoAction();
            pLocoAction->Set(targObj, speed);

            if (act.GetArg(2))
            {
               pLocoAction->AddTags(act.GetArg(1));
            }

            return pLocoAction;
         }
         else
            Warning(("AI pseudo-script has invalid goto target \"%s\"\n", act.GetArg(0)));

         break;
      }

      case kAIPS_Frob:
      {
         if (act.GetArg(0))
         {
            ObjID targObj = OBJ_NULL;
            ObjID withObj = OBJ_NULL;

            tempStr = act.GetArg(0);
            tempStr.Trim();

            targObj = ObjFromString(tempStr);
            if (act.GetArg(1))
            {
               tempStr = act.GetArg(1);
               tempStr.Trim();
               withObj = ObjFromString(tempStr);
            }
            else
               withObj = OBJ_NULL;

            sFrobActivate frob_info;
            frob_info.frobber = m_pAIState->GetID();
            frob_info.ms_down = 1; // since we dont have time data...

            if (withObj == OBJ_NULL)
            {
               AutoAppIPtr_(ContainSys,pContainSys);
               if (pContainSys->Contains(m_pAIState->GetID(),targObj))
                  frob_info.src_loc=kFrobLocInv;
               else
                  frob_info.src_loc=kFrobLocWorld;
               frob_info.src_obj = targObj;
               frob_info.dst_loc = kFrobLocNone;
               frob_info.dst_obj = OBJ_NULL;
            }
            else
            {
               frob_info.src_loc = kFrobLocTool;
               frob_info.src_obj = withObj;
               frob_info.dst_loc = kFrobLocWorld;
               frob_info.dst_obj = targObj;
            }

            FrobInstantExecute(&frob_info);
         }
         break;
      }

      case kAIPS_Wait:
      {
         if (act.GetArg(0))
         {
            cAIWaitAction * pWaitAction = CreateWaitAction();
            pWaitAction->Set(atoi(act.GetArg(0)));
            pWaitAction->AddTags(act.GetArg(1));
            return pWaitAction;
         }

         break;
      }

      case kAIPS_Mprint:
      {
#ifndef SHIP
         if (act.GetArg(0))
            mprintf("%s\n", act.GetArg(0));
#endif
         break;
      }

      case kAIPS_MetaProperty:
      {
         if (act.GetArg(0) && act.GetArg(1))
         {
            ObjID            metaProp;
            cDynArray<ObjID> targets;

            tempStr = act.GetArg(1);
            tempStr.Trim();
            metaProp = ObjFromString(tempStr);

            if (metaProp == OBJ_NULL)
            {
               Warning(("Unknown metaproperty %s\n", act.GetArg(1)));
               break;
            }

            if (act.GetArg(2))
               objListParser.Parse(act.GetArg(2), &targets);
            else
               targets.Append(m_pAIState->GetID());

            if (!targets.Size())
               break;

            tempStr = act.GetArg(0);
            tempStr.Trim();

            if (tempStr.CompareNoCase("add") == 0)
            {
               AddMetaPropertyToMany(metaProp, targets);
            }
            else if (tempStr.CompareNoCase("remove") == 0)
            {
               RemoveMetaPropertyFromMany(metaProp, targets);
            }
            else
               Warning(("Unknown meta-property action \"%s\"\n", act.GetArg(0)));

         }
         break;
      }

      case kAIPS_AddLink:
      case kAIPS_RemoveLink:
      {
         if (!act.GetArg(0))
            break;

         cDynArray<ObjID> sources, destinations;

         if (act.GetArg(1))
         {
            if (strcmp(act.GetArg(1), "*") == 0)
               destinations.Append(LINKOBJ_WILDCARD);
            else
               objListParser.Parse(act.GetArg(1), &destinations);
         }
         else
            destinations.Append(m_pAIState->GetID());

         if (!destinations.Size())
         {
            Warning(("Cannot create links using \"%s\"\n", act.GetArg(1)));
            break;
         }

         if (act.GetArg(2))
         {
            if (strcmp(act.GetArg(2), "*") == 0)
               sources.Append(LINKOBJ_WILDCARD);
            else
               objListParser.Parse(act.GetArg(2), &sources);

            if (!sources.Size())
            {
               Warning(("Cannot create links using \"%s\"\n", act.GetArg(2)));
               break;
            }
         }
         else
            sources.Append(m_pAIState->GetID());

         tempStr = act.GetArg(0);
         tempStr.Trim();

         AutoAppIPtr(LinkManager);
         IRelation * pRelation = pLinkManager->GetRelationNamed(tempStr);

         if (!pRelation)
         {
            Warning(("No relation named \"%s\"\n", act.GetArg(0)));
            break;
         }

         switch (act.type)
         {
            case kAIPS_AddLink:
            {
               CreateLinks(pRelation, sources, destinations);
               break;
            }

            case kAIPS_RemoveLink:
            {
               DestroyLinks(pRelation, sources, destinations);
               break;
            }
         }

         pRelation->Release();

         break;
      }

      case kAIPS_Face:
      {
         if (act.GetArg(0) || (act.GetArg(1) || m_dest))
         {
            cAIOrientAction * pOrientAction = CreateOrientAction();
            BOOL fAngle = !!act.GetArg(0);
            ObjID targObj = OBJ_NULL;

            if (act.GetArg(1) || m_dest)
            {
               if (act.GetArg(1))
               {
                  cDynArray<ObjID> targets;
                  float            curDist, targDist = 999999.0;

                  objListParser.Parse(act.GetArg(1), &targets);

                  for (int i = 0; i < targets.Size(); i++)
                  {
                     if (targets[i] == -1) //special.       
                        curDist = 0; //self
                     else
                        curDist = m_pAIState->DistSq(*GetObjLocation(targets[i]));
                     if (curDist < targDist)
                     {
                        targObj = targets[i];
                        targDist = curDist;
                     }
                  }
               }
               else
                  targObj = m_dest;
            }

            if (fAngle)
            {
               float angle = (float)atof(act.GetArg(0));
               if (act.GetArg(1) && targObj)
               {
                  // Face relative to object
  
                  if (targObj == -1) //special, face offset.	    
                     pOrientAction->Set(floatang(angle + m_pAIState->GetFacingAng()));
                  else
                     pOrientAction->Set(angle, targObj);
               }
               else
               {
                  // Absolute facing
                  pOrientAction->Set(floatang(angle));
               }
            }
            else if (targObj)
            {
               // Face object
               if (targObj == -1) //special face the same way we already are.	 
                  pOrientAction->Set(floatang(m_pAIState->GetFacingAng()));       
               else 
                  pOrientAction->Set(targObj);
            }
            return pOrientAction;
         }
         break;
      }

      case kAIPS_Signal:
      {
         if (act.GetArg(0) && act.GetArg(1))
         {
            cDynArray<ObjID> targets;
            BOOL             fTestDist;
            float            limitSq;

            objListParser.Parse(act.GetArg(1), &targets);

            if (!targets.Size())
               break;

            if (act.GetArg(2))
            {
               limitSq = (float)atof(act.GetArg(2));
               limitSq *= limitSq;
               fTestDist = TRUE;
            }
            else
               fTestDist = FALSE;

            AutoAppIPtr(AIManager);
            IInternalAI * pTarget;
            sAISignal     signal;
            mxs_vector    targetPos;

            signal.name = act.GetArg(0);
            for (int i = 0; i < targets.Size(); i++)
            {
               if (fTestDist &&
                   GetObjLocation(targets[i], &targetPos) &&
                    m_pAIState->DistSq(targetPos) > limitSq)
                  continue;

               pTarget = (IInternalAI *)pAIManager->GetAI(targets[i]);

               if (!pTarget)
                  continue;

               pTarget->NotifySignal(&signal);
               pTarget->Release();
            }
         }

         break;
      }

      default:
         CriticalMsg1("Unknown AI pseudo-script action %d", act.type);
   }
   return NULL;
}

///////////////////////////////////////

BOOL cAIPsdScrAction::Advance()
{
   do
   {
      m_iCurrent++;
   } while (m_iCurrent < m_actions.Size() && m_actions[m_iCurrent].type == kAIPS_Nothing);

   return !IsDone();
}

///////////////////////////////////////

BOOL cAIPsdScrAction::StartActions()
{
   m_iCurrent--;
   return Advance();
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPsdScrAction::Save(ITagFile * pTagFile)
{
   size_t versionSize = sizeof(*this);  // Rough version checking.
   AITagMove(pTagFile, &versionSize);
   SaveActionBase(pTagFile);
   AITagMove(pTagFile, &m_iCurrent);
   AITagMoveDynarray(pTagFile, &m_actions); //!@TBD: is this good enough?
   AITagMove(pTagFile, &m_source);
   AITagMove(pTagFile, &m_dest);
   BOOL hasCurrentAction = !!m_pCurrent;
   AITagMove(pTagFile, &hasCurrentAction);
   if (hasCurrentAction)
      m_pAI->AccessBehaviorSet()->SaveAction(pTagFile, m_pCurrent);
   return TRUE;
}

///////////////////////////////////////


STDMETHODIMP_(BOOL) cAIPsdScrAction::Load(ITagFile * pTagFile)
{
   size_t versionSize;   // Rough version checking.
   AITagMove(pTagFile, &versionSize);
   AssertMsg(versionSize == sizeof(*this), "Invalid saved pseudo action encountered");
   LoadActionBase(pTagFile);
   AITagMove(pTagFile, &m_iCurrent);
   AITagMoveDynarray(pTagFile, &m_actions);
   AITagMove(pTagFile, &m_source);
   AITagMove(pTagFile, &m_dest);
   BOOL hasCurrentAction;
   AITagMove(pTagFile, &hasCurrentAction);
   if (hasCurrentAction)
      m_pCurrent = m_pAI->AccessBehaviorSet()->LoadAndCreateAction(pTagFile, pOwner);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

