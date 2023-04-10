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

// This is for stupid static arrays that want an upper bound on the number of objects there could possibly be
// @TODO: get rid of this.
#define HACK_MAX_OBJ     4096



#endif // OSYSBASE__H
