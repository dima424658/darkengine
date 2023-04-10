/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiaware.cpp,v 1.6 2000/02/19 12:17:21 toml Exp $
//
//
//

#include <lg.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiaware.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
//include <aiapisns.h>

///////////////////////////////////////////////////////////////////////////////

IRelation * g_pAIAwarenessLinks;

///////////////////////////////////////

sAIAwareness g_AINullAwarenessScratch;

///////////////////////////////////////////////////////////////////////////////

static const char * g_pszAIAwarenessFlags[] = 
{ 
   "Seen",
   "Heard",
   "CanRaycast",
   "HaveLOS",
   "Blind",
   "Deaf",
   "Highest",
   "FirstHand"
};
 
///////////////////////////////////////////////////////////////////////////////

static sFieldDesc g_AIAwarenessFieldDesc[] =
{
   { "Flags",              kFieldTypeBits,   FieldLocation(sAIAwareness, flags), kFieldFlagUnsigned, 0, FieldNames(g_pszAIAwarenessFlags)},
   { "Level",              kFieldTypeEnum,   FieldLocation(sAIAwareness, level), kFieldFlagNone,     0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Peak Level",         kFieldTypeEnum,   FieldLocation(sAIAwareness, peak),  kFieldFlagNone,     0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Level enter time",   kFieldTypeInt,    FieldLocation(sAIAwareness, time)                                                          },
   { "Time last contact",  kFieldTypeInt,    FieldLocation(sAIAwareness, lastContact)                                                   },
   { "Pos last contact",   kFieldTypeVector, FieldLocation(sAIAwareness, lastPos)                                                       },
   { "Last pulse level",   kFieldTypeEnum,   FieldLocation(sAIAwareness, peak),  kFieldFlagNone,     0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Vision cone",        kFieldTypeInt,    FieldLocation(sAIAwareness, sourceCone)                                                    }, 
   { "Time last update",     kFieldTypeInt,  FieldLocation(sAIAwareness, updateTime)                                                    },
   { "Time last update LOS", kFieldTypeInt,  FieldLocation(sAIAwareness, LOSTime)                                                        },
   { "Last true contact", kFieldTypeInt,  FieldLocation(sAIAwareness, trueLastContact)                                                        },
   { "Freshness", kFieldTypeInt,  FieldLocation(sAIAwareness, freshness)                                                        },

};

///////////////////////////////////////

static sStructDesc g_AIAwarenessStructDesc = StructDescBuild(sAIAwareness, kStructFlagNone, g_AIAwarenessFieldDesc);

///////////////////////////////////////////////////////////////////////////////

void AIInitAwareness()
{
   StructDescRegister(&g_AIAwarenessStructDesc);
   
   sRelationDesc     awarenessDesc  = { "AIAwareness", kRelationNetworkLocalOnly, 0, 0 };
   
   sRelationDataDesc awareDataDesc     = { "sAIAwareness", sizeof(sAIAwareness), kRelationDataAutoCreate };

   g_pAIAwarenessLinks = CreateStandardRelation(&awarenessDesc, &awareDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown|kQCaseSetBothKnown));
}

///////////////////////////////////////

void AITermAwareness()
{
   SafeRelease(g_pAIAwarenessLinks);
}

///////////////////////////////////////////////////////////////////////////////
