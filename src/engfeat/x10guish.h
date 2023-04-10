/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/x10guish.h,v 1.4 2000/01/31 09:45:58 adurant Exp $
#pragma once

#ifndef __X10GUISH_H
#define __X10GUISH_H

#include <objtype.h>

////////////////////////////////////////////////////////////
// "Men must be indulged or extinguished, for they avenge small offenses..."
//    - Machiavelli

#define PROP_X10GUISH_NAME "ParticleType"

EXTERN BOOL ObjExtinguishable(ObjID obj, struct Label** ppModel);
EXTERN void ObjSetExtinguishable(ObjID obj, struct Label* pModel);

EXTERN void ExtinguishablePropInit(void);


#endif // __X10GUISH_H
