/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/osysbase.h,v 1.10 2000/02/24 23:41:00 mahk Exp $
#pragma once

#ifndef OSYSBASE__H
#define OSYSBASE__H

#include <objtype.h>
#include <objdef.h>
#include <darklimit.h>

////////////////////////////////////////////////////////////
// OBJECT SYSTEM INTERNALS
//

//
// ObjID macros
//



// Min and max objs
// #define MIN_OBJS (gMinObjID)     // abstract objects can be negative 
// #define MAX_OBJS (gMaxObjID)      

EXTERN ObjID gMinObjID;
EXTERN ObjID gMaxObjID;

//
// GLOBALS
//
EXTERN ubyte* BaseActiveArray; // Active array starting at the negative values
#define ObjActiveMask(obj) (1 << ((obj) % 8))
#define ObjActiveIdx(obj) ((obj)/8)
#define BaseActiveElem(obj) (BaseActiveArray[ObjActiveIdx((obj) - gMinObjID)])
#define BaseActiveMask(obj) (ubyte)ObjActiveMask((obj) - gMinObjID)


//
// Determine whether an object exists
//
#define ObjExists(obj)  (BaseActiveElem(obj) & BaseActiveMask(obj))

// This is for static arrays that want an upper bound on the number of objects
// there could possibly be.  NewDark-authored gamesys files commonly reserve
// more than the retail 4096 concrete IDs (TFix 1.28 requests 8184), so keep
// these legacy portal, physics, lighting, and reaction tables in step with the
// extended object system range.
#define HACK_MAX_OBJ     DARK_LIMIT_RUNTIME_CONCRETE_OBJECT_IDS



#endif // OSYSBASE__H
