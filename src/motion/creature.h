/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/creature.h,v 1.50 2000/01/29 13:22:00 adurant Exp $
#pragma once

#ifndef __CREATURE_H
#define __CREATURE_H

#include <mp.h>
#include <motorapi.h>
#include <meshapi.h>
#include <mmresapi.h>
#include <matrixs.h>
#include <crettype.h>
#include <creatext.h>
#include <objtype.h>

// offset to torso * in lengths struct
#define CL_BASE_SIZE (2*sizeof(int))

#define kMPCalibVersion 1

// Creature location validation
#ifdef PLAYTEST

EXTERN BOOL g_AssertValidPosition;
#define VALIDATE_CREATURE_POS(x)   do { g_AssertValidPosition = x; } while (0);
BOOL IsCreaturePositionValid(ObjID objID);

#else

#define VALIDATE_CREATURE_POS(x)
#define IsCreaturePositionValid(x) (TRUE)
#define ValidateCreaturePosition(x)

#endif

typedef struct sCreatureLengths
{
   int nTorsos;
   int nLimbs;
   torso *pTorsos;
   limb  *pLimbs;
   float primScale; // how much limbs off the root toro are scaled from original
                    // motion capture skeleton.
} sCreatureLengths;

typedef struct _sCreatureMotionEndNode
{
   int motionNum;
   int frame;
   ulong flags;
   float timeSlop;
   _sCreatureMotionEndNode *pNext;
} sCreatureMotionEndNode;

class cCreatureFactory
{
public:
   virtual cCreature *Create(int type, ObjID objID) = 0;
   virtual cCreature *CreateFromFile(int type, ObjID objID, fCreatureReadWrite func, int version) = 0;
};

class cCreature: public IMotor, public IMesh
{
public:
// Constructor/Destructor
//
// XXX should remember to make this set multiped rel_orients and sig pointers. KJ 10/97
   cCreature(int type, ObjID objID);
// for reading from file
   cCreature(int type, ObjID objID,fCreatureReadWrite func, int version);
   virtual ~cCreature();

   void Write(fCreatureReadWrite func);

// Motor functions
//
   virtual void SetMotorResolver(IMotorResolver *pResolver);
   virtual int  GetActorType();
   virtual void StartMotion(const int motionNum) { StartMotionWithParam(motionNum,NULL,0,NULL); }
   virtual void StartMotionWithParam(const int motionNum,mps_motion_param *pParam,ulong flags,int startFrame);
   virtual void PoseAtMotionFrame(int motNum, float frame);
   virtual int  SwizzleMotion(int motNum);
   virtual int  GetCurrentMotion();

   virtual BOOL SetTransform(const mxs_vector *, const mxs_angvec *, BOOL movePhys=TRUE);
   virtual BOOL SetPosition(const mxs_vector *, BOOL movePhys=TRUE);
   virtual BOOL SetOrientation(const mxs_angvec *, BOOL movePhys=TRUE);
   virtual void GetTransform(mxs_vector *, mxs_angvec *);

   virtual const Location *GetLocation() const;
   virtual mxs_real GetButtZOffset() const;
   virtual mxs_real GetDefaultButtZOffset() const { return m_DefaultButtZOffset; }
   virtual void GetFeetJointIDs(int *numJoints, int **jointIDs) const;
   virtual ObjID GetObjID() const { return m_ObjID; };
   // OBJ_NULL means no focus
   virtual void SetFocus(ObjID obj,fix fSpeed) { m_MaxAngVel = fSpeed; m_MotorState.focus=obj; };
   virtual void SetFocus(ObjID obj) { SetFocus(obj,0x3800); };

   virtual const sMotorState *GetMotorState() const { return &m_MotorState; }
   virtual void SetContact(ulong flags) { m_MotorState.contactFlags=flags; }
   virtual void SetVelocity(const mxs_angvec *dir, const mxs_real speed) {};
   virtual BOOL IsPhysical() { return m_MotorState.IsPhysical; }
   virtual BOOL MakePhysical();
   virtual void MakeNonPhysical();
   virtual BOOL MakeBallistic(int style, BOOL force = FALSE);
   virtual void MakeNonBallistic(BOOL force = FALSE);
   virtual BOOL IsBallistic() { return m_MotorState.ballisticRefCount>0; }
   virtual BOOL PhysSubModIsBallistic(int index);
   virtual void MakePhysSubModBallistic(int index, int style);  // see creatext for styles
   virtual void SetPhysSubModScale(float scale, int submodel = -1);

   virtual void SetMotFrameOfReference(int type) { m_sMultiped.frame_of_reference=type; }
   virtual int GetMotFrameOfReference() { return m_sMultiped.frame_of_reference; }
   virtual void StopMotion(const int motionNum);
   virtual void Pause();

   virtual const multiped *GetMultiped() const;
   virtual float GetMotionScale() const;
   virtual int   GetRootJointID() const { return m_iRootJoint; }
   
   
// Mesh shape functions
//
   virtual float GetRadius();
   virtual void GetWorldBBox(mxs_vector *pBMin, mxs_vector *pBMax);

// Mesh renderer functions
//
   virtual void MeshJointPosCallback(const int jointID, mxs_trans *pTrans);
   virtual void MeshStretchyJointCallback(const int jointID, mxs_trans *pTrans, quat *pRot);

// Attachment/weapon functions
   virtual void GetWeaponAttachDefaults(int wtype, sCreatureAttachInfo *pInfo);
   virtual BOOL MakeWeaponPhysical(ObjID weapon, int wtype);
   virtual void MakeWeaponNonPhysical(ObjID weapon);
   virtual void AbortWeaponMotion(ObjID weapon);
   virtual sCrPhysModOffsetTable *GetWeaponPhysOffsets(int wtype);
   virtual void GetPhysSubModPos(sCrPhysModOffset *off,mxs_vector *pos);
   virtual void GetSubModelPos(int submod, mxs_vector *pPos);

// creature module functions
//
   void Update(const ulong timeDelta);
   fix GetMaxAngVel() const { return m_MaxAngVel; };

   int GetType() { return m_CreatureType; };
   mxs_vector &GetJoint(int joint) { return m_pJoints[joint]; };
   mxs_matrix &GetJointOrient(int joint) { return m_pOrients[joint]; };
   
   const sCreatureDesc *GetCreatureDesc();

   // moves creature joints, but does not update obj position in object system
   BOOL MoveButt(const mxs_vector *v, const mxs_angvec *, BOOL updatePhysics, BOOL allowTilt=FALSE);

   BOOL IsPlayingMotion() const;

   void PrepareToDie();
   void BeDead();

   IMotorResolver *GetMotorResolver() { return m_pMotorResolver; }

   void QueueCompletedMotion(const sCreatureMotionEndNode *pNode);
   void ProcessCompletedMotions();
   void HackProcessCompletedMotions();
   void ResetLengths();
   void SlamPhysicsModels();
   void FlushRelativeState();
   virtual void FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat);
   virtual void PostUpdateFilter(const ulong dt);

   mxs_matrix &GetHeadOrient();

   void SetTrackingObj(ObjID objID);
   void SetTrackingLoc(const mxs_vector &dir);
   void SetNoTracking();

   void  SetStandingObj(ObjID objID);
   ObjID GetStandingObj() const;

   void        SetStandingOffset(mxs_vector &vec);
   mxs_vector &GetStandingOffset();

   void Freeze() { m_IsFrozen=TRUE; }
   void UnFreeze() { m_IsFrozen=FALSE; }
   BOOL IsFrozen() { return m_IsFrozen; }

   BOOL UpdateObjPosition();

protected:

   BOOL SubFrameUpdate(const ulong dt);
   void RecomputeJointPositions();
   virtual void ComputeBoundingData();
   virtual int GetFocusJoint() { return m_sMultiped.num_joints; }
   virtual void GetFocusRotOffset(quat *pDest, const quat *pBase, const quat *rots) { quat_copy(pDest,(quat *)pBase); }
   
   int           m_ObjID;
   sMotorState   m_MotorState;
   int           m_CreatureType;
   int           m_nPhysModels;
   int           m_nJoints;
   float         m_PrimScale; // how much limbs off the root toro are scaled from original
                    // motion capture skeleton.
   float         m_DefaultButtZOffset;
   
   multiped    m_sMultiped;
   mxs_vector  m_BMin;
   mxs_vector  m_BMax;
   float       m_Radius;
   int         m_iRootJoint;
   mxs_vector *m_pJoints;
   mxs_matrix  *m_pOrients;
   torso      *m_pTorsos;  // number of these comes from multiped
   limb       *m_pLimbs;
   IMotorResolver *m_pMotorResolver; // this pointer should be set by motion coordinator
   sCreatureMotionEndNode *m_CompletedMotions;

   mxs_matrix m_headOrient;

   float      m_scalePhys;
   int        m_ballisticStyle;

   ObjID      m_trackingObj;
   mxs_vector m_trackingLoc;
   BOOL       m_IsFrozen;  

   ObjID      m_standingObj;
   mxs_vector m_standingOffset;
   // Added by Johan, this is used to define the speed the creature can turn by.
   fix		  m_MaxAngVel;
   
};

////////////////////
// Inline cCreature functions

inline const multiped *cCreature::GetMultiped() const
{
   return &m_sMultiped;
}

inline float cCreature::GetMotionScale() const
{
   return m_PrimScale;
}

inline BOOL cCreature::IsPlayingMotion() const
{
   return (mp_list_count(&m_sMultiped.main_motion)>0||m_sMultiped.num_overlays>0);
}

inline const Location *cCreature::GetLocation() const
{
   Position *pPos=ObjPosGet(m_ObjID);

   AssertMsg1(pPos,"No position for obj 0",m_ObjID);

   return &pPos->loc;
}

inline mxs_real cCreature::GetButtZOffset() const
{
   return m_sMultiped.global_pos.z-m_BMin.z;
}

inline mxs_matrix &cCreature::GetHeadOrient()
{
   return m_headOrient;
}

inline void cCreature::SetTrackingObj(ObjID objID)
{
   m_trackingObj = objID;
}

inline void cCreature::SetTrackingLoc(const mxs_vector &loc)
{
   m_trackingLoc = loc;
   m_trackingObj = -1;
}

inline void cCreature::SetNoTracking()
{
   m_trackingObj = OBJ_NULL;
}

inline void cCreature::SetStandingObj(ObjID objID)
{
   m_standingObj = objID;
}

inline ObjID cCreature::GetStandingObj() const
{
   return m_standingObj;
}

inline void cCreature::SetStandingOffset(mxs_vector &vec)
{
   m_standingOffset = vec;
}

inline mxs_vector &cCreature::GetStandingOffset()
{
   return m_standingOffset;
}

struct sCreatureHandle
{
   int type;
   cCreature     *pCreature;

   sCreatureHandle(int t = kCreatureTypeInvalid, cCreature* c = NULL) 
      : type(t),pCreature(c) {}; 

   sCreatureHandle(const sCreatureHandle& c) 
      :type(c.type),pCreature(c.pCreature) {};
};


// Creature system functions
//
EXTERN BOOL CreatureGetDefaultLengthsName(int type, char *name);

EXTERN cCreature *CreatureFromObj(ObjID obj);

#endif
