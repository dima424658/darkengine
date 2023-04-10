/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptnet.h,v 1.1 1999/01/20 18:46:36 JUSTIN Exp $
//
// ObjScript Networking Interface
//
// If a client of scripts is networked, and can have objects distributed
// amongst multiple players, then it should implement this interface and
// place it in the appagg. This defines the functionality that scripting
// needs in order to deal reasonably with distributed objects.
//

#ifndef __SCRPTNET_H
#define __SCRPTNET_H

#include <lg.h>
#include <comtools.h>
#include <multparm.h>

F_DECLARE_INTERFACE(IScriptNet);
typedef int ObjID;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IScriptNet
//

#undef INTERFACE
#define INTERFACE IScriptNet

DECLARE_INTERFACE_(IScriptNet, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Is the specified object a proxy for an object on another machine?
   //
   STDMETHOD_(BOOL, ObjIsProxy)(THIS_ ObjID obj) PURE;

   //
   // Post this message to the object on its owner's machine.
   //
   STDMETHOD(PostToOwner)(THIS_ ObjID from, 
                          ObjID to, 
                          const char *msg,
                          const cMultiParm &data,
                          const cMultiParm &data2,
                          const cMultiParm &data3) PURE;
};

#endif // !__SCRPTNET_H
