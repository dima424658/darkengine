/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ailisten.cpp,v 1.11 1999/11/22 17:53:50 henrys Exp $
//
// Signal listener
//

#include <lg.h>

#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <ailisten.h>
#include <aiprops.h>
#include <aipsdscr.h>
#include <aisignal.h>

#include <psdialog.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAISignalResponseProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Signal response property
//

#define kAIMaxSignalActs 16

struct sAISignalResponse
{
   char         szSignal[32];
   eAIPriority  priority;
   DWORD        reserved[4];
   sAIPsdScrAct acts[kAIMaxSignalActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAISignalResponseProperty
DECLARE_PROPERTY_INTERFACE(IAISignalResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISignalResponse *);
};

static IAISignalResponseProperty * g_pAISignalResponseProperty;

///////////////////

static sPropertyDesc _g_SignalResponsePropertyDesc = 
{
   "AI_SigRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Signal response" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////

typedef cSpecificProperty<IAISignalResponseProperty, &IID_IAISignalResponseProperty, sAISignalResponse *, cHashPropertyStore< cClassDataOps<sAISignalResponse> > > cAISignalResponsePropertyBase;

class cAISignalResponseProperty : public cAISignalResponsePropertyBase
{
public:
   cAISignalResponseProperty()
      : cAISignalResponsePropertyBase(&_g_SignalResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAISignalResponse);
};

///////////////////

#define SIGNAL_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAISignalResponse, acts[i-1])

static sFieldDesc _g_AISignalResponseFieldDesc[] =
{
   { "Signal", kFieldTypeString, FieldLocation(sAISignalResponse, szSignal)                                                    }, 
   { "Priority", kFieldTypeEnum,   FieldLocation(sAISignalResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   SIGNAL_RESPONSE_FIELDS(1),
   SIGNAL_RESPONSE_FIELDS(2),
   SIGNAL_RESPONSE_FIELDS(3),
   SIGNAL_RESPONSE_FIELDS(4),
   SIGNAL_RESPONSE_FIELDS(5),
   SIGNAL_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AISignalResponseStructDesc = StructDescBuild(sAISignalResponse, kStructFlagNone, _g_AISignalResponseFieldDesc);

///////////////////////////////////////

static const sAISignalResponse * AIGetSignalResponse(ObjID obj, const char * pszSignal)
{
   const sAISignalResponse * pResponse = AIGetProperty(g_pAISignalResponseProperty, obj, (sAISignalResponse *)NULL);
   
   if (pResponse && stricmp(pResponse->szSignal, pszSignal) == 0)
      return pResponse;
   return NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSignalResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AISignalResponseStructDesc);
   
   g_pAISignalResponseProperty = new cAISignalResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermSignalResponseAbility()
{
   SafeRelease(g_pAISignalResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISignalResponse
//

#define kAIListenRate kAIT_2Hz

cAISignalResponse::cAISignalResponse()
 : cAITriggeredPScripted(kAIListenRate)
{
}

///////////////////////////////////////

cAISignalResponse::~cAISignalResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAISignalResponse::GetName()
{
   return "Signal listen ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISignalResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_Signal);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISignalResponse::OnSignal(const sAISignal * pSignal)
{
   const sAISignalResponse * pResponse = AIGetSignalResponse(GetID(), pSignal->name);
   if (pResponse)
   {
      SetTriggered(pResponse->priority, pResponse->acts, kAIMaxSignalActs);
   }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The new SignalResponse dialog, with a list and sub-dialogs rather than a single huge one
//

static sFieldDesc gSignalHeaderFieldDesc[] =
{
    { "Signal", kFieldTypeString, FieldLocation(sAISignalResponse, szSignal)                                                    }, 
    { "Priority", kFieldTypeEnum,   FieldLocation(sAISignalResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },
};
static sStructDesc gSignalHeaderStructDesc = StructDescBuild(sAISignalResponse, kStructFlagNone, gSignalHeaderFieldDesc);

IStructEditor* NewSignalResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Signal response", kAIMaxSignalActs, &gSignalHeaderStructDesc, data, ((sAISignalResponse*) data)->acts);
}

#endif
