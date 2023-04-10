/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/crjoint.cpp,v 1.9 1998/10/23 01:49:25 CCAROLLO Exp $
//
// Creature joint accessor 
//

#include <matrix.h>

#include <creatur_.h>
#include <creature.h>
#include <crjoint.h>

#include <objedit.h> // for nice naming in Assert messages

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

// For sdescs and stuff that want string names of joints
char *g_pJointNames[kCJ_NumCreatureJoints + 1] = 
{
   "N/A",
   "Head",
   "Neck",
   "Abdomen",
   "Butt",
   "Left Shoulder",
   "Right Shoulder",
   "Left Elbow",
   "Right Elbow",
   "Left Wrist",
   "Right Wrist",
   "Left Fingers",
   "Right Fingers",
   "Left Hip",
   "Right Hip",
   "Left Knee",
   "Right Knee",
   "Left Ankle",
   "Right Ankle",
   "Left Toe",
   "Right Toe",
   "Tail",
};

////////////////////////////////////////

int GetCreatureJointID(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointId: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   return pCDesc->pJointMap[joint];
}

////////////////////////////////////////

mxs_vector &GetCreatureJointPos(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointPos: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   if (pCDesc->pJointMap[joint] == -1)
   {
      Warning(("Invalid joint %d for creature %s\n", joint, ObjWarnName(creature)));
      return pCreature->GetJoint(0);
   }
       
   return pCreature->GetJoint(pCDesc->pJointMap[joint]);
}

////////////////////////////////////////

mxs_matrix &GetCreatureJointOrient(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointOrient: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   if ((joint == kCJ_Head) && pCDesc->canHeadTrack)
      return pCreature->GetHeadOrient();
   else
   {
      if (pCDesc->pJointMap[joint] == -1)
      {
         Warning(("Invalid joint %d for creature %s\n", joint, ObjWarnName(creature)));
         return pCreature->GetJointOrient(0);
      }
               
      return pCreature->GetJointOrient(pCDesc->pJointMap[joint]);
   }
}

////////////////////////////////////////

void GetCreatureJointPosC(ObjID creature, int joint, mxs_vector *vec)
{
   *vec = GetCreatureJointPos(creature, joint);
}

////////////////////////////////////////

void  GetCreatureJointOrientC(ObjID creature, int joint, mxs_matrix *mat)
{
   *mat = GetCreatureJointOrient(creature, joint);
}

////////////////////////////////////////



