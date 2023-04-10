/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motorapi.h,v 1.18 2000/01/31 09:51:01 adurant Exp $
#pragma once

#ifndef __MMOTRAPI_H
#define __MMOTRAPI_H

#include <matrixs.h>
#include <motrtype.h>
#include <multiped.h>
#include <wrtype.h>
#include <objpos.h> // XXX for inline functions
#include <fix.h>

struct sMotorState
{
   mxs_vector position;
   mxs_angvec facing;
   BOOL IsPhysical;
   ulong  contactFlags;
   mxs_angvec direction;
   float timeSlop; // how much time was left in last frame when motion ended
   ObjID focus;
   int ballisticRefCount;
};

#define kMotStartFlag_ForceBlend 0x1 // blend the motion even if it doesn't by default
#define kMotStartFlag_SetStartFrame 0x2 // start frame is given as optional param

enum eMotorMotFrameOfReference  // These must correspond to MFRT constants in multiped.h 
{
   kMFRT_RelStart,      
   kMFRT_Arm,
   kMFRT_Global,
   kMFRT_Invalid=0xffffffff,
};

class IMotor
{
public:
   virtual void SetMotorResolver(IMotorResolver *pResolver)=0;

   virtual int  GetActorType()=0;
   virtual void StartMotion(const int motionNum)=0;
   virtual void StartMotionWithParam(const int motionNum,mps_motion_param *pParam,ulong flags=0,int startFrame=0)=0;
   virtual void StopMotion(const int)=0;
   virtual void PoseAtMotionFrame(int motNum, float frame)=0;
   // for load/save
   virtual int  SwizzleMotion(int motNum)=0;
   virtual int  GetCurrentMotion()=0;

   virtual BOOL SetTransform(const mxs_vector *, const mxs_angvec *, BOOL movePhys=TRUE)=0;
   virtual BOOL SetPosition(const mxs_vector *, BOOL movePhys=TRUE)=0;
   virtual BOOL SetOrientation(const mxs_angvec *, BOOL movePhys=TRUE)=0;
   virtual void GetTransform(mxs_vector *, mxs_angvec *)=0;
   virtual const Location *GetLocation() const =0;
   virtual mxs_real GetButtZOffset() const =0;
   virtual mxs_real GetDefaultButtZOffset() const =0;
   virtual void GetFeetJointIDs(int *numJoints, int **jointIDs) const =0;
   virtual ObjID GetObjID() const=0;
   virtual void SetFocus(ObjID obj)=0; // obj_null means no focus
   virtual void SetFocus(ObjID obj,fix fSpeed)=0; // obj_null means no focus

   virtual const sMotorState *GetMotorState() const =0;
   virtual void SetContact(ulong)=0;
   virtual void SetVelocity(const mxs_angvec *dir, const mxs_real speed)=0;
   virtual BOOL MakePhysical()=0;
   virtual void MakeNonPhysical()=0;
   virtual BOOL IsBallistic()=0;
   virtual void SetMotFrameOfReference(int type) = 0;
   virtual int GetMotFrameOfReference() = 0;

   virtual void Pause()=0;
   virtual const multiped *GetMultiped() const =0;
   virtual float GetMotionScale() const=0;
   virtual int  GetRootJointID() const=0;
};

#endif
