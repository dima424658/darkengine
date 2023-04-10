/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/idispatc.h,v 1.3 1998/09/30 14:12:30 mahk Exp $
#pragma once  
#ifndef __IDISPATC_H
#define __IDISPATC_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// GENERIC "DISPATCHER" INTERFACE
//

// Forward-declared types (Defined in dspchdef.h)
typedef struct sDispatchMsg sDispatchMsg; 
typedef struct sDispatchListenerDesc sDispatchListenerDesc; 

#undef INTERFACE
#define INTERFACE IMessageDispatch
DECLARE_INTERFACE_(IMessageDispatch,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Listen for Dispatch messages. 
   // Returns E_FAIL if the specified desc uses an existing GUID
   // 
   STDMETHOD(Listen)(THIS_ const sDispatchListenerDesc* desc) PURE; 

   //
   // Stop listening
   // Returns S_FALSE if the specified GUID is not present 
   //
   STDMETHOD(Unlisten)(THIS_ const GUID* listener) PURE; 

   //
   // Dispatch a message to all listeners
   //
   STDMETHOD(SendMsg)(THIS_ const sDispatchMsg* msg) PURE;

}; 

#define IMessageDispatch_Listen(p,a)       COMCall1(p, Listen, a)


// Macro for interfaces that want to derive off of IMessageDispatch
#define DECLARE_DISPATCH_PURE() \
   STDMETHOD(Listen)(THIS_ const sDispatchListenerDesc* desc) PURE; \
   STDMETHOD(Unlisten)(THIS_ const GUID* listener) PURE;    \
   STDMETHOD(SendMsg)(THIS_ const sDispatchMsg* msg) PURE // need final semi

#endif // __IDISPATC_H


