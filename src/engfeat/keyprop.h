/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/keyprop.h,v 1.6 2000/01/29 13:19:43 adurant Exp $
// this is implemented in lockprop.cpp
#pragma once

#ifndef __KEYPROP_H
#define __KEYPROP_H

#include <objtype.h>
#include <property.h>

//
// Key Info Properties
// 
// Src is the key, Dst is the lock
// since only the script client cares, here they are

#define PROP_KEYSRC_NAME "KeySrc"
#define PROP_KEYDST_NAME "KeyDst"

typedef struct {
   bool   master_bit;
   uint region_mask;
   uchar  lock_id;
} sKeyInfo;

#undef INTERFACE
#define INTERFACE IKeyInfoProperty
DECLARE_PROPERTY_INTERFACE(IKeyInfoProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sKeyInfo*); 
}; 
#undef INTERFACE


EXTERN IKeyInfoProperty *KeySrcProp;
EXTERN IKeyInfoProperty *KeyDstProp;

// does this key property data work on this lock property data
EXTERN BOOL KeyWorksOnLock(sKeyInfo *key, sKeyInfo *lock);

// searches through a container (probably a creature, to be fair)
//  looking for a key which can open door door
// OBJ_NULL if it doesnt find one
EXTERN ObjID FindUsefulKey(ObjID container, ObjID door);

#endif // __KEYPROP_H
