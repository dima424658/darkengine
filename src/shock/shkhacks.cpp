/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhacks.cpp,v 1.5 2000/02/19 13:25:19 toml Exp $
//
// wow this whole file looks like it can be killed someday soon...
//

#include <scrnman.h>
#include <string.h>
#include <objtype.h>
#include <playrobj.h>
//#include <prjctile.h>
#include <appsfx.h>
//#include <objhp.h>
//#include <mnamprop.h>
#include <appagg.h>
#include <comtools.h>
//#include <traitman.h>
#include <objmodel.h>
//#include <mprintf.h>
#include <objpos.h>
//#include <portal.h>

extern "C"
{
   #include <matrix.h>
   #include <camera.h>
   #include <wr.h>
}
#include <objsys.h>
#include <osysbase.h>
#include <objshape.h>
#include <property.h>
#include <propbase.h>
//#include <propman.h>
#include <rendobj.h>
#include <particle.h>

// for the CAMBOT
static ObjID gCambotID;
static BOOL gCamAtBot;

extern "C" void shock_init_hacks()
{
}

extern "C" void shock_kill_hacks()
{
}

//------------------------------------------------------------------------------------
// Returns the archetype id given the name.
//------------------------------------------------------------------------------------
int GetArchetypeID(char *label)
{
   AutoAppIPtr_(ObjectSystem,ObjSys);
   ObjID obj = ObjSys->GetObjectNamed(label);
   return (obj);
}

//------------------------------------------------------------------------------------
// Place a camera bot in front of the player.
//------------------------------------------------------------------------------------
void PlopDownCamBot()
{
   // old dead AIR code, useful for reference?
   /*
   // Get player camera info.
   Camera* c = PlayerCamera();
   mxs_vector camPos;
   mxs_angvec camAng;
   CameraGetLocation(c, &camPos, &camAng);

   // Pick a spot in front of player for cambot.
   camAng.tx = camAng.ty = 0;                // z angle is all we care about.
   mxs_matrix mat;
   mx_ang2mat(&mat, &camAng);
   mx_scale_addeq_vec(&camPos, &mat.vec[0], 3.0);

   // Create the cambot object.
   ObjID arch = GetArchetypeID("CamBot");
   ObjID cambotObj = BeginObjectCreate(arch, kObjectConcrete);
   if (cambotObj != OBJ_NULL)
   {
      ObjPosUpdate(cambotObj,&camPos,&camAng);
      EndObjectCreate(cambotObj);
   }

   // Assign it to the cambot.
   gCambotID = cambotObj;
   */
}

//------------------------------------------------------------------------------------
// Toggle between the player and the camera bot.
//------------------------------------------------------------------------------------
extern "C" void ShockHacksToggleCamBot()
{
   /*
   if (gCambotID == OBJ_NULL)
      return;

   if (!gCamAtBot)
   {
      Obj *p = OBJID_TO_PTR(gCambotID);
      mxs_vector botPos = p->pos->loc.vec;
      mxs_angvec botAng = p->pos->fac;
      CameraSetLocation(PlayerCamera(), &botPos, &botAng);
      CameraDetach(PlayerCamera());
   }
   else
   {
      CameraAttach(PlayerCamera(), PlayerObject());
   }

   gCamAtBot = !gCamAtBot;
   */
}

///////////////////////////////
// Bring creature physics up to date...
//

#ifdef EDITOR

#include <creature.h>
#include <creatur_.h>
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phprop.h>
#include <physapi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void UpdateCreaturePhysics(cCreature* pCreature)
{
   if (!pCreature->GetCreatureDesc()->hasSpecialPhysics)
      return;
   ObjID objID = pCreature->GetObjID();
   if (PhysObjHasPhysics(objID))
   {
      cPhysModel *pModel;
      if ((pModel = g_PhysModels.Get(objID)) != NULL)
      {
         pModel->SetFlagState(kPMF_Special, TRUE);
         UpdatePhysProperty(objID, PHYS_TYPE);
      }
   }
}

EXTERN void UpdateCreaturesPhysics(void)
{
   sCreatureHandle *pCHandle;
   int max = max_chandle_id(); 

   for (int i = 0; i < max; i++)
   {
      pCHandle=CreatureHandle(i);
      if(!pCHandle)
         continue;
      AssertMsg1(pCHandle->pCreature,"no creature at entry %d",i);
      UpdateCreaturePhysics(pCHandle->pCreature);
   }
}

#endif
