/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phclimb.cpp,v 1.30 1999/12/02 17:15:45 BFarquha Exp $
//
// Physics climbing api functions
//

#include <lg.h>
#include <config.h>
#include <matrix.h>

#include <port.h>

#include <objpos.h>
#include <objedit.h>
#include <textarch.h>
#include <label.h>
#include <schema.h>
#include <plyrmov.h>
#include <plyrmode.h>
#include <playrobj.h>
#include <simtime.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmodobb.h>
#include <phmodsph.h>
#include <phprop.h>
#include <phcontct.h>
#include <phutils.h>
#include <phmoapi.h>
#include <physcast.h>

#include <phclimb.h>


// Must be last header
#include <dbmem.h>


///////////////////////////////////////

BOOL PhysObjIsClimbing(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjIsClimbing: %s has no active models\n", ObjWarnName(objID)));
      return FALSE;
   }

   return pModel->IsClimbing();
}

///////////////////////////////////////

BOOL PhysObjIsRopeClimbing(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjIsRopeClimbing: %s has no active models\n", ObjWarnName(objID)));
      return FALSE;
   }

   return pModel->IsRopeClimbing();
}

///////////////////////////////////////

BOOL PhysObjIsMantling(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("PhysObjIsMantling: %s has no active models\n", ObjWarnName(objID)));
      return FALSE;
   }

   return pModel->IsMantling();
}

///////////////////////////////////////

void BreakClimb(ObjID objID, BOOL jumping, BOOL jump_thru)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("BreakClimb: %s has no active physics models\n", ObjWarnName(objID)));
      return;
   }

   if (!pModel->IsPlayer() && !pModel->IsAvatar())
      return;

   ObjID climbing_obj = pModel->GetClimbingObj();

   if (climbing_obj != OBJ_NULL)
   {
      jump_thru = pModel->IsRopeClimbing();

      mxs_vector normal;

      mx_zero_vec(&normal);
      if (GetObjTextureIdx(climbing_obj) >= 0)
      {
         // Texture
         cFaceContactList *pFaceContactList;

         for (int i=0; i<pModel->NumSubModels(); i++)
         {
            if (pModel->GetFaceContacts(i, &pFaceContactList))
            {
               cFaceContact *pFaceContact;

               pFaceContact = pFaceContactList->GetFirst();
               while (pFaceContact)
               {
                  if (pFaceContact->GetObjID() == climbing_obj)
                  {
                     mx_copy_vec(&normal, &pFaceContact->GetNormal());
                     break;
                  }

                  pFaceContact = pFaceContact->GetNext();
               }
            }
         }
      }
      else
      {
         // Object
         cPhysModel *pModel2 = g_PhysModels.Get(climbing_obj);

         if (pModel2)
         {
            if (pModel2->GetType(0) == kPMT_OBB)
            {
               cPhysSubModelInst *pObjContact;

               // Are we close enough to the top to jump through?
               mxs_matrix orien;
               mxs_vector top_pt;
               mxs_vector offset;
               mxs_vector proj_pt;

               mx_ang2mat(&orien, &pModel2->GetRotation());
               mx_scale_add_vec(&top_pt, &pModel2->GetLocationVec(), &orien.vec[2], ((cPhysOBBModel *)pModel2)->GetEdgeLengths().z / 2);

               mx_sub_vec(&offset, &pModel->GetLocationVec(), &pModel2->GetLocationVec());
               mx_scale_add_vec(&proj_pt, &pModel2->GetLocationVec(), &orien.vec[2], mx_dot_vec(&offset, &orien.vec[2]));

               if (mx_dist2_vec(&top_pt, &proj_pt) < 1)
                  jump_thru = TRUE;
               else
               {
                  // Find which submodel we're attached to
                  for (int i=0; i<pModel->NumSubModels() && IsZeroVector(normal); i++)
                  {
                     if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pObjContact))
                     {
                        while (pObjContact)
                        {
                           if (pModel2 == pObjContact->GetPhysicsModel())
                           {
                              cFaceContact faceContact((cPhysOBBModel *)pModel2, pObjContact->GetSubModId());
                              mx_copy_vec(&normal, &faceContact.GetNormal());
                              break;
                           }

                           pObjContact = pObjContact->GetNext();
                        }
                     }
                  }
               }
            }
            else
            if (pModel2->IsRope())
               jump_thru = TRUE;
            else
               Warning(("BreakClimb: unknown physics model type on obj %d\n", pModel2->GetObjID()));
         }
         else
            Warning(("BreakClimb: no physics model on climbing obj %d\n", climbing_obj));
      }

      if (jump_thru)
      {
         mxs_matrix orien;

         mx_ang2mat(&orien, &pModel->GetRotation());
         mx_copy_vec(&normal, &orien.vec[0]);
         mx_scaleeq_vec(&normal, 2.0);
         mx_scale_addeq_vec(&normal, &orien.vec[2], 0.1);

         cPhysModel *pClimbModel;

         if ((pClimbModel = g_PhysModels.Get(climbing_obj)) == NULL)
         {
            Warning(("BreakClimb: no physics on climbing obj %d?\n", climbing_obj));
            return;
         }

         if (pClimbModel->IsRope())
            pClimbModel->SetObjectPassThru(1.0);
      }

      if (IsZeroVector(normal))
         Warning(("BreakClimb: unable to find climbing object to break from\n"));
      else
      if (jumping)
      {
         mxs_matrix orien;
         mxs_vector jump_dir;

         mx_ang2mat(&orien, &pModel->GetRotation());

         if (mx_dot_vec(&orien.vec[0], &normal) > 0)
         {
            if (jump_thru)
               mx_copy_vec(&jump_dir, &normal);
            else
            {
               // facing away from climbing surface, just jump
               mx_copy_vec(&jump_dir, &orien.vec[0]);
            }
         }
         else
         {
            // reflect from climbing surface
            mx_copy_vec(&jump_dir, &orien.vec[0]);

            mxs_vector into_norm;
            mx_scale_vec(&into_norm, &normal, mx_dot_vec(&jump_dir, &normal)  * 2);

            mx_subeq_vec(&jump_dir, &into_norm);
            mx_scaleeq_vec(&jump_dir, 0.5);
         }

         mxs_vector velocity;

         mx_copy_vec(&velocity, &pModel->GetVelocity());
         mx_scale_addeq_vec(&velocity, &jump_dir, 5.0);
         pModel->GetDynamics()->SetVelocity(velocity);
      }

      for (int i=0; i<pModel->NumSubModels(); i++)
      {
         pModel->DestroyAllTerrainContacts(i);
         DestroyAllObjectContacts(objID, i, pModel);
      }

      if (pModel->IsPlayer())
         pModel->SetSpringTension(PLAYER_HEAD, DEFAULT_SPRING_TENSION);

      pModel->SetClimbingObj(OBJ_NULL);
      pModel->SetRopeSegment(-1);

      g_pPlayerMovement->SetGroundObj(OBJ_NULL);

      if (jumping)
         g_pPlayerMode->SetMode(kPM_Jump);
      else
         g_pPlayerMode->SetMode(kPM_Stand);
   }
}

////////////////////////////////////////

BOOL CheckClimb(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("CheckClimb: %s has no active physics models\n", ObjWarnName(objID)));
      return FALSE;
   }

   if ((!pModel->IsPlayer() && !pModel->IsAvatar()) || pModel->IsGhost())
      return FALSE;

   mxs_matrix orien;

   mx_ang2mat(&orien, &pModel->GetRotation());

   cFaceContactList *pFaceContactList;

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if (pModel->GetFaceContacts(i, &pFaceContactList))
      {
         cFaceContact *pFaceContact;

         pFaceContact = pFaceContactList->GetFirst();
         while (pFaceContact)
         {
            if (FALSE) // don't climb on walls, spidey
            {
               if (mx_dot_vec(&orien.vec[0], &pFaceContact->GetNormal()) <= 0.0)
               {
                  // Set the internal info
                  pModel->SetClimbingObj(pFaceContact->GetObjID());

                  // Update constraints
                  ConstrainFromTerrain(pModel, i);

                  if (PlayerObjectExists() && (objID == PlayerObject()))
                  {
                     mxs_vector foot_loc;
                     PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &foot_loc);

                     g_pPlayerMode->SetMode(kPM_Climb);
                     g_pPlayerMovement->SetGroundObj(pFaceContact->GetObjID());
                     g_pPlayerMovement->SetLastFoot(foot_loc, GetSimTime());
                  }

                  return TRUE;
               }
            }

            pFaceContact = pFaceContact->GetNext();
         }
      }

      cPhysSubModelInst *pObjContact;

      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pObjContact))
      {
         cPhysModel *pModel2;
         int j;

         while (pObjContact)
         {
            pModel2 = pObjContact->GetPhysicsModel();
            j = pObjContact->GetSubModId();

            if (pModel2->GetType(0) == kPMT_OBB)
            {
               if (((cPhysOBBModel *)pModel2)->GetClimbableSides() & (1 << j))
               {
                  mxs_vector face_normal;

                  ((cPhysOBBModel *)pModel2)->GetNormal(j, &face_normal);

                  if (mx_dot_vec(&orien.vec[0], &face_normal) <= 0.0)
                  {
                     // Set the internal info
                     pModel->SetClimbingObj(pModel2->GetObjID());

                     // Update constraints
                     ConstrainFromObjects(pModel, i);

                     if (PlayerObjectExists() && (objID == PlayerObject()))
                     {
                        mxs_vector foot_loc;
                        PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &foot_loc);

                        g_pPlayerMode->SetMode(kPM_Climb);
                        g_pPlayerMovement->SetGroundObj(pModel2->GetObjID());
                        g_pPlayerMovement->SetLastFoot(foot_loc, GetSimTime());
                     }

                     return TRUE;
                  }
               }
            }
            else
            if (pModel2->IsRope())
            {
               mxs_vector to_rope;

               mx_sub_vec(&to_rope, &pModel2->GetLocationVec(j), &pModel->GetLocationVec(i));
               if (mx_dot_vec(&orien.vec[0], &to_rope) > 0)
               {
                  pModel->SetClimbingObj(pModel2->GetObjID());

                  if (j < 1)
                     pModel->SetRopeSegment(1);
                  else
                  if (j > pModel2->NumSubModels())
                     pModel->SetRopeSegment(pModel2->NumSubModels());
                  else
                     pModel->SetRopeSegment(j);

                  ConstrainFromObjects(pModel, i);

                  mxs_vector vel, add_vel;
                  mx_copy_vec(&vel, &pModel2->GetVelocity(j));

                  if (pModel->IsPlayer())
                     mx_scale_vec(&add_vel, &pModel->GetVelocity(PLAYER_HEAD), 5.0);
                  else
                     mx_scale_vec(&add_vel, &pModel->GetVelocity(), pModel->GetDynamics()->GetMass() * 0.1);
                  add_vel.z = 0;
                  mx_addeq_vec(&vel, &add_vel);
                  pModel2->GetDynamics(j)->SetVelocity(vel);

                  if (PlayerObjectExists() && (objID == PlayerObject()))
                  {
                     mxs_vector foot_loc;
                     PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &foot_loc);

                     g_pPlayerMode->SetMode(kPM_Climb);
                     g_pPlayerMovement->SetGroundObj(pModel2->GetObjID());
                     g_pPlayerMovement->SetLastFoot(foot_loc, GetSimTime());
                  }

                  return TRUE;
               }
            }

            pObjContact = pObjContact->GetNext();
         }
      }
   }

   return FALSE;
}

////////////////////////////////////////

void BreakMantle(ObjID objID)
{
   cPhysModel *pModel;
   mxs_vector offset;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("BreakMantle: %s has no active physics models\n", ObjWarnName(objID)));
      return;
   }

   if (!pModel->IsPlayer())
      return;

   mx_mk_vec(&offset, 0, 0, PLAYER_HEAD_POS);
   pModel->SetSubModOffset(PLAYER_HEAD, offset);
   mx_mk_vec(&offset, 0, 0, PLAYER_BODY_POS);
   pModel->SetSubModOffset(PLAYER_BODY, offset);
   mx_mk_vec(&offset, 0, 0, PLAYER_KNEE_POS);
   pModel->SetSubModOffset(PLAYER_KNEE, offset);
   mx_mk_vec(&offset, 0, 0, PLAYER_SHIN_POS);
   pModel->SetSubModOffset(PLAYER_SHIN, offset);
   mx_mk_vec(&offset, 0, 0, PLAYER_FOOT_POS);
   pModel->SetSubModOffset(PLAYER_FOOT, offset);

   pModel->SetSpringTension(PLAYER_HEAD, DEFAULT_SPRING_TENSION);

   PhysSetModLocation(objID, (mxs_vector *)&pModel->GetLocationVec());

   PlayerMotionActivate(kMoEnable);
   pModel->SetMantlingState(0);

   mxs_matrix orien;
   mx_ang2mat(&orien, &pModel->GetRotation());

   mxs_vector target_loc = pModel->GetLocationVec();
   mxs_vector delta;

   BOOL found_valid = PhysObjValidPos(objID, NULL);

   for (float dist = 0.1; (dist < 4.0) && !found_valid; dist += 0.1)
   {
      mx_scale_add_vec(&target_loc, &pModel->GetLocationVec(), &orien.vec[0], -dist);
      mx_sub_vec(&delta, &target_loc, &pModel->GetLocationVec());

      if (PhysObjValidPos(objID, &delta))
         found_valid = TRUE;

      if (!found_valid)
      {
         mx_scale_add_vec(&target_loc, &pModel->GetLocationVec(), &orien.vec[2], dist);
         mx_sub_vec(&delta, &target_loc, &pModel->GetLocationVec());

         if (PhysObjValidPos(objID, &delta))
            found_valid = TRUE;
      }
   }

   if (found_valid)
      PhysSetModLocation(objID, &target_loc);
}

////////////////////////////////////////

static inline BOOL CanMantleOnto(ObjID objid)
{
   BOOL bCanMantle;

   if (!g_pPhysCanMantleProp || !g_pPhysCanMantleProp->Get(objid, &bCanMantle))
      return TRUE;

   return bCanMantle;
}


////////////////////////////////////////

BOOL CheckMantle(ObjID objID)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(objID)) == NULL)
   {
      Warning(("CheckMantle: %s has no active physics models\n", ObjWarnName(objID)));
      return FALSE;
   }

   if (!pModel->IsPlayer() || !g_pPlayerMode->IsJumping())
      return FALSE;

   mxs_matrix orien;
   mxs_vector ledge_offset;
   mxs_vector movement_dir;
   mxs_real   ledge_up = 0.02;
   mxs_real   radius = ((cPhysSphereModel *)pModel)->GetRadius(PLAYER_HEAD);
   ObjID      hit_obj = 0;

   Location start_loc, end_loc, hit_loc, hit_loc2;

   mx_mk_vec(&ledge_offset, 0, 0, 3.5);

   mx_ang2mat(&orien, &pModel->GetRotation());

   MakeHintedLocationFromVector(&start_loc, &pModel->GetLocationVec(PLAYER_HEAD), &pModel->GetLocation(PLAYER_HEAD));
   MakeHintedLocationFromVector(&end_loc, &start_loc.vec, &start_loc);

   mx_addeq_vec(&end_loc.vec, &ledge_offset);

   // Cast up
   #ifndef SHIP
   if (config_is_defined("MantleSpew"))
   {
      mprintf("Casting up from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("             to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   if (PhysRaycast(start_loc, end_loc, &hit_loc, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone)
   {
      #ifndef SHIP
      if (config_is_defined("MantleSpew"))
         mprintf("Mantle up failed\n");
      #endif

      return FALSE;
   }

   MakeHintedLocationFromVector(&start_loc, &end_loc.vec, &start_loc);

   mx_copy_vec(&movement_dir, &orien.vec[0]);
   movement_dir.z = 0;
   if (!IsZeroVector(movement_dir))
      mx_normeq_vec(&movement_dir);
   mx_scaleeq_vec(&movement_dir, radius * 2);

   mx_addeq_vec(&end_loc.vec, &movement_dir);

   // Cast forward
   #ifndef SHIP
   if (config_is_defined("MantleSpew"))
   {
      mprintf("Casting forward from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("                  to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   hit_obj = 0;
   if (PhysRaycast(start_loc, end_loc, &hit_loc, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone)
   {
      #ifndef SHIP
      if (config_is_defined("MantleSpew"))
         mprintf("Mantle forward failed\n");
      #endif

      return FALSE;
   }

   MakeHintedLocationFromVector(&start_loc, &end_loc.vec, &start_loc);
   mx_scale_addeq_vec(&end_loc.vec, &ledge_offset, -2.0);

   // Cast downward
   #ifndef SHIP
   if (config_is_defined("MantleSpew"))
   {
      mprintf("Casting down from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("               to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   hit_obj = 0;
   if ((PhysRaycast(start_loc, end_loc, &hit_loc, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone) && CanMantleOnto(hit_obj))
   {
      // Find forward surface
      MakeHintedLocationFromVector(&start_loc, &pModel->GetLocationVec(PLAYER_HEAD), &pModel->GetLocation());
      MakeLocationFromVector(&end_loc, &start_loc.vec);

      ComputeCellForLocation(&start_loc);

      mx_addeq_vec(&end_loc.vec, &movement_dir);

      hit_obj = 0;
      if ((PhysRaycast(start_loc, end_loc, &hit_loc2, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone) && CanMantleOnto(hit_obj))
      {
         mxs_vector target_pos;

         mx_copy_vec(&target_pos, &hit_loc2.vec);
         mx_scale_addeq_vec(&target_pos, &movement_dir, -0.55); // pull back a radius
         target_pos.z = hit_loc.vec.z + (radius * 1.02) - PLAYER_HEAD_POS + 1.0; // and just over the lip

         pModel->SetMantlingTargVec(target_pos);
         pModel->SetMantlingState(1);

         PlayerMotionActivate(kMoDisable);

         Label grunt = {"garclimb"};
         SchemaPlay(&grunt, NULL, NULL);
      }
      else
      {
         mx_copy_vec(&start_loc.vec, &pModel->GetLocationVec(PLAYER_BODY));
         mx_add_vec(&end_loc.vec, &start_loc.vec, &movement_dir);

         hit_obj = 0;
         if ((PhysRaycast(start_loc, end_loc, &hit_loc2, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone) && CanMantleOnto(hit_obj))
         {
            mxs_vector target_pos;

            mx_copy_vec(&target_pos, &hit_loc2.vec);
            mx_scale_addeq_vec(&target_pos, &movement_dir, -0.55); // pull back a radius
            target_pos.z = hit_loc.vec.z + (radius * 1.02) - PLAYER_HEAD_POS + 1.0; // and just over the lip

            pModel->SetMantlingTargVec(target_pos);
            pModel->SetMantlingState(1);

            PlayerMotionActivate(kMoDisable);

            Label grunt = {"garclimb"};
            SchemaPlay(&grunt, NULL, NULL);
         }
         else
         {
            mx_copy_vec(&start_loc.vec, &pModel->GetLocationVec(PLAYER_FOOT));
            mx_add_vec(&end_loc.vec, &start_loc.vec, &movement_dir);

            hit_obj = 0;
            if ((PhysRaycast(start_loc, end_loc, &hit_loc2, &hit_obj, 0, kCollideOBB | kCollideTerrain) != kCollideNone) && CanMantleOnto(hit_obj))
            {
               mxs_vector target_pos;

               mx_copy_vec(&target_pos, &hit_loc2.vec);
               mx_scale_addeq_vec(&target_pos, &movement_dir, -0.55); // pull back a radius
               target_pos.z = hit_loc.vec.z + (radius * 1.02) - PLAYER_HEAD_POS + 1.0; // and just over the lip

               pModel->SetMantlingTargVec(target_pos);
               pModel->SetMantlingState(1);

               PlayerMotionActivate(kMoDisable);

               Label grunt = {"garclimb"};
               SchemaPlay(&grunt, NULL, NULL);
            }
            else
            {
               #ifndef SHIP
               if (config_is_defined("MantleSpew"))
                  mprintf("Mantle forward surface find failed, trying best guess\n");
               #endif

               mxs_vector target_pos;

               mx_copy_vec(&target_pos, &pModel->GetLocationVec(PLAYER_BODY));
               target_pos.z = hit_loc.vec.z + (radius * 1.02) - PLAYER_HEAD_POS + 1.0;

               pModel->SetMantlingTargVec(target_pos);
               pModel->SetMantlingState(1);

               PlayerMotionActivate(kMoDisable);

               Label grunt = {"garclimb"};
               SchemaPlay(&grunt, NULL, NULL);
            }
         }
      }
   }

   if (pModel->IsMantling())
   {
      mxs_vector target_pos;
      mxs_vector delta;

      pModel->GetMantlingTargVec(&target_pos);

      mx_sub_vec(&delta, &target_pos, &pModel->GetLocationVec());
      if (!PhysObjValidPos(objID, &delta))
      {
         #ifndef SHIP
         if (config_is_defined("MantleSpew"))
            mprintf("Aborting mantle, can't fit in target position\n");
         #endif

         BreakMantle(objID);
         return FALSE;
      }

     mx_addeq_vec(&delta, &movement_dir);
     delta.z += -PLAYER_FOOT_POS;
     if (!PhysObjValidPos(objID, &delta))
     {
         #ifndef SHIP
         if (config_is_defined("MantleSpew"))
            mprintf("Aborting mantle, can't fit in target position\n");
         #endif

         BreakMantle(objID);
         return FALSE;
      }

      return TRUE;
   }

   return FALSE;
}

////////////////////////////////////////

void UpdateMantling(cPhysModel *pModel, mxs_real dt)
{
   static mxs_real delta_t = 0;
   ObjID  hit_obj;

   switch (pModel->GetMantlingState())
   {
      case 1:
      {
         delta_t += dt;

         if (delta_t > 0.3)
         {
            delta_t = 0;
            pModel->SetMantlingState(2);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d going to mantling state %d\n", pModel->GetObjID(), 2);
            #endif
         }
         else
         {
            mxs_vector targ_vec;

            targ_vec.x = pModel->GetLocationVec(PLAYER_HEAD).x;
            targ_vec.y = pModel->GetLocationVec(PLAYER_HEAD).y;
            targ_vec.z = pModel->GetLocationVec().z;

            pModel->SetTargetLocation(targ_vec);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d in mantling state %d\n", pModel->GetObjID(), 1);
            #endif
         }
         break;
      }

      // Rising vertically, in order to get our head in the proper position
      case 2:
      {
         mxs_vector delta_pos;
         mxs_vector targ_vec;

         // Set up our target location
         pModel->GetMantlingTargVec(&targ_vec);
         pModel->SetTargetLocation(targ_vec);

         mx_sub_vec(&delta_pos, &pModel->GetTargetLocation(PLAYER_HEAD), &pModel->GetLocationVec(PLAYER_HEAD));

         // Are we ready to move on?
         if (mx_mag2_vec(&delta_pos) < 0.0001)
         {
            mxs_vector zero;
            mx_zero_vec(&zero);

            // Shrink inside a single sphere
            pModel->SetLocationVec(pModel->GetLocationVec(PLAYER_HEAD));
            pModel->SetSubModOffset(PLAYER_HEAD, zero);
            pModel->SetSubModOffset(PLAYER_BODY, zero);
            pModel->SetSubModOffset(PLAYER_KNEE, zero);
            pModel->SetSubModOffset(PLAYER_SHIN, zero);
            pModel->SetSubModOffset(PLAYER_FOOT, zero);

            mxs_matrix orien;
            mxs_vector facing;
            mxs_vector targ_loc;

            // Find our forward-moving target
            mx_ang2mat(&orien, &pModel->GetRotation());
            mx_copy_vec(&targ_loc, &pModel->GetLocationVec());

            mx_copy_vec(&facing, &orien.vec[0]);
            facing.z = 0;
            if (!IsZeroVector(facing))
               mx_normeq_vec(&facing);

            mx_scale_addeq_vec(&targ_loc, &facing, PLAYER_RADIUS * 2);

            pModel->SetMantlingTargVec(targ_loc);
            pModel->SetTargetLocation(pModel->GetLocationVec(PLAYER_HEAD));

            pModel->SetSpringTension(PLAYER_HEAD, 0.02 * DEFAULT_SPRING_TENSION);

            pModel->SetMantlingState(3);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d going to mantling state %d\n", pModel->GetObjID(), 3);
            #endif

            delta_t = 0;
         }
         else
         {
            // Adjust spring tension for smooth transition
            mxs_real spring_amt = mx_mag2_vec(&delta_pos);

            if (spring_amt < 1)
               spring_amt = 1.0;
            else
               spring_amt = 1.0 / spring_amt;

            pModel->SetSpringTension(PLAYER_HEAD, spring_amt * DEFAULT_SPRING_TENSION);

            pModel->SetTargetLocation(targ_vec);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d in mantling state %d\n", pModel->GetObjID(), 2);
            #endif
         }

         break;
      }

      // Mashed into a ball, moving forward
      case 3:
      {
         mxs_vector targ_vec;

         // Set up our target location
         pModel->GetMantlingTargVec(&targ_vec);
         pModel->SetTargetLocation(targ_vec);

         delta_t += dt;

         if (delta_t > 0.4)
         {
            mxs_vector targ_loc;
            mxs_vector offset;

            mx_mk_vec(&targ_loc, 0, 0, -PLAYER_FOOT_POS - PLAYER_RADIUS - 1.0);
            mx_addeq_vec(&targ_loc, &pModel->GetLocationVec());

            mx_mk_vec(&offset, 0, 0, PLAYER_HEAD_POS);
            pModel->SetSubModOffset(PLAYER_HEAD, offset);

            pModel->SetMantlingTargVec(targ_loc);
            pModel->SetTargetLocation(targ_loc);

            Label grunt = {"garclimbend"};
            SchemaPlay(&grunt, NULL, NULL);

            pModel->SetMantlingState(4);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d going to mantling state %d\n", pModel->GetObjID(), 4);
            #endif

            delta_t = 0;
         }
         else
         {
            mxs_vector vel;

            mx_copy_vec(&vel, &pModel->GetVelocity(PLAYER_HEAD));
            mx_scaleeq_vec(&vel, 10.0);
            pModel->GetDynamics(PLAYER_HEAD)->SetVelocity(vel);
            pModel->UpdateEndLocation(PLAYER_HEAD, dt);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d in mantling state %d\n", pModel->GetObjID(), 3);
            #endif
         }

         break;
      }

      // Mashed into a ball, moving upward
      case 4:
      {
         mxs_vector delta_pos;
         mxs_vector targ_vec;

         // Set up our target location
         pModel->GetMantlingTargVec(&targ_vec);
         pModel->SetTargetLocation(targ_vec);

         mx_sub_vec(&delta_pos, &pModel->GetTargetLocation(PLAYER_HEAD), &pModel->GetLocationVec(PLAYER_HEAD));

         // Are we ready to move on?
         if (mx_mag2_vec(&delta_pos) < 0.0001)
         {
            mxs_vector offset;

            mx_mk_vec(&offset, 0, 0, PLAYER_BODY_POS);
            pModel->SetSubModOffset(PLAYER_BODY, offset);
            mx_mk_vec(&offset, 0, 0, PLAYER_KNEE_POS);
            pModel->SetSubModOffset(PLAYER_KNEE, offset);
            mx_mk_vec(&offset, 0, 0, PLAYER_SHIN_POS);
            pModel->SetSubModOffset(PLAYER_SHIN, offset);
            mx_mk_vec(&offset, 0, 0, PLAYER_FOOT_POS);
            pModel->SetSubModOffset(PLAYER_FOOT, offset);

            // Raycast to make sure there's room
            Location start, end, hit;

            MakeLocationFromVector(&start, &targ_vec);
            MakeLocationFromVector(&end, &targ_vec);

            ComputeCellForLocation(&start);
            end.vec.z += PLAYER_FOOT_POS - 1.0;

            hit_obj = 0;
            if ((PhysRaycast(start, end, &hit, &hit_obj, 0) != kCollideNone) && CanMantleOnto(hit_obj))
            {
               mx_copy_vec(&targ_vec, &hit.vec);
               targ_vec.z += -PLAYER_FOOT_POS + 0.1;
            }

            pModel->SetTargetLocation(targ_vec);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d going to mantling state %d\n", pModel->GetObjID(), 5);
            #endif

            pModel->SetMantlingState(5);
         }
         else
         {
            mxs_vector vel;

            mx_copy_vec(&vel, &pModel->GetVelocity(PLAYER_HEAD));
            mx_scaleeq_vec(&vel, min(50.0, max(30.0, 30 / mx_mag2_vec(&delta_pos))));
            pModel->GetDynamics(PLAYER_HEAD)->SetVelocity(vel);
            pModel->UpdateEndLocation(PLAYER_HEAD, dt);

            #ifndef SHIP
            if (config_is_defined("MantleSpew"))
               mprintf("obj %d in mantling state %d\n", pModel->GetObjID(), 4);
            #endif
         }

         break;
      }

      // Floating, ready to be expanded
      case 5:
      {
#if 0
         if (pModel->InTerrainContact())
#endif
            BreakMantle(pModel->GetObjID());

         #ifndef SHIP
         if (config_is_defined("MantleSpew"))
            mprintf("obj %d in mantling state %d\n", pModel->GetObjID(), 5);
         #endif
      }
   }
}

////////////////////////////////////////

void GetClimbingDir(ObjID objID, mxs_vector *dir)
{
   if (GetObjTextureIdx(objID) >= 0)
   {
      // texture

      // @TODO: look at the poly's v axis
      mx_unit_vec(dir, 2);

   }
   else
   {
      // object

      mxs_matrix orien;
      mx_ang2mat(&orien, &ObjPosGet(objID)->fac);

      mx_copy_vec(dir, &orien.vec[2]);
   }
}




