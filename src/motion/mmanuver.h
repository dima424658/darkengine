/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mmanuver.h,v 1.9 2000/01/31 09:50:13 adurant Exp $
#pragma once

#ifndef __MMANUVER_H
#define __MMANUVER_H

#include <motorapi.h>
#include <mclntapi.h>
#include <mctype.h>
#include <mctype_.h>
#include <dlist.h>
#include <motdbtyp.h>
#include <tagfile.h>

class cManeuver;
typedef cDList<cManeuver,1> cManeuverList;
typedef cDListNode<cManeuver, 1> cManeuverNode;

class cManeuver: public IManeuver, public cManeuverNode
{
public:
////////
// Motion System Client Functions
////////
   virtual ~cManeuver() {}

   virtual IManeuver *GetNextManeuver() { return GetNext(); }
   virtual IManeuver *GetPrevManeuver() { return GetPrevious(); }
   virtual void GetExpectedEndMoveState(sMcMoveState& moveState) \
      { AssertMsg(FALSE,"GetExpectedEndMoveState not supported yet"); }

////////
// Motion Coordinator Functions
////////
   virtual void Save(ITagFile *pTagFile) {}
   virtual void SetAppData(ulong data)
      { m_AppData = data; }
   virtual ulong GetAppData() { return m_AppData; }

   virtual void Execute()=0;

   // tells maneuver to finish executing.  Often comes just before deletion.
   // this is the appropriate time for the maneuver to store off state to be used
   // by the next executing maneuver.
   virtual void Finish() {}

   virtual void SetMotor(IMotor *pMotor) { m_pMotor=pMotor; }

////////
// Motor Resolver Functions
////////
   virtual void CalcEnvironmentEffect() {}
   virtual void CalcCollisionResponse(const mxs_vector *pForces, const int nForces,\
      const mxs_vector *pCurrentVel, mxs_vector *pNewVel) { mx_zero_vec(pNewVel); }
   virtual void NotifyAboutBeingStuck() {}
   virtual void NotifyAboutMotionAbortRequest() {}

   virtual void NotifyAboutFrameFlags(const int) {}
   virtual void NotifyAboutMotionEnd(int motionNum, int frame, ulong flags) {}

protected:
   IMotor *m_pMotor;
   cMotionCoordinator *m_pCoord;
   ulong m_AppData;
};

class cMotionPlan: public IMotionPlan, public cManeuverList
{
public:
   ~cMotionPlan() 
   {
      while (GetFirst())
         delete Remove(GetFirst());
   }

   virtual IManeuver *GetFirstManeuver() { return GetFirst(); }
   virtual IManeuver *GetLastManeuver() { return GetLast(); }
   virtual IManeuver *PopFirstManeuver() { return Remove( GetFirst() ); }
private:
// yadda yadda yadda.
};

class cManeuverFactory
{
public:
   virtual cMotionPlan *CreatePlan(const cMotionSchema *schema, const sMcMotorState& motState, \
      const sMcMoveState& moveState, const sMcMoveParams& params, IMotor *pMotor, cMotionCoordinator *pCoord)=0;

   virtual IManeuver *LoadManeuver(IMotor *pMotor,cMotionCoordinator *pCoord,ITagFile *pTagFile) { return NULL;}
};

#endif
