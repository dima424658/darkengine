/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/ccrcray.cpp,v 1.3 1998/10/05 17:20:37 mahk Exp $

#include <creature.h>
#include <matrixs.h>
#include <ccrhuman.h>
// must be last header
#include <dbmem.h>

class cCrayCreature : public cHumanoidCreature
{
public:
   cCrayCreature(int type,ObjID objID) :
      cHumanoidCreature(type,objID) {}

   cCrayCreature(int type,ObjID objID, fCreatureReadWrite func, int version) : 
      cHumanoidCreature(type,objID,func,version) {}
   virtual ~cCrayCreature() {}

   virtual void FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat);

private:
   void Init();

   quat m_DefLClawRot;
   quat m_DefRClawRot;
};

class cCrayCreatureFactory : public cCreatureFactory
{
   virtual cCreature *Create(int type, ObjID objID) 
        { return new cCrayCreature(type,objID); }
   virtual cCreature *CreateFromFile(int type, ObjID objID, fCreatureReadWrite func, int version)
        { return new cCrayCreature(type, objID, func, version); }
};

static cCrayCreatureFactory g_CrayCreatureFactory;
EXTERN cCreatureFactory *g_pCrayCreatureFactory=&g_CrayCreatureFactory;

#define TPINCHER 14 // from crcraymn.c
#define BPINCHER 20 // from crcraymn.c

// To allow CrayMen to play human motions, we set bottom pincher rotation to
// be same as TPINCHER rotation if not playing a CrayMan motion, since
// human motions doesn't set this joint.
//
void cCrayCreature::FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat)
{
   cCreature::FilterMotionFrameData(mi,rot,xlat);

   // fix rotations for claws if not set by motion
   if(!QuerySigJoint(mi->sig,BPINCHER))
   {
      quat_copy(&rot[BPINCHER],&rot[TPINCHER]);
   }
}

