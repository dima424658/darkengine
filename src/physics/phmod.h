/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmod.h,v 1.81 2000/01/29 13:32:51 adurant Exp $
//
// Physics Models
//
#pragma once

#ifndef __PHMOD_H
#define __PHMOD_H

#include <lg.h>
#include <math.h>
#include <matrixs.h>
#include <matrix.h>
#include <mds.h>
#include <dlist.h>
#include <hashset.h>

#ifdef PLAYTEST
#include <objedit.h>
#endif

#ifdef DBG_ON
#include <mprintf.h>
#endif

///////////////////////////////////////

#include <wr.h>
// @Note (toml 08-05-97): This is to get at "Position." At some point,
//                        portal should expose Postion and Location in a
//                        separate header so that physics doesn't need to be
//                        100 0ependent on all of wr.h

///////////////////////////////////////

#include <dynarray.h>

#include <phystyp2.h>
#include <phpos.h>

#include <phdyn.h>
#include <phctrl.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//

class cFacePoly;
class cFaceContactList;
class cEdgeContactList;
class cVertexContactList;
class cFaceContact;
class cEdgeContact;
class cVertexContact;

class cPhysModel;
class cPhysModelTable;
class cPhysClsn;


///////////////////////////////////////////////////////////////////////////////
//
// Enums
//

///////////////////////////////////////
//
// Model types
//

enum ePhysModelType  // @Note (toml 08-05-97): An abstraction violation justified in sensitive code. Please don't use this a lot.
{
   kPMT_Sphere,
   kPMT_BSP,
   kPMT_Point,
   kPMT_OBB,
   kPMT_SphereHat,
   kPMT_NumTypes,

   kPMT_Invalid = 0xffffffffL
};

///////////////////////////////////////

enum ePhysModelFlags
{
   kPMF_LowDetail         = 0x00000001,     // i'm a low detail model
   kPMF_Remote            = 0x00000002,     // i'm remote
   kPMF_Inactive          = 0x00000004,     // not current detail level
   kPMF_Stationary        = 0x00000008,     // not moving, but active
   kPMF_Moved             = 0x00000010,     // moved this frame
   kPMF_Sleeping          = 0x00000040,     // i'm asleep
   kPMF_CollisionChecked  = 0x00000080,     // computed collisions for this object/frame
   kPMF_NonMoveable       = 0x00000200,     // can't move
   kPMF_Uncontrollable    = 0x00000400,     // can't be controlled
   kPMF_Player            = 0x00000800,     // is a player's physics model
   kPMF_Creature          = 0x00001000,     // is a creature's physics model
   kPMF_RemovesOnSleep    = 0x00002000,     // removes its physics when it sleeps
   kPMF_Rope              = 0x00004000,     // is a rope
   kPMF_PressurePlate     = 0x00008000,     // is a pressure plate
   kPMF_Weapon            = 0x00010000,     // is a weapon
   kPMF_Squishing         = 0x00020000,     // in the process of squishing something
   kPMF_FacesVel          = 0x00040000,     // always faces velocity
   kPMF_AllowedToSleep    = 0x00080000,     // is allowed to fall alseep
   kPMF_MovingTerrain     = 0x00100000,     // is a moving terrain object
   kPMF_Door              = 0x00200000,     // is a door object
   kPMF_AICollides        = 0x00400000,     // AIs collide with this object
   kPMF_Projectile        = 0x00800000,     // is a projectile
   kPMF_Avatar            = 0x01000000,     // is an avatar for a net player
   kPMF_UpdateRefs        = 0x02000000,     // needs to have its refs updated
   kPMF_PointVsNotSpecial = 0x04000000,     // not a point versus "special"
   kPMF_Special           = 0x08000000,     // is "special"
   kPMF_FancyGhost        = 0x10000000,     // boo, it's a fancy (ooh la la) ghost
   kPMF_TrueGhost         = 0x20000000,     // a true ghost
   kPMF_DiskCheck         = 0x40000000,     // Do OBB vs. sphere disk check
};

///////////////////////////////////////

enum ePhysAxes
{
   NoAxes   = 0x0000,
   XAxis    = 0x0001,
   YAxis    = 0x0002,
   ZAxis    = 0x0004,
   NegXAxis = 0x0008,
   NegYAxis = 0x0010,
   NegZAxis = 0x0020,
   kMakeBig = 0xffff,
};

///////////////////////////////////////
//
// Result of an intersection test
//

enum ePhysIntersectResultEnum
{
   kPIR_OK,
   kPIR_BadTerrain,
   kPIR_BadObject,
   kPIR_Undetermined
};

typedef cDynArray<sAngleLimit *> cAngLimitList;
typedef cDynArray<sTransLimit *> cTransLimitList;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModelTable
//

typedef cHashSet<cPhysModel *, ObjID, cHashFunctions> cPhysModelTableBase;

class cPhysModelTable : public cPhysModelTableBase
{
   virtual tHashSetKey GetKey(tHashSetNode ) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModel
//
// A single physics model
//

typedef cDList <cPhysModel, 0>     cPhysModelList;
typedef cDListNode <cPhysModel, 0> cPhysModelNode;

typedef cDynArray <sPhysForce>     cPhysForceList;

typedef struct
{
   mxs_real tension;
   mxs_real damping;
} tSpringInfo;

struct tConstraint
{
   ObjID       cause;
   mxs_vector  dir;
};

struct tVelocityConstraint
{
   ObjID       cause;
   mxs_vector  dir;
   mxs_real    mag;
};

typedef cDynArray<tConstraint>     cConstraintArray;

///////////////////////////////////////

class cPhysModel : public cPhysModelNode
{
public:

   ////////////////////////////////////
   //
   // Constructor/Destructor
   //
   cPhysModel(ObjID objID, ePhysModelType type, tPhysSubModId numSubModels, unsigned flags);
   cPhysModel(PhysReadWrite func);
   virtual ~cPhysModel();

   ////////////////////////////////////
   //
   // I/O
   //
   virtual void Write(PhysReadWrite func) const;

   ////////////////////////////////////
   //
   // Static state queries
   //
   ObjID                   GetObjID() const;
   virtual ePhysModelType  GetType(tPhysSubModId subModId) const;
   tPhysSubModId           NumSubModels() const;

   ////////////////////////////////////
   //
   // Get a model or submodel position, location
   //
   const Position & GetPosition(tPhysSubModId subModId) const;
   const Position & GetPosition() const;
   const Location & GetLocation(tPhysSubModId subModId) const;
   const Location & GetLocation() const;

   ////////////////////////////////////
   //
   // Get/Set a submodel location vector
   //
   const mxs_vector & GetLocationVec(tPhysSubModId subModId) const;
   void               SetLocationVec(tPhysSubModId subModId, const mxs_vector & locVec);
   void               SetCurrentLocationVec(tPhysSubModId subModId, const mxs_vector & locVec);
   const mxs_vector & GetLocationVec() const;
   void               SetLocationVec(const mxs_vector & locVec, BOOL update = FALSE);
   void               SetCurrentLocationVec(const mxs_vector & locVec, BOOL update = FALSE);

   const mxs_vector & GetSubModOffset(tPhysSubModId subModId) const;
   void               SetSubModOffset(tPhysSubModId subModId, const mxs_vector & offset);

   ////////////////////////////////////
   //
   // Get/Set rotation
   //
   const mxs_angvec & GetRotation(tPhysSubModId subModId) const;
   const mxs_angvec & GetRotation() const;
   void               SetRotation(tPhysSubModId subModId, const mxs_angvec & rot);
   void               SetRotation(const mxs_angvec & rot, BOOL update = FALSE);

   ////////////////////////////////////
   //
   // Get/Set center of gravity
   //
   const mxs_vector   GetCOG() const;
   const mxs_vector & GetCOGOffset() const;
   void               SetCOG(const mxs_vector & cog);
   void               SetCOGOffset(const mxs_vector & cog_offset);

   ////////////////////////////////////
   //
   // Get overall model size
   //
   virtual mxs_real GetSize() const = 0;

   ////////////////////////////////////
   //
   // Per frame collision checking flag
   //
   BOOL CollisionChecked() const;
   void SetCollisionChecked(BOOL state);

   ////////////////////////////////////
   //
   // Per dynamics update constraint list
   //
   void ClearConstraints();
   void AddConstraint(ObjID cause, const mxs_vector &dir, mxs_real mag);
   void AddConstraint(ObjID cause, const mxs_vector &norm);
   void AddConstraint(ObjID cause, tPhysSubModId i, const mxs_vector &norm);

   void ApplyConstraints(mxs_vector *vec);
   void ApplyConstraints(tPhysSubModId i, mxs_vector *vec);

   void GetConstraints(mxs_vector vec_list[], int *num_vecs);

   ////////////////////////////////////
   //
   // Force list updating
   //
   virtual BOOL ApplyForces(tPhysSubModId subModId, sPhysForce *flist, int nforces, mxs_real dt,
                            mxs_vector &start_loc, mxs_vector *end_loc, mxs_vector *force,
                            mxs_real *ctime, BOOL terrain_check, BOOL object_check);

   ////////////////////////////////////
   //
   // Control status
   //
   BOOL IsControlled();
   BOOL IsAxisVelocityControlled();
   BOOL IsVelocityControlled();
   BOOL IsRotationalVelocityControlled();
   BOOL IsLocationControlled();
   BOOL IsRotationControlled();

   ////////////////////////////////////
   //
   // Contact
   //
   void CreateTerrainContact(tPhysSubModId subModId, const cFacePoly *poly, ObjID objID);
   void CreateTerrainContact(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end);
   void CreateTerrainContact(tPhysSubModId subModId, const mxs_vector &point);

   void DestroyTerrainContact(tPhysSubModId subModId, const cFacePoly *poly);
   void DestroyTerrainContact(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector & end);
   void DestroyTerrainContact(tPhysSubModId subModId, const mxs_vector &point);

   void DestroyAllTerrainContacts(tPhysSubModId subModId);

   BOOL GetFaceContacts(tPhysSubModId subModId, cFaceContactList **pFaceContactList) const;
   BOOL GetEdgeContacts(tPhysSubModId subModId, cEdgeContactList **pEdgeContactList) const;
   BOOL GetVertexContacts(tPhysSubModId subModId, cVertexContactList **pVertexContactList) const;

   BOOL InObjectContact();
   void SetObjectContact(BOOL state);

   BOOL InTerrainContact();
   BOOL InTerrainFaceContact();
   BOOL InTerrainEdgeContact();
   BOOL InTerrainVertexContact();

   void IncTerrainFaceContact();    // don't screw with these unless you know what you're doing
   void IncTerrainEdgeContact();
   void IncTerrainVertexContact();

   ////////////////////////////////////
   //
   // Object reference frame
   //
   void  SetObjReferenceFrame(ObjID obj);
   ObjID GetObjReferenceFrame() const;

   ////////////////////////////////////
   //
   // End-of-frame Locations and Rotations
   //
   void               UpdateEndLocation(mxs_real dt);
   void               UpdateEndLocation(tPhysSubModId i, mxs_real dt);

   const Location   & GetEndLocation(tPhysSubModId subModId = -1) const;
   const mxs_vector & GetEndLocationVec(tPhysSubModId subModId = -1) const;
   void               SetEndLocationVec(const mxs_vector &end_loc);
   void               SetEndLocationVec(tPhysSubModId subModId,  const mxs_vector &end_loc);
   const mxs_angvec & GetEndRotationVec(tPhysSubModId subModId = -1) const;
   void               SetEndRotationVec(const mxs_angvec &end_ang);
   void               SetEndRotationVec(tPhysSubModId subModId, const mxs_angvec &end_ang);

   void               PreventPlayerFall();

   void               UpdateModel(mxs_real dt);

   void               SetTargetLocation(const mxs_vector &loc);
   const mxs_vector & GetTargetLocation(tPhysSubModId subModId = -1) const;

   void               UpdateTargetLocation(mxs_real dt);
   void               UpdateTargetLocation(tPhysSubModId subModId, mxs_real dt);

   void               UpdateSpringMechanics(tPhysSubModId i, mxs_real dt);

   ////////////////////////////////////
   //
   // Check for media transitions
   //
   void UpdateMedium();
   int  GetMediaState() const;

   ////////////////////////////////////
   //
   // Base friction
   //
   mxs_real GetBaseFriction() const;
   void     SetBaseFriction(mxs_real base_friction);

   ////////////////////////////////////
   //
   // Spring constant for a submodel
   //
   void SetSpringTension(tPhysSubModId subModId, mxs_real tension);
   void SetSpringDamping(tPhysSubModId subModId, mxs_real damping);

   mxs_real GetSpringTension(tPhysSubModId subModId) const;
   mxs_real GetSpringDamping(tPhysSubModId subModId) const;

   ////////////////////////////////////
   //
   // Distance from given terrain
   //
   virtual mxs_real TerrainDistance(tPhysSubModId subModId, cFaceContact *pFactContact) const = 0;
   virtual mxs_real TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const = 0;
   virtual mxs_real TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const = 0;

   ////////////////////////////////////
   //
   // Check for a terrain collision in the specified time
   //
   virtual BOOL CheckTerrainCollision(tPhysSubModId subModId, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn) = 0;

   ////////////////////////////////////
   //
   // Object collision temp disabler
   //
   void SetObjectPassThru(mxs_real dt);
   void DecObjectPassThru(mxs_real dt);
   BOOL ObjectPassThru() const;

   ////////////////////////////////////
   //
   // Do we have dynamics?
   //
   BOOL IsMoveable() const;

   ////////////////////////////////////
   //
   // Do we have a control?
   //
   BOOL IsControllable() const;

   ////////////////////////////////////
   //
   // Update refs from a cell list
   //
   void UpdateRefs(uint cellCount, const short *pCells);

   ////////////////////////////////////
   //
   // Update physics refs from stationary position
   //
   virtual void UpdateRefs() = 0;

   ////////////////////////////////////
   //
   // Location/rotation intersection tests: valid only if is moveable
   //
   virtual ePhysIntersectResult TestLocationVec(int subModId,
                                                const mxs_vector & Location) const = 0;
   virtual ePhysIntersectResult TestRotation(int subModId,
                                             const mxs_vector & Rotation) const = 0;

   ////////////////////////////////////
   //
   // Is the model local/remote?
   //
   BOOL IsRemote() const;
   void SetRemote(BOOL state);

   ////////////////////////////////////
   //
   // Is the modal a player/creature?
   //
   BOOL IsPlayer() const;
   BOOL IsCreature() const;
   BOOL IsAvatar() const;
   BOOL IsRope() const;

   ////////////////////////////////////
   //
   // Moving terrain
   //
   void SetMovingTerrain(BOOL state);
   BOOL IsMovingTerrain() const;

   ////////////////////////////////////
   //
   // Door
   //
   void SetDoor(BOOL state);
   BOOL IsDoor() const;

   ////////////////////////////////////
   //
   // AI Collides
   //
   void SetAICollides(BOOL state);
   BOOL AICollides() const;

   ////////////////////////////////////
   //
   // Projectile
   //
   void SetProjectile(BOOL state);
   BOOL IsProjectile() const;

   ////////////////////////////////////
   //
   // Ghost
   //
   BOOL IsGhost() const;

   void SetFancyGhost(BOOL state);
   BOOL IsFancyGhost() const;

   void SetTrueGhost(BOOL state);
   BOOL IsTrueGhost() const;

   ////////////////////////////////////
   //
   // Pressure plates
   //
   BOOL     IsPressurePlate() const;
   void     SetPressurePlate(BOOL state);

   mxs_real GetPPlateActivationWeight() const;
   mxs_real GetPPlateTravel() const;
   mxs_real GetPPlateSpeed() const;
   mxs_real GetPPlatePause() const;

   int      GetPPlateState() const;
   void     SetPPlateState(int state);

   mxs_real GetPPlateCurPause() const;
   void     SetPPlateCurPause(mxs_real cur_pause);

   ////////////////////////////////////
   //
   // Weapons
   //
   BOOL IsWeapon(tPhysSubModId subModId) const;
   BOOL IsHalo(tPhysSubModId subModId) const;

   ////////////////////////////////////
   //
   // Does it remove itself when it goes to sleep?
   //
   BOOL RemovesOnSleep() const;
   void SetRemovesOnSleep(BOOL state);

   ////////////////////////////////////
   //
   // Climbing / Mantling
   //
   BOOL  IsClimbing() const;
   BOOL  IsRopeClimbing() const;

   ObjID GetClimbingObj() const;
   void  SetClimbingObj(ObjID climbing_obj);

   int  GetRopeSegment() const;
   void SetRopeSegment(int segment);

   mxs_real GetRopeSegmentPct() const;
   void SetRopeSegmentPct(mxs_real percent);

   BOOL IsMantling() const;
   int  GetMantlingState() const;
   void SetMantlingState(int state);

   void GetMantlingTargVec(mxs_vector *targ_vec) const;
   void SetMantlingTargVec(mxs_vector &targ_vec);

   void UpdateRopeClimbing(mxs_real dt);
   void ComputeRopeSegPct();

   ////////////////////////////////////
   //
   // Squish
   //
   void Squish(mxs_real magnitude);

   void SetSquishingState(BOOL state);
   BOOL IsSquishing() const;

   ////////////////////////////////////
   //
   // Faces vel
   //
   void SetFacesVel(BOOL state);
   BOOL FacesVel() const;

   ////////////////////////////////////
   //
   // Activation state
   //
   BOOL IsActive() const;
   BOOL IsStationary() const;
   void Activate();
   void SetStationary(BOOL state);
   void Deactivate();

   ////////////////////////////////////
   //
   // Sleep state
   //
   BOOL AllowedToSleep() const;
   void SetAllowedToSleep(BOOL state);

   BOOL IsSleeping() const;
   void SetSleep(BOOL state);

   ////////////////////////////////////
   //
   // Is a point vs foo?
   //
   BOOL IsPointVsTerrain() const;
   void SetPointVsTerrain(BOOL state);

   BOOL IsPointVsNotSpecial() const;
   void SetPointVsNotSpecial(BOOL state);

   BOOL IsSpecial() const;
   void SetSpecial(BOOL state);

   // Special Treating OBB against sphere, where sphere is flatened into disk.
   BOOL DoDiskCheck();
   void SetDiskCheck(BOOL state);

   ////////////////////////////////////
   //
   // Needs refs updated?
   //
   BOOL UpdateRefsPending() const;
   void SetUpdateRefs(BOOL state);

   ////////////////////////////////////
   //
   // Attachments
   //
   int  NumAttachments() const;
   void AddAttachment();
   void RemoveAttachment();

   BOOL IsAttached() const;
   void SetAttached(BOOL state);

   ////////////////////////////////////
   //
   // Rotation axes
   //
   ePhysAxes GetRotAxes() const;
   void      SetRotAxes(int axes);

   ePhysAxes GetRestAxes() const;
   void      SetRestAxes(int axes);

   BOOL      AtRest() const;
   void      SetRest(BOOL state);

   ////////////////////////////////////
   //
   // Being translated?
   //
   BOOL IsTranslating() const;

   ////////////////////////////////////
   //
   // affected by gravity?
   //
   BOOL Gravity() const;
   mxs_real GetGravity() const;
   void     SetGravity(mxs_real value);

   ////////////////////////////////////
   //
   // Get velocity: for convenience, if moveable
   //
   const mxs_vector & GetVelocity(tPhysSubModId subModId = -1) const;
   mxs_vector *       GetVelocityPtr(tPhysSubModId subModId = -1);

   ////////////////////////////////////
   //
   // Get the dynamics/control data if we're moveable or controllable
   //
   cPhysDynData *  GetDynamics(tPhysSubModId subModId = -1) const;
   cPhysCtrlData * GetControls(tPhysSubModId subModId = -1) const;

   ////////////////////////////////////
   //
   // Rotation limits
   //
   void ClearAngleLimits();
   void AddAngleLimit(int axis, int angle, LimitCallback callback);
   BOOL CheckAngleLimits(mxs_angvec start, mxs_angvec end, mxs_angvec *limit);

   ////////////////////////////////////
   //
   // Translation limits
   //
   void ClearTransLimits();
   void AddTransLimit(const mxs_vector &loc, LimitCallback callback);
   BOOL CheckTransLimits(const mxs_vector &start, const mxs_vector &end, mxs_vector *limit);
   void GetTransLimits(mxs_vector *limit_list, int *limit_list_size, int max_list_size) const;

   void SetFlagState(unsigned flag, BOOL state);

#ifndef SHIP
   ////////////////////////////////////
   //
   // Internal storage size
   //
   virtual long InternalSize() const;
#endif

protected:

   void SetType(tPhysSubModId subModId, ePhysModelType type);

private:
   friend tHashSetKey cPhysModelTable::GetKey(tHashSetNode) const;
   void UpdateDiskInfo();

   #define AssertSubModIdInRange(subModId) \
           AssertMsg1((subModId >= 0) && (subModId < m_nSubModels), "Submodel index out of range: 0", subModId);

   ////////////////////////////////////

   ObjID            m_objID;

   int              m_nSubModels;

   mxs_real         m_gravity;

   cDynArray<tSpringInfo> m_springInfo;

   unsigned         m_flags;

   int              m_nFaceContacts;
   int              m_nEdgeContacts;
   int              m_nVertexContacts;
   int              m_nObjectContacts;

   cPhysPos         m_pos;              // Overall model location and facing
   mxs_vector       m_cog;              // Overall model center of gravity (offset from pos)

   mxs_real         m_base_friction;    // Minimum friction value (for flying)

   int              m_medium;

   BOOL             m_pointVsTerrain;

   mxs_real         m_passThruDT;

   int              m_nAttachments;
   BOOL             m_isAttached;

   ePhysAxes        m_rotAxes;
   ePhysAxes        m_restAxes;
   BOOL             m_atRest;

   ObjID            m_climbingObj;
   int              m_mantlingState;
   mxs_vector       m_mantlingTargVec;

   ObjID            m_referenceFrameObj;

   int              m_ropeSegment;
   mxs_real         m_ropePct;

   long             m_lastSquishTime;

   ePhysModelType * m_pType;

   cDynArray<tConstraint> m_ConstraintList;
   cDynClassArray<cConstraintArray> m_SubModConstraintList;

   cDynArray<tVelocityConstraint> m_VelConstraintList;

   cPhysForceList * m_pForceList;
   cPhysPos *       m_pPosition;                  // one per submodel

   cDynArray<mxs_vector>  m_Offset;

   cAngLimitList    m_AngLimitList;
   cTransLimitList  m_TransLimitList;

   // For rigid bodies
   cPhysDynData     m_DynamicsData;
   cPhysCtrlData    m_ControlData;

   // For non-rigid bodies
   cDynClassArray<cPhysDynData>  m_pDynamicsData;    // one per submodel, if we're moveable
   cDynClassArray<cPhysCtrlData> m_pControlData;     // one per submodel, if we're controllable

   BOOL bDiskCheck; // We need to set our bits first time we're asked for them
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModel, inline functions
//

///////////////////////////////////////

inline ObjID cPhysModel::GetObjID() const
{
   return m_objID;
}

///////////////////////////////////////

inline ePhysModelType cPhysModel::GetType(tPhysSubModId subModId) const
{
   AssertSubModIdInRange(subModId);
   return m_pType[subModId];
}

///////////////////////////////////////

inline void cPhysModel::SetType(tPhysSubModId subModId, ePhysModelType type)
{
   AssertSubModIdInRange(subModId);
   m_pType[subModId] = type;
}

///////////////////////////////////////

inline tPhysSubModId cPhysModel::NumSubModels() const
{
   return m_nSubModels;
}

///////////////////////////////////////

inline const mxs_vector & cPhysModel::GetLocationVec(tPhysSubModId subModId) const
{
   AssertSubModIdInRange(subModId);
   return m_pPosition[subModId].m_position.loc.vec;
}

///////////////////////////////////////

inline const mxs_vector & cPhysModel::GetLocationVec() const
{
   return m_pos.m_position.loc.vec;
}

///////////////////////////////////////

inline const Location & cPhysModel::GetLocation(tPhysSubModId subModId) const
{
   AssertSubModIdInRange(subModId);
   return m_pPosition[subModId].m_position.loc;
}

///////////////////////////////////////

inline const Location & cPhysModel::GetLocation() const
{
   return m_pos.m_position.loc;
}

///////////////////////////////////////

inline const Position & cPhysModel::GetPosition(tPhysSubModId subModId) const
{
   AssertSubModIdInRange(subModId);
   return m_pPosition[subModId].m_position;
}

///////////////////////////////////////

inline const Position & cPhysModel::GetPosition() const
{
   return m_pos.m_position;
}

///////////////////////////////////////

inline const mxs_angvec & cPhysModel::GetRotation(tPhysSubModId subModId) const
{
   AssertSubModIdInRange(subModId);
   return m_pPosition[subModId].m_position.fac;
}

///////////////////////////////////////

inline const mxs_angvec & cPhysModel::GetRotation() const
{
   return m_pos.m_position.fac;
}

///////////////////////////////////////

inline void cPhysModel::SetRotation(tPhysSubModId subModId, const mxs_angvec & rot)
{
   AssertSubModIdInRange(subModId);
   mxs_angvec limit;

   CheckAngleLimits(m_pPosition[subModId].m_position.fac, rot, &limit);

   m_pPosition[subModId].m_position.fac = m_pPosition[subModId].m_targetposition.fac = limit;
}

///////////////////////////////////////

inline const mxs_vector &cPhysModel::GetSubModOffset(tPhysSubModId subModId) const
{
   return m_Offset[subModId];
}

///////////////////////////////////////

inline void cPhysModel::SetSubModOffset(tPhysSubModId subModId, const mxs_vector & offset)
{
#ifdef PLAYTEST
   if (mx_mag2_vec(&offset) > 10000.0)
      Warning((" has weirded physics offsets\n", ObjWarnName(GetObjID())));
#endif
   mx_copy_vec(&m_Offset[subModId], &offset);
}

///////////////////////////////////////

inline const mxs_vector & cPhysModel::GetCOGOffset() const
{
   return m_cog;
}

///////////////////////////////////////

inline void cPhysModel::SetCOG(const mxs_vector &cog)
{
   mx_sub_vec(&m_cog, (mxs_vector *) &cog, &m_pos.m_position.loc.vec);
}

///////////////////////////////////////

inline void cPhysModel::SetCOGOffset(const mxs_vector & cog_offset)
{
   mx_copy_vec(&m_cog, (mxs_vector *) &cog_offset);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsRemote() const
{
   return !!(m_flags & kPMF_Remote);
}

///////////////////////////////////////

inline void cPhysModel::SetRemote(BOOL state)
{
   SetFlagState(kPMF_Remote, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsPlayer() const
{
   return !!(m_flags & kPMF_Player);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsCreature() const
{
   return !!(m_flags & kPMF_Creature);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsAvatar() const
{
   return !!(m_flags & kPMF_Avatar);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsRope() const
{
   return !!(m_flags & kPMF_Rope);
}

///////////////////////////////////////

inline void cPhysModel::SetMovingTerrain(BOOL state)
{
   SetFlagState(kPMF_MovingTerrain, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsMovingTerrain() const
{
   return !!(m_flags & kPMF_MovingTerrain);
}

///////////////////////////////////////

inline void cPhysModel::SetDoor(BOOL state)
{
   SetFlagState(kPMF_Door, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsDoor() const
{
   return !!(m_flags & kPMF_Door);
}

///////////////////////////////////////

inline void cPhysModel::SetAICollides(BOOL state)
{
   SetFlagState(kPMF_AICollides, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::AICollides() const
{
   return !!(m_flags & kPMF_AICollides);
}

///////////////////////////////////////

inline void cPhysModel::SetProjectile(BOOL state)
{
   SetFlagState(kPMF_Projectile, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsProjectile() const
{
   return !!(m_flags & kPMF_Projectile);
}

///////////////////////////////////////

inline void cPhysModel::SetFancyGhost(BOOL state)
{
   SetFlagState(kPMF_FancyGhost, state);
   if (state)
      SetFlagState(kPMF_TrueGhost, FALSE);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsFancyGhost() const
{
   return !!(m_flags & kPMF_FancyGhost);
}

///////////////////////////////////////

inline void cPhysModel::SetTrueGhost(BOOL state)
{
   SetFlagState(kPMF_TrueGhost, state);
   if (state)
      SetFlagState(kPMF_FancyGhost, FALSE);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsTrueGhost() const
{
   return !!(m_flags & kPMF_TrueGhost);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsGhost() const
{
   return ((m_flags & kPMF_FancyGhost) || (m_flags & kPMF_TrueGhost));
}

///////////////////////////////////////

inline BOOL cPhysModel::IsPressurePlate() const
{
   return !!(m_flags & kPMF_PressurePlate);
}

///////////////////////////////////////

inline void cPhysModel::SetPressurePlate(BOOL state)
{
   SetFlagState(kPMF_PressurePlate, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsWeapon(tPhysSubModId subModId) const
{
   if ((m_flags & kPMF_Weapon) && (subModId < 4))
      return TRUE;
   else
      return FALSE;
}

///////////////////////////////////////

inline BOOL cPhysModel::IsHalo(tPhysSubModId subModId) const
{
   if ((m_flags & kPMF_Weapon) && (subModId >= 4))
      return TRUE;
   else
      return FALSE;
}

///////////////////////////////////////

inline BOOL cPhysModel::RemovesOnSleep() const
{
   return !!(m_flags & kPMF_RemovesOnSleep);
}

///////////////////////////////////////

inline void cPhysModel::SetRemovesOnSleep(BOOL state)
{
   SetFlagState(kPMF_RemovesOnSleep, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsClimbing() const
{
   return (m_climbingObj != OBJ_NULL);
}

///////////////////////////////////////

inline ObjID cPhysModel::GetClimbingObj() const
{
   return m_climbingObj;
}

///////////////////////////////////////

inline void cPhysModel::SetClimbingObj(ObjID climbing_obj)
{
   m_climbingObj = climbing_obj;
}

///////////////////////////////////////

inline int cPhysModel::GetRopeSegment() const
{
   return m_ropeSegment;
}

///////////////////////////////////////

inline void cPhysModel::SetRopeSegment(int segment)
{
   m_ropeSegment = segment;
}

///////////////////////////////////////

inline mxs_real cPhysModel::GetRopeSegmentPct() const
{
   return m_ropePct;
}

///////////////////////////////////////

inline void cPhysModel::SetRopeSegmentPct(mxs_real percent)
{
   m_ropePct = percent;
}

///////////////////////////////////////

inline BOOL cPhysModel::IsMantling() const
{
   return ((m_mantlingState > 0) && (m_mantlingState <= 4));
}

///////////////////////////////////////

inline int cPhysModel::GetMantlingState() const
{
   return m_mantlingState;
}

///////////////////////////////////////

inline void cPhysModel::SetMantlingState(int state)
{
   m_mantlingState = state;
}

///////////////////////////////////////

inline void cPhysModel::GetMantlingTargVec(mxs_vector *targ_vec) const
{
   mx_copy_vec(targ_vec, &m_mantlingTargVec);
}

///////////////////////////////////////

inline void cPhysModel::SetMantlingTargVec(mxs_vector &targ_vec)
{
   mx_copy_vec(&m_mantlingTargVec, &targ_vec);
}

///////////////////////////////////////

inline void cPhysModel::SetSquishingState(BOOL state)
{
   SetFlagState(kPMF_Squishing, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsSquishing() const
{
   return !!(m_flags & kPMF_Squishing);
}

///////////////////////////////////////

inline void cPhysModel::SetFacesVel(BOOL state)
{
   SetFlagState(kPMF_FacesVel, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::FacesVel() const
{
   return !!(m_flags & kPMF_FacesVel);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsActive() const
{
   return !(m_flags & kPMF_Inactive);
}

///////////////////////////////////////

inline void cPhysModel::SetStationary(BOOL state)
{
   SetFlagState(kPMF_Stationary, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsStationary() const
{
   return !!(m_flags & kPMF_Stationary);
}

///////////////////////////////////////

inline BOOL cPhysModel::CollisionChecked() const
{
   return !!(m_flags & kPMF_CollisionChecked);
}

///////////////////////////////////////

inline void cPhysModel::SetCollisionChecked(BOOL state)
{
   SetFlagState(kPMF_CollisionChecked, state);
}

///////////////////////////////////////

inline void cPhysModel::ClearConstraints()
{
   m_ConstraintList.SetSize(0);
   for (int i=0; i<m_nSubModels; i++)
      m_SubModConstraintList[i].SetSize(0);
   m_VelConstraintList.SetSize(0);
}

////////////////////////////////////////

inline void cPhysModel::SetObjReferenceFrame(ObjID obj)
{
   m_referenceFrameObj = obj;
}

///////////////////////////////////////

inline ObjID cPhysModel::GetObjReferenceFrame() const
{
   return m_referenceFrameObj;
}

///////////////////////////////////////

inline BOOL cPhysModel::InObjectContact()
{
   return (m_nObjectContacts > 0);
}

///////////////////////////////////////

inline void cPhysModel::SetObjectContact(BOOL state)
{
   if (state)
      m_nObjectContacts++;
   else
      m_nObjectContacts--;
}

///////////////////////////////////////

inline BOOL cPhysModel::InTerrainContact()
{
   return (InTerrainFaceContact() || InTerrainEdgeContact() || InTerrainVertexContact());
}

///////////////////////////////////////

inline BOOL cPhysModel::InTerrainFaceContact()
{
   return (m_nFaceContacts > 0);
}

///////////////////////////////////////

inline BOOL cPhysModel::InTerrainEdgeContact()
{
   return (m_nEdgeContacts > 0);
}

///////////////////////////////////////

inline BOOL cPhysModel::InTerrainVertexContact()
{
   return (m_nVertexContacts > 0);
}

///////////////////////////////////////

inline void cPhysModel::IncTerrainFaceContact()
{
   m_nFaceContacts++;
}

///////////////////////////////////////

inline void cPhysModel::IncTerrainEdgeContact()
{
   m_nEdgeContacts++;
}

///////////////////////////////////////

inline void cPhysModel::IncTerrainVertexContact()
{
   m_nVertexContacts++;
}

///////////////////////////////////////

inline void cPhysModel::SetEndLocationVec(const mxs_vector &end_loc)
{
   SetEndLocationVec(-1, end_loc);
}

///////////////////////////////////////

inline void cPhysModel::SetEndRotationVec(const mxs_angvec &end_ang)
{
   SetEndRotationVec(-1, end_ang);
}

///////////////////////////////////////

inline const mxs_vector &cPhysModel::GetTargetLocation(tPhysSubModId subModId) const
{
   if (subModId == -1)
      return m_pos.m_targetposition.loc.vec;
   else
   {
      AssertSubModIdInRange(subModId);
      return m_pPosition[subModId].m_targetposition.loc.vec;
   }
}

///////////////////////////////////////

inline int cPhysModel::GetMediaState() const
{
   return m_medium;
}

///////////////////////////////////////

inline mxs_real cPhysModel::GetBaseFriction() const
{
   return m_base_friction;
}

///////////////////////////////////////

inline void cPhysModel::SetBaseFriction(mxs_real base_friction)
{
   m_base_friction = base_friction;
}

///////////////////////////////////////

inline void cPhysModel::SetSpringTension(tPhysSubModId subModId, mxs_real tension)
{
   m_springInfo[subModId].tension = tension;
}

///////////////////////////////////////

inline void cPhysModel::SetSpringDamping(tPhysSubModId subModId, mxs_real damping)
{
   m_springInfo[subModId].damping = damping;
}

///////////////////////////////////////

inline mxs_real cPhysModel::GetSpringTension(tPhysSubModId subModId) const
{
   return m_springInfo[subModId].tension;
}

///////////////////////////////////////

inline mxs_real cPhysModel::GetSpringDamping(tPhysSubModId subModId) const
{
   return m_springInfo[subModId].damping;
}
///////////////////////////////////////

inline void cPhysModel::SetObjectPassThru(mxs_real dt)
{
   m_passThruDT = dt;
   SetSleep(FALSE);
}

///////////////////////////////////////

inline void cPhysModel::DecObjectPassThru(mxs_real dt)
{
   m_passThruDT -= dt;
}

///////////////////////////////////////

inline BOOL cPhysModel::ObjectPassThru() const
{
   return (m_passThruDT > 0);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsMoveable() const
{
   return ((m_flags & kPMF_NonMoveable) ? FALSE : TRUE);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsControllable() const
{
   return ((m_flags & kPMF_Uncontrollable) ? FALSE : TRUE);
}

///////////////////////////////////////

inline cPhysDynData *cPhysModel::GetDynamics(tPhysSubModId subModId) const
{
   if ((subModId == -1) || (m_pType[0] == kPMT_OBB))
      return (cPhysDynData *)&m_DynamicsData;
   else
   {
      AssertSubModIdInRange(subModId);
      return (cPhysDynData *)&m_pDynamicsData[subModId];
   }
}

///////////////////////////////////////

inline cPhysCtrlData *cPhysModel::GetControls(tPhysSubModId subModId) const
{
   if ((subModId == -1) || (m_pType[0] == kPMT_OBB))
      return (cPhysCtrlData *)&m_ControlData;
   else
   {
      AssertSubModIdInRange(subModId);
      return (cPhysCtrlData *)&m_pControlData[subModId];
   }
}

///////////////////////////////////////

inline BOOL cPhysModel::AllowedToSleep() const
{
   return !!(m_flags & kPMF_AllowedToSleep);
}

///////////////////////////////////////

inline void cPhysModel::SetAllowedToSleep(BOOL state)
{
   SetFlagState(kPMF_AllowedToSleep, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsSleeping() const
{
   return !!(m_flags & kPMF_Sleeping);
}

///////////////////////////////////////

inline BOOL cPhysModel::IsPointVsTerrain() const
{
   return m_pointVsTerrain;
}

///////////////////////////////////////

inline void cPhysModel::SetPointVsTerrain(BOOL state)
{
   m_pointVsTerrain = state;
}

///////////////////////////////////////

inline BOOL cPhysModel::IsPointVsNotSpecial() const
{
   return !!(m_flags & kPMF_PointVsNotSpecial);
}

///////////////////////////////////////

inline void cPhysModel::SetPointVsNotSpecial(BOOL state)
{
   SetFlagState(kPMF_PointVsNotSpecial, state);
}


///////////////////////////////////////

inline BOOL cPhysModel::DoDiskCheck()
{
   if (!bDiskCheck)
      UpdateDiskInfo();

   return !!(m_flags & kPMF_DiskCheck);
}

///////////////////////////////////////

inline void cPhysModel::SetDiskCheck(BOOL state)
{
   bDiskCheck = TRUE;
   SetFlagState(kPMF_DiskCheck, state);
}


///////////////////////////////////////

inline BOOL cPhysModel::IsSpecial() const
{
   return !!(m_flags & kPMF_Special);
}

///////////////////////////////////////

inline void cPhysModel::SetSpecial(BOOL state)
{
   SetFlagState(kPMF_Special, state);
}

///////////////////////////////////////

inline BOOL cPhysModel::UpdateRefsPending() const
{
   return !!(m_flags & kPMF_UpdateRefs);
}

///////////////////////////////////////

inline void cPhysModel::SetUpdateRefs(BOOL state)
{
   SetFlagState(kPMF_UpdateRefs, state);
}

///////////////////////////////////////

inline int cPhysModel::NumAttachments() const
{
   return m_nAttachments;
}

///////////////////////////////////////

inline void cPhysModel::AddAttachment()
{
   m_nAttachments++;
}

///////////////////////////////////////

inline void cPhysModel::RemoveAttachment()
{
   AssertMsg1(m_nAttachments > 0, "Number of attachments on obj 0 less than zero?\n", ObjID());

   m_nAttachments--;
}

///////////////////////////////////////

inline BOOL cPhysModel::IsAttached() const
{
   return m_isAttached;
}

///////////////////////////////////////

inline void cPhysModel::SetAttached(BOOL state)
{
   m_isAttached = state;
}

///////////////////////////////////////

inline ePhysAxes cPhysModel::GetRotAxes() const
{
   return m_rotAxes;
}

///////////////////////////////////////

inline void cPhysModel::SetRotAxes(int axes)
{
   m_rotAxes = (ePhysAxes)axes;
}

///////////////////////////////////////

inline ePhysAxes cPhysModel::GetRestAxes() const
{
   return m_restAxes;
}

///////////////////////////////////////

inline void cPhysModel::SetRestAxes(int axes)
{
   m_restAxes = (ePhysAxes)axes;
}

///////////////////////////////////////

inline BOOL cPhysModel::AtRest() const
{
   return m_atRest;
}

///////////////////////////////////////

inline void cPhysModel::SetRest(BOOL state)
{
   m_atRest = state;
}

///////////////////////////////////////

inline BOOL cPhysModel::Gravity() const
{
   return (fabs(m_gravity) > 0.0001);
}

///////////////////////////////////////

inline mxs_real cPhysModel::GetGravity() const
{
   return m_gravity;
}

///////////////////////////////////////

inline void cPhysModel::SetGravity(mxs_real value)
{
   m_gravity = value;
}

///////////////////////////////////////

inline const mxs_vector & cPhysModel::GetVelocity(tPhysSubModId subModId) const
{
   Assert_(IsMoveable());
   return GetDynamics(subModId)->GetVelocity();
}

///////////////////////////////////////

inline mxs_vector *cPhysModel::GetVelocityPtr(tPhysSubModId subModId)
{
   Assert_(IsMoveable());
   return GetDynamics(subModId)->GetVelocityPtr();
}

///////////////////////////////////////

inline void cPhysModel::SetFlagState(unsigned flag, BOOL state)
{
   if (state)
      m_flags |= flag;
   else
      m_flags &= ~flag;
}

///////////////////////////////////////

inline void cPhysModel::ClearAngleLimits()
{
   for (int i=0; i<m_AngLimitList.Size(); i++)
      delete m_AngLimitList[i];
   m_AngLimitList.SetSize(0);
}

///////////////////////////////////////

inline void cPhysModel::ClearTransLimits()
{
   for (int i=0; i<m_TransLimitList.Size(); i++)
      delete m_TransLimitList[i];
   m_TransLimitList.SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHMOD_H */
