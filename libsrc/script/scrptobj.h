/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptobj.h,v 1.1 1999/11/02 16:21:57 Justin Exp $
//
// Primitive "object" types for the script system
//

#ifndef __SCRPTOBJ_H
#define __SCRPTOBJ_H

#pragma once

// The core object type
typedef int ObjID;

//////////
//
// Simple object class, so that cMultiParm can have a separate concept of
// object and int
//
class cObj 
{
protected:
   ObjID id;

public:
   void Set(ObjID o) { id = o; }
   ObjID Get() const { return id; }

   BOOL operator ==(const cObj & o) { return o.id == id; }
};

#endif  // !__SCRPTOBJ_H
