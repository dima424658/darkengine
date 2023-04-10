/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __AISUSLNK_H
#define __AISUSLNK_H
#include <objtype.h>
#include <comtools.h>
#include <linkbase.h>

////////////////////////////////////////////////////////////////
//  "SUSPICIOUS" API FOR AI'S KEEPING TRACK OF SUSPICIOUS THINGS
//

//The idea is that when an AI sees something "suspicious", it creates
//a suspicious link to that item.  As long as this link is active, the
//AI will no longer react to that suspicious item.  The AI tests the
//item when it can see it and it's timer comes up (3-4 seconds at the
//moment).  If the item is not suspicious, but there is a link to it,
//then it removes the link, so that when the item becomes suspicious
//again, it will notice again.   

F_DECLARE_INTERFACE(IRelation);

void AIInitSuspiciousLinks();
void AITermSuspiciousLinks();
BOOL AIClearSuspiciousLinks(ObjID viewer, ObjID viewee=LINKOBJ_WILDCARD);
BOOL AIGetSingleCuspiciousLink(ObjID viewer, ObjID viewee, LinkID *newlink);
BOOL AIGetSuspiciousLinkTime(LinkID thelink, ulong *thetime);
BOOL AIIsSuspiciousOfObj(ObjID viewer, ObjID viewee);
BOOL AICreateSuspiciousLink(ObjID viewer, ObjID viewee);

#endif // __AISUSLNK_H
