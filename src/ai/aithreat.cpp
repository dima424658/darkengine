/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aithreat.cpp,v 1.4 1999/11/22 17:49:12 henrys Exp $
//
// Threat listener
//

#include <lg.h>

#include <dmgbase.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <weapcb.h>

#include <aiprops.h>
#include <aiprcore.h>
#include <aipsdscr.h>
#include <aiteams.h>
#include <aithreat.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIThreatResponseProperty);

///////////////////////////////////////////////////////////////////////////////

static const char * _g_ppszAIThreats[kAIT_NumThreats] = 
{
   "Raised weapon",
   "Damage"
};

///////////////////////////////////////////////////////////////////////////////
//
// Threat response property
//

#define kAIMaxThreatActs 16

struct sAIThreatResponse
{
   eAIThreat    threat;
   eAIPriority  priority;
   DWORD        reserved[4];
   sAIPsdScrAct acts[kAIMaxThreatActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIThreatResponseProperty
DECLARE_PROPERTY_INTERFACE(IAIThreatResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIThreatResponse *);
};

static IAIThreatResponseProperty * g_pAIThreatResponseProperty;

///////////////////

static sPropertyDesc _g_ThreatResponsePropertyDesc = 
{
   "AI_ThrtRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Threat response" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////

typedef cSpecificProperty<IAIThreatResponseProperty, &IID_IAIThreatResponseProperty, sAIThreatResponse *, cHashPropertyStore< cClassDataOps<sAIThreatResponse> > > cAIThreatResponsePropertyBase;

class cAIThreatResponseProperty : public cAIThreatResponsePropertyBase
{
public:
   cAIThreatResponseProperty()
      : cAIThreatResponsePropertyBase(&_g_ThreatResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIThreatResponse);
};

///////////////////

#define THREAT_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAIThreatResponse, acts[i-1])

static sFieldDesc _g_AIThreatResponseFieldDesc[] =
{
   { "Threat", kFieldTypeEnum, FieldLocation(sAIThreatResponse, threat),              kFieldFlagNone, 0, FieldNames(_g_ppszAIThreats) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   THREAT_RESPONSE_FIELDS(1),
   THREAT_RESPONSE_FIELDS(2),
   THREAT_RESPONSE_FIELDS(3),
   THREAT_RESPONSE_FIELDS(4),
   THREAT_RESPONSE_FIELDS(5),
   THREAT_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AIThreatResponseStructDesc = StructDescBuild(sAIThreatResponse, kStructFlagNone, _g_AIThreatResponseFieldDesc);

///////////////////////////////////////

static const sAIThreatResponse * AIGetThreatResponse(ObjID obj, eAIThreat threat)
{
   const sAIThreatResponse * pResponse = AIGetProperty(g_pAIThreatResponseProperty, obj, (sAIThreatResponse *)NULL);
   
   if (pResponse && pResponse->threat == threat)
      return pResponse;
   return NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitThreatResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AIThreatResponseStructDesc);
   
   g_pAIThreatResponseProperty = new cAIThreatResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermThreatResponseAbility()
{
   SafeRelease(g_pAIThreatResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIThreatResponse
//

cAIThreatResponse::cAIThreatResponse()
 : cAITriggeredPScripted(kAIT_2Hz)
{
}

///////////////////////////////////////

cAIThreatResponse::~cAIThreatResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIThreatResponse::GetName()
{
   return "Threat response ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIThreatResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_Damage | kAICN_Weapon);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIThreatResponse::OnDamage(const sDamageMsg * pMsg, ObjID realCulpritID)
{
   if (pMsg->kind != kDamageMsgDamage)
      return;

   CheckThreat(kAIT_Damage);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIThreatResponse::OnWeapon(eWeaponEvent ev, ObjID victim, ObjID culprit)
{
   if (culprit != GetID() && AITeamCompare(AIGetTeam(culprit), AIGetTeam(GetID())) == kAI_Opponents)
   {
      CheckThreat(kAIT_Weapon);
   }
}

///////////////////////////////////////

void cAIThreatResponse::CheckThreat(eAIThreat threat)
{
   const sAIThreatResponse * pResponse = AIGetThreatResponse(GetID(), threat);
   if (pResponse)
   {
      SetTriggered(pResponse->priority, pResponse->acts, kAIMaxThreatActs);
   }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The threat pseudo-script dialog
//
static sFieldDesc gThreatHeaderFieldDesc[] =
{
   { "Threat", kFieldTypeEnum, FieldLocation(sAIThreatResponse, threat), kFieldFlagNone, 0, FieldNames(_g_ppszAIThreats) }
};
static sStructDesc gThreatHeaderStructDesc = StructDescBuild(sAIThreatResponse, kStructFlagNone, gThreatHeaderFieldDesc);

IStructEditor* NewThreatResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Threat response", kAIMaxThreatActs, &gThreatHeaderStructDesc, data, ((sAIThreatResponse*) data)->acts);
}

#endif

