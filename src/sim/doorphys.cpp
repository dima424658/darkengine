/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/doorphys.cpp,v 1.53 2000/03/20 09:48:02 adurant Exp $
// door system

#include <lg.h>
#include <matrix.h>

#include <objpos.h>
#include <objshape.h>

#include <comtools.h>
#include <appagg.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <propbase.h>
#include <traitman.h>

#include <roomsys.h>
#include <room.h>
#include <rooms.h>

#include <psnd.h>

#include <physapi.h>
#include <phconst.h>
#include <phmod.h>
#include <phmods.h>
#include <phcore.h>
#include <phctrl.h>
#include <phmodobb.h>
#include <phprop.h>
#include <phoprop.h>
#include <phnet.h>
#include <plyrmode.h>
#include <playrobj.h>

#include <netman.h>
#include <iobjnet.h>

#include <doorprop.h>
#include <doorphys.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <drscrpt.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

#define DOOR_TOGGLE (0)
#define DOOR_OPEN   (1)
#define DOOR_CLOSE  (2)

////////////////////////////////////////////////////////////////////////////////
//
// Script messaging
//

static void SendDoorMessages(ObjID object, int what_happened, int what_was_happening)
{
   sDoorMsg::eDoorAction action_type;
   sDoorMsg::eDoorAction prev_action_type;
   BOOL isProxy = FALSE;

#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      AutoAppIPtr(ObjectNetworking);
      if (pObjectNetworking->ObjHostedHere(object)) {
         PhysBroadcastDoor(object, what_happened, what_was_happening);
      } else if (pObjectNetworking->ObjIsProxy(object)) {
         // Unlike most physics messages, we *do* let this squeak
         // through, so that scripts can do sound stuff. However,
         // we tell the scripts that it's a proxy, and it's on their
         // heads to do the right thing...
         isProxy = TRUE;
      }
   }
#endif

   switch (what_happened)
   {
      case kDoorClosed:
         action_type = sDoorMsg::kClose;
         break;

      case kDoorOpen:
         action_type = sDoorMsg::kOpen;
         break;

      case kDoorClosing:
         action_type = sDoorMsg::kClosing;
         break;

      case kDoorOpening:
         action_type = sDoorMsg::kOpening;
         break;

      case kDoorHalt:
         action_type = sDoorMsg::kHalt;
         break;

      default:
         CriticalMsg("Unknown door type for script message\n");
         break;
   }

   switch (what_was_happening)
   {
      case kDoorClosed:
         prev_action_type = sDoorMsg::kClose;
         break;

      case kDoorOpen:
         prev_action_type = sDoorMsg::kOpen;
         break;

      case kDoorClosing:
         prev_action_type = sDoorMsg::kClosing;
         break;

      case kDoorOpening:
         prev_action_type = sDoorMsg::kOpening;
         break;

      case kDoorHalt:
         prev_action_type = sDoorMsg::kHalt;
         break;

      default:
         CriticalMsg("Unknown door type for script message\n");
         break;
   }

   AutoAppIPtr(ScriptMan);
   sDoorMsg door_message(object, action_type, prev_action_type, isProxy);

   pScriptMan->SendMessage(&door_message);
}

////////////////////////////////////////////////////////////////////////////////
//
// Rotating door functions
//

BOOL RotDoorOpenCallback(int o_id)
{
   cPhysModel    *pModel = g_PhysModels.GetActive(o_id);
   sRotDoorProp  *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(g_pRotDoorProperty->IsRelevant(o_id), "Attempt to stop opening rotating door of non-rotating-door object");

   if (pModel == NULL)
      return FALSE;

   g_pRotDoorProperty->Get(o_id, &pDoorProp);

   // If we're closing, ignore
   if (pDoorProp->status == kDoorClosing)
      return FALSE;

   // Let go of the door
   pModel->GetControls()->StopControlRotationalVelocity();

   // Slam angle
   pModel->SetRotation(((sRotDoorProp *)pDoorProp)->base_open_facing);
   pModel->GetControls()->ControlRotation(((sRotDoorProp *)pDoorProp)->base_open_facing);

   // Slam position
   pModel->SetLocationVec(pDoorProp->base_open_location);

   pModel->SetSleep(TRUE);

   // Make sure it gets updated
   ObjPosUpdate(pModel->GetObjID(), &pModel->GetLocationVec(), 
                                    &pModel->GetRotation());


   SendDoorMessages(o_id, kDoorOpen, pDoorProp->status);

   // Set its status to open
   ROT_DOOR_LOCK;
   pDoorProp->status = kDoorOpen;
   g_pRotDoorProperty->Set(o_id, pDoorProp);
   ROT_DOOR_UNLOCK;

   return DoorHardLimits(o_id);
}

BOOL RotDoorCloseCallback(int o_id)
{
   cPhysModel    *pModel = g_PhysModels.GetActive(o_id);
   sRotDoorProp  *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(g_pRotDoorProperty->IsRelevant(o_id), "Attempt to stop closing rotating door of non-rotating-door object");

   if (pModel == NULL)
      return FALSE;

   g_pRotDoorProperty->Get(o_id, &pDoorProp);

   // If we're opening, ignore
   if (pDoorProp->status == kDoorOpening)
      return FALSE;

   // Let go of the door
   pModel->GetControls()->StopControlRotationalVelocity();

   // Slam angle
   pModel->SetRotation(((sRotDoorProp *)pDoorProp)->base_closed_facing);
   pModel->GetControls()->ControlRotation(((sRotDoorProp *)pDoorProp)->base_closed_facing);

   // Slam position
   pModel->SetLocationVec(pDoorProp->base_closed_location);

   pModel->SetSleep(TRUE);

   // Make sure it gets updated
   ObjPosUpdate(pModel->GetObjID(), &pModel->GetLocationVec(), 
                                    &pModel->GetRotation());

   SendDoorMessages(o_id, kDoorClosed, pDoorProp->status);

   // Set its status to closed
   ROT_DOOR_LOCK;
   pDoorProp->status = kDoorClosed;
   g_pRotDoorProperty->Set(o_id, pDoorProp);
   ROT_DOOR_UNLOCK;

   // Inform sound system of blocking sound
   AddDoorBlocking(o_id);

   return DoorHardLimits(o_id);
}

////////////////////////////////////////////////////////////////////////////////
//
// Translating door functions
//
BOOL TransDoorOpenCallback(int o_id)
{
   cPhysModel      *pModel = g_PhysModels.GetActive(o_id);
   sTransDoorProp  *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(g_pTransDoorProperty->IsRelevant(o_id), "Attempt to stop opening translating door of non-translating-door object");

   if (pModel == NULL)
      return FALSE;

   g_pTransDoorProperty->Get(o_id, &pDoorProp);

   // If we're closing, ignore
   if (pDoorProp->status == kDoorClosing)
      return FALSE;

   // Let go of the door
   pModel->GetControls()->StopControlVelocity();

   // Slam position
   pModel->SetLocationVec(pDoorProp->base_open_location);
   pModel->GetControls()->ControlLocation(pDoorProp->base_open_location);

   // Make sure it gets updated
   ObjPosUpdate(pModel->GetObjID(), &pModel->GetLocationVec(),
                                    &pModel->GetRotation());

   SendDoorMessages(o_id, kDoorOpen, pDoorProp->status);

   // Set its status to open
   pDoorProp->status = kDoorOpen;
   g_pTransDoorProperty->Set(o_id, pDoorProp);

   return TRUE;
}

BOOL TransDoorClosedCallback(int o_id)
{
   cPhysModel      *pModel = g_PhysModels.GetActive(o_id);
   sTransDoorProp  *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(g_pTransDoorProperty->IsRelevant(o_id), "Attempt to stop closing translating door of non-translating-door object");

   if (pModel == NULL)
      return FALSE;

   g_pTransDoorProperty->Get(o_id, &pDoorProp);

   // If we're opening, ignore
   if (pDoorProp->status == kDoorOpening)
      return FALSE;

   // Let go of the door
   pModel->GetControls()->StopControlVelocity();

   // Slam position
   pModel->SetLocationVec(pDoorProp->base_closed_location);
   pModel->GetControls()->ControlLocation(pDoorProp->base_closed_location);

   // Make sure it gets updated
   ObjPosUpdate(pModel->GetObjID(), &pModel->GetLocationVec(),
                                    &pModel->GetRotation());

   SendDoorMessages(o_id, kDoorClosed, pDoorProp->status);

   // Set its status to closed
   pDoorProp->status = kDoorClosed;
   g_pTransDoorProperty->Set(o_id, pDoorProp);

   // Inform sound system of blocking sound
   AddDoorBlocking(o_id);

   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// General door functions
//

void DoorSlamOpen(int o_id)
{
   sRotDoorProp *pRotDoorProp = NULL;
   sTransDoorProp *pTransDoorProp = NULL;
   sDoorProp *pDoorProp;

   // Get the door property
   if (!g_pRotDoorProperty->Get(o_id, &pRotDoorProp))
   {
      if (!g_pTransDoorProperty->Get(o_id, &pTransDoorProp))
      {
         Warning(("DoorSlamOpen: no door property on obj %d\n", o_id));
         return;
      }
      else
         pDoorProp = pTransDoorProp;
   }
   else
      pDoorProp = pRotDoorProp;

   PhysStopControlVelocity(o_id);
   PhysStopControlRotationalVelocity(o_id);

   PhysControlLocation(o_id, &pDoorProp->base_open_location);
   if (pRotDoorProp)
   {
      PhysControlRotation(o_id, &pRotDoorProp->base_open_facing);
      ObjPosUpdate(o_id, &pDoorProp->base_open_location, &pRotDoorProp->base_open_facing);
   }
   else
      ObjTranslate(o_id, &pDoorProp->base_open_location);
 
   UpdateDoorPhysics(o_id);

   SendDoorMessages(o_id, kDoorOpen, pDoorProp->status);
   pDoorProp->status = kDoorOpen;

   if (pRotDoorProp)
      g_pRotDoorProperty->Set(o_id, pRotDoorProp);
   else
   if (pTransDoorProp)
      g_pTransDoorProperty->Set(o_id, pTransDoorProp);

   RemoveDoorBlocking(o_id);
}

void DoorSlamClosed(int o_id)
{
   sRotDoorProp *pRotDoorProp = NULL;
   sTransDoorProp *pTransDoorProp = NULL;
   sDoorProp *pDoorProp;

   // Get the door property
   if (!g_pRotDoorProperty->Get(o_id, &pRotDoorProp))
   {
      if (!g_pTransDoorProperty->Get(o_id, &pTransDoorProp))
      {
         Warning(("DoorSlamClosed: no door property on obj %d\n", o_id));
         return;
      }
      else
         pDoorProp = pTransDoorProp;
   }
   else
      pDoorProp = pRotDoorProp;
   

   PhysStopControlVelocity(o_id);
   PhysStopControlRotationalVelocity(o_id);

   PhysControlLocation(o_id, &pDoorProp->base_closed_location);
   if (pRotDoorProp)
   {
      PhysControlRotation(o_id, &pRotDoorProp->base_closed_facing);
      ObjPosUpdate(o_id, &pDoorProp->base_closed_location, &pRotDoorProp->base_closed_facing);
   }
   else
      ObjTranslate(o_id, &pDoorProp->base_closed_location);

   UpdateDoorPhysics(o_id);

   SendDoorMessages(o_id, kDoorClosed, pDoorProp->status);
   pDoorProp->status = kDoorClosed;

   if (pRotDoorProp)
      g_pRotDoorProperty->Set(o_id, pRotDoorProp);
   else
   if (pTransDoorProp)
      g_pTransDoorProperty->Set(o_id, pTransDoorProp);

   AddDoorBlocking(o_id);
}

BOOL DoorStartOpening(int o_id)
{
   cPhysModel  *pModel = g_PhysModels.GetActive(o_id);
   sDoorProp   *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(IsDoor(o_id), "Attempt to start opening door of non-door object");

   if (pModel == NULL)
      return FALSE;

   pDoorProp = GetDoorProperty(o_id);

   if (mx_dist2_vec(&ObjPosGet(o_id)->loc.vec, &pDoorProp->base_open_location) < 0.01)
   {    
      pDoorProp->status = kDoorOpening;

      switch (pDoorProp->type)
      {
         case ROTATING_DOOR:     RotDoorOpenCallback(o_id);  break;
         case TRANSLATING_DOOR:  TransDoorOpenCallback(o_id);  break;
      }

      return TRUE;
   }

   SendDoorMessages(o_id, kDoorOpening, pDoorProp->status);

   // Set its status to opening
   pDoorProp->status = kDoorOpening;

   switch (pDoorProp->type)
   {
      case ROTATING_DOOR:
      {
         mxs_matrix obj_rotation;

         sRotDoorProp *pRotDoorProp = (sRotDoorProp *)pDoorProp;
         mxs_vector   rotation;
         mxs_vector   objrel_rot;

         ROT_DOOR_LOCK;
         g_pRotDoorProperty->Set(o_id, pRotDoorProp);
         ROT_DOOR_UNLOCK;

         // Make rotation possible
         pModel->GetControls()->StopControlRotation();

         // Figure out how to rotate
         mx_zero_vec(&rotation);
         rotation.el[pDoorProp->axis] = (pRotDoorProp->clockwise) ? -pDoorProp->base_speed : pDoorProp->base_speed;

         mx_ang2mat(&obj_rotation, &pDoorProp->base_angle);
         mx_mat_mul_vec(&objrel_rot, &obj_rotation, &rotation);

         // And do it
         pModel->GetControls()->SetControlRotationalVelocity(objrel_rot);

         break;
      }
      case TRANSLATING_DOOR:
      {
         mxs_real dir;

         g_pTransDoorProperty->Set(o_id, (sTransDoorProp *)pDoorProp);

         // Make translation possible
         pModel->GetControls()->StopControlLocation();
         pModel->SetGravity(0);
         pModel->SetBaseFriction(kFrictionFactor * kGravityAmt);

         // Figure out how to translate
         if (pDoorProp->open > pDoorProp->closed)
            dir = 1.0;
         else
            dir = -1.0;

         // And do it
         pModel->GetControls()->SetControlVelocity(pDoorProp->axis, pDoorProp->base_speed * dir);

         break;
      }
   }

   UpdatePhysProperty(o_id, PHYS_CONTROL);

   // Unblock the door
   RemoveDoorBlocking(o_id);

   return TRUE;
}

BOOL DoorStartClosing(int o_id)
{
   cPhysModel  *pModel = g_PhysModels.GetActive(o_id);
   sDoorProp   *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(IsDoor(o_id), "Attempt to start closing door of non-door object");

   if (pModel == NULL)
      return FALSE;

   pDoorProp = GetDoorProperty(o_id);

   if (mx_dist2_vec(&ObjPosGet(o_id)->loc.vec, &pDoorProp->base_closed_location) < 0.01)
   {    
      pDoorProp->status = kDoorClosing;

      switch (pDoorProp->type)
      {
         case ROTATING_DOOR:     RotDoorCloseCallback(o_id);  break;
         case TRANSLATING_DOOR:  TransDoorClosedCallback(o_id);  break;
      }

      return TRUE;
   }

   SendDoorMessages(o_id, kDoorClosing, pDoorProp->status);

   // Set its status to closing
   pDoorProp->status = kDoorClosing;

   switch (pDoorProp->type)
   {
      case ROTATING_DOOR:
      {
         mxs_matrix obj_rotation;

         sRotDoorProp *pRotDoorProp = (sRotDoorProp *)pDoorProp;
         mxs_vector    rotation;
         mxs_vector    objrel_rot;

         ROT_DOOR_LOCK;
         g_pRotDoorProperty->Set(o_id, pRotDoorProp);
         ROT_DOOR_UNLOCK; 

         // Make rotation possible
         pModel->GetControls()->StopControlRotation();

         // Figure out how to rotate
         mx_zero_vec(&rotation);
         rotation.el[pDoorProp->axis] = (pRotDoorProp->clockwise) ? pDoorProp->base_speed : -pDoorProp->base_speed;

         mx_ang2mat(&obj_rotation, &pDoorProp->base_angle);
         mx_mat_mul_vec(&objrel_rot, &obj_rotation, &rotation);

         // And do it
         pModel->GetControls()->SetControlRotationalVelocity(objrel_rot);

         break;
      }
      case TRANSLATING_DOOR:
      {
         mxs_real dir;

         g_pTransDoorProperty->Set(o_id, (sTransDoorProp *)pDoorProp);

         // Figure out hot to translate
         if (pDoorProp->open > pDoorProp->closed)
            dir = -1.0;
         else
            dir = 1.0;

         // And do it
         pModel->GetControls()->SetControlVelocity(pDoorProp->axis, pDoorProp->base_speed * dir);

         break;
      }
   }

   UpdatePhysProperty(o_id, PHYS_CONTROL);

   return TRUE;
}

// physically activate the door
BOOL DoorPhysActivate(int o_id, int what_to_do)
{
   cPhysModel     *pModel;
   cPhysModel     *pModel2;
   sDoorProp      *pDoorProp;

   AssertMsg(IsDoor(o_id), "Attempt to activate door of non-door object");
   pDoorProp = GetDoorProperty(o_id);

   pModel = g_PhysModels.GetActive(o_id);
   AssertMsg2(pModel, 
              "Door %d has no physics model? It %s inactive.", 
              o_id,
              ((pModel2 = g_PhysModels.GetInactive(o_id))
               ? "is" : "is not"));

   if (pModel == NULL)
      return FALSE;

   switch (what_to_do)
   {
      case DOOR_OPEN:
      {
         DoorStartOpening(o_id);
         return TRUE;
      }

      case DOOR_CLOSE:
      {
         DoorStartClosing(o_id);
         return TRUE;
      }

      case DOOR_TOGGLE:
      {
         switch (GetDoorStatus(o_id))
         {
            case kDoorClosed:
            case kDoorClosing:
            case kDoorHalt:
               DoorStartOpening(o_id);
               break;

            case kDoorOpen:
            case kDoorOpening:
               DoorStartClosing(o_id);
               break;          
  
            default:
               return FALSE;
         }
         return TRUE;
      }

      default:
         return FALSE;
   }
}

// abort the door's motion
void DoorPhysAbort(int o_id)
{
   cPhysModel  *pModel = g_PhysModels.GetActive(o_id);
   sDoorProp   *pDoorProp;

   pDoorProp = GetDoorProperty(o_id);

   // If we're not in an active state, ignore
   if ((pDoorProp->status == kDoorClosed) || (pDoorProp->status == kDoorOpen))
      return;

   // Let go of door
   pModel->GetControls()->StopControlRotationalVelocity();
   pModel->GetDynamics()->Stop();
   pModel->SetSleep(TRUE);

   SendDoorMessages(o_id, kDoorHalt, pDoorProp->status);

   pDoorProp->status = kDoorHalt;

   if (g_pRotDoorProperty->IsRelevant(o_id))
   {
      ROT_DOOR_LOCK;
      g_pRotDoorProperty->Set(o_id, (sRotDoorProp *)pDoorProp);
      ROT_DOOR_UNLOCK;
      //recontrol our Rotation.
      pModel->GetControls()->ControlRotation(pModel->GetRotation());
   }
   else
   {
      g_pTransDoorProperty->Set(o_id, (sTransDoorProp *)pDoorProp);
      //recontrol our Location.
      pModel->GetControls()->ControlLocation(pModel->GetLocationVec());
   }
}

// get the door's status
int GetDoorStatus(int o_id)
{
   sDoorProp *pDoorProp;

   AssertMsg1(IsDoor(o_id), "Attempt to get door status of non-door object %d",o_id);
   pDoorProp = GetDoorProperty(o_id);

   return pDoorProp->status;
}

// update the door property from the brush
void UpdateDoorBrush(int o_id)
{
   sDoorProp *pDoorProp;

   AssertMsg(IsDoor(o_id), "Attempt to update door property of non-door object");
   pDoorProp = GetDoorProperty(o_id);

   switch (pDoorProp->type)
   {
      case ROTATING_DOOR:
      {
         pDoorProp->base = ObjPosGet(o_id)->fac.el[pDoorProp->axis] * 180.0 / MX_ANG_PI;
         break;
      }
      case TRANSLATING_DOOR:
      {
         pDoorProp->base = ObjPosGet(o_id)->loc.vec.el[pDoorProp->axis];
         break;
      }
   }

   if (pDoorProp->status == kDoorClosed)
   {
      pDoorProp->base_angle = ObjPosGet(o_id)->fac;
      mx_copy_vec(&pDoorProp->base_location, &ObjPosGet(o_id)->loc.vec);
   }

   // Remove previous blocking
   RemoveDoorBlocking(o_id);

   GenerateBaseDoorLocations(o_id);
   GenerateDoorRooms(o_id);

   UpdateDoorPhysics(o_id);

   // Add new blocking
   if (pDoorProp->status == kDoorClosed)
      AddDoorBlocking(o_id);
}

// updates the door's physics from the property
void UpdateDoorPhysics(int o_id)
{
   cPhysModel *pModel;
   sDoorProp  *pDoorProp;
   mxs_vector cog;
   mxs_vector edge_lengths;

   pModel = g_PhysModels.GetActive(o_id);
   AssertMsg(pModel, "Door has no physics model?");

   if (pModel == NULL)
      return;

   pModel->SetDoor(TRUE);

   AssertMsg(IsDoor(o_id), "Attempt to update door physics of non-door object");

   pDoorProp = GetDoorProperty(o_id);

   // find our archetype so we can determine whether we need to init parts 
   AutoAppIPtr_(TraitManager, pTraitMan);

   ObjID door_arch = pTraitMan->GetArchetype(o_id);

   BOOL init_cog = !g_pPhysAttrProp->IsRelevant(door_arch);
   BOOL init_ctrl = !g_pPhysControlProp->IsRelevant(door_arch);

   if (!g_pPhysTypeProp->IsRelevant(o_id))
      PhysRegisterOBBDefault(o_id);

   if (init_ctrl)
   {
      pModel->GetControls()->ControlLocation(pModel->GetLocationVec());
      pModel->GetControls()->ControlRotation(pModel->GetRotation());
   }

   if (init_cog)
   {
      mx_copy_vec(&cog, &pModel->GetCOGOffset());

      if (mx_mag2_vec(&cog) == 0.0)
      {
         mx_copy_vec(&edge_lengths, &((cPhysOBBModel *)pModel)->GetEdgeLengths());

         if (pDoorProp->type == ROTATING_DOOR)
         {
            switch (pDoorProp->axis)
            {
               case 0:
               case 1:
               {
                  cog.el[2] = edge_lengths.el[2] / 2;
                  pModel->SetCOGOffset(cog);
                  break;
               }
               case 2:
               {
                  cog.el[0] = edge_lengths.el[0] / 2;
                  pModel->SetCOGOffset(cog);
                  break;
               }
            }
         }
      }
   }

   // Set limits
   switch (pDoorProp->type)
   {
      case ROTATING_DOOR:
      {
         pModel->ClearAngleLimits();

         if (pDoorProp->status == kDoorClosed)
         {
            pModel->AddAngleLimit(pDoorProp->axis, pDoorProp->open, RotDoorOpenCallback);
            pModel->AddAngleLimit(pDoorProp->axis, 0, RotDoorCloseCallback);
         }
         else
         {
            int closed = pDoorProp->closed + 360 - pDoorProp->open;

            pModel->AddAngleLimit(pDoorProp->axis, 0, RotDoorOpenCallback);
            pModel->AddAngleLimit(pDoorProp->axis, closed, RotDoorCloseCallback);
         }
         break;
      }
      case TRANSLATING_DOOR:
      {
         pModel->ClearTransLimits();
         pModel->AddTransLimit(pDoorProp->base_open_location, TransDoorOpenCallback);
         pModel->AddTransLimit(pDoorProp->base_closed_location, TransDoorClosedCallback);

         break;
      }
   }

   UpdatePhysProperty(o_id, PHYS_ATTR | PHYS_STATE | PHYS_CONTROL);
}

// generate the base (open & closed) locations and store them off
void GenerateBaseDoorLocations(int o_id)
{
   cPhysModel *pModel = g_PhysModels.GetActive(o_id);
   sDoorProp  *pDoorProp;

   AssertMsg(pModel, "Door has no physics model?");
   AssertMsg(IsDoor(o_id), "Attempt to update door base locations of non-door object");

   if (pModel == NULL)
      return;

   pDoorProp = GetDoorProperty(o_id);

   switch (pDoorProp->type)
   {
      case ROTATING_DOOR:
      {
         mxs_angvec ang_offset_open;
         mxs_angvec ang_offset_closed;

         mx_mk_angvec(&ang_offset_open, 0, 0, 0);
         mx_mk_angvec(&ang_offset_closed, 0, 0, 0);

         ang_offset_open.el[pDoorProp->axis] = pDoorProp->open * MX_ANG_PI / 180;
         ang_offset_closed.el[pDoorProp->axis] = pDoorProp->closed * MX_ANG_PI / 180;

         mxs_matrix rotation, object, both_rot;
         mxs_vector base_cog_offset;
         mxs_vector end_cog_offset;
         mxs_vector add_offset;

         mx_ang2mat(&object, &pDoorProp->base_angle);

         // open
         mx_ang2mat(&rotation, &ang_offset_open);

         mx_mat_mul_vec(&base_cog_offset, &object, &pModel->GetCOGOffset());
         mx_mul_mat(&both_rot, &object, &rotation);
         mx_mat_mul_vec(&end_cog_offset, &both_rot, &pModel->GetCOGOffset());

         mx_sub_vec(&add_offset, &end_cog_offset, &base_cog_offset);
         mx_add_vec(&pDoorProp->base_open_location, &pDoorProp->base_location, &add_offset);
#if 0
         if ((((short)pDoorProp->open + (short)pDoorProp->base) % 360) > (((short)pDoorProp->closed + (short)pDoorProp->base) % 360))
#endif
         if (((sRotDoorProp *)pDoorProp)->clockwise)
            ang_offset_open.el[pDoorProp->axis] += 1;
         else
            ang_offset_open.el[pDoorProp->axis] -= 1;

         mx_ang2mat(&rotation, &ang_offset_open);
         mx_mul_mat(&both_rot, &object, &rotation);
         mx_mat2ang(&((sRotDoorProp *)pDoorProp)->base_open_facing, &both_rot);

         // closed
         mx_ang2mat(&rotation, &ang_offset_closed);

         mx_mat_mul_vec(&base_cog_offset, &object, &pModel->GetCOGOffset());
         mx_mul_mat(&both_rot, &object, &rotation);
         mx_mat_mul_vec(&end_cog_offset, &both_rot, &pModel->GetCOGOffset());

         mx_sub_vec(&add_offset, &end_cog_offset, &base_cog_offset);
         mx_add_vec(&pDoorProp->base_closed_location, &pDoorProp->base_location, &add_offset);

#if 0
         if ((((short)pDoorProp->open + (short)pDoorProp->base) % 360) > (((short)pDoorProp->closed + (short)pDoorProp->base) % 360))
#endif
         if (((sRotDoorProp *)pDoorProp)->clockwise)
            ang_offset_closed.el[pDoorProp->axis] -= 1;
         else
            ang_offset_closed.el[pDoorProp->axis] += 1;

         mx_ang2mat(&rotation, &ang_offset_closed);
         mx_mul_mat(&both_rot, &object, &rotation);
         mx_mat2ang(&((sRotDoorProp *)pDoorProp)->base_closed_facing, &both_rot);

         break;
      }
      case TRANSLATING_DOOR:
      {
         mxs_matrix object;
         mxs_vector offset, rot_offset;

         mx_zero_vec(&offset);

         if ((pDoorProp->status == kDoorClosed) || (pDoorProp->status == kDoorOpen))
         {

            mx_copy_vec(&pDoorProp->base_closed_location, &pModel->GetLocationVec());
            mx_copy_vec(&pDoorProp->base_open_location, &pModel->GetLocationVec());

            switch (pDoorProp->status)
            {
               case kDoorClosed: 
               {
                  offset.el[pDoorProp->axis] = pDoorProp->open - pDoorProp->closed;

                  // Rotate offset into door's frame of reference
                  mx_ang2mat(&object, &ObjPosGet(o_id)->fac);
                  mx_mat_mul_vec(&rot_offset, &object, &offset);

                  mx_addeq_vec(&pDoorProp->base_open_location, &rot_offset);
                  break;
               }
               case kDoorOpen:
               {
                  offset.el[pDoorProp->axis] = pDoorProp->closed - pDoorProp->open; 

                  // Rotate offset into door's frame of reference
                  mx_ang2mat(&object, &ObjPosGet(o_id)->fac);
                  mx_mat_mul_vec(&rot_offset, &object, &offset);

                  mx_addeq_vec(&pDoorProp->base_closed_location, &rot_offset);
                  break;
               }
            }
         }

         break;
      }
   }
}

#define CREEP_SPEED     (0.25)
#define START_CREEP     (0.1)
#define MAX_CREEP_LEN2  (25)

// Determine the rooms on each side of every door
void GenerateAllDoorRooms()
{
   IObjectQuery *iter;
   AutoAppIPtr(ObjectSystem);

   iter = pObjectSystem->Iter(kObjectConcrete);

   while(!iter->Done())
   {
      if (IsDoor(iter->Object()))
         UpdateDoorBrush(iter->Object());

      iter->Next();
   }
   // Always release your queries! 
   SafeRelease(iter); 
}

// Determine the rooms on each side of the door
void GenerateDoorRooms(int o_id)
{
   sDoorProp *pDoorProp;
   mxs_vector bbox;
   int        best_axis = -1;
   mxs_real   best_size;
   int        i;

   pDoorProp = GetDoorProperty(o_id);

   // Find smallest bbox axis
   ObjGetUnscaledDims(o_id, &bbox);
   for (i=0; i<3; i++)
   {
      if ((best_axis == -1) || (bbox.el[i] < best_size))
      {
         best_size = bbox.el[i];
         best_axis = i;
      }
   }

   mxs_matrix object;
   mxs_vector unit, forward, backward;
   mxs_vector forward_pos, backward_pos;
  
   // Make unit vectors
   mx_ang2mat(&object, &ObjPosGet(o_id)->fac);
   mx_unit_vec(&unit, best_axis);

   mx_mat_mul_vec(&forward, &object, &unit);
   mx_copy_vec(&backward, &forward);

   mx_scaleeq_vec(&forward, START_CREEP);
   mx_scaleeq_vec(&backward, -START_CREEP);

   int room1 = -1;
   int room2 = -1;
   cRoom *pRoom;

   while (((room1 == room2) || (room1 == -1) || (room2 == -1)) && (mx_mag2_vec(&forward) < MAX_CREEP_LEN2))
   {
      // Set up positions
      mx_add_vec(&forward_pos, &ObjPosGet(o_id)->loc.vec, &forward);
      mx_add_vec(&backward_pos, &ObjPosGet(o_id)->loc.vec, &backward);

      if ((pRoom = g_pRooms->RoomFromPoint(forward_pos)) != NULL)
         room1 = pRoom->GetRoomID();

      if ((pRoom = g_pRooms->RoomFromPoint(backward_pos)) != NULL)
         room2 = pRoom->GetRoomID();

      mx_scaleeq_vec(&forward, 1 + CREEP_SPEED);
      mx_scaleeq_vec(&backward, 1 + CREEP_SPEED);
   }

   pDoorProp->room1 = room1;
   pDoorProp->room2 = room2;
}


// helper for RemoveDoorBlocking and AddDoorBlocking, below--
// finds the Location for the door when it's closed
static void FindClosedLoc(sDoorProp *pDoorProp, Location *pLoc)
{
   pLoc->hint = CELL_INVALID;
   pLoc->vec = pDoorProp->base_closed_location;
   ComputeCellForLocation(pLoc);
}


// @HACK for EEE: The visibility blocking traverses over contiguous
// sets of world rep cells, rather than having an explicit list of
// cells for each door.  This means that two adjacent doors can,
// effectively, close each other.  So after we close a door we have to
// enforce the state of all doors which are open.  This suggests that
// some part of this should be restructured.
void UnblockVisibilityForAllOpen()
{
   sPropertyObjIter iter;
   ObjID obj;
   sRotDoorProp *pRotDoor;
   sTransDoorProp *pTransDoor;
   Location Loc;

   g_pRotDoorProperty->IterStart(&iter);
   while (g_pRotDoorProperty->IterNext(&iter, &obj)) {

      if (OBJ_IS_ABSTRACT(obj))
         continue;

      g_pRotDoorProperty->Get(obj, &pRotDoor);
      if (pRotDoor->vision_blocking
       && pRotDoor->status != kDoorClosed) {
         FindClosedLoc(pRotDoor, &Loc);
         PortalUnblockVisionFromLocation(&Loc);
      }
   }
   g_pRotDoorProperty->IterStop(&iter);

   g_pTransDoorProperty->IterStart(&iter);
   while (g_pTransDoorProperty->IterNext(&iter, &obj)) {

      if (OBJ_IS_ABSTRACT(obj))
         continue;

      g_pTransDoorProperty->Get(obj, &pTransDoor);
      if (pTransDoor->vision_blocking
       && pTransDoor->status != kDoorClosed) {
         FindClosedLoc(pTransDoor, &Loc);
         PortalUnblockVisionFromLocation(&Loc);
      }
   }
   g_pTransDoorProperty->IterStop(&iter);
}


// Remove a door's blocking effects
void RemoveDoorBlocking(int o_id)
{
   Assert_(IsDoor(o_id));

   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   RemoveDoorSoundBlocking( o_id );

   if (pDoorProp->vision_blocking) {
      Location Loc;
      FindClosedLoc(pDoorProp, &Loc);
      PortalUnblockVisionFromLocation(&Loc);
   }

#ifdef NEW_NETWORK_ENABLED
   // If this door is a proxy, then the host will set the PhysAICollide
   // property when it's good and ready; we're just doing the look-and-feel
   // locally.
   AutoAppIPtr(NetManager);
   AutoAppIPtr(ObjectNetworking);
   if (!pNetManager->Networking() || !pObjectNetworking->ObjIsProxy(o_id)) {
#endif
      g_pPhysAICollideProp->Set(o_id, FALSE);
#ifdef NEW_NETWORK_ENABLED
   }
#endif
}

// Remove a door's sound blocking effects
void RemoveDoorSoundBlocking(int o_id)
{
   Assert_(IsDoor(o_id));

   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   if ((pDoorProp->room1 != pDoorProp->room2) && 
       (pDoorProp->room1 != -1) && (pDoorProp->room2 != -1) &&
       (g_pRooms->GetNumRooms() > 0) &&
       (pDoorProp->room1 < g_pRooms->GetNumRooms()) &&
       (pDoorProp->room2 < g_pRooms->GetNumRooms())) 
   {
      g_pPropSnd->RemoveBlockingFactor((short)pDoorProp->room1, (short)pDoorProp->room2);
   }
}


// Enable a door's blocking effects
void AddDoorBlocking(int o_id)
{
   Assert_(IsDoor(o_id));

   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   AddDoorSoundBlocking( o_id );

   if (pDoorProp->vision_blocking) {
      Location Loc;
      FindClosedLoc(pDoorProp, &Loc);
      PortalBlockVisionFromLocation(&Loc);

      UnblockVisibilityForAllOpen();
   }

#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   AutoAppIPtr(ObjectNetworking);
   if (!pNetManager->Networking() || !pObjectNetworking->ObjIsProxy(o_id)) {
#endif
      g_pPhysAICollideProp->Set(o_id, TRUE);
#ifdef NEW_NETWORK_ENABLED
   }
#endif
}

// Enable a door's sound blocking effects
void AddDoorSoundBlocking(int o_id)
{
   Assert_(IsDoor(o_id));

   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   if ((pDoorProp->room1 != pDoorProp->room2) && 
       (pDoorProp->room1 != -1) && (pDoorProp->room2 != -1) &&
       (g_pRooms->GetNumRooms() > 0))
   {
      g_pPropSnd->SetBlockingFactor((short)pDoorProp->room1, (short)pDoorProp->room2, pDoorProp->sound_blocking / 100.0);
   }
}
   
static ObjID gCurDoorLeanObj = OBJ_NULL;

// See if a door is blocking sound
BOOL
GetDoorSoundBlocking( int o_id )
{
   Assert_(IsDoor(o_id));
   mxs_real blockingFactor;

   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   if ( pDoorProp && (pDoorProp->room1 != pDoorProp->room2) && 
        (pDoorProp->room1 != -1) && (pDoorProp->room2 != -1) &&
        (g_pRooms->GetNumRooms() > 0) &&
        (pDoorProp->room1 < g_pRooms->GetNumRooms()) &&
        (pDoorProp->room2 < g_pRooms->GetNumRooms()) ) {

      blockingFactor = g_pPropSnd->GetBlockingFactor((short)pDoorProp->room1, (short)pDoorProp->room2);
      return (blockingFactor != 0.0);

   }

   return FALSE;
}
   

void CheckDoorLeanSoundBlocking()
{
   BOOL bLeaning = IsLeaning();
   BOOL bDoorContact = FALSE;
   ObjID doorObj;

   cPhysModel *pPlayerModel = g_PhysModels.Get(PlayerObject());
   if (pPlayerModel == NULL)
   {
      CriticalMsg("Player object has no physics?");
      return;
   }

   cPhysSubModelInst *pContacts;
   if (GetObjectContacts(PlayerObject(), PLAYER_HEAD, pPlayerModel, &pContacts))
   {
      while (pContacts)
      {
         if (IsDoor(pContacts->GetObjID()))
         {
            doorObj = pContacts->GetObjID();
            bDoorContact = TRUE;
            break;
         }

         pContacts = pContacts->GetNext();
      }
   }

   if (bLeaning && bDoorContact)
   {
      // Successful lean, remove door blocking

      sDoorProp *pDoorProp = GetDoorProperty(doorObj);
      if (pDoorProp == NULL)
      {
         CriticalMsg1("Door %s has no door property?", ObjWarnName(doorObj));
         return;
      }

      if ((gCurDoorLeanObj != OBJ_NULL) && (gCurDoorLeanObj != doorObj))
      {
         Warning(("Leaning against %s while still leaning against %s\n", ObjWarnName(doorObj), ObjWarnName(gCurDoorLeanObj)));
         return;
      }

      if (pDoorProp->status == kDoorClosed) 
      {
         RemoveDoorSoundBlocking(doorObj);
         gCurDoorLeanObj = doorObj;
      }
   }
   else
   {
      if (gCurDoorLeanObj != OBJ_NULL)
      {
         // No valid leans, but there is a current lean obj, so re-enable its blocking

         sDoorProp *pDoorProp = GetDoorProperty(gCurDoorLeanObj);
         if (pDoorProp == NULL)
         {
            CriticalMsg1("Door %s has no door property?", ObjWarnName(doorObj));
            return;
         }

         if (pDoorProp->status == kDoorClosed) 
         {
            AddDoorSoundBlocking(gCurDoorLeanObj);
            gCurDoorLeanObj = OBJ_NULL;
         }
      }
   }
}


// is this object a door?
BOOL IsDoor(int o_id)
{
   cPhysModel *pModel = g_PhysModels.Get(o_id);

   if (pModel)
      return pModel->IsDoor();
   else
      return (g_pRotDoorProperty->IsRelevant(o_id) || g_pTransDoorProperty->IsRelevant(o_id));
}

// how big a mass can this door push?
mxs_real GetDoorPushMass(int o_id)
{
   sDoorProp *pDoorProp = GetDoorProperty(o_id);

   return pDoorProp->push_mass;
}

// check if the door has hard limits
BOOL DoorHardLimits(int o_id)
{
   sDoorProp *pDoorProp;

   AssertMsg(IsDoor(o_id), "Attempt to get door hard-limit status of non-door object");
   pDoorProp = GetDoorProperty(o_id);

   return pDoorProp->hard_limits;
}
