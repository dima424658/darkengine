/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/simpdmg.cpp,v 1.30 2000/02/05 19:37:27 adurant Exp $

#ifndef __SIMPDMG_H
#define __SIMPDMG_H

#include <simpdmg_.h>
#include <objhp.h>
#include <dmgbase.h>
#include <collprop.h>
#include <dmgprop.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <lnkquery.h>
#include <osetlnkq.h>
#include <propbase.h>

#include <traitman.h>
#include <traitbas.h>

#include <iobjsys.h>
#include <objpos.h>
#include <osystype.h>
#include <osysbase.h>
#include <wrtype.h>
#include <flinder.h>

#include <config.h>
#include <cfgdbg.h>

#include <chevkind.h>

#include <playrobj.h>

#include <netman.h>
#include <iobjnet.h>

#include <stimprop.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cSimpleDamageModel implementation
//

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif 

F_DECLARE_INTERFACE(IPropertyManager); 
F_DECLARE_INTERFACE(ILinkManager); 

static struct sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_IPropertyManager }, 
   { kConstrainAfter, &IID_ILinkManager }, 

   { kNullConstraint, },
};

struct sFlinder
{
   int   count;
   float impulse;
   
   BOOL       scatter;
   mxs_vector offset;
};

//------------------------------------------------------------
// Construction/destruction
//

cSimpleDamageModel* cSimpleDamageModel::TheDamageModel = NULL;

cSimpleDamageModel::cSimpleDamageModel(IUnknown* pOuter)
   : cBaseDamageModel(pOuter,Constraints),mpCorpses(NULL),mpFlinders(NULL)
{
   TheDamageModel = this; 
}

cSimpleDamageModel::~cSimpleDamageModel()
{

}

//------------------------------------------------------------
// IDamageModel Methods
//

STDMETHODIMP_(eDamageResult) cSimpleDamageModel::HandleImpact(ObjID victim, ObjID culprit, sImpact* impact, sChainedEvent* cause)
{
   // @TBD: this is a tad crude, so we should think about how to handle
   // it better. Basically, we need to make sure that we don't spuriously
   // damage avatars, because that damage can propagate back to the
   // player. Players are responsible for their own damage. It would be
   // nice to handle this with fewer side-effects, though.
   // @NOTE: Removed 10/19/99, since Thief actively doesn't want this
   // and Shock now deals with it at a better level...
   //
   //if (IsAPlayer(victim) && !(victim == PlayerObject())) {
   //   return kDamageNoOpinion;
   //}

   sDamageMsgData msgdata = { kDamageMsgImpact, victim, culprit, impact}; 
   sDamageMsg msg(kEventKindImpact,&msgdata,cause);

   eDamageResult result = SendMessage(&msg);
   
   if (result > kDamageSlay)
      return ApplyResult(victim,culprit,result,&msg);

   if (culprit != OBJ_NULL && victim != OBJ_NULL && ObjHasWeaponDamage(culprit))
   {
      sDamage damage;
      int kind = kDamageKindImpact;

      damage.amount = ObjGetWeaponDamage(culprit);
      ObjGetWeaponType(culprit,&kind);
      damage.kind = kind;

      eDamageResult opinion = DamageObject(victim,culprit,&damage,&msg);
      result = max(opinion,result);
   }
   
   return result; 
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cSimpleDamageModel::DamageObject(ObjID victim, ObjID culprit, sDamage* damage, sChainedEvent* cause, BOOL allowzero)
{  
   eDamageResult result = ApplyFilters(victim,culprit,damage);

   // don't take zero damage
   if ((damage->amount == 0) && (!allowzero))
      return kDamageStatusQuo; 

   // don't damage dead people 
   if (AlreadySentMsg(kDamageMsgSlay,victim))
      return kDamageStatusQuo; 

   int hp;
   if (ObjGetHitPoints(victim,&hp))
   {
      hp -= damage->amount;
      if (hp <= 0)
         result = max(result,kDamageSlay);
      ObjSetHitPoints(victim,hp);
   }
   
   sDamageMsgData msgdata = { kDamageMsgDamage, victim, culprit, damage}; 
   sDamageMsg msg(kEventKindDamage,&msgdata,cause);

   ConfigSpew("damage_spew",("%d was damaged by %d with %d of kind %d\n",victim,culprit,damage->amount,damage->kind)); 
   
   eDamageResult opinion = SendMessage(&msg);
   result = max(opinion,result);

   result = ApplyResult(victim,culprit,result,&msg);
   return result;
}

////////////////////////////////////////


static eDamageResult slay_result_map[] = 
{ kDamageNoOpinion, kDamageStatusQuo, kDamageTerminate, kDamageDestroy }; 

STDMETHODIMP_(eDamageResult) cSimpleDamageModel::SlayObject(ObjID victim, ObjID culprit, sChainedEvent* cause)
{
#ifdef NEW_NETWORK_ENABLED
   // If we're in multiplayer, don't kill things we don't own:
   AutoAppIPtr(ObjectNetworking);
   if (pObjectNetworking->ObjIsProxy(victim))
      return kDamageNoOpinion;
#endif

   tDamageKind kind = 0; 
   // grovel through history looking for damage
   if (cause)
   {
      sDamageMsg* msg = (sDamageMsg*)cause->Find(kEventKindDamage);
      if (msg)
         kind = msg->data.damage->kind; 
   }

   sDamageMsgData msgdata = { kDamageMsgSlay, victim, culprit, (void*)kind }; 
   sDamageMsg msg(kEventKindSlay,&msgdata,cause); 
   
   eDamageResult result = SendMessage(&msg);
   if (result == kDamageNoOpinion)
      result = slay_result_map[ObjGetSlayResult(victim)]; 
   if (result == kDamageNoOpinion && victim != PlayerObject())
      result = kDamageTerminate; 
   if (result > kDamageSlay)
   {
      sDamage dmg = { 0, kind }; 
      result = ApplyResult(victim,culprit,result,&msg);   
   }
   else
      result = kDamageSlay;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cSimpleDamageModel::TerminateObject(ObjID victim, sChainedEvent* cause)
{
   sDamageMsgData msgdata = { kDamageMsgTerminate, victim, OBJ_NULL, NULL}; 
   sDamageMsg msg(kEventKindTerminate,&msgdata,cause);
   
   eDamageResult result = SendMessage(&msg);
   
   if (result <= kDamageTerminate)
   {
      
      if (result == kDamageNoOpinion)
         result = kDamageDestroy; 
      else
         result = kDamageTerminate;

#ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(NetManager);
      AutoAppIPtr(ObjectNetworking);
      BOOL isLocalOnly = pObjectNetworking->ObjLocalOnly(victim);
#endif

      cCorpseIter corpseIter(victim, mpCorpses);
      while (!corpseIter.Finished())
      {
         result = kDamageDestroy; 

         // Replace the victim with a corpse
         ObjPos pos = *ObjPosGet(victim);

         // Actually instantiate the corpse
#ifdef NEW_NETWORK_ENABLED
         // If the victim was a local-only object, then the corpse should
         // be local-only as well. By suspending messaging, we tell object
         // networking to do this.
         if (isLocalOnly)
         {
            pNetManager->SuspendMessaging();
         }
#endif
         ObjID corpse = mpObjSys->BeginCreate(corpseIter.Get(),kObjectConcrete); 
#ifdef NEW_NETWORK_ENABLED
         if (isLocalOnly)
         {
            pNetManager->ResumeMessaging();
         }
#endif
         ObjPosUpdate(corpse,&pos.loc.vec,&pos.fac);           // position it correctly 

         PropagateCulpability(victim,corpse,kCulpTransitive); 

         if (BOOL(corpseIter.GetData()))
         {
            float scale;
            if (g_pSourceScaleProperty->Get(victim, &scale))
               g_pSourceScaleProperty->Set(corpse, scale);
         }

         mpObjSys->EndCreate(corpse); 

         corpseIter.Next();
      }

      cCorpseIter flinderIter(victim, mpFlinders);
      if (!flinderIter.Finished())
         result = kDamageDestroy;

      while (!flinderIter.Finished())
      {
         sFlinder *pFlinder = (sFlinder *)flinderIter.GetData();
         CreateFlinders(victim, flinderIter.Get(), pFlinder->count, pFlinder->scatter, pFlinder->impulse, pFlinder->offset);

         flinderIter.Next();
      }
   }

   if (result > kDamageTerminate)
      result = ApplyResult(victim,OBJ_NULL,result,&msg);   

   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eDamageResult) cSimpleDamageModel::ResurrectObject(ObjID victim, ObjID culprit, sChainedEvent* cause)
{
   int hp; 
   ObjGetHitPoints(victim,&hp); 
   int maxhp = hp; 
   ObjGetMaxHitPoints(victim,&maxhp); 
   if (maxhp <= 0) // can't resurrect a guy who's max hitpoints is zero or fewer
      return kDamageStatusQuo; 

   if (hp < maxhp)
      ObjSetHitPoints(victim,maxhp); 

   return cBaseDamageModel::ResurrectObject(victim,culprit,cause); 
}

////////////////////////////////////////

STDMETHODIMP cSimpleDamageModel::Init()
{
   cBaseDamageModel::Init(); 
   InitDamageProps(); 
   mpTraitMan = AppGetObj(ITraitManager); 
   CreateCorpseRelation();
   CreateFlinderizeRelation();
   CreateCulpability(); 
   return S_OK; 
}


STDMETHODIMP cSimpleDamageModel::End()
{
   SafeRelease(mpTraitMan); 
   TermDamageProps(); 
   cBaseDamageModel::End(); 
   SafeRelease(mpCorpses); 
   SafeRelease(mpCulpable);
   SafeRelease(mpCulpableFor);

   return S_OK; 
}

////////////////////////////////////////
//
// Corpse Relation
//
static sRelationDesc corpse_rdesc = 
{
   "Corpse",
}; 

static sRelationDataDesc corpse_ddesc = {"Propagate Source Scale?", sizeof(BOOL),}; 

static sFieldDesc corpse_fdesc = 
   { "Propagate Source Scale?", kFieldTypeBool, sizeof(BOOL), 0, 0 };
static sStructDesc corpse_sdesc = 
   { "Propagate Source Scale?", sizeof(BOOL), kStructFlagNone, 1, &corpse_fdesc};

void cSimpleDamageModel::CreateCorpseRelation()
{
   mpCorpses = CreateStandardRelation(&corpse_rdesc,&corpse_ddesc,kQCaseSetSourceKnown|kQCaseSetBothKnown); 
   StructDescRegister(&corpse_sdesc);
}

////////////////////////////////////////
//
// Fliderize Relation
//
static sRelationDesc flinderize_rdesc = 
{
   "Flinderize",
}; 

static sRelationDataDesc flinderize_ddesc = LINK_DATA_DESC_FLAGS(sFlinder, kRelationDataAutoCreate);

static sFieldDesc flinderize_data_fields[] = 
{
   {"Count",    kFieldTypeInt,    FieldLocation(sFlinder, count),   kFieldFlagNone },
   {"Impulse",  kFieldTypeFloat,  FieldLocation(sFlinder, impulse), kFieldFlagNone },
   {"Scatter?", kFieldTypeBool,   FieldLocation(sFlinder, scatter), kFieldFlagNone },
   {"Offset",   kFieldTypeVector, FieldLocation(sFlinder, offset),  kFieldFlagNone },
};

static sStructDesc flinderize_sdesc = StructDescBuild(sFlinder, kStructFlagNone, flinderize_data_fields);

void cSimpleDamageModel::CreateFlinderizeRelation()
{
   mpFlinders = CreateStandardRelation(&flinderize_rdesc, &flinderize_ddesc, kQCaseSetSourceKnown);
   StructDescRegister(&flinderize_sdesc);
}

//////////////////////////////////////////////////////////////
//
// Culpability 
// 

void cSimpleDamageModel::PropagateCulpability(ObjID from, ObjID to, ulong flags)
{
   // can't be culpable for an archetype
   if (OBJ_IS_ABSTRACT(to))
   {
      ConfigSpew("culpable_spew",("Trying to spread culpability from %d to %d\n",from,to)); 
      return ; 
   }

   ObjID src = OBJ_NULL; 
   BOOL culpable = FALSE; 

   // if the "to" object can be culpable, then culpability doesn't propagate
   // to it. 
   if (mpCulpable->Get(to,&culpable) && culpable)
      return ; 

   culpable = FALSE; 
   mpCulpable->Get(from,&culpable); 

   // If I can be culpable, link straight to me
   if (culpable)
      src = from; 
   else if (flags & kCulpTransitive)
   {
      // is anyone culpable for me?
      LinkID culplink = mpCulpableFor->GetSingleLink(LINKOBJ_WILDCARD,from); 
      if (culplink != NULL)
      {
         sLink link; 
         mpCulpableFor->Get(culplink,&link); 
         src = link.source; 
      }
   }

   // add a new link 
   if (src != OBJ_NULL)
   {
      // remove any old link that might exist
      LinkID link = mpCulpableFor->GetSingleLink(LINKOBJ_WILDCARD,to); 
      if (link != LINKID_NULL)
         mpCulpableFor->Remove(link); 
      mpCulpableFor->Add(src,to); 
   }
}

////////////////////////////////////////

ObjID cSimpleDamageModel::GetRealCulprit(ObjID culprit)
{
   if (culprit == OBJ_NULL)
      return culprit; 

   LinkID id = mpCulpableFor->GetSingleLink(LINKOBJ_WILDCARD,culprit); 
   if (id == LINKID_NULL)
      return culprit; 

   sLink link; 
   mpCulpableFor->Get(id,&link);
   
   return link.source;
}

////////////////////////////////////////

static sRelationDesc culp_rdesc = 
{
   "CulpableFor",
   // For timing reasons, we have to allow proxies to delete
   // CulpableFor links:
   kRelationNetProxyChangeable,
}; 

static sPropertyDesc culp_pdesc = 
{
   "Culpable",
   0, // flags
   NULL, 
   0, 0, 
   { "Game: Damage Model", "Culpable"   }
};


void cSimpleDamageModel::CreateCulpability()
{
   // create the property and relation
   mpCulpable = CreateBoolProperty(&culp_pdesc,kPropertyImplSparseHash); 
   sRelationDataDesc ddesc = LINK_NO_DATA; 
   mpCulpableFor = CreateStandardRelation(&culp_rdesc,&ddesc,kQCaseSetDestKnown); 
}

////////////////////////////////////////
//
// Corpse iterator - so we can have multiple corpses/victim
//

cCorpseIter::cCorpseIter(ObjID objID, IRelation *pRelation):
   m_startObj(OBJ_NULL)
{
   AutoAppIPtr(TraitManager);
   // @TODO: use trait cache 
   IObjectQuery* donors = pTraitManager->Query(objID, kTraitQueryAllDonors); 
   cLinkQueryFactory* factory = CreateSourceSetQueryFactory(pRelation, LINKOBJ_WILDCARD); 

   m_pQuery = CreateObjSetLinkQuery(donors,factory); 
   SafeRelease(donors);
}

cCorpseIter::~cCorpseIter(void)
{
   SafeRelease(m_pQuery); 
}

ObjID cCorpseIter::Get(void)
{
   ObjID result = OBJ_NULL;

   Assert_(m_pQuery != NULL);
   if (!Finished())
   {
      sLink link;
      m_pQuery->Link(&link);
      result = link.dest; 
      // set start object if not already set
      if (m_startObj == OBJ_NULL)
         m_startObj = link.source;
   }
   return result;
}

void *cCorpseIter::GetData(void)
{
   void *result = NULL;

   Assert_(m_pQuery != NULL);
   if (!Finished())
      result = m_pQuery->Data();

   return result;
}

BOOL cCorpseIter::Finished(void)
{
   Assert_(m_pQuery != NULL);
   if (!m_pQuery->Done())
   {
      // if query not done, check that we are still looking at links
      // from the initial object (to ensure that we don't return links
      // from archetypes higher up in the hierarchy)
      if (m_startObj == OBJ_NULL)
         return FALSE;
      sLink link;
      m_pQuery->Link(&link);
      return (m_startObj != link.source);
   }
   return TRUE;
}

void cCorpseIter::Next(void)
{
   Assert_(m_pQuery != NULL);
   m_pQuery->Next();
}

////////////////////////////////////////
void InitSimpleDamageModel(void)
{
   AutoAppIPtr(Unknown);
   new cSimpleDamageModel(pUnknown);
}

////////////////////////////////////////////////////////////
// culpable.h api
//

void PropagateCulpability(ObjID from, ObjID to, ulong flags)
{
   cSimpleDamageModel::TheDamageModel->PropagateCulpability(from,to,flags); 
}

ObjID GetRealCulprit(ObjID culprit)
{
   return cSimpleDamageModel::TheDamageModel->GetRealCulprit(culprit); 
}

////////////////////////////////////////

static void LGAPI culp_link_listener(sRelationListenMsg* msg, RelationListenerData data)
{
   ulong flags = (ulong)data; 
   PropagateCulpability(msg->link.source,msg->link.dest,flags); 
}



void AddCulpabilityRelation(IRelation* pRel, ulong flags)
{
   pRel->Listen(kListenLinkBirth,culp_link_listener,(void*)flags); 
}

#endif // __SIMPDMG_H





