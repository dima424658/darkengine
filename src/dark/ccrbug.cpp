/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/ccrbug.cpp,v 1.3 1998/10/05 17:20:34 mahk Exp $

#include <creature.h>
#include <matrixs.h>
#include <ccrhuman.h>

// must be last header
#include <dbmem.h>

class cBugCreature : public cHumanoidCreature
{
public:
   cBugCreature(int type,ObjID objID) :
      cHumanoidCreature(type,objID) { Init(); }

   cBugCreature(int type,ObjID objID, fCreatureReadWrite func, int version) : 
      cHumanoidCreature(type,objID,func,version) { Init(); }
   virtual ~cBugCreature() {}

   virtual void FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat);

private:
   void Init();

   quat m_DefLClawRot;
   quat m_DefRClawRot;
};

class cBugCreatureFactory : public cCreatureFactory
{
   virtual cCreature *Create(int type, ObjID objID) 
        { return new cBugCreature(type,objID); }
   virtual cCreature *CreateFromFile(int type, ObjID objID, fCreatureReadWrite func, int version)
        { return new cBugCreature(type, objID, func, version); }
};

static cBugCreatureFactory g_BugCreatureFactory;
EXTERN cCreatureFactory *g_pBugCreatureFactory=&g_BugCreatureFactory;

#define LFINGER  16 // from crbugbst.c
#define RFINGER  17
#define LCLAW    20

static mxs_angvec g_DefLClawRot={0xb000,0,0};
static mxs_angvec g_DefRClawRot={0x5000,0,0};

void cBugCreature::Init()
{
   mxs_matrix mat;

   mx_ang2mat(&mat,&g_DefLClawRot);
   quat_from_matrix(&m_DefLClawRot,&mat);

   mx_ang2mat(&mat,&g_DefRClawRot);
   quat_from_matrix(&m_DefRClawRot,&mat);
}

// @NOTE: what we really want to do here is check the joint signature to see
// if LFINGER or RFINGER are set by the motion.  The motion
// processor currently sets the joint signature for a non-overlay motion to
// either be all joints up to the max joint id in .jm file, and it used to set it to be
// sig for all joints, even if there aren't rotations for them.  (there are lots
// of joints that don't have rotations, namely the end effector joints like
// top of the head, fingers and toes which are used for bbox computation and
// phys model placement purposes).  It doesn't need to do this any more, but
// it'll be a pain to reprocess all the motions.
// For now, we can differentiate between human and bugbeast motions by whether
// the finger joint actually has a component motion.    (KJ 6/98)
static BOOL IsBugBeastMotion(const mps_motion_info  *mi)
{
   int i;
   mps_motion *m=mp_motion_list+mi->mot_num;
   mps_comp_motion *cm=m->components;

   for(i=0;i<m->num_components;i++,cm++)
   {
      if(cm->joint_id==LFINGER)
         break;
   }
   return (i<m->num_components);  // then this must not be a human motion, so assume bugbeast
}

// To allow bugbeasts to play human motions, we set default values for the
// left and right finger rotations if not playing a bug-beast motion, since
// human motions don't set these joints.
//
void cBugCreature::FilterMotionFrameData(const mps_motion_info *mi, quat *rot, mxs_vector *xlat)
{
   cCreature::FilterMotionFrameData(mi,rot,xlat);

   // fix rotations for claws if not set by motion
   if(!IsBugBeastMotion(mi))
   {
      quat_copy(&rot[LFINGER],&m_DefLClawRot);
      quat_copy(&rot[RFINGER],&m_DefRClawRot);
   }
}

