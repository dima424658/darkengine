/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/invlimb.h,v 1.1 1998/05/22 00:42:01 mahk Exp $
#pragma once  
#ifndef __INVLIMB_H
#define __INVLIMB_H

#include <objtype.h>
//
// Limb model stuff
// 

EXTERN struct Label *invGetLimbModelName(ObjID o);

EXTERN void invLimbInit(void);
EXTERN void invLimbTerm(void); 


#endif // __INVLIMB_H
