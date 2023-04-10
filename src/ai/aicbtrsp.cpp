/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbtrsp.cpp,v 1.4 1999/11/22 17:51:35 henrys Exp $
//
//
//

#include <lg.h>

#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiapisns.h>
#include <aicbtrsp.h>
#include <aiprops.h>
#include <aipsdscr.h>
#include <aisndtyp.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAISenseCombatResponseProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Combat sound response property
//

#define kAIMaxCbtRspActs 16

struct sAISenseCombatResponse
{
   eAIPriority  priority;
   DWORD        reserved[4];
   sAIPsdScrAct acts[kAIMaxCbtRspActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAISenseCombatResponseProperty
DECLARE_PROPERTY_INTERFACE(IAISenseCombatResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISenseCombatResponse *);
};

static IAISenseCombatResponseProperty * g_pAISenseCombatResponseProperty;

///////////////////

static sPropertyDesc _g_SenseCombatResponsePropertyDesc = 
{
   "AI_CbtRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Sense combat response" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAISenseCombatResponseProperty, &IID_IAISenseCombatResponseProperty, sAISenseCombatResponse *, cHashPropertyStore< cClassDataOps<sAISenseCombatResponse> > > cAISenseCombatResponsePropertyBase;

class cAISenseCombatResponseProperty : public cAISenseCombatResponsePropertyBase
{
public:
   cAISenseCombatResponseProperty()
      : cAISenseCombatResponsePropertyBase(&_g_SenseCombatResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAISenseCombatResponse);
};

///////////////////

#define CBTSND_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAISenseCombatResponse, acts[i-1])

static sFieldDesc _g_AISenseCombatResponseFieldDesc[] =
{
   //{ "Priority", kFieldTypeEnum,   FieldLocation(sAISenseCombatResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   CBTSND_RESPONSE_FIELDS(1),
   CBTSND_RESPONSE_FIELDS(2),
   CBTSND_RESPONSE_FIELDS(3),
   CBTSND_RESPONSE_FIELDS(4),
   CBTSND_RESPONSE_FIELDS(5),
   CBTSND_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AISenseCombatResponseStructDesc = StructDescBuild(sAISenseCombatResponse, kStructFlagNone, _g_AISenseCombatResponseFieldDesc);

///////////////////////////////////////

#define AIGetSenseCombatResponse(obj) AIGetProperty(g_pAISenseCombatResponseProperty, obj, (sAISenseCombatResponse *)NULL)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSenseCombatResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AISenseCombatResponseStructDesc);
   
   g_pAISenseCombatResponseProperty = new cAISenseCombatResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermSenseCombatResponseAbility()
{
   SafeRelease(g_pAISenseCombatResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISenseCombatResponse
//

#define kAIListenRate kAIT_2Hz

cAISenseCombatResponse::cAISenseCombatResponse()
 : cAITriggeredPScripted(kAIListenRate)
{
}

///////////////////////////////////////

cAISenseCombatResponse::~cAISenseCombatResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAISenseCombatResponse::GetName()
{
   return "Combat listen ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenseCombatResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_WitnessCombat);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenseCombatResponse::OnWitnessCombat()
{
   if (InControl() || IsTriggered())
      return;
      
   const sAISenseCombatResponse * pResponse = AIGetSenseCombatResponse(GetID());
   if (pResponse)
      SetTriggered(pResponse->priority, pResponse->acts, kAIMaxCbtRspActs);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The combat sense response pseudo-script dialog
//
static sFieldDesc gCbtRspHeaderFieldDesc[] =
{
   { "Priority", kFieldTypeEnum, FieldLocation(sAISenseCombatResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },
};
static sStructDesc gCbtRspHeaderStructDesc = StructDescBuild(sAISenseCombatResponse, kStructFlagNone, gCbtRspHeaderFieldDesc);

IStructEditor* NewCombatResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Combat sense response", kAIMaxCbtRspActs, &gCbtRspHeaderStructDesc, data, ((sAISenseCombatResponse*) data)->acts);
}

#endif


