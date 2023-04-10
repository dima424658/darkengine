/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklinks.cpp,v 1.4 2000/02/19 13:25:36 toml Exp $

#include <objtype.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <label.h>

#include <shklinks.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRelation * g_pReplicatorLinks;
IRelation * g_pMutateLinks;

void ShockLinksInit(void)
{
   sRelationDesc replicatorRelationDesc  = { "Replicator", 0, 0, 0 };
   sRelationDataDesc noDataDesc        = { "None", 0 };

   g_pReplicatorLinks = CreateStandardRelation(&replicatorRelationDesc, &noDataDesc, kQCaseSetSourceKnown);

   sRelationDesc mutateRelationDesc  = { "Mutate", 0, 0, 0 };

   g_pMutateLinks = CreateStandardRelation(&mutateRelationDesc, &noDataDesc, kQCaseSetSourceKnown);

   sRelationDesc scriptDesc = { "ScriptParams" };
   sRelationDataDesc scriptDataDesc = LINK_DATA_DESC(Label);
   IRelation* pRelation = CreateStandardRelation(&scriptDesc, &scriptDataDesc, kQCaseSetSourceKnown|kQCaseSetDestKnown);
   SafeRelease(pRelation);
}

void ShockLinksTerm(void)
{
   SafeRelease(g_pReplicatorLinks);
   SafeRelease(g_pMutateLinks);
}
