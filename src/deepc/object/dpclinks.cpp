/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <objtype.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <label.h>

#include <dpclinks.h>

IRelation* g_pReplicatorLinks = NULL;
IRelation* g_pMutateLinks     = NULL;

void DPCLinksInit(void)
{
// @NOTE:  Since I don't know whether Deep Cover has special links, I'm going to leave this as sample code. - Bodisafa
#if 0
   sRelationDesc replicatorRelationDesc  = { "Replicator", 0, 0, 0 };
   sRelationDataDesc noDataDesc          = { "None", 0 };

   g_pReplicatorLinks = CreateStandardRelation(&replicatorRelationDesc, &noDataDesc, kQCaseSetSourceKnown);

   sRelationDesc mutateRelationDesc  = { "Mutate", 0, 0, 0 };

   g_pMutateLinks = CreateStandardRelation(&mutateRelationDesc, &noDataDesc, kQCaseSetSourceKnown);

   sRelationDesc scriptDesc = { "ScriptParams" };
   sRelationDataDesc scriptDataDesc = LINK_DATA_DESC(Label);
   IRelation* pRelation = CreateStandardRelation(&scriptDesc, &scriptDataDesc, kQCaseSetSourceKnown|kQCaseSetDestKnown);
   SafeRelease(pRelation);
#endif // 0
}

void DPCLinksTerm(void)
{
   SafeRelease(g_pReplicatorLinks);
   SafeRelease(g_pMutateLinks);
}
