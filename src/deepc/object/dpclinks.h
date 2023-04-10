/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCLINKS_H
#define __DPCLINKS_H

#ifndef __COMTOOLS_H
#include <comtools.h>
#endif // !__COMTOOLS_H

F_DECLARE_INTERFACE(IRelation);

EXTERN IRelation * g_pReplicatorLinks;
EXTERN IRelation * g_pMutateLinks;

EXTERN void DPCLinksInit(void);
EXTERN void DPCLinksTerm(void);
#endif // __DPCLINKS_H