/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/RCS/reshelp.h 1.4 1998/10/26 23:46:18 TOML Exp $
//
// Resource Manager Resource Helper Methods
//
// IResManHelper is a side-interface from the Resource Manager into
// resources. It deals with all of the work that needs to be done on
// the Resource Manager side for many of the IRes methods.
//
// This interface is not intended for general consumption; rather,
// it should be used solely by IRes implementations.
//

#ifndef __RESHELP_H
#define __RESHELP_H

#include <lg.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IResManHelper);

#undef INTERFACE
#define INTERFACE IResManHelper

DECLARE_INTERFACE_(IResManHelper, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Lock the Resource -- that is, read it into memory and do any
   // associated management, returning a pointer to the data for the
   // resource.
   //
   STDMETHOD_(void *, LockResource) (THIS_ IRes *) PURE;

   //
   // Really don't look in memory. Grab it from storage.
   //
   STDMETHOD_(void *, ExtractResource) (THIS_ IRes *, void *pBuf) PURE;

   //
   // Returns 0 if not already loaded (not in LRU), else returns raw
   // data. Dangerous to use returned data, since LRU may be threaded,
   // and mem may get freed before you use it, but there you go.  This
   // is a purely passive call. If the RESDATA_... flags are set, then
   // call will fail.
   //
   STDMETHOD_(void *, FindResource) (THIS_ IRes *, long *pSize) PURE;

   //
   //
   //
   STDMETHOD_(void, UnlockResource) (THIS_ IRes *) PURE;

   //
   //
   //
   STDMETHOD_(unsigned, GetResourceLockCount)(THIS_ IRes *) PURE;
   
   //
   // Will drop raw data from memory if resource not currently locked.
   //
   STDMETHOD_(BOOL, DropResource) (THIS_ IRes *) PURE;

   //
   // Return a resource's size. If we don't already know it, calculate it. 
   // Negative on error.
   //
   STDMETHOD_(long, GetResourceSize) (THIS_ IRes *) PURE;

   //
   // Cleans up any info associated with IRes management. 
   // Should be called when IRes is destroyed.
   //
   STDMETHOD_(void, UnregisterResource) (THIS_ 
                                         IRes *pRes,
                                         DWORD ResManData) PURE;

   //
   // Lock (that is, read in and get the data for) the given resource 
   // asynchronously, calling the given callback when it is ready.
   //
   // Priority is a 'prikind.h' priority.
   //
   STDMETHOD_(BOOL, AsyncLock) (THIS_ 
                                IRes *, 
                                int nPriority) PURE;
   //
   // Similar to AsyncLock, but place the data into the given buffer.
   // Memory management on this buffer is up to the caller.
   //
   STDMETHOD_(BOOL, AsyncExtract) (THIS_ 
                                   IRes *, 
                                   int nPriority, 
                                   void *pBuf, 
                                   long bufSize) PURE;
   //
   // Similar to AsyncLock, but doesn't actually lock the Resource, just
   // loads it in on the assumption that it'll get locked Real Soon Now.
   //
   STDMETHOD_(BOOL, AsyncPreload) (THIS_ IRes *) PURE;

   //
   // Returns TRUE if async request has been fulfilled.
   //
   STDMETHOD_(BOOL, IsAsyncFulfilled) (THIS_ IRes *) PURE;

   //
   // Kills async request associated with this resource.
   //
   STDMETHOD(AsyncKill) (THIS_ IRes *) PURE;

   // 
   // Forces synchronous completion of async request. ppResult is as
   // passed back from the ARQ fulfiller.
   //
   STDMETHOD(GetAsyncResult) (THIS_ IRes *, void **ppResult) PURE;
};

#endif
