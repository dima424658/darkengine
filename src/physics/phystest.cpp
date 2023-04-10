/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phystest.cpp,v 1.36 2000/02/19 12:32:29 toml Exp $
//
// Physics test code
//

#ifndef SHIP

#include <stdlib.h>

#include <appagg.h>

#include <objtype.h>
#include <osysbase.h>
#include <iobjsys.h>
#include <objpos.h>
#include <objshape.h>
#include <collprop.h>
#include <config.h>

#include <playrobj.h>
extern "C" {
#include <camera.h>
}
#include <matrixs.h>
#include <matrix.h>
#include <portal.h>

#include <mnamprop.h>

#include <phystest.h>
#include <physapi.h>
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phctrl.h>
#include <phflist.h>
#include <phdbg.h>
#include <physcast.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//#define PHYS_TEST_APPLY_FORCES
//#define PHYS_TEST_ROTATIONAL_VELOCITY
#define PHYS_TEST_VELOCITY


BOOL OpenPos(ObjID objID)
{
   PhysAxisControlRotationalVelocity(objID, 2, -3);
   return TRUE;
}

BOOL ClosePos(ObjID objID)
{
   PhysAxisControlRotationalVelocity(objID, 2, 3);
   return FALSE;
}

void PhysicsTestRaycast(ObjID objID)
{
   Location loc;
   Location end;
   Location hit;
   mxs_angvec ang;
   ObjID hit_obj;

   CameraGetLocation(PlayerCamera(), &loc.vec, &ang);

   mxs_matrix mat;
   mxs_vector dir;

   mx_ang2mat(&mat, &ang);

   mxs_real length = 10.0;
   config_get_float("physcast_length", &length);

   mx_scale_vec(&dir, &mat.vec[0], length);

   mx_add_vec(&end.vec, &loc.vec, &dir);

   MakeLocationFromVector(&loc, &loc.vec);
   MakeLocationFromVector(&end, &end.vec);

   int result = PhysRaycast(loc, end, &hit, &hit_obj, kCollideAll);

   if (result != kCollideNone)
   {
      ObjPosUpdate(objID, &hit.vec, &ang);
   }
   else
   {
      ObjPosUpdate(objID, &end.vec, &ang);
   }
}

void PhysicsLaunchSphere(mxs_real speed)
{
   AutoAppIPtr(ObjectSystem);
   
   ObjID objID;

   objID = pObjectSystem->Create(-506, kObjectConcrete);
#if 0
   ObjSetCollisionType(objID, COLLISION_BOUNCE | COLLISION_NO_SOUND);
   ObjSetModelName(objID, "wcratec");
#endif
   ObjPosUpdate(objID, &PlayerCamera()->pos, &PlayerCamera()->ang);

   PhysDeregisterModel(objID);
   PhysRegisterSphereHat(objID, 0, 1);
   g_PhysModels.GetActive(objID)->SetRotAxes(XAxis | YAxis | ZAxis);
   g_PhysModels.GetActive(objID)->SetRestAxes(XAxis | YAxis | ZAxis| NegXAxis | NegYAxis | NegZAxis);
   PhysAxisSetVelocity(objID, 0, speed);

   PhysSetMass(objID, 20);
   PhysSetDensity(objID, .9);
   PhysSetElasticity(objID, 0);
}

void PhysicsLaunchOBB(mxs_real speed)
{
   IObjectSystem *pObjSys = AppGetObj(IObjectSystem);
   ObjID objID;
   objID = pObjSys->Create(ROOT_ARCHETYPE, kObjectConcrete);
   ObjSetCollisionType(objID, COLLISION_BOUNCE);
   ObjSetModelName(objID, "table");
   ObjPosUpdate(objID, &(PlayerCamera()->pos), &(PlayerCamera()->ang));
   PhysRegisterOBBDefault(objID);

   PhysAxisSetVelocity(objID, 0, speed);
}

void PhysicsJoyride()
{
   static BOOL status = TRUE;

   if (!PlayerObjectExists())
      return;

   cPhysModel *pModel = g_PhysModels.GetActive(PlayerObject());

   if (pModel == NULL)
      return;

   if (status == TRUE)
   {
      pModel->SetSpringTension(0, .8);
      pModel->SetSpringDamping(0, .94);

      status = FALSE;
   }
   else
   {
      pModel->SetSpringTension(0, .4);
      pModel->SetSpringDamping(0, .2);

      status = TRUE;
   }
}

#ifdef PHYS_TEST_VELOCITY
void PhysicsSetControl(mxs_real speed)
{
   cPhysModel *pModel;
   cPhysCtrlData *pCtrl;
   mxs_vector pos, unit, ctrl;
   mxs_angvec ang;
   mxs_matrix m;

   if (!PhysObjHasPhysics(4))
      return;

   CameraGetLocation(PlayerCamera(), &pos, &ang);
   mx_ang2mat(&m, &ang);
   mx_unit_vec(&unit, 0);
   mx_mat_mul_vec(&ctrl, &m, &unit);
   mx_scaleeq_vec(&ctrl, speed);

   pModel = g_PhysModels.GetActive(4);
   if (pModel->IsControllable())
   {
      pCtrl = pModel->GetControls(0);
     
      pCtrl->SetControlVelocity(ctrl);
   }
}


void PhysicsStopControl()
{
   if (!PhysObjHasPhysics(4))
      return;

   cPhysModel *pModel = g_PhysModels.GetActive(4);
   if (pModel->IsControllable())
   {
      cPhysCtrlData *pCtrl = pModel->GetControls(0);

      pCtrl->StopControlVelocity();
   }
}  
#endif

#ifdef PHYS_TEST_APPLY_FORCES
void PhysicsSetControl(mxs_real speed)
{
   mxs_vector     pos, end, force, unit, offset;
   mxs_angvec     ang;
   mxs_matrix     m;
   mxs_real       time;
   BOOL           result;
   sPhysForce    *flist;
   int            nforces;

   if (!PhysObjHasPhysics(4))
      return;

   CameraGetLocation(PlayerCamera(), &pos, &ang);
   mx_ang2mat(&m, &ang);
   mx_unit_vec(&unit, 0);
   mx_mat_mul_vec(&offset, &m, &unit);
   mx_scaleeq_vec(&offset, speed);
   mx_addeq_vec(&pos, &offset);

   PhysGetForceList(4, 0, &flist, &nforces);
   result  = PhysApplyForceList(4, 0, NULL, 0, .3, &pos, &end, &force, &time);

   if (result) 
   {
      cPhysModel *pModel = g_PhysModels.GetActive(4);

      ObjUpdateLocation(pModel->GetObjID(), OBJID_TO_PTR(pModel->GetObjID()),
                        pModel->GetLocationVec(0).x,
                        pModel->GetLocationVec(0).y,
                        pModel->GetLocationVec(0).z);
   }
   else
   {
      PhysClearForceList(4, 0);
   }
} 
   
void PhysicsStopControl()
{
   if (!PhysObjHasPhysics(4))
      return;

   cPhysModel *pModel = g_PhysModels.GetActive(4);
   cPhysCtrlData *pCtrl = pModel->GetControls(0);

   pCtrl->StopControlLocation();
}
#endif


#ifdef PHYS_TEST_ROTATIONAL_VELOCITY
void PhysicsSetControl(mxs_real speed)
{
   cPhysModel *pModel, *pModelNext;
   cPhysCtrlData *pCtrl;
   mxs_vector rot_speed = {speed * rand() / 32768, speed * rand() / 32768, speed * rand() / 32768};

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      pModelNext = g_PhysModels.GetNextActive(pModel);
      pCtrl = pModel->GetControls(0);
      pCtrl->SetControlRotationalVelocity(rot_speed);
      pModel = pModelNext;
   }
}

void PhysicsStopControl()
{
   cPhysModel *pModel, *pModelNext;
   cPhysCtrlData  *pCtrl;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      pModelNext = g_PhysModels.GetNextActive(pModel);
      pCtrl = pModel->GetControls(0);
      pCtrl->StopControlRotationalVelocity();
      pModel = pModelNext;
   }
}
#endif

#ifdef DBG_ON
///////////////////////////////////////
//
// Set debugging info for specific model/type of event
//

void PhysicsDebugObj(ObjID objID)
{
   g_PhysicsDebug.SetDebugObj(objID, kPhysicsDebugAll);
}

///////////////////////////////////////

void PhysicsDebug(int what)
{
   g_PhysicsDebug.SetDebugAll(what);
}
#endif // DBG_ON

#endif





