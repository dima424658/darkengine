/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Dead model name property header
//
#pragma once

#ifndef __DPCDMODPROP_H
#define __DPCDMODPROP_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // ! _OBJTYPE_H

#define PROP_DEATHMODELNAME_NAME "DeathModelName"

EXTERN void DeathModelNamePropInit(void);
EXTERN BOOL ObjGetDeathModelName(ObjID obj, char *name);
EXTERN void ObjSetDeathModelName(ObjID obj, char *name);

#endif   // __DPCDMODPROP_H
