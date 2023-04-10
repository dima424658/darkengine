/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propnet.h,v 1.1 1998/10/08 16:37:04 MROWLEY Exp $
#pragma once  
#ifndef __PROPNET_H
#define __PROPNET_H

////////////////////////////////////////////////////////////
// PROPERTY NETWORKING API 
//

#undef INTERFACE 
#define INTERFACE IPropertyNetworking
DECLARE_INTERFACE_(IPropertyNetworking,IUnknown)
{
   //
   // Standard IUnknown Stuff
   //
   DECLARE_UNKNOWN_PURE(); 

   //
   // Send network message regarding a change to this property.  If type's 
   // kListenPropRequest bit is set, then this will send a change request of to obj's
   // host machine, otherwise it will send a broadcast of a change that has happened.
   // Returns FALSE when it is already processing a receive for this property.
   //
   STDMETHOD_(BOOL,SendPropertyMsg)(ObjID obj, sDatum value, ePropertyListenMsg type) PURE;

   // 
   // Process an incoming message regarding this property.
   //
   STDMETHOD_(void,ReceivePropertyMsg)(const sNetMsg_Generic *pMsg, ulong size, ObjID fromPlayer) PURE;
}; 

#endif // __PROPNET_H
