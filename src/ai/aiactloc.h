/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactloc.h,v 1.12 2000/03/05 15:50:23 toml Exp $
//
//
//

#ifndef __AIACTLOC_H
#define __AIACTLOC_H

#include <aibasact.h>
#include <aimovsug.h>
#include <aipthtyp.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIPathfinder);
F_DECLARE_INTERFACE(IAIPath);

class cAIGoal;
class cAIGotoGoal;
class cAIMoveAction;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILocoAction
//

#define kAILA_DefaultAccuracy  (sq(3.0))
#define kAILA_DefaultAccuracyZ (6.0833)
#define kAILA_NoZAccuracy      kFloatMax

enum eAILocoActionFlags
{
   kAIAF_HeadingDirect = kAIAF_BaseFirstAvail
};

///////////////////////////////////////

class cAILocoAction : public cAIAction
{
public:
   cAILocoAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAILocoAction();

   //
   // Set to an action that encompasses an entire "goto" goal
   //
   void SetFromGoal(const cAIGotoGoal *);

   //
   // Locomote to a point, using an object as a hint fort determining
   // the destination cell
   //
   void Set(const cMxsVector & dest,
            ObjID              hintObject,
            eAISpeed           speed      = kAIS_Normal,
            float              accuracySq = kAILA_DefaultAccuracy,
            float              accuracyZ  = kAILA_DefaultAccuracyZ);

   //
   // Locomote to an object
   //
   void Set(ObjID    object,
            eAISpeed speed      = kAIS_Normal,
            float    accuracySq = kAILA_DefaultAccuracy,
            float    accuracyZ  = kAILA_DefaultAccuracyZ);

   //
   // Locomote to a point
   //
   void Set(const cMxsVector & dest,
            eAISpeed           speed      = kAIS_Normal,
            float              accuracySq = kAILA_DefaultAccuracy,
            float              accuracyZ  = kAILA_DefaultAccuracyZ);

   //
   // Set the move kind. Default is "Loco"
   //
   void SetMoveKind(eAIMoveSuggKind kind);

   //
   // Force re-pathfind, i.e., reset with same values
   //
   void Reset();

   //
   // Clear out to neutral state
   //
   void Clear();

   //
   // Access internals
   //
   const cMxsVector & GetDest() const;
   ObjID              GetObj() const;
   eAISpeed           GetSpeed() const;
   void               SetSpeed(eAISpeed speed);
   float              GetAccuracy() const;
   float              ComputePathDist();
   const cMxsVector & GetLogicalDest() const; // the destination as provided by client

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   STDMETHOD_(IAIPath *, GetPath)();

   // Do the pathfind. Note, you should only explicitly pathfind if you
   // really need to know success or failure *in advance*
   BOOL Pathfind(BOOL bTryNearEnough = FALSE, float fNearDist = -1);

   // Set path. Does not AddRef.
   void SetPath(IAIPath *pPath);

   void SetPathNearDist(float fDist);

   static void StrokeDest(mxs_vector *pDest, tAIPathCellID *hintCell, const ObjID id);

protected:
   // Check progress
   eAIResult UpdatePathProgress();

   // Clear out our path and action info
   void ClearPathAndMove();

   // Generate suggestions to continue on path
   HRESULT GenerateSuggestions(tAIPathOkBits nStressBits);

private:
   //
   // Client ability members
   //

   // The true action location, if any
   cMxsVector      m_dest;
   
   // The action location, as provided by client, if any
   cMxsVector      m_logicalDest;

   // The goal object, if any
   ObjID           m_object;

   // How fast should we move
   eAISpeed        m_speed;

   // How accurate must we be
   float           m_accuracySq;
   float           m_accuracyZ;

   // Step-wise move sub-action
   cAIMoveAction * m_pMoveAction;

   // Kind of suggestions to make
   eAIMoveSuggKind m_moveSuggKind;

   // The path (may be null)
   IAIPath *       m_pPath;

   // Cached Target cell
   tAIPathCellID   m_hintCell;

   float fPathNearDist;
};

///////////////////////////////////////

inline cAILocoAction::cAILocoAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Locomote, pOwner, data),
   m_dest(kInvalidLoc),
   m_logicalDest(kInvalidLoc),
   m_object(OBJ_NULL),
   m_speed(kAIS_Normal),
   m_accuracySq(kAILA_DefaultAccuracy),
   m_accuracyZ(kAILA_DefaultAccuracyZ),
   m_pMoveAction(NULL),
   m_moveSuggKind(kAIMS_Loco),
   m_pPath(NULL),
   m_hintCell(0),
   fPathNearDist(-1)
{
}

///////////////////////////////////////

inline void cAILocoAction::SetPathNearDist(float fDist)
{
   fPathNearDist = fDist;
}


///////////////////////////////////////

inline void cAILocoAction::Set(const cMxsVector & dest, ObjID hintObject, eAISpeed speed, float accuracySq, float accuracyZ)
{
   ClearPathAndMove();
   m_dest        = dest;
   m_logicalDest = m_dest;
   m_object      = hintObject;
   m_speed       = speed;
   m_accuracySq  = accuracySq;
   m_accuracyZ   = accuracyZ;
}

///////////////////////////////////////

inline void cAILocoAction::Set(ObjID object, eAISpeed speed, float accuracySq, float accuracyZ)
{
   ClearPathAndMove();
   GetObjLocation(object, &m_dest);
   m_object      = object;
   m_speed       = speed;
   m_accuracySq  = accuracySq;
   m_accuracyZ   = accuracyZ;
   m_logicalDest = m_dest;
}

///////////////////////////////////////

inline void cAILocoAction::Set(const cMxsVector & dest, eAISpeed speed, float accuracySq, float accuracyZ)
{
   ClearPathAndMove();
   m_dest        = dest;
   m_object      = OBJ_NULL;
   m_speed       = speed;
   m_accuracySq  = accuracySq;
   m_accuracyZ   = accuracyZ;
   m_logicalDest = m_dest;
}

///////////////////////////////////////

inline void cAILocoAction::Reset()
{
   ClearPathAndMove();
}

///////////////////////////////////////

inline void cAILocoAction::Clear()
{
   ClearPathAndMove();

   m_dest.Set(0,0,0);
   m_object      = OBJ_NULL;
   m_speed       = kAIS_Normal;
   m_accuracySq  = kAILA_DefaultAccuracy;
   m_hintCell    = 0;
   m_logicalDest = m_dest;
}

///////////////////////////////////////

inline const cMxsVector & cAILocoAction::GetDest() const
{
   return m_dest;
}

///////////////////////////////////////

inline ObjID cAILocoAction::GetObj() const
{
   return m_object;
}

///////////////////////////////////////

inline eAISpeed cAILocoAction::GetSpeed() const
{
   return m_speed;
}

///////////////////////////////////////

inline void cAILocoAction::SetSpeed(eAISpeed speed)
{
   m_speed = speed;
}

///////////////////////////////////////

inline float cAILocoAction::GetAccuracy() const
{
   return m_accuracySq;
}

///////////////////////////////////////

inline const cMxsVector & cAILocoAction::GetLogicalDest() const
{
   return m_logicalDest;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTLOC_H */
