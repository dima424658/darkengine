/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/objscrt.h,v 1.6 1999/11/02 16:30:28 Justin Exp $
#pragma once  
#ifndef __OBJSCRT_H
#define __OBJSCRT_H

#include <scrptbas.h>

//
// "object type" can be either a string or an id
//
class object : public cObj
{
public:
   object(int i = 0) { id = i; };
   object(const char* name); 
   object(cMultiParm &parm) { id = cObj(parm).Get(); };

   operator long() const { return id;}; 
   operator int() const { return id;}; 
   operator string() const { return string(id); }; 

   bool operator ==(const object& o) { return id == o.id;}; 
   bool operator ==(long o)          { return id == o;}; 
   bool operator !=(const object& o) { return id != o.id;}; 

}; 

#ifndef SCRIPT

#include <objremap.h>

#define PersistentObject(obj)  \
   do {                             \
   Persistent(*(int*)&(obj));       \
   if (Reading())                   \
      obj = ObjRemapOnLoad(obj);    \
   } while(0)
//
// Convert to ObjID, for use by services
// This is obsolete, only there for completeness. 
//
#define ScriptObjID(o) ((long)o)

#endif  // !SCRIPT


#endif // __OBJSCRT_H
