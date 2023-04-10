/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aialtrsp.cpp,v 1.4 1999/11/22 17:50:00 henrys Exp $
//
// Alert listener
//

#include <lg.h>

#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiprops.h>
#include <aipsdscr.h>
#include <aialtrsp.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIAlertResponseProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Alert response property
//

#define kAIMaxAlertActs 16

struct sAIAlertResponse
{
   eAIAlertLevel level;
   eAIPriority   priority;
   DWORD         reserved[4];
   sAIPsdScrAct  acts[kAIMaxAlertActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIAlertResponseProperty
DECLARE_PROPERTY_INTERFACE(IAIAlertResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAlertResponse *);
};

static IAIAlertResponseProperty * g_pAIAlertResponseProperty;

///////////////////

static sPropertyDesc _g_AlertResponsePropertyDesc = 
{
   "AI_AlrtRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Alert response" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAIAlertResponseProperty, &IID_IAIAlertResponseProperty, sAIAlertResponse *, cHashPropertyStore< cClassDataOps<sAIAlertResponse> > > cAIAlertResponsePropertyBase;

class cAIAlertResponseProperty : public cAIAlertResponsePropertyBase
{
public:
   cAIAlertResponseProperty()
      : cAIAlertResponsePropertyBase(&_g_AlertResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAlertResponse);
};

///////////////////

#define ALERT_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAIAlertResponse, acts[i-1])

static sFieldDesc _g_AIAlertResponseFieldDesc[] =
{
   { "Alert level", kFieldTypeEnum, FieldLocation(sAIAlertResponse, level),              kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Priority", kFieldTypeEnum,    FieldLocation(sAIAlertResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   ALERT_RESPONSE_FIELDS(1),
   ALERT_RESPONSE_FIELDS(2),
   ALERT_RESPONSE_FIELDS(3),
   ALERT_RESPONSE_FIELDS(4),
   ALERT_RESPONSE_FIELDS(5),
   ALERT_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AIAlertResponseStructDesc = StructDescBuild(sAIAlertResponse, kStructFlagNone, _g_AIAlertResponseFieldDesc);

///////////////////////////////////////

static const sAIAlertResponse * AIGetAlertResponse(ObjID obj, eAIAlertLevel level)
{
   const sAIAlertResponse * pResponse = AIGetProperty(g_pAIAlertResponseProperty, obj, (sAIAlertResponse *)NULL);
   
   if (pResponse && pResponse->level == level)
      return pResponse;
   return NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitAlertResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AIAlertResponseStructDesc);
   
   g_pAIAlertResponseProperty = new cAIAlertResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermAlertResponseAbility()
{
   SafeRelease(g_pAIAlertResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAlertResponse
//

cAIAlertResponse::cAIAlertResponse()
 : cAITriggeredPScripted(kAIT_2Hz)
{
}

///////////////////////////////////////

cAIAlertResponse::~cAIAlertResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIAlertResponse::GetName()
{
   return "Alertness response ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAlertResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_Alertness);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAlertResponse::OnAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   const sAIAlertResponse * pResponse = AIGetAlertResponse(GetID(), current);
   if (pResponse)
      SetTriggered(pResponse->priority, pResponse->acts, kAIMaxAlertActs);
}
   
///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The alert response pseudo-script dialog
//
static sFieldDesc gAlertHeaderFieldDesc[] =
{
   { "Alert level", kFieldTypeEnum, FieldLocation(sAIAlertResponse, level), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Priority", kFieldTypeEnum,    FieldLocation(sAIAlertResponse, priority), kFieldFlagNone, 0, FieldNames(g_ppszAIPriority) },
};
static sStructDesc gAlertHeaderStructDesc = StructDescBuild(sAIAlertResponse, kStructFlagNone, gAlertHeaderFieldDesc);

IStructEditor* NewAlertResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Alert response", kAIMaxAlertActs, &gAlertHeaderStructDesc, data, ((sAIAlertResponse*) data)->acts);
}

#endif


