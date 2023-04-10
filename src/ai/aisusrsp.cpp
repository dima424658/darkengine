/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


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
#include <aisusrsp.h>
#include <susprop.h>
#include <str.h>
#include <psdialog.h>

#include <dbmem.h>

#define SUSPICIOUSOBJECTID "SuspiciousObjectID"
#define SUSPICIOUSOBJECTTYPE "SuspiciousObjectType"

F_DECLARE_INTERFACE(IAISuspiciousResponseProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Suspicion response property
//

#define kAIMaxSuspiciousActs 16

struct sAISuspiciousResponse
{
   eAIPriority   priority;
   DWORD         reserved[4];
   sAIPsdScrAct  acts[kAIMaxSuspiciousActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAISuspiciousResponseProperty
DECLARE_PROPERTY_INTERFACE(IAISuspiciousResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISuspiciousResponse *);
};

static IAISuspiciousResponseProperty * g_pAISuspiciousResponseProperty;

///////////////////

static sPropertyDesc _g_SuspiciousResponsePropertyDesc = 
{
   "AI_SuspRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RESPONSE_CAT, "Suspicious response" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAISuspiciousResponseProperty, &IID_IAISuspiciousResponseProperty, sAISuspiciousResponse *, cHashPropertyStore< cClassDataOps<sAISuspiciousResponse> > > cAISuspiciousResponsePropertyBase;

class cAISuspiciousResponseProperty : public cAISuspiciousResponsePropertyBase
{
public:
   cAISuspiciousResponseProperty()
      : cAISuspiciousResponsePropertyBase(&_g_SuspiciousResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAISuspiciousResponse);
};

///////////////////

#define SUSPICIOUS_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAISuspiciousResponse, acts[i-1])

static sFieldDesc _g_AISuspiciousResponseFieldDesc[] =
{
   { "Priority", kFieldTypeEnum,    FieldLocation(sAISuspiciousResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   SUSPICIOUS_RESPONSE_FIELDS(1),
   SUSPICIOUS_RESPONSE_FIELDS(2),
   SUSPICIOUS_RESPONSE_FIELDS(3),
   SUSPICIOUS_RESPONSE_FIELDS(4),
   SUSPICIOUS_RESPONSE_FIELDS(5),
   SUSPICIOUS_RESPONSE_FIELDS(6),
};

static sStructDesc _g_AISuspiciousResponseStructDesc = StructDescBuild(sAISuspiciousResponse, kStructFlagNone, _g_AISuspiciousResponseFieldDesc);

///////////////////////////////////////

#define AIGetSuspiciousResponse(obj) AIGetProperty(g_pAISuspiciousResponseProperty, obj, (sAISuspiciousResponse *)NULL)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSuspiciousResponseAbility(IAIManager *)
{
   StructDescRegister(&_g_AISuspiciousResponseStructDesc);
   
   g_pAISuspiciousResponseProperty = new cAISuspiciousResponseProperty;
   return TRUE;
}

///////////////////////////////////////

BOOL AITermSuspiciousResponseAbility()
{
   SafeRelease(g_pAISuspiciousResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISuspiciousResponse
//

cAISuspiciousResponse::cAISuspiciousResponse()
 : cAITriggeredPScripted(kAIT_2Hz)
{
}

///////////////////////////////////////

cAISuspiciousResponse::~cAISuspiciousResponse()
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAISuspiciousResponse::GetName()
{
   return "Suspicious response ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISuspiciousResponse::Init()
{
   cAITriggeredPScripted::Init();
   SetNotifications(kAICN_FoundSuspicious);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISuspiciousResponse::OnFoundSuspicious(ObjID object)
{
  const sAISuspiciousResponse * pResponse = AIGetSuspiciousResponse(GetID());
  if (pResponse)
    {
      sAIPsdScrAct newacts[kAIMaxSuspiciousActs];
      int i,j,k;
      char susptype[32];
      char suspid[32];
      if (!GetSuspiciousType(object,susptype,32))
         strcpy(susptype,"Generic"); //make it Generic
      //iterate over arguments to pseudoscripts, replacing tags with
      //useful information 
      memcpy(newacts,pResponse->acts,sizeof(newacts));
      for (i = 0; i<kAIMaxSuspiciousActs; i++)
      {
         for (j = 0; j < (sizeof(newacts[i].args)/sizeof(newacts[i].args[0])); j++)
         {
            cAnsiStr newstring = newacts[i].args[j];
            int result;
            result = newstring.Find(SUSPICIOUSOBJECTTYPE);
            if (result != -1) //found
            {
               newstring.Remove(result,strlen(SUSPICIOUSOBJECTTYPE));
               newstring.Insert(susptype,result);
               strcpy((char *)newacts[i].args[j],newstring);
            }
            //yeah we don't reset newstring... but this allows you
            //to have string AND obj in same one.  
            result = newstring.Find(SUSPICIOUSOBJECTID);
            if (result != -1) //found
            {
               newstring.Remove(result,strlen(SUSPICIOUSOBJECTID));
               sprintf(suspid,"%d",object);
               newstring.Insert(suspid,result);
               strcpy((char *)newacts[i].args[j],newstring);
            }
         }
      }
      SetTriggered(pResponse->priority, newacts, kAIMaxSuspiciousActs);
    }
}
   
///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

//  The alert response pseudo-script dialog
//
static sFieldDesc gSuspiciousHeaderFieldDesc[] =
{
   { "Priority", kFieldTypeEnum, FieldLocation(sAISuspiciousResponse, priority), kFieldFlagNone, 0, FieldNames(g_ppszAIPriority) },
};
static sStructDesc gSuspiciousHeaderStructDesc = StructDescBuild(sAISuspiciousResponse, kStructFlagNone, gSuspiciousHeaderFieldDesc);

IStructEditor* NewSuspiciousResponseDialog (void* data)
{
   return NewPseudoScriptDialog ("Suspicious response", kAIMaxSuspiciousActs, &gSuspiciousHeaderStructDesc, data, ((sAISuspiciousResponse*) data)->acts);
}

#endif


