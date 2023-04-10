/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/puppet.cpp,v 1.12 1998/07/27 13:12:21 CCAROLLO Exp $

#include <puppet.h>
#include <pupprop.h>
#include <propbase.h>
#include <property.h>
#include <mprintf.h>
#include <creatapi.h>
#include <osysbase.h>
#include <motdesc.h>

// script stuff
#include <comtools.h>
#include <appagg.h>
#include <scrptapi.h> 
#include <scrptbas.h>
#include <scrptsrv.h>
#include <bodscrpt.h>

// Include this last
#include <dbmem.h>


static IPuppetProperty *g_pPuppetProperty=NULL;	

void PuppetsInit()
{
   g_pPuppetProperty=PuppetPropertyInit();
   Assert_(g_pPuppetProperty);
}

BOOL ObjIsPuppet(ObjID obj)
{
   cPuppet* pPuppet;
   return g_pPuppetProperty->Get(obj,&pPuppet);
}

ICerebellum *PuppetGetCerebellum(ObjID obj)
{
   cPuppet* pPuppet;

   if(!g_pPuppetProperty)
      return NULL;
   if(g_pPuppetProperty->Get(obj,&pPuppet))
   {
      return pPuppet; 
   }
   return NULL;
}

EXTERN void LGAPI PuppetCreatureListener(sPropertyListenMsg* msg, PropListenerData data)
{
   cPuppet* pPuppet; 

   if(msg->type&(kListenPropSet|kListenPropUnset|kListenPropModify))
   {
      if(OBJ_IS_CONCRETE(msg->obj)&&g_pPuppetProperty->Get(msg->obj,&pPuppet))
         if(pPuppet)
         {
            pPuppet->ResetMotor();
         }
   }
}


cPuppet::cPuppet(ObjID objID)
{
   m_pMCoord = MotSysCreateMotionCoordinator();
   m_pMnvrRequest=NULL;
   SetOwner(objID);
}

cPuppet::cPuppet(const cPuppet& )
{
   m_pMCoord = MotSysCreateMotionCoordinator();
   m_pMnvrRequest=NULL;
   SetOwner(OBJ_NULL);
}

cPuppet::~cPuppet()
{
   if(m_pMCoord)
      delete m_pMCoord;
   if(m_pMnvrRequest)
      delete m_pMnvrRequest;
}

void cPuppet::SetOwner(ObjID objID)
{
   m_ObjID=objID;
   ResetMotor();
}

void cPuppet::ResetMotor()
{
   if(m_pMCoord)
      m_pMCoord->SetMotor(CreatureGetMotorInterface(m_ObjID));  // this is null if not a creature
}

void cPuppet::SetManeuverRequest(sPuppetMnvrRequest *pRequest)
{
   if(m_pMnvrRequest)
      delete m_pMnvrRequest;
   m_pMnvrRequest=pRequest;
}

// XXX puppets ignore priority for now @TODO: make them use it KJ 1/98
void cPuppet::PlayMotion(const char *name, eManeuverTransitionType trans, eManeuverPriority priority)
{
   static cTagSet SpecMotion("PlaySpecMotion");

   sPuppetMnvrRequest *pRequest= new sPuppetMnvrRequest;

   pRequest->trans=trans;
   pRequest->params.tags = SpecMotion;
   pRequest->params.motionNum=MotDescNameGetNum((char *)name);
   pRequest->params.mask=kMotParmFlag_MotNum;
   if(pRequest->params.motionNum>0) // motion is okay
   {
      SetManeuverRequest(pRequest);
   } else // tell script service motion is done
   {
      AutoAppIPtr(ScriptMan);
      sBodyMsg bodyMessage(m_ObjID,sBodyMsg::kMotionEnd,name,NULL);
      pScriptMan->SendMessage(&bodyMessage);
   }
}

void cPuppet::Update(ulong dt)
{
   AssertMsg1(m_pMCoord,"No motion coordinator for puppet %d\n",m_ObjID);

   sPuppetMnvrRequest *pRequest=GetManeuverRequest();

   if(!pRequest)
      return;

   const sMcMoveState *state=m_pMCoord->GetNextEndState();
   IMotionPlan *pPlan;

   if(pRequest->trans==kMnvrTrans_Immediate)
   {
      if(NULL!=(pPlan=m_pMCoord->BuildPlan(state,&pRequest->params)))
      {
         IManeuver *pManeuver = pPlan->PopFirstManeuver();
         m_pMCoord->SetCurrentManeuver(pManeuver);
         delete pPlan;
      } else
      {
         AssertMsg(FALSE,"Unable to build motion plan");
      }
      SetManeuverRequest(NULL);
   } else if(m_pMCoord->GetStatus()==kMCoord_Idle)
   {
      if(NULL!=(pPlan=m_pMCoord->BuildPlan(state,&pRequest->params)))
      {
         IManeuver *pManeuver = pPlan->PopFirstManeuver();
         m_pMCoord->SetNextManeuver(pManeuver);
         delete pPlan;
      } else
      {
         AssertMsg(FALSE,"Unable to build motion plan");
      }
      SetManeuverRequest(NULL);
   }
}
