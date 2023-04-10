/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgmodel.cpp,v 1.19 2000/02/05 19:37:52 adurant Exp $

#include <appagg.h>
#include <dmgmodl_.h>
#include <iobjsys.h>

#include <propface.h>
#include <propbase.h>

#include <scrptapi.h>
#include <damgscrm.h>

#include <chevkind.h>

#include <sdestool.h>
#include <sdesbase.h>

//
// INCLUDE THESE ABSOLUTELY LAST
//
#include <initguid.h>
#include <dmgguid.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Death Stage Property
//

static sPropertyDesc DeathDesc = 
{
   PROP_DEATH_STAGE_NAME,
   kPropertyNoInherit,
   NULL, 0, 0, 
   { "Game: Damage Model", "Death Stage" },
};

#define DEATH_STAGE_IMPL kPropertyImplHash 


////////////////////////////////////////////////////////////
// cBaseDamageModel implementation
//

//------------------------------------------------------------
// Construction/deconstruction
//

F_DECLARE_INTERFACE(IPropertyManager);

static struct sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_IPropertyManager }, 
   { kNullConstraint, },
};


cBaseDamageModel::cBaseDamageModel(IUnknown* pOuter, sRelativeConstraint* constraints)
   : death_stage(NULL), mpScriptMan(NULL)
{
   if (constraints == NULL) constraints = Constraints; 
   MI_INIT_AGGREGATION_1(pOuter,IDamageModel,kPriorityNormal,constraints);
};

////////////////////////////////////////

cBaseDamageModel::~cBaseDamageModel()
{
}


//------------------------------------------------------------
// AGG PROTOCOL
//

STDMETHODIMP cBaseDamageModel::Init()
{
   death_stage = CreateIntProperty(&DeathDesc,DEATH_STAGE_IMPL);
   mpScriptMan = AppGetObj(IScriptMan); 
   mpObjSys = AppGetObj(IObjectSystem); 
   return S_OK;
}

STDMETHODIMP cBaseDamageModel::End()
{
   SafeRelease(death_stage);
   SafeRelease(mpScriptMan); 
   SafeRelease(mpObjSys); 
   return S_OK;
}

//------------------------------------------------------------
// Internal 
//

#define DONT_RESEND_THESE (kDamageMsgSlay|kDamageMsgTerminate)

BOOL cBaseDamageModel::AlreadySentMsg(ulong msg, ObjID obj)
{
   if (msg & DONT_RESEND_THESE)
   {
      Assert_(death_stage);
      int sent = 0; 
      death_stage->Get(obj,&sent);

      if (sent & msg)
         return TRUE; 

   }
   return FALSE; 
}

eDamageResult cBaseDamageModel::SendMessage(const sDamageMsg* msg)
{
   eDamageResult result = kDamageNoOpinion;

   if (msg->victim == OBJ_NULL)
   {
      //      Warning(("Message victim is NULL obj\n"));
      return result;
   }
   
   if (msg->kind & DONT_RESEND_THESE)
   {
      Assert_(death_stage);
      int sent = 0; 
      death_stage->Get(msg->victim,&sent);

      if (sent & msg->kind)
         return kDamageStatusQuo;

      // set the death stage now, to avoid recursion and other bad stuff.
      death_stage->Set(msg->victim,sent|msg->kind);
   }

   for (int i = 0; i < mListeners.Size(); i++)
   {
      sListener& client = mListeners[i];
      if (client.interests & msg->kind)
      {
         eDamageResult opinion = client.func(msg,client.data);
         if (opinion > result) 
            result = opinion;
      }
   }

   SendScriptMessage(msg); 

   return result;
}

////////////////////////////////////////

#define SCRIPT_MESSAGES (kDamageMsgDamage|kDamageMsgSlay)

void cBaseDamageModel::SendScriptMessage(const sDamageMsg* msg)
{
   if (mpScriptMan == NULL)
      return;

   if (msg->kind & kDamageMsgDamage)
   {
         sDamageScrMsg* scrmsg = new sDamageScrMsg(msg->victim,msg->culprit, msg->data.damage->kind,msg->data.damage->amount);
         mpScriptMan->SendMessage(scrmsg);
         scrmsg->Release();
   }

   if (msg->kind &  kDamageMsgSlay)
   {
      sSlayMsg* scrmsg = new sSlayMsg(msg->victim,msg->culprit,msg->data.slay);
      mpScriptMan->SendMessage(scrmsg); 
      scrmsg->Release(); 
   }
}

////////////////////////////////////////

eDamageResult cBaseDamageModel::ApplyResult(ObjID victim, ObjID culprit, eDamageResult result, sChainedEvent* cause)
{
   Assert_(result < kNumDamageResults);
   switch (result)
   {
      case kDamageSlay:
         result = SlayObject(victim,culprit,cause);
         break;

      case kDamageTerminate:
         result = TerminateObject(victim,cause);
         break;

      case kDamageResurrect:
         result = ResurrectObject(victim,culprit,cause);
         break; 

      case kDamageDestroy:
         mpObjSys->Destroy(victim);
         break;

      default: 
         break;
   }
   return result;
}



////////////////////////////////////////

eDamageResult cBaseDamageModel::ApplyFilters(ObjID victim, ObjID culprit, sDamage* damage)
{  
   eDamageResult result = kDamageNoOpinion;

   for (int i = 0; i < mFilters.Size(); i++)
   {
      eDamageResult opinion = mFilters[i].func(victim,culprit,damage,mFilters[i].data);
      if (opinion > result)
         result = opinion;
   }
   return result;
}

//------------------------------------------------------------
// IDamageModel Methods
//

STDMETHODIMP_(eDamageResult) cBaseDamageModel::HandleImpact(ObjID victim, ObjID culprit, sImpact* impact, sChainedEvent* cause)
{  
   sDamageMsgData msgdata = { kDamageMsgImpact, victim, culprit, impact}; 
   sDamageMsg msg(kEventKindImpact,&msgdata,cause); 

   eDamageResult result = SendMessage(&msg);
   result = ApplyResult(victim,culprit,result,&msg);
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cBaseDamageModel::DamageObject(ObjID victim, ObjID culprit, sDamage* damage, sChainedEvent* cause, BOOL allowzero)
{  
   eDamageResult result = ApplyFilters(victim,culprit,damage);
   
   sDamageMsgData msgdata = { kDamageMsgDamage, victim, culprit, damage}; 
   sDamageMsg msg(kEventKindDamage,&msgdata,cause); 
   
   eDamageResult opinion = SendMessage(&msg);
   if (opinion > result)
      result = opinion;

   result = ApplyResult(victim,culprit,result,&msg);
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cBaseDamageModel::SlayObject(ObjID victim, ObjID culprit,sChainedEvent* cause)
{
   tDamageKind kind = 0; 

   // grovel through history looking for damage
   if (cause)
   {
      sDamageMsg* msg = (sDamageMsg*)cause->Find(kEventKindDamage);
      if (msg)
         kind = msg->data.damage->kind; 
   }

   sDamageMsgData msgdata = { kDamageMsgSlay, victim, culprit, (void*)kind}; 
   sDamageMsg msg(kEventKindSlay,&msgdata,cause);
   
   eDamageResult result = SendMessage(&msg);
   if (result > kDamageSlay)
      result = ApplyResult(victim,culprit,result,&msg);   
   else
      result = kDamageSlay;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cBaseDamageModel::TerminateObject(ObjID victim, sChainedEvent* cause)
{
   sDamageMsgData msgdata = { kDamageMsgTerminate, victim, OBJ_NULL, NULL}; 
   sDamageMsg msg(kEventKindTerminate,&msgdata,cause); 
   
   eDamageResult result = SendMessage(&msg);
   if (result > kDamageTerminate)
      result = ApplyResult(victim,OBJ_NULL,result,&msg);   
   else
      result = kDamageTerminate;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cBaseDamageModel::ResurrectObject(ObjID victim, ObjID culprit,sChainedEvent* cause)
{
   death_stage->Delete(victim); 

   sDamageMsgData msgdata = { kDamageMsgResurrect, victim, culprit,}; 
   sDamageMsg msg(kEventKindResurrect,&msgdata,cause);
   
   eDamageResult result = SendMessage(&msg);
   if (result > kDamageResurrect)
      result = ApplyResult(victim,culprit,result,&msg);   
   else
      result = kDamageResurrect;
   return result;
}

////////////////////////////////////////

STDMETHODIMP cBaseDamageModel::Filter(tDamageFilterFunc func, tDamageCallbackData data)
{
   sFilter filter = { func,data};
   mFilters.Append(filter);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cBaseDamageModel::Listen(tDamageMsgMask interests, tDamageListenerFunc func, tDamageCallbackData data)
{
   sListener listener = { interests, func,data};
   mListeners.Append(listener);
   return S_OK;
}


////////////////////////////////////////

void InitEmptyDamageModel(void)
{
   AutoAppIPtr(Unknown);
   new cBaseDamageModel(pUnknown);
}




