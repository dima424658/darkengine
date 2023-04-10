/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibdyrsp.cpp,v 1.3 1999/11/22 17:50:56 henrys Exp $
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

#include <aiprops.h>
#include <aipsdscr.h>
#include <aibdyrsp.h>
#include <aiteams.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIBodyResponseProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Body response property
//

#define kAIMaxBodyActs 16

struct sAIBodyResponse
{
   eAIPriority   priority;
   DWORD         reserved[4];
   sAIPsdScrAct  acts[kAIMaxBodyActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIBodyResponseProperty
DECLARE_PROPERTY_INTERFACE(IAIBodyResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIBodyResponse *);
};

static IAIBodyResponseProperty * g_pAIBodyResponseProperty;

///////////////////

static sPropertyDesc _g_BodyResponsePropertyDesc = 
{
   "AI_BodyRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Body response" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAIBodyResponseProperty, &IID_IAIBodyResponseProperty, sAIBodyResponse *, cHashPropertyStore< cClassDataOps<sAIBodyResponse> > > cAIBodyResponsePropertyBase;

class cAIBodyResponseProperty : public cAIBodyResponsePropertyBase
{
public:
   cAIBodyResponseProperty()
      : cAIBodyResponsePropertyBase(&_g_BodyResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIBodyResponse);
};

///////////////////

#define BODY_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAIBodyResponse, acts[i-1])

static sFieldDesc _g_AIBodyResponseFieldDesc[] =
{
   { "Priority", kFieldTypeEnum,    FieldLocation(sAIBodyResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   BODY_RESPONSE_FIELDS(1),
   BODY_RESPONSE_FIELDS(2),
   BODY_RESPONSE_FIELDS(3),
   BODY_RESPONSE_FIELDS(4),
   BODY_RESPONSE_FIELDS(5),
   BODY_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AIBodyResponseStructDesc = StructDescBuild(sAIBodyResponse, kStructFlagNone, _g_AIBodyResponseFieldDesc);

///////////////////////////////////////

#define AIGetBodyResponse(obj) AIGetProperty(g_pAIBodyResponseProperty, obj, (sAIBodyResponse *)NULL)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitBodyResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AIBodyResponseStructDesc);
   
   g_pAIBodyResponseProperty = new cAIBodyResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermBodyResponseAbility()
{
   SafeRelease(g_pAIBodyResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBodyResponse
//

cAIBodyResponse::cAIBodyResponse()
 : cAITriggeredPScripted(kAIT_2Hz)
{
}

///////////////////////////////////////

cAIBodyResponse::~cAIBodyResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIBodyResponse::GetName()
{
   return "Body response ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBodyResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_FoundBody);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIBodyResponse::OnFoundBody(ObjID body)
{
   if (AITeamCompare(GetID(), body) == kAI_Teammates)
   {
      const sAIBodyResponse * pResponse = AIGetBodyResponse(GetID());
      if (pResponse)
         SetTriggered(pResponse->priority, pResponse->acts, kAIMaxBodyActs);
   }
}
   
///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The alert response pseudo-script dialog
//
static sFieldDesc gBodyHeaderFieldDesc[] =
{
   { "Priority", kFieldTypeEnum, FieldLocation(sAIBodyResponse, priority), kFieldFlagNone, 0, FieldNames(g_ppszAIPriority) },
};
static sStructDesc gBodyHeaderStructDesc = StructDescBuild(sAIBodyResponse, kStructFlagNone, gBodyHeaderFieldDesc);

IStructEditor* NewBodyResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Body response", kAIMaxBodyActs, &gBodyHeaderStructDesc, data, ((sAIBodyResponse*) data)->acts);
}

#endif


