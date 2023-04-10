/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/rcs/reshack.h 1.1 1998/07/20 10:27:56 JUSTIN Exp $
//
// Resource Hacks
//
// This defines the interface for the hacks we've put into stdres to
// support Dark for the time being...
//

#ifndef _RESHACK_H
#pragma once
#define _RESHACK_H

//////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResHack
//
// These are methods in the interface to get Dark out the door quickly.
// They SHOULD NOT be used by later systems. They will hopefully be removed
// from the API and from stdres as soon as feasible.
//

F_DECLARE_INTERFACE(IResHack);

#undef INTERFACE
#define INTERFACE IResHack

DECLARE_INTERFACE_(IResHack, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Assign the given data as the "real" data for this resource.
   //
   // This takes a pointer to some arbitrary memory, managed by an outside
   // module. It will use that pointer, from now on, when returning
   // information about this resource. The resource must be Unlocked when
   // this is called. Memory management on the given data is entirely the
   // responsibility of the caller. No reference counting is maintained in
   // this case.
   //
   // This particular bit of hackery is principally here to support the
   // Dark engine, which gets an image resource, then mipmaps it, and
   // "assigns" the mipmap as the new version of the resource. It is
   // intended solely for the short term; this model should NOT be emulated.
   // Instead, later systems should create custom resource types for cases
   // like this. When practicable, this method will be removed.
   //
   STDMETHOD_(void, SetData)(THIS_ void * pNewData) PURE;
   STDMETHOD_(BOOL, HasSetData)(THIS) PURE;
};

#define IResHack_SetData(p, a)                 COMCall1(p, SetData, a)
#define IResHack_HasSetData(p)                 COMCall0(p, HasSetData)

#endif
