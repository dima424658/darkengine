/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motset.h,v 1.6 2000/01/31 09:51:04 adurant Exp $
#pragma once

#ifndef __MOTSET_H
#define __MOTSET_H

#include <tagfile.h>
#include <label.h>
#include <dynarray.h>
#include <motion.h>
#include <motdbtyp.h>
#include <matrixs.h>

class IMotionSet
{
public:
   virtual ~IMotionSet() {}

// run-time functions
   virtual BOOL NeckIsFixed(int motHandle)=0;
   virtual int  BlendLength(int motHandle)=0;
   virtual Label *GetName(int motHandle)=0;
   virtual void GetStartEndFrames(int motHandle, int *pStart, int *pEnd)=0;
   virtual int GetNumFrames(int motHandle)=0;
   virtual mxs_ang GetEndHeading(int motHandle)=0;
   virtual void GetTranslation(int motHandle, mxs_vector *pXlat)=0;
   virtual float GetDuration(int motHandle)=0;
   virtual void GetSlidingVelocity(int motHandle, mxs_vector *pVel)=0;
   virtual BOOL WantInPlace(int motHandle)=0;

   virtual int  GetMotHandleFromName(const Label *name)=0;
   virtual int  NumMotions()=0;

// load/save functions
   virtual BOOL Load(ITagFile *pFile)=0;
   virtual BOOL Save(ITagFile *pFile)=0;
   virtual BOOL GetRunHandle(int index, int *pRunHandle)=0;

// set-building functions
   virtual void Clear()=0;
   virtual void StartBuild()=0;
   virtual void EndBuild()=0;
   virtual BOOL AddMotion(const Label *name, const sMotStuff *pStuff, int *pIndex)=0;

// virtual motion dealing.  These need to be registered before "add motion" gets called.
   virtual void RegisterVirtualMotion(const Label *name,const mps_motion *pMotion)=0;
};

IMotionSet *NewMotionSet();


#endif
