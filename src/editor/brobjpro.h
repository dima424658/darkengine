/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brobjpro.h,v 1.2 2000/01/29 13:11:15 adurant Exp $
// expose brush property info to brush/editor system
// mostly for objects which have props which make them behave like other brush types
#pragma once

#ifndef __BROBJPRO_H
#define __BROBJPRO_H

#include <objtype.h>

// returns <0.0 if no light on obj, else 0->n light level
EXTERN float brObjProp_getLightLevel(ObjID o_id);

// returns the br filter code for the object
EXTERN int   brObjProp_getFilterType(ObjID o_id);

#endif  // __BROBJPRO_H
