/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Source: x:/prj/tech/libsrc/darkloop/RCS/loopapi.h $
// $Author: CCAROLLO $
// $Date: 1998/10/05 08:33:09 $
// $Revision: 1.21 $
//

#ifndef __LOOPAPI_H
#define __LOOPAPI_H

#include <comtools.h>
#include <looptype.h>
#include <loopguid.h>

F_DECLARE_INTERFACE(ILoopClient);
F_DECLARE_INTERFACE(ILoopClientFactory);
F_DECLARE_INTERFACE(ILoopMode);
F_DECLARE_INTERFACE(ILoopDispatch);
F_DECLARE_INTERFACE(ILoopManager);
F_DECLARE_INTERFACE(ILoop);

///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

EXTERN const char * LGAPI LoopGetMessageName(eLoopMessage);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ILoopClientLoop client interface
//

#define kVerLoopClient 1
// Version 1:
//      Original interface

#undef INTERFACE
#define INTERFACE ILoopClient

DECLARE_INTERFACE_(ILoopClient, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Get the version of the loop client interface
   //
   STDMETHOD_(short, GetVersion)(THIS) PURE;

   //
   // Get the loop client information
   //
   STDMETHOD_(const sLoopClientDesc *, GetDescription)(THIS) PURE;

   //
   // Handle a message from the owning loop mode dispatcher
   //
   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(THIS_ eLoopMessage, tLoopMessageData hData) PURE;

};

#define ILoopClient_QueryInterface(p, a, b) COMQueryInterface(p, a, b)
#define ILoopClient_AddRef(p)               COMAddRef(p)
#define ILoopClient_Release(p)              COMRelease(p)
#define ILoopClient_GetVersion(p)           COMCall1(p, GetVersion)
#define ILoopClient_GetDescription(p)       COMCall1(p, GetDescription)
#define ILoopClient_ReceiveMessage(p, a, b) COMCall1(p, ReceiveMessage, a, b)


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ILoopClientFactory
//

DECLARE_INTERFACE_(ILoopClientFactory, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

    //
    // Get the version of the loop client interface
    //
   STDMETHOD_(short, GetVersion)(THIS) PURE;

   //
   // Query what kinds of clients the factory makes as a NULL terminated list
   //
   STDMETHOD_(tLoopClientID **, QuerySupport)(THIS) PURE;
   STDMETHOD_(BOOL, DoesSupport)(THIS_ tLoopClientID *) PURE;

   //
   // Find/create a client
   //
   STDMETHOD (GetClient)(THIS_ tLoopClientID *, tLoopClientData, ILoopClient **) PURE;

};


//////////////////////////////////////////////////
//
// Simple factory creation tool
//

//
// Create a factory from a null-terminated array of descriptors
//

EXTERN ILoopClientFactory * LGAPI CreateLoopFactory(const sLoopClientDesc** descs);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ILoopMode
//

///////////////////////////////////////
//
// Loop mode API
//

//
// Create a loop mode factory, given description.  If using ILoopManager,
// you probably will not need to use this function.
//

EXTERN ILoopMode * LGAPI _LoopModeCreate(const sLoopModeDesc *);


///////////////////////////////////////
//
//
// ILoopMode creates the client set for a loopmode
//

#undef INTERFACE
#define INTERFACE ILoopMode

DECLARE_INTERFACE_(ILoopMode, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Get the info on the loop mode
   //
   STDMETHOD_(const sLoopModeName *, GetName)(THIS) PURE;

   //
   // Create the mode dispatch chain
   //
   STDMETHOD (CreateDispatch)(THIS_ sLoopModeInitParmList, ILoopDispatch** ) PURE;

   //
   // Create a dispatch chain that only handles certain messages
   //
   STDMETHOD (CreatePartialDispatch)(THIS_ sLoopModeInitParmList, tLoopMessageSet, ILoopDispatch** ) PURE;

   //
   // Describe this mode
   //
   STDMETHOD_(const sLoopModeDesc*, Describe)(THIS) PURE;

};

#define ILoopMode_QueryInterface(p, a, b)     COMQueryInterface(p, a, b)
#define ILoopMode_AddRef(p)                   COMAddRef(p)
#define ILoopMode_Release(p)                  COMRelease(p)
#define ILoopMode_GetName(p)                  COMCall0(p, GetName)
#define ILoopMode_Describe(p)                 COMCall0(p, Describe)
#define ILoopMode_GetDispatch(p,a,b)          COMCall2(p, GetDispatch, a, b)
#define ILoopMode_GetPartialDispatch(p,a,b,c) COMCall3(p, GetPartialDispatch, a, b, c)

////////////////////////////////////////////////////////////
//
// INTERFACE: ILoopDispatch
//
// ILoopDispatch provides a message-based dispatch system specification.
//

#undef INTERFACE
#define INTERFACE ILoopDispatch

DECLARE_INTERFACE_(ILoopDispatch, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Send a message down/up the dispatch chain
   //
   STDMETHOD (SendMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) PURE;

   //
   // Send a message forward down the dispatch chain with no data
   //
   STDMETHOD (SendSimpleMessage)(THIS_ eLoopMessage) PURE;

   //
   // Post a message to be sent down/up the dispatch chain
   // the next time ProcessQueue() is called
   //
   STDMETHOD (PostMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) PURE;

    //
    // Post a message to be sent forward down the dispatch chain with no data
    // the next time ProcessQueue() is called
    //
   STDMETHOD (PostSimpleMessage)(THIS_ eLoopMessage) PURE;

   //
   // Send all posted messages in queue
   //
   STDMETHOD (ProcessQueue)(THIS) PURE;

   //
   // Describe my instantiaion
   //
   STDMETHOD_ (const sLoopModeName*, Describe)(THIS_  sLoopModeInitParmList* list) PURE;

#ifndef SHIP
   //
   // Set/Get the diagnostic mode
   //
   STDMETHOD_(void, SetDiagnostics)(THIS_ unsigned fDiagnostics, tLoopMessageSet messages) PURE;
   STDMETHOD_(void, GetDiagnostics)(THIS_ unsigned * pfDiagnostics, tLoopMessageSet * pMessages) PURE;
   
   // Set messages and optional client to use the profileable dispatcher
   STDMETHOD_(void, SetProfile)(THIS_ tLoopMessageSet messages, tLoopClientID *) PURE;
   STDMETHOD_(void, GetProfile)(THIS_ tLoopMessageSet * pMessages, tLoopClientID **) PURE;

   STDMETHOD_(void, ClearTimers)(void) PURE;
   STDMETHOD_(void, DumpTimerInfo)(void) PURE;
#endif
};

#define ILoopDispatch_QueryInterface(p, a, b)   COMQueryInterface(p, a, b)
#define ILoopDispatch_AddRef(p)                 COMAddRef(p)
#define ILoopDispatch_Release(p)                COMRelease(p)
#define ILoopDispatch_SendMessage(p,a,b,c)      COMCall3(p, SendMessage)
#define ILoopDispatch_SendSimpleMessage(p,a)    COMCall1(p, SendSimpleMessage, a)
#define ILoopDispatch_PostMessage(p,a,b,c)      COMCall3(p, PostMessage, a, b,c)
#define ILoopDispatch_PostSimpleMessage(p, a)   COMCall1(p, PostSimpleMessage, a)
#define ILoopDispatch_ProcessQueue(p)           COMCall0(p, ProcessQueue)
#define ILoopDispatch_Describe(p, a)            COMCall1(p, Describe, a)


///////////////////////////////////////////////////////////////////////////////
//
// Loop management API
//

//
// Create a loop manmager and add it to the global app-object
//

#define LoopManagerCreate() \
_LoopManagerCreate(IID_TO_REFIID(IID_ILoopManager), NULL, AppGetObj(IUnknown), 0)

   //
   // Creates a loop manager, aggregating it with specfied pOuter,
   // use IAggregate protocol if ppLoopManager is NULL, else self-init
   //

   EXTERN tResult LGAPI _LoopManagerCreate(REFIID, ILoopManager ** ppLoopManager, IUnknown * pOuter, unsigned nMaxModes);


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ILoopManager
//
// ILoopManager provides high level loop client and loopmode management
// specifications.
//

#undef INTERFACE
#define INTERFACE ILoopManager

DECLARE_INTERFACE_(ILoopManager, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

    //
    // Add a client
    //
   STDMETHOD (AddClient)(THIS_ ILoopClient *, ulong * pCookie) PURE;

   //
   // Remove a client
   //
   STDMETHOD (RemoveClient)(THIS_ ulong cookie) PURE;

   //
   // Add a client factory
   //
   STDMETHOD (AddClientFactory)(THIS_ ILoopClientFactory *, ulong * pCookie) PURE;

   //
   // Remove a client factory
   //
   STDMETHOD (RemoveClientFactory)(THIS_ ulong cookie) PURE;

   //
   // Find/create a client
   //
   STDMETHOD (GetClient)(THIS_ tLoopClientID *, tLoopClientData, ILoopClient **) PURE;

   //
   // Add a mode
   //
   STDMETHOD (AddMode)(THIS_ const sLoopModeDesc *) PURE;

   //
   // Get a mode
   //
   STDMETHOD_(ILoopMode*, GetMode)(THIS_ tLoopModeID* ) PURE;

   //
   // Remove a mode
   //
   STDMETHOD (RemoveMode)(THIS_ tLoopModeID *) PURE;

   //
   // Set/Get the elements shared by all modes
   //
   STDMETHOD (SetBaseMode)(THIS_ tLoopModeID *) PURE;
   STDMETHOD_(ILoopMode*, GetBaseMode)(THIS) PURE;

};

#define ILoopManager_QueryInterface(p, a, b)        COMQueryInterface(p, a, b)
#define ILoopManager_AddRef(p)                      COMAddRef(p)
#define ILoopManager_Release(p)                     COMRelease(p)
#define ILoopManager_AddClient(p, a, b)             COMCall2(p, AddClient, a, b)
#define ILoopManager_RemoveClient(p, a)             COMCall1(p, RemoveClient, a)
#define ILoopManager_AddClientFactory(p, a, b)      COMCall2(p, AddClientFactory, a, b)
#define ILoopManager_RemoveClientFactory(p, a)      COMCall1(p, RemoveClientFactory, a)
#define ILoopManager_GetClient(p, a, b)             COMCall2(p, GetClient, a, b)
#define ILoopManager_AddMode(p, a)                  COMCall1(p, AddMode, a)
#define ILoopManager_GetMode(p, a)                  COMCall1(p, GetMode, a)
#define ILoopManager_RemoveMode(p, a)               COMCall1(p, RemoveMode, a)
#define ILoopManager_SetBaseMode(p, a)              COMCall1(p, SetBaseMode, a)
#define ILoopManager_GetBaseMode(p)                 COMCall0(p, GetBaseMode)


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ILoop
//
// ILoop provides high level loop
//

#undef INTERFACE
#define INTERFACE ILoop

DECLARE_INTERFACE_(ILoop, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Run the loop
   //
   STDMETHOD_(int, Go)(THIS_ sLoopInstantiator* loop) PURE;

   //
   // Unwind stack and exit loop at end of current loop iteration
   //
   STDMETHOD (EndAllModes)(THIS_ int goRetVal) PURE;

   //
   // Clean up all outstanding loops and prepare to shut down the app.
   // Only intended to be used for cleaning up state right before calling exit();
   //
   STDMETHOD(Terminate)(THIS) PURE;

   //
   // Query for current frame info. Values undefined if not actually looping
   //
   STDMETHOD_(const sLoopFrameInfo *, GetFrameInfo)(THIS) PURE;

   //
   // Change mode at end of current loop iteration
   //
   STDMETHOD (ChangeMode)(THIS_ eLoopModeChangeKind, sLoopInstantiator* loop) PURE;

   //
   // End current mode at end of current loop iteration,
   // exit loop if current mode is bottom of stack
   //
   STDMETHOD (EndMode)(THIS_ int goRetVal) PURE;

   //
   // Get the currently running mode
   //
   STDMETHOD_(ILoopMode *, GetCurrentMode)(THIS) PURE;

   //
   // Get the current mode's dispatcher
   //
   STDMETHOD_(ILoopDispatch*, GetCurrentDispatch)(THIS) PURE;

   //
   // Pause/unpause the game
   //
   STDMETHOD_(void, Pause)(THIS_ BOOL) PURE;
   STDMETHOD_(BOOL, IsPaused)(THIS) PURE;

   //
   // Change the minor mode
   //
   STDMETHOD (ChangeMinorMode)(THIS_ int minorMode) PURE;
   STDMETHOD_(int, GetMinorMode)(THIS) PURE;

   //
   // Convenience functions to dispatch messsages to current loop mode dispatcher
   //
   STDMETHOD (SendMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) PURE;
   STDMETHOD (SendSimpleMessage)(THIS_ eLoopMessage) PURE;
   STDMETHOD (PostMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags) PURE;
   STDMETHOD (PostSimpleMessage)(THIS_ eLoopMessage) PURE;
   STDMETHOD (ProcessQueue)(THIS) PURE;

   //
   // Debugging features
   //
#ifndef SHIP
   STDMETHOD_(void, SetDiagnostics)(THIS_ unsigned fDiagnostics, tLoopMessageSet messages) PURE;
   STDMETHOD_(void, GetDiagnostics)(THIS_ unsigned * pfDiagnostics, tLoopMessageSet * pMessages) PURE;
   
   // Set messages and optional client to use the profileable dispatcher
   STDMETHOD_(void, SetProfile)(THIS_ tLoopMessageSet messages, tLoopClientID *) PURE;
   STDMETHOD_(void, GetProfile)(THIS_ tLoopMessageSet * pMessages, tLoopClientID **) PURE;
#endif

};

#define ILoop_QueryInterface(p, a, b)    COMQueryInterface(p, a, b)
#define ILoop_AddRef(p)                  COMAddRef(p)
#define ILoop_Release(p)                 COMRelease(p)
#define ILoop_Go(p, a)                   COMCall1(p, Go, a)
#define ILoop_Exit(p, a)                 COMCall1(p, Exit, a)
#define ILoop_GetFrameInfo(p)            COMCall0(p, GetFrameInfo)
#define ILoop_ChangeMode(p, a, b)        COMCall2(p, ChangeMode, a, b)
#define ILoop_EndMode(p, a)              COMCall1(p, EndMode, a)
#define ILoop_Pause(p, a)                COMCall1(p, Pause, a)
#define ILoop_GetCurrentMode(p)          COMCall0(p, GetCurrentMode)
#define ILoop_IsPaused(p)                COMCall0(p, IsPaused)
#define ILoop_ChangeMinorMode(p, a)      COMCall1(p, ChangeMinorMode, a)
#define ILoop_GetMinorMode(p)            COMCall0(p, GetMinorMode)
#define ILoop_SendMessage(p, a, b, c)    COMCall3(p, SendMessage, a, b, c)
#define ILoop_SendSimpleMessage(p, a)    COMCall1(p, SendSimpleMessage, a)
#define ILoop_PostMessage(p, a, b, c)    COMCall3(p, PostMessage, a, b, c)
#define ILoop_PostSimpleMessage(p, a)    COMCall1(p, PostSimpleMessage, a)
#define ILoop_ProcessQueue(p)            COMCall0(p, ProcessQueue)
#define ILoop_SetDiagnostics(p, a, b)    COMCall2(p, SetDiagnostics, a, b)
#define ILoop_GetDiagnostics(p, a, b)    COMCall2(p, GetDiagnostics, a, b)
#define ILoop_SetProfile(p, a, b)        COMCall2(p, SetProfile, a, b)
#define ILoop_GetProfile(p, a, b)        COMCall2(p, GetProfile, a, b)

///////////////////////////////////////////////////////////////////////////////
//
// Stock client/factory implementations
//

///////////////////////////////////////////////////////////////////////////////
//
// Convenience templates for C++ based loop clients
//
// Using this, one need only implement ReceiveMessage()
//

#ifdef __cplusplus

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cLoopClient
//

template <int FLAGS>
class cLoopClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, FLAGS> 
{
public:
   cLoopClient(const sLoopClientDesc * pDesc)
    : m_pLoopClientDesc(pDesc)
   {
   }
   
   // Get the version of the loop client interface
   STDMETHOD_(short, GetVersion)()
   {
        return kVerLoopClient;
   }
   
   // Get the loop client information
   STDMETHOD_(const sLoopClientDesc *, GetDescription)()
   {
     return m_pLoopClientDesc;
   }

   // Handle a message from the owning loop mode dispatcher, derived must implement
   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData) PURE;

private:
   const sLoopClientDesc * const m_pLoopClientDesc;
};

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Convenience wrapper for simple C based loop clients.  Less efficient
// then creating ILoopClient implementor yourself, but really convenient
// for moving C over to interface based loop system.
//

typedef eLoopMessageResult (LGAPI * tLoopClientReceiveMessageFunc)(void * pContext, eLoopMessage, tLoopMessageData hData);


EXTERN ILoopClient * LGAPI
CreateSimpleLoopClient(tLoopClientReceiveMessageFunc pCallback,
                       void *                        pContext,
                       const sLoopClientDesc *       pClientDesc);


///////////////////////////////////////////////////////////////////////////////

#endif /* !__LOOPAPI_H */
