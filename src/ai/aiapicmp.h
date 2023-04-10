/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapicmp.h,v 1.26 2000/02/16 12:06:10 bfarquha Exp $
//
// Specification of the AI component interface
//

#ifndef __AIAPICMP_H
#define __AIAPICMP_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <comtools.h>
#include <dynarray.h>

#include <aiapi.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(ITagFile);

struct sDamageMsg;
typedef int eWeaponEvent;
typedef struct sPropertyListenMsg sPropertyListenMsg;
typedef struct sPhysListenMsg sPhysListenMsg;

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IInternalAI);
F_DECLARE_INTERFACE(IAIBehaviorSet);
F_DECLARE_INTERFACE(IAIComponent);
F_DECLARE_INTERFACE(IAISenses);
F_DECLARE_INTERFACE(IAIAbility);
F_DECLARE_INTERFACE(IAIMoveRegulator);
F_DECLARE_INTERFACE(IAIAction);

class  cAIGoal;
struct sAIMoveSuggestion;
class  cAIMoveSuggestions;
struct sAIAlertness;
struct sAISoundType;
typedef unsigned eAIScriptCmd;
struct sAIDefendPoint;
struct sAISignal;

struct sSoundInfo;

typedef cDynArray<IAIComponent *> cAIComponentPtrs;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIComponent
//
// This interface serves as the base for all other AI components is almost
// entirely a notification interface.
//
// @TBD (toml 07-24-98): need a command briefly describing eahc of the following

enum eAIComponentNotifications
{
   kAICN_SimStart       =     0x0001,
   kAICN_SimEnd         =     0x0002,

   kAICN_BeginFrame     =     0x0004,
   kAICN_EndFrame       =     0x0008,

   kAICN_UpdateProps    =     0x0010,

   kAICN_ActionProgress =     0x0020,
   kAICN_GoalProgress   =     0x0040,

   kAICN_ModeChange     =     0x0080,
   kAICN_GoalChange     =     0x0100,
   kAICN_ActionChange   =     0x0200,

   kAICN_Alertness      =     0x0400,

   kAICN_Damage         =     0x0800,
   kAICN_Death          =     0x1000,

   kAICN_Weapon         =     0x2000,

   kAICN_GameEvent      =     0x4000,

   kAICN_Sound          =     0x8000,

   kAICN_FastObj        = 0x00010000,

   kAICN_FoundBody      = 0x00020000,

   kAICN_ScriptCommand  = 0x00040000,

   kAICN_Property       = 0x00080000,

   kAICN_Defend         = 0x00100000,

   kAICN_Signal         = 0x00200000,

   kAICN_WitnessCombat  = 0x00400000,

   kAICN_GoalsReset     = 0x00800000,

   kAICN_Collision      = 0x01000000,

   kAICN_Tripwire       = 0x02000000,

   kAICN_Frustration    = 0x04000000,

   kAICN_HighAlert      = 0x08000000,

   kAICN_FoundSuspicious= 0X10000000,

   kAICN_Awareness      = 0X20000000,

   kAICN_All            = 0xffffffff
};


struct IAIPath;

///////////////////////////////////////

DECLARE_INTERFACE_(IAIComponent, IUnknown)
{
   //
   // Hook up the component. Because we're effectively aggregating,
   // the component should not hold a reference to the outer AI
   //
   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *) PURE;
   STDMETHOD_(void, ConnectComponent)(IInternalAI * pOuterAI, unsigned ** ppNotifications) PURE;

   //
   // Init the component
   //
   STDMETHOD_(void, Init)() PURE;

   //
   // Clean-up the component
   //
   STDMETHOD_(void, End)() PURE;

   //
   // Get the outer AI, note reference count is not incremented
   //
   STDMETHOD_(IInternalAI *, AccessOuterAI)() PURE;

   //
   // Find out the component name
   //
   STDMETHOD_(const char *, GetName)() PURE;

   //
   // Notifications
   //
   STDMETHOD_(void, OnSimStart)() PURE;
   STDMETHOD_(void, OnSimEnd)() PURE;

   STDMETHOD_(void, OnBeginFrame)() PURE;
   STDMETHOD_(void, OnEndFrame)() PURE;

   STDMETHOD_(void, OnUpdateProperties)(eAIPropUpdateKind kind) PURE;

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction) PURE;
   STDMETHOD_(void, OnGoalProgress)(const cAIGoal * pGoal) PURE;
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode) PURE;
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal) PURE;
   STDMETHOD_(void, OnActionChange)(IAIAction * pPrevious, IAIAction * pAction) PURE;

   STDMETHOD_(void, OnAwareness)(ObjID source,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw) PURE;

   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw) PURE;

   STDMETHOD_(void, OnHighAlert)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw) PURE;

   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID) PURE;
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg) PURE;
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit) PURE;

   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType) PURE;
   STDMETHOD_(void, OnFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity) PURE;

   STDMETHOD_(void, OnFoundBody)(ObjID body) PURE;

   STDMETHOD_(void, OnFoundSuspicious)(ObjID suspobj) PURE;

   STDMETHOD_(void, OnScriptCommand)(eAIScriptCmd command, void *) PURE;

   STDMETHOD_(void, OnProperty)(IProperty *, const sPropertyListenMsg *) PURE;

   STDMETHOD_(void, OnDefend)(const sAIDefendPoint *) PURE;

   STDMETHOD_(void, OnSignal)(const sAISignal *) PURE;

   STDMETHOD_(void, OnWitnessCombat)() PURE;

   STDMETHOD_(void, OnGoalsReset)(const cAIGoal * pPrevious) PURE;

   STDMETHOD_(void, OnCollision)(const sPhysListenMsg *) PURE;

   STDMETHOD_(void, OnTripwire)(ObjID objID, BOOL enter) PURE;

   // pass in object that is frustrating us.
   STDMETHOD_(void, OnFrustration)(ObjID source, ObjID dest, IAIPath *pPath) PURE;

   //
   // Special callbacks
   //
   STDMETHOD_(void, OnGameEvent)(void *) PURE;

   ////////////////////////////////////
   //
   // Save/load
   //
   STDMETHOD_(BOOL, PreSave)() PURE;
   STDMETHOD_(BOOL, Save)(ITagFile *) PURE;
   STDMETHOD_(BOOL, PostSave)() PURE;

   STDMETHOD_(BOOL, PreLoad)() PURE;
   STDMETHOD_(BOOL, Load)(ITagFile *) PURE;
   STDMETHOD_(BOOL, PostLoad)() PURE;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPICMP_H */
