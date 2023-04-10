/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phdbg.h,v 1.5 2000/01/29 13:32:44 adurant Exp $
//
// Physics debugging tools
//
// Basic idea is that you can turn on debugging for an object/event type pair, including
// turning on all events for a given object or a given event for all objects
//
#pragma once

#ifndef __PHDBG_H
#define __PHDBG_H

#ifdef DBG_ON

#include <lg.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <mprintf.h>
#include <objtype.h>

///////////////////////////////////////////////////////////////////////////////

// Event types
enum ePhysicsDebugFlagsEnum
{
   kPhysicsDebugNone     = 0x00,
   kPhysicsDebugRegister = 0x01,                 // model creation/deletion
   kPhysicsDebugCollide  = 0x02,                  // collisions
   kPhysicsDebugMove     = 0x04,                     // integration info
   kPhysicsDebugLoc      = 0x08,                      // location
   kPhysicsDebugContact  = 0x10,                  // contact events
   kPhysicsDebugAll      = 0xffffffff
};
// spews are mprintfs, debugs are commands
// checks for enabling for object/type pair

#define PHYSICS_REGISTER_SPEW(objID, msg) \
{ \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugRegister))  \
      mprintf msg;  \
}

#define PHYSICS_CONTACT_SPEW(objID, msg) \
{ \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugContact))  \
      mprintf msg;  \
}

#define PHYSICS_COLLIDE_SPEW(objID, msg) \
{ \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugCollide))  \
      mprintf msg;  \
}

#define PHYSICS_COLLIDE_DEBUG(objID, cmd) \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugCollide)) cmd

#define PHYSICS_MOVE_SPEW(objID, msg) \
{ \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugMove))  \
      mprintf msg;  \
}

#define PHYSICS_MOVE_DEBUG(objID, cmd) \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugMove)) cmd

#define PHYSICS_LOC_DEBUG(objID, cmd) \
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugLoc)) cmd

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysicsDebug
//
// the "debug class"
//

class cPhysicsDebug
{
public:

   // set debugging for an object/event pair
   void SetDebugObj(ObjID objID, unsigned flags);

   // are we debugging this object/event pair?
   BOOL TestDebugObj(ObjID objID, unsigned flags);

   // debug event for all objects
   void SetDebugAll(unsigned flags);

private:
   // hey, implement with a hash table!
   typedef cHashTableFunctions<ObjID> cDebugHashFunctions;
   typedef cHashTable<ObjID, unsigned, cDebugHashFunctions> cPhysicsDebugHash;

   cPhysicsDebugHash m_debugHash;
   unsigned          m_debugAll;
};

///////////////////////////////////////////////////////////////////////////////

extern cPhysicsDebug g_PhysicsDebug;

///////////////////////////////////////////////////////////////////////////////

inline void cPhysicsDebug::SetDebugObj(ObjID objID, unsigned flags)
{
   unsigned curFlags;
   if (m_debugHash.Lookup(objID, &curFlags))
      flags |= curFlags;
   m_debugHash.Insert(objID, flags);
}

inline BOOL cPhysicsDebug::TestDebugObj(ObjID objID, unsigned flags)
{
   unsigned val;
   return ((m_debugAll & flags) || (m_debugHash.Lookup(objID, &val) ? (flags & val) : FALSE));
}

inline void cPhysicsDebug::SetDebugAll(unsigned flags)
{
   m_debugAll |= flags;
}

///////////////////////////////////////////////////////////////////////////////

#else

#define PHYSICS_REGISTER_SPEW(objID, msg)
#define PHYSICS_MOVE_SPEW(objID, msg)
#define PHYSICS_MOVE_DEBUG(objID, cmd)
#define PHYSICS_LOC_DEBUG(objID, cmd)
#define PHYSICS_COLLIDE_SPEW(objID, msg)
#define PHYSICS_COLLIDE_DEBUG(objID, cmd)
#define PHYSICS_CONTACT_SPEW(objID, msg)

#endif // DBG_ON

///////////////////////////////////////////////////////////////////////////////
#endif /* !__PHDBG_H */
