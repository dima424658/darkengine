/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapiiai.h,v 1.40 2000/02/28 19:35:10 bfarquha Exp $
//
// Internal AI object interface. (curse 8.3)
//
// @Note (toml 04-01-98): Internal AI APIs are *not* COM -- the use some COM
// concepts and comtools support classes to enforce some weak coupling within
// the AI and as for the convenience of a reference counting and interface
// acquisition protocol. These interfaces do allow class object parameters and
// return values.
//

#ifndef __AIAPIIAI_H
#define __AIAPIIAI_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <comtools.h>
#include <dynarray.h>

#include <wrtype.h>

#include <aiapi.h>
#include <aidist.h>
#include <aitype.h>
#include <aipthtyp.h>
#include <aiacttyp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(ITagFile);
F_DECLARE_INTERFACE(IAIPath);

typedef ulong eDamageResult;
typedef void* tDamageCallbackData;
struct sDamageMsg;

typedef int eWeaponEvent;

typedef struct sPropertyListenMsg sPropertyListenMsg;
typedef struct sPhysListenMsg sPhysListenMsg;

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IInternalAI);
F_DECLARE_INTERFACE(IAIBehaviorSet);
F_DECLARE_INTERFACE(IAIComponent);
F_DECLARE_INTERFACE(IAISenses);
F_DECLARE_INTERFACE(IAIActor);
F_DECLARE_INTERFACE(IAIAbility);
F_DECLARE_INTERFACE(IAIAction);
F_DECLARE_INTERFACE(IAIMoveRegulator);
F_DECLARE_INTERFACE(IAIMoveEnactor);
F_DECLARE_INTERFACE(IAISoundEnactor);
F_DECLARE_INTERFACE(IAIPathfinder);

struct sAIMoveSuggestion;
class cAIMoveSuggestions;
struct sAIMoveGoal;

struct sAIRawSense;
struct sAIAwareness;

struct sAISoundType;
typedef unsigned eAIScriptCmd;
struct sAIDefendPoint;
struct sAISignal;

class cAIGoal;

///////////////////////////////////////

typedef unsigned tAIAbilitySignals;
typedef cDynArray<tAIPathCellID> cAICellArray;

///////////////////////////////////////////////////////////////////////////////
//
// State flags
//

enum eAIStateFlags
{
   // The AI is remote
   kAI_IsRemote   = 0x01,

   // The AI is not in a valid part of the world
   kAI_OutOfWorld = 0x02,

   // AI is a motion capture creature
   kAI_IsCreature = 0x04,

   // The AI object and action decisions have been handed to another machine
   // in a network game.  This machine is retaining the AI "brains" (goal chosing).
   kAI_IsBrainsOnly = 0x08,

   kAI_StateFlags_IntMax = 0xffffffff // force it use an int
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIState
//
// A stucture used to distribute core AI state information.
//
// Care must be taken as much of this data is duplicate state provided
// through this structure for efficiency -- beware duplicate state
// bugs! (toml 03-31-98)
//

struct sAIState
{
   // Object ID
   ObjID         id;

   // Current mode
   eAIMode       mode;

   // Location and orientation
   union
   {
      Position   position;
      mxs_vector loc;
   };

   mxs_vector    pathLoc;

   tAIPathCellID cell;
   floatang      facingAng;

   // Alertness level
   eAIAwareLevel alertness;

   eAIAwareLevel last_alertness;
   tSimTime nLastAlertChangeTime;
   tAIPathOkBits nStressBits;

   // Flags
   unsigned      flags;

   IAIPath *pCurrentPath;
   int nCurrentMedium;

   BOOL bDidHighAlert;

   ObjID LastHighAwareObject;
   BOOL bHighAwareLostContact;

   BOOL bNeedsLargeDoors;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIState
//
// A wrapper class to control access to the state structure. This class must
// not have any virtual functions as it is intended to be safely down-castable
// from an sAIState.
//

class cAIState : private sAIState
{
public:
   //
   // Get/Set the AIs ObjID
   //
   ObjID GetID() const;
   void  SetID(ObjID newId);

   //
   // Get/Set the mode of the AI
   //
   eAIMode GetMode() const;
   eAIMode SetMode(eAIMode newMode);

   //
   // Get/Set the location of the AI
   //
   const cMxsVector * GetLocation() const;
   void GetLocation(cMxsVector *) const;
   const Location * GetPortalLocation() const;

   //
   // Get/Set the path cell of the AI
   //
   const mxs_vector * GetPathLocation() const;
   tAIPathCellID GetPathCell() const;
   tAIPathCellID SetPathCell(tAIPathCellID newCell);

   //
   // Get/Set the orientation of the AI
   //
   floatang GetFacingAng() const;
   void SetFacingAng(floatang newAng);

   //
   // Get the alertness level
   //
   eAIAwareLevel GetAlertness() const;

   tAIPathOkBits GetStressBits() const;

   IAIPath *GetCurrentPath() const;
   void SetCurrentPath(IAIPath *pPath);


   //
   // Query flags
   //
   BOOL IsRemote() const;
   BOOL IsInWorld() const;
   BOOL IsDead() const;
   BOOL IsCreature() const;
   BOOL IsBrainsOnly() const;

   unsigned GetFlags() const;
   void SetFlags(unsigned flags);
   void ClearFlags(unsigned flags);

   //
   // Computational tools
   //

   // @TBD (toml 05-26-98): change these to const mxs_vector &
   // Angle from AI location
   floatang AngleTo(const mxs_vector & toLoc) const;

   // Angle from AI facing to facing point
   floatang RelAngleTo(const mxs_vector & toLoc) const;

   // These return values -PI to PI unlike regular floatangs
   float PitchTo(const mxs_vector & toLoc) const;
   floatang RelPitchTo(const mxs_vector & toLoc) const;

   // Distance calculations
   float    DistSq(const mxs_vector & toLoc) const;
   float    DistXYSq(const mxs_vector & toLoc) const;
   float    DistXY(const mxs_vector & toLoc) const;
   float    DiffZ(const mxs_vector & toLoc) const;

   // Inside tests
   BOOL     InsideSphere(const mxs_vector & center, float radiusSq) const;
   BOOL     InsideCylinder(const mxs_vector & center, float radiusSq, float halfHeight) const;
   BOOL     InsideSphere(const mxs_vector & center, float radiusSq, float * pDistSq) const;
   BOOL     InsideCylinder(const mxs_vector & center, float radiusSq, float halfHeight, float * pDistSq) const;

   // Needs large doors?
   BOOL NeedsLargeDoors() const;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IInternalAI
//

DECLARE_INTERFACE_(IInternalAI, IAI)
{
   ////////////////////////////////////
   //
   // Get another AI
   //
   STDMETHOD_(IInternalAI *, AccessOtherAI)(ObjID) PURE;
   STDMETHOD_(void, AccessAIs)(IInternalAI ***, int *) PURE;

   ////////////////////////////////////
   //
   // AI Component management
   //

   //
   // Plug the AI together
   //
   STDMETHOD (Init)(ObjID id, IAIBehaviorSet *) PURE;
   STDMETHOD (End)() PURE;

   //
   // Access a component, Note: reference *is* incremented
   //
   STDMETHOD (GetComponent)(const char * pszComponent, IAIComponent **) PURE;

   //
   // Component iteration protocol
   //
   STDMETHOD_(IAIComponent *, GetFirstComponent)(tAIIter *) PURE;
   STDMETHOD_(IAIComponent *, GetNextComponent)(tAIIter *) PURE;
   STDMETHOD_(void, GetComponentDone)(tAIIter *) PURE;

   //
   // Access fixed components, Note: reference is not incremented
   //
   STDMETHOD_(IAIMoveEnactor *, AccessMoveEnactor)() PURE;
   STDMETHOD_(IAISoundEnactor *, AccessSoundEnactor)() PURE;
   STDMETHOD_(IAISenses *, AccessSenses)() PURE;
   STDMETHOD_(IAIPathfinder *, AccessPathfinder)() PURE;

   //
   // Access the AI behavior set, Note: reference is not incremented
   //
   STDMETHOD_(IAIBehaviorSet *, AccessBehaviorSet)() PURE;

   ////////////////////////////////////
   //
   // Query the state of the AI
   //
   STDMETHOD_(const cAIState *, GetState)() PURE;

   //
   // Freshen the AI path cell
   //
   STDMETHOD_(tAIPathCellID, UpdatePathCell)() PURE;

   //
   // Get recently visited cells
   //
   STDMETHOD_(void, GetRecentCells)(cAICellArray * pResult) PURE;

   //
   // Get the desired Z for the AI when non-biped locomoted
   //
   STDMETHOD_(float, GetGroundOffset)() PURE;

   //
   // Utility to get the closest player.  Most useful in networked games.
   //
   STDMETHOD_(ObjID, GetClosestPlayer)() PURE;

   ////////////////////////////////////
   //
   // Sense accessor
   //
   // Never returns NULL. Returned structure should never be considered
   // valid across calls to this function or across frames.
   //
   STDMETHOD_(const sAIAwareness *, GetAwareness)(ObjID target) PURE;

   ////////////////////////////////////
   //
   // Notifiers
   //

   STDMETHOD_(void, NotifySimStart)() PURE;
   STDMETHOD_(void, NotifySimEnd)() PURE;
   STDMETHOD_(eDamageResult, NotifyDamage)(const sDamageMsg * pMsg) PURE;
   STDMETHOD_(void, NotifyWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit) PURE;

   ////////////////////////////////////
   //
   // Projectile hook
   //
   STDMETHOD_(void, NotifyFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity) PURE;

   ////////////////////////////////////
   //
   // Sound hook
   //
   STDMETHOD_(void, NotifySound)(const sSoundInfo *, const sAISoundType *) PURE;

   ////////////////////////////////////
   //
   // Notify of found body
   //
   STDMETHOD_(void, NotifyFoundBody)(ObjID body) PURE;

   ////////////////////////////////////
   //
   // Notify of found something suspicious
   //
   STDMETHOD_(void, NotifyFoundSuspicious)(ObjID suspobj) PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifyScriptCommand)(eAIScriptCmd command, void *) PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifyProperty)(IProperty *, const sPropertyListenMsg *) PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifyDefend)(const sAIDefendPoint *) PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifySignal)(const sAISignal *) PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifyWitnessCombat)() PURE;

   ////////////////////////////////////

   STDMETHOD_(void, NotifyCollision)(const sPhysListenMsg *) PURE;

   ////////////////////////////////////
   // Is this just a proxy for an AI controlled by another machine in a network game.
   // (True if the instance is a cAIProxy class).
   STDMETHOD_(BOOL, IsNetworkProxy)() PURE;

   // Start the specified action (which was created from a message off the net).
   STDMETHOD_(void, StartProxyAction)(IAIAction *pAction, ulong deltaTime) PURE;

   // Get the current action of this type.  Returns null if there is more than one of that type.
   STDMETHOD_(IAIAction *, GetTheActionOfType)(tAIActionType type) PURE;

   // Stop an action of this type.  This way we don't have to synchronize action IDs on the net.
   STDMETHOD_(void, StopActionOfType)(tAIActionType type) PURE;

   // Set the AI's goal.  Used to set network proxy goal.
   STDMETHOD_(void, SetGoal)(cAIGoal *pGoal) PURE;

   // Set the AI's mode.  Used by network proxies.
   STDMETHOD_(void, SetModeAndNotify)(eAIMode) PURE;

   // Mark the AI to only do mode and goal choosing.
   STDMETHOD_(void, SetIsBrainsOnly)(BOOL) PURE;


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

   ////////////////////////////////////
   //
   // Debugging options
   //
#ifndef SHIP
   STDMETHOD_(void, DebugDraw)() PURE;
   STDMETHOD_(void, DebugSetPrimitiveMove)(const cAIMoveSuggestions *, const sAIMoveGoal *) PURE;
   STDMETHOD_(void, DebugGetPrimitiveMove)(const cAIMoveSuggestions **, const sAIMoveGoal **) PURE;
#endif

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIState, inline functions
//

inline ObjID cAIState::GetID() const
{
   return id;
}

///////////////////////////////////////

inline void cAIState::SetID(ObjID newId)
{
   id = newId;
}

///////////////////////////////////////

inline eAIMode cAIState::GetMode() const
{
   return mode;
}

///////////////////////////////////////

inline eAIMode cAIState::SetMode(eAIMode newMode)
{
   return mode = newMode;
}

///////////////////////////////////////

inline const cMxsVector * cAIState::GetLocation() const
{
   return (cMxsVector *)&loc;
}

///////////////////////////////////////

inline void cAIState::GetLocation(cMxsVector * pLoc) const
{
   *pLoc = loc;
}

///////////////////////////////////////

inline const Location * cAIState::GetPortalLocation() const
{
   return &position.loc;
}

///////////////////////////////////////

inline const mxs_vector * cAIState::GetPathLocation() const
{
   return &pathLoc;
}

///////////////////////////////////////

inline tAIPathCellID cAIState::GetPathCell() const
{
   return cell;
}

///////////////////////////////////////

inline tAIPathCellID cAIState::SetPathCell(tAIPathCellID newCell)
{
   return cell = newCell;
}

///////////////////////////////////////

inline floatang cAIState::GetFacingAng() const
{
   return facingAng;
}

///////////////////////////////////////

inline void cAIState::SetFacingAng(floatang newAng)
{
   facingAng = newAng;
}

///////////////////////////////////////

inline eAIAwareLevel cAIState::GetAlertness() const
{
   return alertness;
}

///////////////////////////////////////

inline tAIPathOkBits cAIState::GetStressBits() const
{
   return nStressBits;
}

///////////////////////////////////////

inline IAIPath *cAIState::GetCurrentPath() const
{
   return pCurrentPath;
}

///////////////////////////////////////

inline void cAIState::SetCurrentPath(IAIPath *pPath)
{
   pCurrentPath = pPath;
}

///////////////////////////////////////

inline BOOL cAIState::IsRemote() const
{
   return !!(flags & kAI_IsRemote);
}

///////////////////////////////////////

inline BOOL cAIState::IsInWorld() const
{
   return !(flags & kAI_OutOfWorld);
}

///////////////////////////////////////

inline BOOL cAIState::IsDead() const
{
   return (mode == kAIM_Dead);
}

///////////////////////////////////////

inline BOOL cAIState::IsCreature() const
{
   return !!(flags & kAI_IsCreature);
}

///////////////////////////////////////

inline BOOL cAIState::IsBrainsOnly() const
{
   return !!(flags & kAI_IsBrainsOnly);
}

///////////////////////////////////////

inline unsigned cAIState::GetFlags() const
{
   return flags;
}

///////////////////////////////////////

inline void cAIState::SetFlags(unsigned toSet)
{
   flags |= toSet;
}

///////////////////////////////////////

inline void cAIState::ClearFlags(unsigned toClear)
{
   flags &= ~toClear;
}

///////////////////////////////////////

inline floatang cAIState::AngleTo(const mxs_vector & toLoc) const
{
   return floatang(loc.x, loc.y, toLoc.x, toLoc.y);
}

///////////////////////////////////////

inline floatang cAIState::RelAngleTo(const mxs_vector & toLoc) const
{
   return Delta(facingAng, floatang(loc.x, loc.y, toLoc.x, toLoc.y));
}

///////////////////////////////////////

inline float cAIState::PitchTo(const mxs_vector & toLoc) const
{
   float dsq = (toLoc.x-loc.x)*(toLoc.x-loc.x)+(toLoc.y-loc.y)*(toLoc.y-loc.y);
   float dzsq = (toLoc.z-loc.z)*(toLoc.z-loc.z);
   float theta;

   theta = atan(sqrt(dzsq/dsq));
   if ((toLoc.z-loc.z)>0)
      return -theta;
   else
      return theta;
}

///////////////////////////////////////

inline float cAIState::DistSq(const mxs_vector & toLoc) const
{
   return AIDistanceSq(loc, toLoc);
}

///////////////////////////////////////

inline float cAIState::DistXYSq(const mxs_vector & toLoc) const
{
   return AIXYDistanceSq(loc, toLoc);
}

///////////////////////////////////////

inline float cAIState::DistXY(const mxs_vector & toLoc) const
{
   return AIXYDistance(loc, toLoc);
}

///////////////////////////////////////

inline float cAIState::DiffZ(const mxs_vector & toLoc) const
{
   return ffabsf(loc.z -  toLoc.z);
}

///////////////////////////////////////

inline BOOL cAIState::InsideSphere(const mxs_vector & center, float radiusSq) const
{
   return AIInsideSphere(loc, center, radiusSq);
}

///////////////////////////////////////

inline BOOL cAIState::InsideCylinder(const mxs_vector & center, float radiusSq, float halfHeight) const
{
   return AIInsideCylinder(loc, center, radiusSq, halfHeight);
}

///////////////////////////////////////

inline BOOL cAIState::InsideSphere(const mxs_vector & center, float radiusSq, float * pDistSq) const
{
   return AIInsideSphere(loc, center, radiusSq, pDistSq);
}

///////////////////////////////////////

inline BOOL cAIState::InsideCylinder(const mxs_vector & center, float radiusSq, float halfHeight, float * pDistSq) const
{
   return AIInsideCylinder(loc, center, radiusSq, halfHeight, pDistSq);
}

///////////////////////////////////////

inline BOOL cAIState::NeedsLargeDoors() const
{
   return bNeedsLargeDoors;
}


///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIIAI_H */
