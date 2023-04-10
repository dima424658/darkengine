/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdmprp.h,v 1.2 2000/01/31 09:55:38 adurant Exp $
#pragma once

//
// Dead model name property header
//

#ifndef __DMODPROP_H
#define __DMODPROP_H

#include <objtype.h>

#define PROP_DEATHMODELNAME_NAME "DeathModelName"

EXTERN void DeathModelNamePropInit(void);
EXTERN BOOL ObjGetDeathModelName(ObjID obj, char *name);
EXTERN void ObjSetDeathModelName(ObjID obj, char *name);

#endif   // __DMODPROP_H
