/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapi.h,v 1.19 1999/12/09 20:09:02 BODISAFA Exp $
//
// The primary API for external access to the AI
//

#ifndef __AIAPI_H
#define __AIAPI_H

#pragma once

#include <objtype.h>

#ifdef __cplusplus

#include <comtools.h>
#include <simtime.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIManager);
F_DECLARE_INTERFACE(IAI);
F_DECLARE_INTERFACE(IAIBehaviorSet);
F_DECLARE_INTERFACE(IProperty);
F_DECLARE_INTERFACE(IAIConversationManager);
F_DECLARE_INTERFACE(IAIAppServices);
// used to typedef ObjID here, but objtype does that?

// couldnt get this working
#ifdef __cplusplus
class IMotor;
class cTagSet;
#else
typedef void IMotor;
typedef void cTagSet;
#endif

typedef struct sSoundInfo sSoundInfo;
struct mxs_vector;

///////////////////////////////////////

DECLARE_HANDLE(tAIIter);

///////////////////////////////////////////////////////////////////////////////
//
// Create the AI and add it to the global app-object
//

#define AIManagerCreate() \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _AIManagerCreate(IID_TO_REFIID(IID_IAIManager), NULL, pAppUnknown); \
    COMRelease(pAppUnknown); \
}

//
// Creates an AI manager, aggregating it with specfied pOuter,
// use IAggregate protocol if ppScriptMan is NULL, else self-init
//
EXTERN tResult LGAPI
_AIManagerCreate(REFIID, IAIManager ** ppAIManager, IUnknown * pOuter);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIManager
//

EXTERN const char * g_pszAIDefBehaviorSet;

///////////////////////////////////////

DECLARE_INTERFACE_(IAIManager, IUnknown)
{
   //
   // Expose a behavior factory to the AI
   //
   STDMETHOD (InstallBehaviorSet)(THIS_ IAIBehaviorSet * pBehaviorSet) PURE;

   //
   // Find the AI for a given object
   //
   STDMETHOD_(IAI *, GetAI)(THIS_ ObjID objId) PURE;

   //
   // Make/destory an AI of a given object
   //
   STDMETHOD_(BOOL, CreateAI)(THIS_ ObjID objId, const char * pszBehaviorSet = g_pszAIDefBehaviorSet) PURE;
   STDMETHOD_(BOOL, DestroyAI)(THIS_ ObjID objId) PURE;
   STDMETHOD_(BOOL, SelfSlayAI)(THIS_ ObjID objId) PURE;

   //
   // Count the AIs
   //
   STDMETHOD_(unsigned, CountAIs)(THIS) PURE;

   //
   // Iteration protocol
   //
   STDMETHOD_(IAI *, GetFirst)(THIS_ tAIIter *) PURE;
   STDMETHOD_(IAI *, GetNext)(THIS_ tAIIter *) PURE;
   STDMETHOD_(void, GetDone)(THIS_ tAIIter *) PURE;

   //
   // Motion hooks
   //
   STDMETHOD (SetMotor)(THIS_ ObjID obj, IMotor * pMotor) PURE;
   STDMETHOD (ResetMotionTags)(THIS_ ObjID obj DEFAULT_TO(OBJ_NULL) ) PURE;

   //
   // Projectile hook
   //
   STDMETHOD (NotifyFastObj)(THIS_ ObjID firer, ObjID projectile, const mxs_vector & velocity) PURE;
   
   //
   // Sound hook
   //
   STDMETHOD (NotifySound)(THIS_ const sSoundInfo *) PURE;
   STDMETHOD_(void, SetIgnoreSoundTime)(tSimTime time) PURE;
   
   //
   // Conversation Manager
   //
   STDMETHOD_(BOOL, StartConversation)(THIS_ ObjID conversationID) PURE;   // short-cut to conv manager fn
   STDMETHOD_(IAIConversationManager*, GetConversationManager)(THIS) PURE;

   ////////////////////////////////////
   //
   // Common property listening. Routed through component notificaton. 
   // @TBD (toml 07-15-98): this should be made internal, perhaps through an IAIInternalManager or something
   //

   STDMETHOD_(void, ListenToProperty)(IProperty *) PURE;

   //
   // Physics listening, triggered from components. 
   //
   STDMETHOD_(void, ListenForCollisions)(ObjID) PURE;
   STDMETHOD_(void, UnlistenForCollisions)(ObjID) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAI
//

DECLARE_INTERFACE_(IAI, IUnknown)
{
   //
   // Get the AIs ObjID
   //
   STDMETHOD_(ObjID, GetObjID)(THIS) PURE;

   //
   // Dispatch a custom notification to AI components
   //
   STDMETHOD_(void, NotifyGameEvent)(THIS_ void *) PURE;

   //
   // Tripwires
   //
   STDMETHOD_(void, NotifyTripwire)(ObjID objID, BOOL enter) PURE;

   //
   // Live/dead management
   //
   STDMETHOD_(void, Kill)() PURE;
   STDMETHOD_(void, Resurrect)() PURE;
   STDMETHOD_(BOOL, IsDead)() PURE;
   STDMETHOD_(BOOL, IsDying)() PURE;

   //
   // Other state changes
   //
   STDMETHOD_(void, Freeze)(tSimTime duration) PURE;
   STDMETHOD_(void, Unfreeze)(void) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIAppServices
//
// The game can install one of these into the application aggregate to provide
// additional info to the AI
//

DECLARE_INTERFACE_(IAIAppServices, IUnknown)
{
   STDMETHOD (AddSoundTags)(THIS_ cTagSet * pTagSet) PURE;
};

#pragma pack()

#endif // cpluscplus

///////////////////////////////////////////////////////////////////////////////
//
// Helper functions/macros
//

EXTERN BOOL ObjIsAI(ObjID obj);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIAPI_H */
