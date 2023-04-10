/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicamera.cpp,v 1.13 2000/03/24 13:35:46 adurant Exp $
//
// AI Combat - cameras
//

#include <aicamera.h>

#include <comtools.h>
#include <appagg.h>
#include <cfgdbg.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <linkman.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <autolink.h>

#include <playrobj.h>

#include <scrptapi.h>

#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aigoal.h>
#include <aiprcore.h>
#include <aisndtyp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

IRelation * g_pAICameraLinks;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICamera
//

cAICamera::cAICamera(void)
{
   SetTargetAlertnessThreshold(kAIAL_Moderate);
}

///////////////////////////////////////

cAICamera::~cAICamera()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAICamera::GetName()
{
   return "Camera";
}

///////////////////////////////////////

STDMETHODIMP cAICamera::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   if (m_pAIState->GetAlertness()>=kAIAL_Moderate)
      cAIDevice::SuggestGoal(pPrevious, ppNew);
   
   if (*ppNew == NULL)
   {
      cAIInvestigateGoal *pGoal = new cAIInvestigateGoal(this);
      pGoal->priority = kAIP_Low;
      *ppNew = pGoal;
      SignalAction();
   }
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAICamera::OnAlertness(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAIDevice::OnAlertness(source, previous,  current, pRaw);

   if (!InControl() || !m_pAI->AccessSoundEnactor())
      return;

   eAISoundConceptEnum concept = kAISC_CoreNum;
   
   switch (current)
   {
      case kAIAL_Lowest:
         concept = kAISC_AlertDownToZero;
         break;

      case kAIAL_Low:
         if (previous < kAIAL_Low)
            concept = kAISC_AlertToOne;
         break;

      case kAIAL_Moderate:
         if (previous < kAIAL_Moderate)
            concept = kAISC_AlertToTwo;
         break;

      case kAIAL_High:
         concept = kAISC_AlertToThree;
         BroadcastSwitches();
         break;
   }
   
   if (concept != kAISC_CoreNum)
   {
      m_pAI->AccessSoundEnactor()->HaltCurrent();
      m_pAI->AccessSoundEnactor()->RequestConcept(concept);
   }
}


///////////////////////////////////////

STDMETHODIMP_(void) cAICamera::OnModeChange(eAIMode previous, eAIMode mode) // Inherits notification bit from base ability
{

   //get cameras to stop making noise when efficent AMSD
   if (mode <= kAIM_Efficient)
   {   
      IInternalAI *pIAI = (IInternalAI *)AccessOuterAI();
      cAIJointScanAction *pScanAction;
      pScanAction = (cAIJointScanAction *)pIAI->GetTheActionOfType(kAIAT_JointScan);
      if (pScanAction) //oops, almost forgot.
         pScanAction->End();  //die die die.
   }
}


///////////////////////////////////////

STDMETHODIMP cAICamera::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   // We only have suggestions if we are investigating
   if (pGoal->GetType() != kAIGT_Investigate)
      return cAIDevice::SuggestActions(pGoal, previous, pNew);
   else if (previous.Size() > 0)
   {
      cAIJointScanAction *pScanAction;
      if (previous.Size() > 1)
         return cAIDevice::SuggestActions(pGoal,previous,pNew);
      //so now we know exactly one action.
      //if scanning already, keep on keeping on
      pScanAction = new cAIJointScanAction(this);
      if (previous[0]->GetType() == pScanAction->GetType())
         ContinueActions(previous, pNew);
      return cAIDevice::SuggestActions(pGoal,previous,pNew);
      //ok, if we have EXACTLY 1 previous action, of type scan,
      //keep it going.  Else ask our mommy (aidevice) for help
      //on what to do next. AMSD
   }
   else if (m_pAIState->GetAlertness()>=kAIAL_Moderate) //next bit keeps cameras from "scanning" when red/yellow.
      return cAIDevice::SuggestActions(pGoal, previous, pNew);
   else
   {
      // Start a Scan scan
      sAIDeviceParams *pDeviceParams;
      sAICameraParams *pCameraParams;
      cAIJointScanAction *pScanAction;

      pDeviceParams = AIGetDeviceParams(m_pAIState->GetID());
      pCameraParams = AIGetCameraParams(m_pAIState->GetID());
      pScanAction = new cAIJointScanAction(this);
      pScanAction->Set(pDeviceParams->m_jointRotate, 
         DEGREES(pCameraParams->m_scanAngle1), 
         DEGREES(pCameraParams->m_scanAngle2), 
         DEGREES(pCameraParams->m_scanSpeed), 
         DEGREES(-pCameraParams->m_scanSpeed));
      pNew->Append(pScanAction);
   }
   return S_OK;
}

///////////////////////////////////////

void cAICamera::BroadcastSwitches(void)
{
   AutoAppIPtr(ScriptMan);
   cAutoLinkQuery query(g_pAICameraLinks, m_pAIState->GetID()); 
  
   while (!query->Done())
   {
      sLink slink;
      sAICameraLinkData *pData = (sAICameraLinkData*)(query->Data());
      
      query->Link(&slink);
      if (pData != NULL)
         pScriptMan->PostMessage2(m_pAIState->GetID(), slink.dest, pData->m_msg, 
            pData->m_data[0], pData->m_data[1], pData->m_data[2]);
      else
         pScriptMan->PostMessage2(m_pAIState->GetID(), slink.dest, NULL, NULL, NULL, NULL);
      query->Next();
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

static sFieldDesc g_AICameraFieldDesc[] =
{
   { "Msg", kFieldTypeString, FieldLocation(sAICameraLinkData, m_msg), kFieldFlagNone,},
   { "Data1", kFieldTypeString, FieldLocation(sAICameraLinkData, m_data[0]), kFieldFlagNone,},
   { "Data2", kFieldTypeString, FieldLocation(sAICameraLinkData, m_data[1]), kFieldFlagNone,},
   { "Data3", kFieldTypeString, FieldLocation(sAICameraLinkData, m_data[2]), kFieldFlagNone,},
};

static sStructDesc g_AICameraStructDesc = StructDescBuild(sAICameraLinkData, kStructFlagNone, g_AICameraFieldDesc);

///////////////////////////////////////

BOOL AIInitCameraAbility(IAIManager *)
{
   StructDescRegister(&g_AICameraStructDesc);

   sRelationDesc cameraLinkDesc = {"AICamera", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc cameraLinkDataDesc = { "sAICameraLinkData", sizeof(sAICameraLinkData), kRelationDataAutoCreate};

   g_pAICameraLinks = CreateStandardRelation(&cameraLinkDesc, &cameraLinkDataDesc, (kQCaseSetSourceKnown));
   return TRUE;
}

///////////////////////////////////////

BOOL AITermCameraAbility(void)
{
   SafeRelease(g_pAICameraLinks);
   return TRUE;
}

