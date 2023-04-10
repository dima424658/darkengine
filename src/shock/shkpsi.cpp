/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsi.cpp,v 1.48 1999/11/19 14:55:58 adurant Exp $
// general psionics powers
// for the moment, assumes player not any old AI

#include <cfgdbg.h>

#include <stdlib.h>

#include <appagg.h>
#include <aggmemb.h>

#include <matrixc.h>
#include <objhp.h>
#include <objquery.h>
#include <physapi.h>
#include <playrobj.h>

#include <shkpsapi.h>
#include <shkpsibs.h>
#include <shkpsipr.h>
#include <shkpsipw.h>

#include <shkplayr.h>
#include <shkplcst.h>
#include <shkpgapi.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkovrld.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkutils.h>

#include <pgrpprop.h>
#include <gunflash.h>

// for shots
#include <gunapi.h>
#include <gunproj.h>

#include <camera.h>
#include <traitbas.h>
#include <traitman.h>
#include <scrptapi.h>
#include <scrptbas.h>

// sound
#include <esnd.h>
#include <ctagset.h>
#include <schema.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <netman.h>

// psi tag prop
#include <propert_.h>
#include <esndprop.h>

// utilities
#include <iobjsys.h>

#ifndef SHIP
#include <command.h>
#include <mprintf.h>
#endif

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>

//------------------------------------------------------------
// Forward declare the iterator class
//

DEFINE_LG_GUID(IID_IPsiActiveIter, 0x1d3);

class cPsiActiveIter : public IPsiActiveIter 
{
private:
   IObjectQuery *m_pQuery;
   ObjID m_objID;
   ePsiPowers m_power;

   void Match(void)
   {
      sPsiPower *pPower;

      while (!Done())
      {
         m_objID = m_pQuery->Object();
         if (g_pPsiPowerProperty->Get(m_objID, &pPower))
         {
            m_power = pPower->m_power;
            return;
         }
         m_pQuery->Next();
      }
   }

public:

   cPsiActiveIter(ObjID objID)
   {
      AutoAppIPtr(TraitManager);
      m_pQuery = pTraitManager->Query(objID, kTraitQueryDonors);
      Match();
   }
   ~cPsiActiveIter()
   {
      SafeRelease(m_pQuery);
   }

   DECLARE_UNAGGREGATABLE(); 

   STDMETHOD_(BOOL,Done)(void) {return m_pQuery->Done();}
   STDMETHOD(Next)(void) {if (!Done()) m_pQuery->Next(); Match(); return S_OK;}

   STDMETHOD_(const char*, GetName)() {if (!Done()) return psiPowerNames[m_power]; else return NULL;};
   STDMETHOD_(ePsiPowers, GetPower)() {if (!Done()) return m_power; else return kPsiNone;};

};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cPsiActiveIter,IPsiActiveIter);

////////////////////////////////////////

DEFINE_LG_GUID(IID_IPlayerPsi, 0x1d3);

class cPlayerPsi:
   public cCTDelegating<IPlayerPsi>, 
   public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cPlayerPsi(IUnknown* pOuter):
      m_sustainedPower(kPsiNone),
      m_noDeactivateSound(FALSE)
   {
      for (int i=0; i<kPsiMax; i++)
         m_schemaHandle[i] = SCH_HANDLE_NULL;
      MI_INIT_AGGREGATION_1(pOuter, IPlayerPsi, kPriorityNormal, NULL);
   }
   
   STDMETHOD(Frame)(tSimTime deltaTime);

   STDMETHOD(Select)(ePsiPowers power);
   STDMETHOD_(ePsiPowers, GetSelectedPower)(void);

   STDMETHOD_(BOOL, CanActivate)(void);
   STDMETHOD(Activate)(void);
   STDMETHOD(OnDeactivate)(ePsiPowers power);
   STDMETHOD(Deactivate)(ePsiPowers power);
   STDMETHOD_(BOOL, IsActive)(ePsiPowers power);

   STDMETHOD_(int, GetPoints)(void);
   STDMETHOD_(int, GetMaxPoints)(void);
   STDMETHOD(SetMaxPoints)(int maxPoints);
   STDMETHOD(SetPoints)(int points);

   STDMETHOD_(ePsiPowers, GetSustainedPower)(void) {return m_sustainedPower;};

   void StartSchema(ObjID psiObjID, ePsiPowers power, int type);
   void EndSchema(ePsiPowers power);
   STDMETHOD_(void, OnSchemaEnd)(int hSchema);

   STDMETHOD_(IPsiActiveIter*, Iter)(void);

   STDMETHOD(BeginOverload)(void);
   STDMETHOD_(BOOL,EndOverload)(void);

   STDMETHOD(PsiTarget)(ObjID what);

private:
   int m_schemaHandle[kPsiMax];
   cHashTable<int, ePsiPowers, cHashTableFunctions<int> > m_schemaPowerHash;
   BOOL m_noDeactivateSound;

   ePsiPowers m_sustainedPower;

   static void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData);
};

////////////////////////////////////////

STDMETHODIMP cPlayerPsi::Select(ePsiPowers power)
{
   PsiStateSetCurrentPower(PlayerObject(), power);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(ePsiActivateStatus) cPlayerPsi::CanActivate(void)
{
   sPsiState* pPsiState = PsiStateGet(PlayerObject());
   sPsiPower* pPsiPower;

   // check that we have a power selected
   Assert_(pPsiState!=NULL);
   if (pPsiState->m_currentPower == kPsiNone) 
      return kPsiInvalidPower;
   // check that it has a power property
   if (!PsiPowerGet(pPsiState->m_currentPower, &pPsiPower))
      return kPsiInvalidPower;

   // check that we have the points to start
   int cost = pPsiPower->m_startCost;
   if (IsActive(kPsiPsi))
      cost = cost * 2;

   if (pPsiState->m_points < cost)
      return kPsiInsufficientPoints;

   return kPsiOK;
}

////////////////////////////////////////

static void PayPsiPoints(int cost)
{
   sPsiState* pPsiState;
   pPsiState = PsiStateGet(PlayerObject());

   int thresh = pPsiState->m_maxPoints / 5;
   if ((pPsiState->m_points > thresh) && (pPsiState->m_points - cost <= thresh))
      SchemaPlay((Label *)"bb01",NULL);         

   pPsiState->m_points -= cost;
   if (pPsiState->m_points < 0)
      pPsiState->m_points = 0;
}

STDMETHODIMP cPlayerPsi::Activate(void)
{
   sPsiState* pPsiState;
   sPsiPower* pPsiPower = NULL;
   sScrMsg msg(PlayerObject(), "TurnOn");
   ObjID psiObjID;
   ObjID projID;

   if (CanActivate() != kPsiOK)
      return E_FAIL;

   pPsiState = PsiStateGet(PlayerObject());
   Assert_(pPsiState);
   PsiPowerGet(pPsiState->m_currentPower, &pPsiPower);
   Assert_(pPsiPower);

   AutoAppIPtr(ShockPlayer);
   int psistat = pShockPlayer->GetStat(kStatPsi);
   if (PsiOverloaded(PlayerObject(),pPsiState->m_currentPower))
      psistat = psistat + 2;
   if (psistat > 8)
      psistat = 8;

   // compute cost early, to avoid double-billing if we are turning
   // on the metapsi power
   int cost = pPsiPower->m_startCost;
   if (IsActive(kPsiPsi))
      cost = cost * 2;
   /*
   // extra overload cost cut 5/25/99 Xemu
   if (PsiOverloaded(PlayerObject(),pPsiState->m_currentPower))
      cost = cost + 2;
   */
   
   psiObjID = PsiPowerGetObjID(pPsiState->m_currentPower);
   Assert_(psiObjID != OBJ_NULL);

   // specials per-type
   switch (pPsiPower->m_type)
   {
   case kPsiTypeShot:
      // launch shot
      // first look for one matching the psi stat
      projID = GetProjectileByOrder(psiObjID,psistat);
      // then look for a "default" one
      if (projID == OBJ_NULL)
         projID = GetProjectileByOrder(psiObjID,0);
      if (projID != OBJ_NULL)
      {
         sLaunchParams launchParams = g_defaultLaunchParams;
         launchParams.flags = kLaunchLocOverride|kLaunchPitchOverride|kLaunchTellAI;
         launchParams.loc = PlayerCamera()->pos;
         launchParams.pitch = PlayerCamera()->ang.ty;
         if (pPsiPower->m_data[0] != 0)
            launchParams.intensityMult = psistat*pPsiPower->m_data[0];
         GunLaunchProjectile(PlayerObject(), projID, &launchParams);
      }
      break;
   case kPsiTypeSustained:
      m_sustainedPower = pPsiState->m_currentPower;
      break;
   case kPsiTypeShield:
      // create shield "flashes"
      ParticleGroupCreateFromArchetype(psiObjID, PlayerObject());
//      CreateGunFlashes(psiObjID, PlayerObject(), ObjPosGet(PlayerObject())->fac.tz, TRUE);
      break;
   case kPsiTypeCursor:
      // don't allow if cursor already zany
      if (shock_cursor_mode != SCM_NORMAL)
         return(E_FAIL);
      else   
      {
         // switch modes
         MouseMode(TRUE,FALSE);
         // set the cursor mode
         IRes *hnd;
         shock_cursor_mode = SCM_PSI;
         hnd = LoadPCX("psicur","iface\\");
         SetCursorByHandle(hnd); 
         SafeFreeHnd(&hnd);
      }

      break;
   }

   PayPsiPoints(cost);

   StartSchema(psiObjID, pPsiState->m_currentPower, pPsiPower->m_type);

   // Finally, add the meta-prop
   // shots don't get scripts or meta-props
   if (pPsiPower->m_type != kPsiTypeShot)
   {
      AutoAppIPtr(TraitManager);
      // deactivate if we already have this power active
      if (IsActive(pPsiState->m_currentPower))
      {
         m_noDeactivateSound = TRUE;
         Deactivate(pPsiState->m_currentPower);
      }
      // add the meta-prop (start the script)
      pTraitManager->AddObjMetaProperty(PlayerObject(), psiObjID);
   }

   return S_OK;
}

//////////////////////////////////////////////////////////////////
// A psi power has deactivated (the script is informing us)
// 

STDMETHODIMP cPlayerPsi::OnDeactivate(ePsiPowers power)
{
   AutoAppIPtr(TraitManager);
   ObjID psiObjID;
   sPsiPower* pPsiPower = NULL;

   // play de-activate sound
   psiObjID = PsiPowerGetObjID(power);
   if (psiObjID == OBJ_NULL)
      Warning(("Current psi power (%s) has no psi meta-property\n", psiPowerNames[power]));
   else
   {
      if (!m_noDeactivateSound)
      {
         EndSchema(power);
         cTagSet eventTags("Event Deactivate");
         sESndTagList* pESndPsiTagList;
         if (ObjGetESndPsi(psiObjID, &pESndPsiTagList))
            eventTags.Append(cTagSet(*pESndPsiTagList->m_pTagSet));
         ESndPlay(&eventTags, psiObjID, OBJ_NULL); 
      }
      else
         m_noDeactivateSound = FALSE;
   }
   pTraitManager->RemoveObjMetaProperty(PlayerObject(), PsiPowerGetObjID(power));

   // Type specifc stuff
   PsiPowerGet(power, &pPsiPower);
   Assert_(pPsiPower);
   switch (pPsiPower->m_type)
   {
   case kPsiTypeSustained:
      Assert_(m_sustainedPower == power);
      m_sustainedPower = kPsiNone;
      break;
   case kPsiTypeShield:
      // create shield "flashes"
      ParticleGroupDestroyAttached(PlayerObject());
      break;
   }
   return S_OK;
}

//////////////////////////////////////////////////////////////////
// Tell the script to deactivate
// 
STDMETHODIMP cPlayerPsi::Deactivate(ePsiPowers power)
{
   AutoAppIPtr(ScriptMan);
   sScrMsg msg(PlayerObject(), PlayerObject(), "DeactivatePsi", power); 
   pScriptMan->SendMessage(&msg); 
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerPsi::Frame(tSimTime deltaTime)
{
   sPsiState* pPsiState = PsiStateGet(PlayerObject());

   if (pPsiState->m_currentPower == kPsiNone) 
      return S_OK;
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerPsi::BeginOverload(void)
{
   sPsiState* pPsiState = PsiStateGet(PlayerObject());
   ShockOverlayChange(kOverlayOverload,kOverlayModeOn);
   PsiSetOverload(PlayerObject(),pPsiState->m_currentPower,FALSE);
   return(S_OK);
}

////////////////////////////////////////
// returns whether or not to actually start the power
STDMETHODIMP_(BOOL) cPlayerPsi::EndOverload(void)
{
   // check for overload
   int overload;
   overload = ShockOverloadFinish();
   sPsiState* pPsiState = PsiStateGet(PlayerObject());
   switch(overload)
   {
   case kOverloadNormal:
      PsiSetOverload(PlayerObject(),pPsiState->m_currentPower,FALSE);
      return(TRUE);
      break;
   case kOverloadBurnout:
      PsiSetOverload(PlayerObject(),pPsiState->m_currentPower,FALSE);
      // pay the psi cost anyways
      {
         sPsiPower* pPsiPower;
         PsiPowerGet(pPsiState->m_currentPower, &pPsiPower);
         int cost = pPsiPower->m_startCost;
         if (IsActive(kPsiPsi))
            cost = cost * 2;
         PayPsiPoints(cost);
      }
      return(FALSE);
      break;
   case kOverloadSuccess:
      PsiSetOverload(PlayerObject(),pPsiState->m_currentPower,TRUE);
      return(TRUE);
      break;
   }

   return(FALSE); // should never get here
}

////////////////////////////////////////

STDMETHODIMP_(ePsiPowers) cPlayerPsi::GetSelectedPower(void)
{
   return PsiStateGet(PlayerObject())->m_currentPower;
}

////////////////////////////////////////

STDMETHODIMP_(int) cPlayerPsi::GetPoints(void)
{
   return PsiStateGet(PlayerObject())->m_points;
}

////////////////////////////////////////

STDMETHODIMP_(int) cPlayerPsi::GetMaxPoints(void)
{
   return PsiStateGet(PlayerObject())->m_maxPoints;
}

////////////////////////////////////////

STDMETHODIMP cPlayerPsi::SetMaxPoints(int maxPoints)
{
   PsiStateSetMaxPoints(PlayerObject(), maxPoints);
   // we should really check whether we set or not, or something
   return S_OK;
}
 
////////////////////////////////////////

STDMETHODIMP cPlayerPsi::SetPoints(int points)
{
   PsiStateSetPoints(PlayerObject(), points);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(IPsiActiveIter*) cPlayerPsi::Iter(void)
{
   return (IPsiActiveIter*) new cPsiActiveIter(PlayerObject());
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cPlayerPsi::IsActive(ePsiPowers power)
{
   BOOL found = FALSE;
   IPsiActiveIter* pIter = Iter();

   while (!pIter->Done() && !found)
   {
      found = (pIter->GetPower() == power);
      pIter->Next();
   }
   SafeRelease(pIter);
   return found;
}

////////////////////////////////////////

void cPlayerPsi::EndSchema(ePsiPowers power)
{
   // stop schema associated with previous instance of same power if any
   if (m_schemaHandle[power] != SCH_HANDLE_NULL)
      SchemaPlayHalt(m_schemaHandle[power]);
}

////////////////////////////////////////

void cPlayerPsi::StartSchema(ObjID psiObjID, ePsiPowers power, int type)
{
   EndSchema(power);
   cTagSet eventTags("Event Activate");
   sESndTagList* pESndPsiTagList;
   if (ObjGetESndPsi(psiObjID, &pESndPsiTagList))
      eventTags.Append(cTagSet(*pESndPsiTagList->m_pTagSet));
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;
   callParams.flags |= SCH_SET_CALLBACK;
   callParams.callback = SchemaEndCallback;
   // We like to play these sounds non-spatially, but that's screwy
   // for multiplayer, especially for shots, which we really ought to
   // be able to hear. We'd like to network everything, but there are
   // VOs in some of the other powers...
   AutoAppIPtr(NetManager);
   if ((type == kPsiTypeShot) && pNetManager->Networking())
   {
      m_schemaHandle[power] = ESndPlayLoc(&eventTags, psiObjID, OBJ_NULL, &(PlayerCamera()->pos), &callParams);
   } else {
      m_schemaHandle[power] = ESndPlay(&eventTags, psiObjID, OBJ_NULL, &callParams); 
   }
   m_schemaPowerHash.Insert(m_schemaHandle[power], power);
}

////////////////////////////////////////

STDMETHODIMP_(void) cPlayerPsi::OnSchemaEnd(int hSchema)
{
   ePsiPowers power;

   if (m_schemaPowerHash.Lookup(hSchema, &power))
   {
      m_schemaPowerHash.Delete(hSchema);
      m_schemaHandle[power] = SCH_HANDLE_NULL;
   }
   else
      Warning(("cPlayerPsi::OnSchemaEnd: no record of schema handle %d\n", hSchema));
}

////////////////////////////////////////

void cPlayerPsi::SchemaEndCallback(int hSchema, ObjID schemaID, void *pData)
{
   AutoAppIPtr(PlayerPsi);
   pPlayerPsi->OnSchemaEnd(hSchema);
}

////////////////////////////////////////
STDMETHODIMP cPlayerPsi::PsiTarget(ObjID what)
{
   // send all of our active psi powers a "target" message
   AutoAppIPtr(ScriptMan);
   sScrMsg msg(what, PlayerObject(), "PsiTarget", NULL); 
   pScriptMan->SendMessage(&msg); 

   return(S_OK);
}

////////////////////////////////////////

void ShockPlayerPsiCreate(void)
{
   AutoAppIPtr(Unknown); 
   new cPlayerPsi(pUnknown); 
}

/////////////////////////////////////////////

#ifndef SHIP

void SetPsiPoints(int points)
{
   AutoAppIPtr(PlayerPsi);
   pPlayerPsi->SetPoints(points);
}

void SetMaxPsiPoints(int points)
{
   AutoAppIPtr(PlayerPsi);
   pPlayerPsi->SetMaxPoints(points);
   pPlayerPsi->SetPoints(points);
}

void SpewActive(void)
{
   AutoAppIPtr(PlayerPsi);
   IPsiActiveIter* pIter = pPlayerPsi->Iter();

   while (!pIter->Done())
   {
      mprintf("Active power %s\n", pIter->GetName());
      pIter->Next();
   }
   SafeRelease(pIter);
}

/////////////////////////////////////////////

void GivePsiAll(void)
{
   AutoAppIPtr(ShockPlayer);
   for (int i=0; i<kPsiMax; i++)
      pShockPlayer->AddPsiPower(PlayerObject(), (ePsiPowers)i);
}

/////////////////////////////////////////////

static Command psiCommands[] =
{
   { "set_psi_points", FUNC_INT, SetPsiPoints, "Set the current psi points"},
   { "set_max_psi", FUNC_INT, SetMaxPsiPoints, "Set max psi points"},
   { "active_psi", FUNC_VOID, SpewActive, "Spew active psi powers"},
   { "psi_all", FUNC_VOID, GivePsiAll, "Give player all psi powers"},
};

#endif

/////////////////////////////////////////////

void PsiInit(void)
{
#ifndef SHIP
   // commands
   COMMANDS(psiCommands,HK_ALL);
#endif
   PsiPropertiesInit();
   PsiPowersInit();
}

////////////////////////////////////////

void PsiTerm(void)
{
   PsiPropertiesTerm();
}

////////////////////////////////////////

BOOL IsCharmed(ObjID objID)
{
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   ObjID charmID = pObjectSystem->GetObjectNamed("charmed");

   if (charmID == OBJ_NULL)
      return FALSE;
   return pTraitManager->ObjHasDonor(objID, charmID);
}
