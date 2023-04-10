/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mnamprop.h,v 1.3 2000/01/29 13:39:16 adurant Exp $
#pragma once

//
// Model name property header
//

#ifndef __MNAMPROP_H
#define __MNAMPROP_H

#include <objtype.h>

#define PROP_MODELNAME_NAME "ModelName"

EXTERN void ModelNamePropInit(void);
EXTERN void ModelNamePropTerm(void);
EXTERN BOOL ObjGetModelName(ObjID obj, char *name);
EXTERN void ObjSetModelName(ObjID obj, char *name);

#endif   // __MNAMPROP_H
