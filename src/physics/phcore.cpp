/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phcore.cpp,v 1.344 2000/03/09 14:40:47 ccarollo Exp $
//
// Physics MainLoops
//

///////////////////////////////////////

//#define PROFILE_ON
//#define PROFILE_UD
//#define PER_FRAME_TIMING
//#define OBJ_COLL_TIMING

//#define CLSN_COUNT

#include <lg.h>

#include <math.h>
#include <timings.h>
#include <cfgdbg.h>
#include <config.h>
#include <matrix.h>
#include <matrixd.h>
#include <dynarray.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <portal.h>    // for PortalPointInPolygon
#include <medmotn.h>
#include <partprop.h>
#include <wrdbrend.h>
#include <config.h>
#include <rand.h>
#include <prjctile.h>
#include <propface.h>
#include <weapon.h>
#include <aiapi.h>
#include <plyrmov.h>
#include <plyrmode.h>
#include <playrobj.h>
#include <aiteams.h>
#include <doorprop.h>
#include <autolink.h>
#include <ghostmvr.h>

#include <creatext.h>
#include <creature.h>
#include <creatur_.h>

#include <textarch.h>

#include <lnkquery.h>
#include <linkbase.h>

#include <lazyagg.h>

#include <timer.h>

#include <mprintf.h>

///////////////////////////////////////

#include <gen_bind.h> //HEY, we include every other thing, 
                      //why not this too? (cry) AMSD

///////////////////////////////////////

#include <objsys.h>
#include <objtype.h>
#include <iobjsys.h>
#include <objshape.h>
#include <collide.h>
#include <objedit.h>

#include <refsys.h>
#include <camera.h>

#include <brlist.h>

#include <collide.h>


///////////////////////////////////////

#include <portal.h>
#include <objpos.h>
#include <collprop.h>
#include <doorphys.h>

///////////////////////////////////////

#include <physapi.h>

#include <objmedia.h>
#include <phclimb.h>
#include <phcore.h>
#include <phprop.h>
#include <phoprop.h>
#include <phppscpt.h>
#include <phmod.h>
#include <phmods.h>
#include <phmodsph.h>
#include <phmodbsp.h>
#include <phmodobb.h>
#include <phcontct.h>
#include <phconst.h>
#include <phclsn.h>
#include <phutils.h>
#include <phmodutl.h>
#include <phmterr.h>
#include <phref.h>
#include <phmoapi.h>
#include <phmsg.h>
#include <phflist.h>
#include <phscrt.h>
#include <phlistn_.h>

#include <phystest.h>
#include <phdbg.h>

#include <property.h>
#include <propbase.h>

#include <command.h>

#include <sphrcst.h>
#include <sphrcsts.h>

// Must be last header
#include <dbmem.h>

#undef ref
#undef vector

#define sq(x)  ((x)*(x))

extern int inworld_checks;

///////////////////////////////////////////////////////////////////////////////
//
// Globals
//

//
// Model lists
//
static cPhysModels _g_PhysModels;
cPhysModels & g_PhysModels = _g_PhysModels;

//
// Collision list
//
static cPhysClsns _g_PhysClsns;
cPhysClsns & g_PhysClsns = _g_PhysClsns;

//
// user-definable constant variable declaration
//
mxs_real kFrictionFactor;
int      kMaxFrameLen;

mxs_vector kGravityDir;
mxs_real   kGravityAmt;

BOOL net_cap_physics;

int sim_frames;
int phys_frames;

// climbing ladders by touch
BOOL g_climb_on_touch; //unitialized value

//
// Physics internal revision number
// g_PhysVersion can differ during loads of old levels
//
const int g_PhysCurrentVersion = 32;
int g_PhysVersion = g_PhysCurrentVersion;

// The master switch
BOOL g_PhysicsOn = TRUE;

// Subscriptions
class cSubscribeService;
cSubscribeService *pPhysSubscribeService = NULL;

// Listeners
cPhysListeners* g_pPhysListeners = NULL;

//
// Debugging tools
//
#ifdef DBG_ON
cPhysicsDebug g_PhysicsDebug;
#endif

BOOL gInsideMT = FALSE;

///////////////////////////////////////////////////////////////////////////////

static void CheckSubModelCollisions(cPhysModel * pModel, tPhysSubModId i, mxs_real t0, mxs_real dt);
static void ZeroAcceleration(cPhysModel * pModel);
static void Integrate(cPhysModel * pModel, mxs_real dt);
static void PostMovingTerrainUpdate(cPhysModel *pModel);

///////////////////////////////////////////////////////////////////////////////

static BOOL SubModOnPoly(cPhysModel * pModel, tPhysSubModId subModId, cFaceContact *pFaceContact, BOOL grow_poly)
{
   mxs_vector *submod_vec = (mxs_vector *) &pModel->GetLocationVec(subModId);
   mxs_real    plane_const;
   mxs_vector  normal;

   // hack for obbs
   if (pModel->GetType(subModId) == kPMT_OBB)
   {
      cFaceContact faceContact((cPhysOBBModel *)pModel, subModId);
      mxs_vector pos;
      mxs_matrix orien;

      mx_ang2mat(&orien, &pModel->GetRotation());
      mx_mat_mul_vec(&pos, &orien, &faceContact.GetNormal());
      mx_scaleeq_vec(&pos, ((cPhysOBBModel *)pModel)->GetEdgeLengths().el[subModId % 3] / 2);

      mx_addeq_vec(&pos, &pModel->GetLocationVec());

      submod_vec = &pos;
   }

   // Check point against each edge plane
   for (int i=0; i<pFaceContact->GetPoly()->edgePlaneList.Size(); i++)
   {
      plane_const = pFaceContact->GetPoly()->edgePlaneList[i]->d;
      mx_copy_vec(&normal, &pFaceContact->GetPoly()->edgePlaneList[i]->normal);

      if (grow_poly)
      {
         if ((pModel->GetType(subModId) == kPMT_Sphere) || (pModel->GetType(subModId) == kPMT_SphereHat))
         {
            if (pModel->IsFancyGhost() && (((cPhysSphereModel *)pModel)->GetRadius(subModId) > 1.0))
               plane_const += 1.0;
            else
               plane_const += ((cPhysSphereModel *)pModel)->GetRadius(subModId);
         }
         else
            plane_const += 0.1;
      }
      else
         plane_const += 0.1;

      if ((mx_dot_vec(&normal, submod_vec) - plane_const) > .01)
         return FALSE;
   }

   return TRUE;
}

void ConstrainFromTerrain(cPhysModel * pModel, tPhysSubModId i)
{
   cFaceContactList    *pFaceContactList;
   cEdgeContactList    *pEdgeContactList;
   cVertexContactList  *pVertexContactList;
   BOOL on_poly;

   // Check for face contact
   if (pModel->GetFaceContacts(i, &pFaceContactList))
   {
      cFaceContact *pFaceContact;
      cFaceContact *pFaceContactNext;
      mxs_real      surfaceDistance;

      pFaceContact = pFaceContactList->GetFirst();
      while (pFaceContact != NULL)
      {
         pFaceContactNext = pFaceContact->GetNext();

         surfaceDistance = pModel->TerrainDistance(i, pFaceContact);

         on_poly = SubModOnPoly(pModel, i, pFaceContact, FALSE);

         // Decide whether to break contact or constrain to surface
         if ((surfaceDistance < kBreakTerrainContactDist) && on_poly)
         {
            pModel->AddConstraint(pFaceContact->GetObjID(), i, pFaceContact->GetNormal());
         }
         else
         {
            BOOL new_contact = FALSE;

            if (!on_poly &&
                (pModel->IsPlayer() || pModel->IsAvatar()) &&
                (i == PLAYER_FOOT))
            {
               // Look for poly we're stepping onto
               Location start, end, hit;
               mxs_vector look_offset;

               MakeHintedLocationFromVector(&start, &pModel->GetLocation(i).vec, &pModel->GetLocation(i));
               MakeLocationFromVector(&end, &start.vec);

               mx_scale_vec(&look_offset, &pFaceContact->GetNormal(), 0.1);
               mx_subeq_vec(&end.vec, &look_offset);

               if (!PortalRaycast(&start, &end, &hit, 0))
               {
                  int cell_id = PortalRaycastCell;
                  int poly_id = PortalRaycastFindPolygon();

                  if ((cell_id != CELL_INVALID) && (poly_id != -1))
                  {
                     cFaceContact faceContact(cell_id, poly_id);

                     pModel->CreateTerrainContact(i, faceContact.GetPoly(), faceContact.GetObjID());
                     pModel->AddConstraint(faceContact.GetObjID(), i, faceContact.GetNormal());

                     #ifndef SHIP
                     if (config_is_defined("ContactSpew"))
                     {
                        mprintf("obj %d (%d) direct contact transition\n", pModel->GetObjID(), i);
                        mprintf("   new: cell = %d, poly = %d\n", cell_id, poly_id);
                     }
                     #endif

                     if (pModel->IsPlayer() && (i == PLAYER_FOOT))
                        g_pPlayerMovement->SetGroundObj(faceContact.GetObjID());

                     if (pModel->IsPlayer() &&
                         (g_pPlayerMode->GetMode() != kPM_Stand) &&
                         (g_pPlayerMode->GetMode() != kPM_Crouch) &&
                         (g_pPlayerMode->GetMode() != kPM_Swim) &&
                         (g_pPlayerMode->GetMode() != kPM_Dead))
                        g_pPlayerMode->SetMode(kPM_Stand);

                     new_contact = TRUE;
                  }
               }
            }

            pModel->DestroyTerrainContact(i, pFaceContact->GetPoly());

            if (!new_contact)
            {
               #ifndef SHIP
               if (config_is_defined("ContactSpew"))
               {
                  mprintf("obj %d (%d) destroying terrain face contact\n", pModel->GetObjID(), i);
                  mprintf("   dist = %g\n", surfaceDistance);
               }
               #endif

               if (pModel->IsPlayer() && (i == PLAYER_FOOT))
               {
                  cFaceContactList *pDummy;

                  // Only lift off if we're the last contact
                  if (!pModel->GetFaceContacts(i, &pDummy))
                     g_pPlayerMovement->LeaveGround();
               }
            }
         }

         pFaceContact = pFaceContactNext;
      }
   }

   // Check for edge contact
   if (pModel->GetEdgeContacts(i, &pEdgeContactList))
   {
      cEdgeContact *pEdgeContact;
      cEdgeContact *pEdgeContactNext;
      mxs_real      surfaceDistance;

      pEdgeContact = pEdgeContactList->GetFirst();
      while (pEdgeContact != NULL)
      {
         pEdgeContactNext = pEdgeContact->GetNext();

         surfaceDistance = pModel->TerrainDistance(i, pEdgeContact);

         if (surfaceDistance < kBreakTerrainContactDist)
         {
            pModel->AddConstraint(OBJ_NULL, i, pEdgeContact->GetNormal(pModel->GetLocationVec(i)));
         }
         else
         {
            pModel->DestroyTerrainContact(i, pEdgeContact->GetStart(), pEdgeContact->GetEnd());

            #ifndef SHIP
            if (config_is_defined("ContactSpew"))
            {
               mprintf("obj %d (%d) destroying terrain edge contact\n", pModel->GetObjID(), i);
               mprintf("   dist = %g\n", surfaceDistance);
            }
            #endif
         }

         pEdgeContact = pEdgeContactNext;
      }
   }

   // Check for vertex contact
   if (pModel->GetVertexContacts(i, &pVertexContactList))
   {
      cVertexContact *pVertexContact;
      cVertexContact *pVertexContactNext;
      mxs_real      surfaceDistance;

      pVertexContact = pVertexContactList->GetFirst();
      while (pVertexContact != NULL)
      {
         pVertexContactNext = pVertexContact->GetNext();

         surfaceDistance = pModel->TerrainDistance(i, pVertexContact);

         if (surfaceDistance < kBreakTerrainContactDist)
         {
            pModel->AddConstraint(OBJ_NULL, i, pVertexContact->GetNormal(pModel->GetLocationVec(i)));
         }
         else
         {
            pModel->DestroyTerrainContact(i, pVertexContact->GetPoint());

            #ifndef SHIP
            if (config_is_defined("ContactSpew"))
            {
               mprintf("obj %d (%d) destroying terrain vertex contact\n", pModel->GetObjID(), i);
               mprintf("   dist = %g\n", surfaceDistance);
            }
            #endif
         }

         pVertexContact = pVertexContactNext;
      }
   }
}

///////////////////////////////////////

BOOL ConstrainFromObjects(cPhysModel * pModel, tPhysSubModId i)
{
   cPhysSubModelInst *pSubModel;
   cPhysModel *pModel2;
   tPhysSubModId j;
   BOOL on_model = TRUE;

   BOOL contacting_moving_objects = FALSE;

   if (pModel->IsRope() || pModel->IsAttached())
      return FALSE;

   // check for object contact
   if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubModel))
   {
      cPhysSubModelInst *pNextSubModel;
      mxs_real objectDistance;
      mxs_real maxObjectDistance;
      mxs_vector normal;

      while (pSubModel != NULL)
      {
         pNextSubModel = pSubModel->GetNext();

         pModel2 = pSubModel->GetPhysicsModel();
         j = pSubModel->GetSubModId();

         if (pModel2->IsPressurePlate() || pModel2->IsMovingTerrain() || pModel2->IsDoor() ||
             (mx_mag2_vec(&pModel2->GetVelocity(j)) > 0.0001))
            contacting_moving_objects = TRUE;

         // All climbing contact is only broken when it wants to be, and is done
         // after the main loop
         if (pModel2->GetObjID() == pModel->GetClimbingObj())
            goto next_obj_contact;

         // Don't let stuff on pressure plates or moving terrain fall asleep
         if (pModel->IsPressurePlate() || pModel->IsMovingTerrain() || pModel->IsAttached())
            pModel2->SetSleep(FALSE);

         // See if we're on the face of the model
         if (pModel2->GetType(0) == kPMT_OBB)
         {
            cFaceContact faceContact((cPhysOBBModel *)pModel2, pSubModel->GetSubModId());

            on_model = SubModOnPoly(pModel, i, &faceContact, TRUE);
         }
         else
         if ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 1))
         {
            cFaceContact faceContact((cPhysSphereModel *)pModel2, ((cPhysSphereModel *)pModel2)->GetRadius(0));

            if (faceContact.GetPoly() != NULL)
               on_model = SubModOnPoly(pModel, i, &faceContact, TRUE);
            else
               on_model = FALSE;
         }

         // Set normal, regardless
         objectDistance = PhysGetObjsNorm(pModel, i, pModel2,j, normal);

         // FancyGhosts can be arbitrarily far off moving terrain and not
         // break contact
         if ((pModel->IsFancyGhost() && pModel2->IsMovingTerrain()) ||
             (pModel2->IsFancyGhost() && pModel->IsMovingTerrain()))
         {
            objectDistance = 0;
         }

         if (pModel->IsDoor() || pModel2->IsDoor() || pModel->IsAttached() || pModel2->IsAttached())
            maxObjectDistance = kBreakObjectContactDist * 3;
         else
            maxObjectDistance = kBreakObjectContactDist;

         // decide whether to break contact or constrain to model
         if ((objectDistance > maxObjectDistance) || !on_model)
         {
            #ifndef SHIP
            if (config_is_defined("ContactSpew"))
            {
               mprintf("obj %d breaks contact with obj %d\n", pModel->GetObjID(), pSubModel->GetObjID());
               mprintf("  dist = %g, on_model = %d\n", objectDistance, on_model);
            }
            #endif

            // If we're breaking contact with our reference object, set velocity
            // to global reference frame

            if (pModel->GetObjReferenceFrame() == pModel2->GetObjID())
               PostMovingTerrainUpdate(pModel);

            DestroyObjectContact(pModel->GetObjID(), i, pModel,
                                 pModel2->GetObjID(), j, pModel2);
         }
         else
         {
            if (!pModel->IsPlayer() && !pModel2->IsPlayer() &&
                !pModel->IsPressurePlate() && !pModel2->IsPressurePlate() &&
                !pModel->IsMovingTerrain() && !pModel2->IsMovingTerrain())
            {
               pModel->AddConstraint(pModel2->GetObjID(), i, normal);
            }
            else
            if (pModel2->GetType(0) == kPMT_OBB)
            {
               if (pModel2->IsAttached())
               {
                  if (pModel->GetObjReferenceFrame() == OBJ_NULL)
                  {
                     mxs_real mag;

                     mag = mx_dot_vec(&normal, &pModel2->GetVelocity());

                     if ((fabs(mag) > 0.0001) && !pModel2->IsSquishing())
                        pModel->AddConstraint(pModel2->GetObjID(), normal, mag);
                     else
                        pModel->AddConstraint(pModel2->GetObjID(), normal);
                  }
                  else
                     pModel->AddConstraint(pModel2->GetObjID(), normal);
               }
               else
               if (pModel2->IsLocationControlled() && pModel2->IsRotationControlled())
                  pModel->AddConstraint(pModel2->GetObjID(), i, normal);
               else
               if (pModel2->IsLocationControlled() && (j == 2))
                  pModel->AddConstraint(pModel2->GetObjID(), normal);
               else
               if (pModel2->IsDoor())
                  pModel->AddConstraint(pModel2->GetObjID(), normal);
               else
               if (pModel2->IsMovingTerrain() || pModel2->IsPressurePlate())
               {
                  mxs_real mag;

                  mag = mx_dot_vec(&normal, &pModel2->GetVelocity());

                  if ((fabs(mag) > 0.0001) && !pModel2->IsSquishing())
                     pModel->AddConstraint(pModel2->GetObjID(), normal, mag);
                  else
                     pModel->AddConstraint(pModel2->GetObjID(), normal);
               }
            }
            else
	      if ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 1))
            {
               // @TODO: fix this, shouldn't always be limited
               pModel->AddConstraint(pModel2->GetObjID(), normal);
            }
            else
            if (pModel2->IsLocationControlled() && !pModel2->IsRope())  // for creatures
            {
               pModel->AddConstraint(pModel2->GetObjID(), i, normal);
            }
            else
            if (!pModel2->IsPlayer() && !pModel2->IsGhost() &&
                ((pModel2->IsLocationControlled() &&
                 pModel2->IsRotationControlled()) ||
                (pModel2->IsVelocityControlled() &&
                 pModel2->IsRotationControlled())))
            {
               pModel->AddConstraint(pModel2->GetObjID(), i, normal);
            }
         }

next_obj_contact:
         pSubModel = pNextSubModel;
      }
   }

   if (pModel->IsClimbing())
   {
      if (pModel->IsRopeClimbing())
      {
         ObjID rope = pModel->GetClimbingObj();
         int   segment = pModel->GetRopeSegment();

         cPhysModel *pRopeModel = g_PhysModels.GetActive(rope);

         Assert_(pRopeModel);
         Assert_((segment > 0) && (segment < pRopeModel->NumSubModels()));

         mxs_vector seg;
         mxs_vector axis;
         mxs_vector constraint;

         mx_sub_vec(&seg, &pRopeModel->GetLocationVec(segment-1), &pRopeModel->GetLocationVec(segment));
         mx_normeq_vec(&seg);

         mx_unit_vec(&axis, 0);
         mx_cross_vec(&constraint, &seg, &axis);

         pModel->AddConstraint(rope, constraint);
         mx_scaleeq_vec(&constraint, -1.0);
         pModel->AddConstraint(rope, constraint);

         mx_unit_vec(&axis, 1);
         mx_cross_vec(&constraint, &seg, &axis);

         pModel->AddConstraint(rope, constraint);
         mx_scaleeq_vec(&constraint, -1.0);
         pModel->AddConstraint(rope, constraint);

      }
      else
      {
         ObjID climbingObj = pModel->GetClimbingObj();
         cPhysModel *pClimbingModel = g_PhysModels.Get(climbingObj);

#ifndef SHIP
         AssertMsg2(pClimbingModel,"constrain: %s NULL pModel (%s)",ObjWarnName(climbingObj),ObjWarnName(pModel->GetObjID()));
         AssertMsg1(pModel->IsPlayer() || pModel->IsAvatar(),"constrain: %s thinks it is climbing",ObjWarnName(pModel->GetObjID()));
         if (pClimbingModel)
         {  // lets not fault, if we are doing extra debug testing anyway....
            AssertMsg2(pClimbingModel->GetType(0) == kPMT_OBB,"constrain: %s not at OBB (%s)",ObjWarnName(climbingObj),ObjWarnName(pModel->GetObjID()));
         }
#endif

         if (!pClimbingModel)  // lets keep not faulting, eh?
            return contacting_moving_objects;

         mxs_vector normals[6];
         mxs_real   constants[6];
         int i;

         ((cPhysOBBModel *)pClimbingModel)->GetNormals(normals);
         ((cPhysOBBModel *)pClimbingModel)->GetConstants(constants, normals);

         // Which side/edge are we latched onto?
         int sides_in_front = 0;
         int num_sides_in_front = 0;

         for (i=0; i<6; i++)
         {
            mxs_real dot = mx_dot_vec(&normals[i], &pModel->GetLocationVec(2));
            mxs_real dist = dot - constants[i];

            if (dist > 0)
            {
               sides_in_front |= (1 << i);

               if (((cPhysOBBModel *)pClimbingModel)->GetClimbableSides() & (1 << i))
                  num_sides_in_front++;
               else
                  pModel->AddConstraint(climbingObj, normals[(i + 3) % 6]);
            }

            if (dist > 2.0)
            {
               BreakClimb(pModel->GetObjID(), FALSE, FALSE);
               num_sides_in_front = -1;
               break;
            }
         }

         switch(num_sides_in_front)
         {
            case -1:
               break;

            case 0:
            {
               Warning(("%s inside climbing obj %s\n", ObjWarnName(pModel->GetObjID()), ObjWarnName(climbingObj)));
               break;
            }

            case 1:
            {
               // Simple case, on a face, constrain into and out of it
               for (i=0; i<6; i++)
               {
                  if (sides_in_front & (1 << i))
                  {
                     if (((cPhysOBBModel *)pClimbingModel)->GetClimbableSides() & (1 << i))
                     {
                        pModel->AddConstraint(climbingObj, normals[i]);
                        pModel->AddConstraint(climbingObj, normals[(i + 3) % 6]);
                     }
                     break;
                  }
               }
               break;
            }

            case 2:
            {
               // Edge case
               int face_1 = -1;
               int face_2 = -1;

               // find the two faces
               for (i=0; i<6; i++)
               {
                  if (sides_in_front & (1 << i))
                  {
                     if (face_1 == -1)
                        face_1 = i;
                     else
                        face_2 = i;
                  }
               }

               // edge contact, so project onto each normal, add, and re-normalize
               mxs_vector face_1_component;
               mxs_vector face_2_component;
               mxs_vector edge_normal;
               mxs_real face_1_dist;
               mxs_real face_2_dist;

               face_1_dist = mx_dot_vec(&normals[face_1], &pModel->GetLocationVec(2)) - constants[face_1];
               face_2_dist = mx_dot_vec(&normals[face_2], &pModel->GetLocationVec(2)) - constants[face_2];

               mx_scale_vec(&face_1_component, &normals[face_1], face_1_dist);
               mx_scale_vec(&face_2_component, &normals[face_2], face_2_dist);

               mx_add_vec(&edge_normal, &face_1_component, &face_2_component);

#if 0
               mprintf("[%d] comp = %g %g %g\n", face_1, face_1_component.x, face_1_component.y, face_1_component.z);
               mprintf("[%d] comp = %g %g %g\n", face_2, face_2_component.x, face_2_component.y, face_2_component.z);
               mprintf("edge = %g %g %g\n", edge_normal.x, edge_normal.y, edge_normal.z);
               mprintf("dist = %g\n", mx_mag_vec(&edge_normal));
               mprintf("\n");
#endif

               mx_normeq_vec(&edge_normal);
               pModel->AddConstraint(climbingObj, edge_normal);
               mx_scaleeq_vec(&edge_normal, -1.0);
               pModel->AddConstraint(climbingObj, edge_normal);

               break;
            }

            case 3:
            {
               // Vertex case


            }

            default:
            {
               Warning(("%s in front of more than 3 sides of climbing obj %s?\n",
                        ObjWarnName(pModel->GetObjID()), ObjWarnName(climbingObj)));
               break;
            }
         }
      }
   }

   return contacting_moving_objects;
}


///////////////////////////////////////////////////////////////////////////////

static void UpdateObjectContacts(cPhysModel *pModel, mxs_real dt)
{
   cPhysSubModelInst *pSubMod;
   cPhysModel *pModel2;
   int i, j;

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubMod))
      {
         // rotating
         mxs_angvec cur_ang;
         mxs_angvec delta_ang;
         mxs_matrix obj_rot;
         mxs_matrix obj_rot_delta;
         mxs_vector world_cog;
         mxs_vector center_pt;
         mxs_vector rot_arm;
         mxs_vector dest_rot_arm;
         mxs_vector rot_axis;
         mxs_vector delta_pos;
         mxs_vector rel_pos;
         mxs_vector add_velocity;
         mxs_vector net_velocity;
         mxs_angvec obj_rotation;

         // pushing
         mxs_vector norm, revnorm;
         mxs_real   dp1, dp2;
         mxs_vector vel1, vel2;
         mxs_vector tvel1, tvel2;
         mxs_vector nvel1, nvel2;
         mxs_vector resvel;
         mxs_vector relvel;
         mxs_vector relpos;
         mxs_real   mass1, mass2;

         while (pSubMod != NULL)
         {
            pModel2 = pSubMod->GetPhysicsModel();
            j = pSubMod->GetSubModId();

            // ROTATING

            if (pModel2->IsRotationalVelocityControlled() && (!pModel2->IsPlayer()))
            {
               pModel2->UpdateModel(dt);

               cur_ang = pModel2->GetRotation();
               delta_ang = pModel2->GetEndRotationVec();
               delta_ang.tx -= cur_ang.tx;
               delta_ang.ty -= cur_ang.ty;
               delta_ang.tz -= cur_ang.tz;

               mx_ang2mat(&obj_rot, &cur_ang);
               mx_ang2mat(&obj_rot_delta, &delta_ang);

               // convert cog to world space
               mx_copy_vec(&world_cog, &pModel2->GetCOGOffset());
               if (!IsZeroVector(world_cog))
               {
                  mx_mat_mul_vec(&center_pt, &obj_rot, &world_cog);
                  mx_scaleeq_vec(&center_pt, -1.0);
                  mx_addeq_vec(&center_pt, &pModel2->GetLocationVec());
               }
               else
                  mx_copy_vec(&center_pt, &pModel2->GetLocationVec());

               // find rotation arm (project into plane of rotation)
               mx_sub_vec(&rot_arm, &pModel->GetLocationVec(), &center_pt);

               mx_copy_vec(&rot_axis, &pModel2->GetControls()->GetControlRotationalVelocity());
               mx_normeq_vec(&rot_axis);

               PhysRemNormComp(&rot_arm, rot_axis);
               mx_scaleeq_vec(&rot_axis, -1.0);
               PhysRemNormComp(&rot_arm, rot_axis);

               // rotate arm into object space
               mxs_vector obj_rot_arm;
               mx_mat_tmul_vec(&obj_rot_arm, &obj_rot, &rot_arm);

               // rotate arm by delta
               mxs_vector mid_rot_arm;
               mx_mat_mul_vec(&mid_rot_arm, &obj_rot_delta, &obj_rot_arm);

               // rotate arm back
               mx_mat_mul_vec(&dest_rot_arm, &obj_rot, &mid_rot_arm);

               // find difference in position
               mx_sub_vec(&delta_pos, &dest_rot_arm, &rot_arm);

               // back-compute into a velocity
               mx_scale_vec(&add_velocity, &delta_pos, 1 / dt);

               // are we pushing or pulling?
               PhysGetObjsNorm(pModel, i, pModel2, j, norm);
               if (mx_dot_vec(&norm, &add_velocity) > 0)
               {
                  if (pModel2->IsDoor())
                  {
                     BOOL blocked, too_heavy;
                     mxs_vector cons_velocity;

                     mx_copy_vec(&cons_velocity, &add_velocity);
                     pModel->ApplyConstraints(&cons_velocity);

                     blocked = !EqualVectors(cons_velocity, add_velocity);

                     // @TODO: implement door blocking so trap doors work
                     blocked = FALSE;

                     too_heavy = (GetDoorPushMass(pModel2->GetObjID()) < pModel->GetDynamics()->GetMass());

                     if (blocked || too_heavy)
                     {
                        DoorPhysAbort(pModel2->GetObjID());

                        pModel2->UpdateModel(dt);
                     }

                     if (!blocked && too_heavy)
                     {
                        mxs_real scale_factor;

                        scale_factor = pModel2->GetDynamics()->GetMass() / pModel->GetDynamics()->GetMass();
                        mx_scaleeq_vec(&add_velocity, min(scale_factor, 1.0));
                     }

                     // see if we need to contribute to velocity to reach add_velocity
                     mx_sub_vec(&net_velocity, &pModel->GetVelocity(), &add_velocity);
                     if (mx_dot_vec(&net_velocity, &add_velocity) < 0)
                     {
                        mxs_vector vel_dir;

                        mx_norm_vec(&vel_dir, &add_velocity);
                        mx_scaleeq_vec(&vel_dir, -1.0);
                        mx_copy_vec(&net_velocity, &pModel->GetVelocity());
                        PhysRemNormComp(&net_velocity, vel_dir);
                        mx_addeq_vec(&net_velocity, &add_velocity);

                        pModel->GetDynamics()->SetVelocity(net_velocity);
                     }

                     // adjust object's rotation too
                     if (!pModel->IsRotationControlled() && !pModel->IsDoor() && !pModel->IsPlayer())
                     {
                        obj_rotation = pModel->GetRotation();
                        obj_rotation.tx += delta_ang.tx;
                        obj_rotation.ty += delta_ang.ty;
                        obj_rotation.tz += delta_ang.tz;
                        pModel->SetRotation(obj_rotation);
                     }
                  }
               }
            }

            // PUSHING

            if (pModel->IsAttached() || pModel2->IsAttached() ||
                pModel->IsMovingTerrain() || pModel2->IsMovingTerrain() ||
                pModel->IsLocationControlled() || pModel2->IsLocationControlled() ||
                pModel->IsPressurePlate() || pModel2->IsPressurePlate() ||
                pModel->IsDoor() || pModel2->IsDoor())
               goto next_submod;

            // Get normal between objects
            PhysGetObjsNorm(pModel, i, pModel2, j, norm);
            mx_scale_vec(&revnorm, &norm, -1.0);

            if ((pModel->GetType(i) == kPMT_Sphere) && (pModel2->GetType(j) == kPMT_Sphere))
            {
               if (fabs(norm.z) < 1.0)
               {
                  norm.z = 0;
                  mx_normeq_vec(&norm);
                  revnorm.z = 0;
                  mx_normeq_vec(&revnorm);
               }
            }

            // Get velocities
            mx_copy_vec(&vel1, &pModel->GetVelocity());
            mx_copy_vec(&vel2, &pModel2->GetVelocity());

            pModel->ApplyConstraints(&vel1);
            pModel2->ApplyConstraints(&vel2);

            // Project velocities onto normal
            dp1 = mx_dot_vec(&vel1, &norm);
            dp2 = mx_dot_vec(&vel2, &norm);

            mx_scale_vec(&tvel1, &norm, dp1);
            mx_scale_vec(&tvel2, &norm, dp2);

            // Find resulting averaged velocity
            mass1 = pModel->GetDynamics()->GetMass();
            mass2 = pModel2->GetDynamics()->GetMass();

            Assert_(mass1 + mass2 > 0);

            mx_scaleeq_vec(&tvel1, mass1);
            mx_scaleeq_vec(&tvel2, mass2);

            mx_add_vec(&resvel, &tvel1, &tvel2);
            mx_scaleeq_vec(&resvel, 0.5 / (mass1 + mass2));

            // Find relative position & velocity
            mx_sub_vec(&relvel, &vel1, &vel2);
            mx_sub_vec(&relpos, &pModel->GetLocationVec(i), &pModel2->GetLocationVec(j));

            #ifndef SHIP
            if (config_is_defined("PushSpew"))
            {
               mprintf("[%d] to [%d]\n", pModel->GetObjID(), pModel2->GetObjID());
               mprintf("relvel = %g %g %g\n", relvel.x, relvel.y, relvel.z);
               mprintf("relpos = %g %g %g\n", relpos.x, relpos.y, relpos.z);
               mprintf("norm = %g %g %g\n", norm.x, norm.y, norm.z);
            }
            #endif

            // Towards each other?
            if ((mx_dot_vec(&relvel, &norm) < 0) && (fabs(mx_dot_vec(&relvel, &norm)) > 0.001))
            {
               #ifndef SHIP
               if (config_is_defined("PushSpew"))
               {
                  mprintf("\nobj %d (%d) into obj %d (%d)\n", pModel->GetObjID(), i, pModel2->GetObjID(), j);
                  mprintf("  vel1 = %g %g %g\n", vel1.x, vel1.y, vel1.z);
                  mprintf("  vel2 = %g %g %g\n", vel2.x, vel2.y, vel2.z);
                  mprintf("  dp1 = %g\n", dp1);
                  mprintf("  dp2 = %g\n", dp2);
                  mprintf("  resvel = %g %g %g\n", resvel.x, resvel.y, resvel.z);
               }
               #endif

               // Remove velocity components into each other
               PhysRemNormComp(&vel1, norm);
               PhysRemNormComp(&vel2, norm);
               PhysRemNormComp(&vel1, revnorm);
               PhysRemNormComp(&vel2, revnorm);

               // Add resulting velocity in
               mx_addeq_vec(&vel1, &resvel);
               mx_addeq_vec(&vel2, &resvel);

               // If we constrain an object's new velocity, add it's normal as
               // a constraint to the *other* object
               mx_copy_vec(&nvel1, &vel1);
               pModel->ApplyConstraints(&nvel1);

               mx_copy_vec(&nvel2, &vel2);
               pModel2->ApplyConstraints(&nvel2);

               if (!EqualVectors(nvel1, vel1))
               {
                  #ifndef SHIP
                  if (config_is_defined("PushSpew"))
                  {
                     mprintf("  1 vel went from %g %g %g\n", vel1.x, vel1.y, vel1.z);
                     mprintf("               to %g %g %g\n", nvel1.x, nvel1.y, nvel1.z);
                     mprintf("  constraining 2\n");
                  }
                  #endif
                  pModel2->AddConstraint(pModel->GetObjID(), revnorm);
               }
               if (!EqualVectors(nvel2, vel2))
               {
                  #ifndef SHIP
                  if (config_is_defined("PushSpew"))
                  {
                     mprintf("  2 vel went from %g %g %g\n", vel2.x, vel2.y, vel2.z);
                     mprintf("               to %g %g %g\n", nvel2.x, nvel2.y, nvel2.z);
                     mprintf("  constraining 1\n");
                  }
                  #endif
                  pModel->AddConstraint(pModel2->GetObjID(), norm);
               }

               // Constrain each velocity
               pModel->ApplyConstraints(&vel1);
               pModel2->ApplyConstraints(&vel2);

               #ifndef SHIP
               if (config_is_defined("PushSpew"))
               {
                  mprintf("  tvel1 = %g %g %g\n", tvel1.x, tvel1.y, tvel1.z);
                  mprintf("  tvel2 = %g %g %g\n", tvel2.x, tvel2.y, tvel2.z);
                  mprintf("  post-vel1 = %g %g %g\n", vel1.x, vel1.y, vel1.z);
                  mprintf("  post-vel2 = %g %g %g\n", vel2.x, vel2.y, vel2.z);

               }
               #endif

               // Update velocities
               pModel->GetDynamics()->SetVelocity(vel1);
               pModel2->GetDynamics()->SetVelocity(vel2);

               pModel2->SetSleep(FALSE);
            }

next_submod:
            pSubMod = pSubMod->GetNext();
         }
      }
   }
}

////////////////////////////////////////

static void PreMovingTerrainUpdate(cPhysModel *pModel)
{
   cPhysSubModelInst *pSubMod;
   cPhysModel *pModel2;
   mxs_vector  velocity;
   mxs_vector  sub_velocity;
   mxs_vector  norm;
   int i, j;

   pModel->SetObjReferenceFrame(OBJ_NULL);

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubMod))
      {
         while (pSubMod != NULL)
         {
            pModel2 = pSubMod->GetPhysicsModel();
            j = pSubMod->GetSubModId();

            if (((pModel2->GetType(0) == kPMT_OBB) && (j == 2)) || // @TODO: calc this
                ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 1)))
            {
               if (!pModel->IsRope())
               {
                  PhysGetObjsNorm(pModel, i, pModel2, j, norm);

                  mx_copy_vec(&velocity, &pModel->GetVelocity());
                  mx_copy_vec(&sub_velocity, &pModel2->GetVelocity());

                  PhysRemNormComp(&sub_velocity, norm);
                  mx_scaleeq_vec(&norm, -1.0);
                  PhysRemNormComp(&sub_velocity, norm);

                  mx_subeq_vec(&velocity, &sub_velocity);
                  pModel->GetDynamics()->SetVelocity(velocity);

                  pModel->SetObjReferenceFrame(pModel2->GetObjID());
               }
            }

            pSubMod = pSubMod->GetNext();
         }
      }
   }
}

////////////////////////////////////////

static void PostMovingTerrainUpdate(cPhysModel *pModel)
{
   cPhysSubModelInst *pSubMod;
   cPhysModel *pModel2;
   mxs_vector  velocity;
   mxs_vector  add_velocity;
   mxs_vector  norm;
   int i, j;

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubMod))
      {
         while (pSubMod != NULL)
         {
            pModel2 = pSubMod->GetPhysicsModel();
            j = pSubMod->GetSubModId();

            if (((pModel2->GetType(0) == kPMT_OBB) && (j == 2)) ||
                ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 1)))
            {
               pModel->SetObjReferenceFrame(OBJ_NULL);

               if (!pModel2->IsSquishing() && !pModel->IsRope())
               {
                  PhysGetObjsNorm(pModel, i, pModel2, j, norm);

                  mx_copy_vec(&velocity, &pModel->GetVelocity());
                  mx_copy_vec(&add_velocity, &pModel2->GetVelocity());

                  PhysRemNormComp(&add_velocity, norm);
                  mx_scaleeq_vec(&norm, -1.0);
                  PhysRemNormComp(&add_velocity, norm);

                  mx_addeq_vec(&velocity, &add_velocity);
                  pModel->GetDynamics()->SetVelocity(velocity);
               }
               else
               {
                  mx_copy_vec(&velocity, &pModel->GetVelocity());
                  mx_copy_vec(&add_velocity, &pModel2->GetVelocity());

                  mx_normeq_vec(&add_velocity);

                  PhysRemNormComp(&velocity, add_velocity);
                  mx_scaleeq_vec(&add_velocity, -1.0);
                  PhysRemNormComp(&velocity, add_velocity);

                  pModel->GetDynamics()->SetVelocity(velocity);
               }
            }

            pSubMod = pSubMod->GetNext();
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

static void UpdateAttachment(cPhysModel *pModel, mxs_real dt)
{
   ILinkQuery *query = g_pPhysAttachRelation->Query(pModel->GetObjID(), LINKOBJ_WILDCARD);

   if (!query->Done())
   {
      cPhysModel *pModelAttach;
      sPhysAttachData *pAttachData;
      sLink link;

      query->Link(&link);
      pAttachData = (sPhysAttachData *)query->Data();

      pModelAttach = g_PhysModels.GetActive(link.dest);
      if (pModelAttach != NULL)
      {
#if 1
         PhysSetVelocity(pModel->GetObjID(), (mxs_vector *)&pModelAttach->GetVelocity());
#else
         mxs_vector target_pos;
         mx_add_vec(&target_pos, &pModelAttach->GetLocationVec(), &pAttachData->offset);

         mxs_vector velocity;
         mx_sub_vec(&velocity, &target_pos, &pModel->GetLocationVec());
         mx_scaleeq_vec(&velocity, 1 / dt);

         // Attachment updating stuff
         PhysSetVelocity(pModel->GetObjID(), &velocity);
#endif
      }
   }

   SafeRelease(query);
}

///////////////////////////////////////////////////////////////////////////////

static void UpdateRopeDynamics(cPhysModel *pModel)
{
   int i;

   // Don't let the top model move
   cPhysCtrlData *pCtrl = pModel->GetControls(0);
   pCtrl->ControlLocation(pModel->GetLocationVec());

   mxs_vector prev_sub_vel;
   mxs_vector sub_vel;
   mxs_vector next_sub_vel;

   Assert_(pModel->NumSubModels() == 8);

   mxs_vector result_vel[8];

   // "Blur" velocities from adjacent nodes
   for (i=1; i<pModel->NumSubModels(); i++)
   {
      mx_copy_vec(&prev_sub_vel, &pModel->GetVelocity(i - 1));
      mx_copy_vec(&sub_vel, &pModel->GetVelocity(i));
      if (i == pModel->NumSubModels() - 1)
         mx_copy_vec(&next_sub_vel, &pModel->GetVelocity(i - 1));
      else
         mx_copy_vec(&next_sub_vel, &pModel->GetVelocity(i + 1));

      mx_scale_vec(&result_vel[i], &sub_vel, 0.35);
      mx_scale_addeq_vec(&result_vel[i], &prev_sub_vel, 0.80);
      mx_scale_addeq_vec(&result_vel[i], &next_sub_vel, 0.25);
   }

   // Add in water current
   Location loc;
   mxs_vector flow;

   for (i=1; i<pModel->NumSubModels(); i++)
   {
      MakeHintedLocationFromVector(&loc, &pModel->GetLocationVec(i), &pModel->GetLocation(i));

      mx_zero_vec(&flow);
      if (CellFromLoc(&loc) != CELL_INVALID)
      {
         uchar motion_index = WR_CELL(loc.cell)->motion_index;

         if (motion_index == 0)
            continue;

         mx_copy_vec(&flow, &g_aMedMoCellMotion[motion_index].center_change);
         mxs_real scale_jitter = ((float)(Rand() % 1024) / 1024.0) + 0.5;

         mx_scaleeq_vec(&flow, scale_jitter);
      }

      mx_addeq_vec(&result_vel[i], &flow);
   }

   // Limit velocities to plane of rope segment
   mxs_vector norm;
   mxs_vector norm_comp;

   for (i=1; i<pModel->NumSubModels(); i++)
   {
      mx_sub_vec(&norm, &pModel->GetLocationVec(i-1), &pModel->GetLocationVec(i));
      if (!IsZeroVector(norm))
      {
         mx_normeq_vec(&norm);

         mx_scale_vec(&norm_comp, &norm, mx_dot_vec(&result_vel[i], &norm));
         mx_subeq_vec(&result_vel[i], &norm_comp);
      }
   }

   // Update velocities
   for (i=1; i<pModel->NumSubModels(); i++)
      pModel->GetDynamics(i)->SetVelocity(result_vel[i]);

   // Update submodel offsets
   mxs_matrix orien;
   mx_ang2mat(&orien, &pModel->GetRotation());

   mxs_vector rot_offset;
   mxs_vector unrot_offset;

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      mx_sub_vec(&rot_offset, &pModel->GetLocationVec(i), &pModel->GetLocationVec());
      mx_mat_tmul_vec(&unrot_offset, &orien, &rot_offset);

      pModel->SetSubModOffset(i, unrot_offset);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void UpdateModelTransDynamics(cPhysModel *pModel, BOOL base_friction, mxs_real friction_amt, mxs_real dt)
{
   cPhysDynData  *pDynamics = pModel->GetDynamics();
   mxs_vector     velocity;
   mxs_vector     control_velocity;
   mxs_vector     actual_velocity;
   mxs_vector     ideal_velocity;
   mxs_vector     sum_forces;

   mxs_vector     friction;
   mxs_vector     *pConveyorVel;

   BOOL           in_water;
   BOOL           on_ground;
   BOOL           on_platform;
   BOOL           on_conveyor;

   // Make sure our locations are going to get updated
   UpdateChangedLocation((Location *)&pModel->GetLocation());
   for (int i=0; i<pModel->NumSubModels(); i++)
      UpdateChangedLocation((Location *)&pModel->GetLocation(i));

   in_water = PhysObjInWater(pModel->GetObjID()) && !pModel->IsDoor() && !pModel->IsMovingTerrain();
   on_ground = PhysObjOnGround(pModel->GetObjID());
   on_platform = PhysObjOnPlatform(pModel->GetObjID());

   // Set/Clear Base Friction
   if (pModel->Gravity())
   {
      if (in_water)
         pModel->SetBaseFriction(.3);
      else
         pModel->SetBaseFriction(0);
   }

   // Get velocity & acceleration
   mx_copy_vec(&velocity, &pDynamics->GetVelocity());
   mx_copy_vec(&control_velocity, &(pModel->GetControls()->GetControlVelocity()));
   mx_copy_vec(&sum_forces, &pDynamics->GetAcceleration());

   // if climbing, get rid of uncontrolled falls
   if (pModel->IsClimbing())
     {
       //if we are going down but our controls say we shouldn't be,
       //well then we darn well shouldn't be.
       if ((velocity.z < 0) && (control_velocity.z >= 0))
	 {
	   velocity.z = 0;
	 }
     }

   // add in conveyor belt speed (if exists) to player here
   on_conveyor = pModel->IsPlayer() && g_pPhysConveyorVelProp->Get( pModel->GetObjID(), &pConveyorVel );
   if ( on_conveyor ) {
      mxs_vector tmpVec;
      const float t = 0.2;      // t is the time it takes the conveyor to accelerate you to full speed

      // the conveyor adds in a force sufficient to accelerate player from current
      //   velocity to (conveyor velocity + control velocity) in t seconds
      mx_sub_vec( &tmpVec, pConveyorVel, &velocity );
      mx_addeq_vec( &tmpVec, &control_velocity );
      mx_scale_addeq_vec( &sum_forces, &tmpVec, pDynamics->GetMass() / t );
   }

   if (in_water)
      mx_scaleeq_vec(&sum_forces, 0.5);

   // Gravity
   if (pModel->Gravity() && !pModel->IsClimbing() && !pModel->IsMantling())
   {
      mx_scale_addeq_vec(&sum_forces, (mxs_vector *) &kGravityDir,
                         kGravityAmt * pDynamics->GetMass() * pModel->GetGravity());
   }

   // Buoyancy
   if (in_water)
   {
      mxs_real   flow_const;
      mxs_vector flow;
      mxs_vector buoyancy;

      mx_unit_vec(&buoyancy, 2);
      mx_scaleeq_vec(&buoyancy, (1 / pDynamics->GetDensity()) * kGravityAmt);

      mx_scale_addeq_vec(&sum_forces, &buoyancy, pDynamics->GetMass());

#ifndef SHIP
      if (config_is_defined("FlowConstant"))
         config_get_float("FlowConstant", &flow_const);
      else
#endif
         flow_const = 0.8;

      // Current flow
      PhysObjGetFlow(pModel->GetObjID(), &flow);

      for (int j=0; j<3; j++)
      {
         if (flow.el[j] < 0)
         {
            if ((flow.el[j] * flow_const) < velocity.el[j])
               sum_forces.el[j] += flow.el[j] * pDynamics->GetMass() * 2 * ((j==2)?4.0:1.0);
         }
         else
         if (flow.el[j] > 0)
         {
            if ((flow.el[j] * flow_const) > velocity.el[j])
              sum_forces.el[j] += flow.el[j] * pDynamics->GetMass() * 2;
         }
      }
   }

   // Determine ideal velocity
   mx_scale_add_vec(&ideal_velocity, &velocity, &sum_forces, dt / pDynamics->GetMass());

   // Factor in friction
   if ( !on_conveyor && (!pModel->IsVelocityControlled() || pModel->IsMantling()) )
   {
      friction_amt *= pDynamics->GetMass() * kGravityAmt;

      if (!IsZeroVector(ideal_velocity))
      {
         mx_norm_vec(&friction, &ideal_velocity);
         mx_scaleeq_vec(&friction, -friction_amt);

         if (on_platform && (ideal_velocity.z < 0))
            friction.z = 0;
         else
            friction.z *= 1.4;

         if (in_water && !pModel->IsPlayer())
            mx_scaleeq_vec(&friction, 0.2);

         if (base_friction)
         {
            mxs_real drag_scale_factor;

            drag_scale_factor = (50 * mx_mag_vec(&velocity)) / pDynamics->GetMass();
            if (drag_scale_factor > 10)
               drag_scale_factor = 10;
            else
            if (drag_scale_factor < .25)
               drag_scale_factor = .25;
            mx_scaleeq_vec(&friction, drag_scale_factor);
         }

         mx_addeq_vec(&sum_forces, &friction);
      }

      mx_scale_add_vec(&actual_velocity, &velocity, &sum_forces, dt / pDynamics->GetMass());

      if (mx_dot_vec(&actual_velocity, &ideal_velocity) < 0)
         mx_zero_vec(&actual_velocity);
   }
   else
      mx_copy_vec(&actual_velocity, &ideal_velocity);

   pDynamics->SetAcceleration(sum_forces);

   pDynamics->SetVelocity(actual_velocity);
}

///////////////////////////////////////

static void AdjustForRestAxes(cPhysModel *pModel, mxs_vector *rot_vel, mxs_vector *sum_forces,
                              mxs_real friction_amt, mxs_real dt)
{
   mxs_matrix obj_rot;
   mx_ang2mat(&obj_rot, &pModel->GetRotation());

   // find the best-z face contact
   cFaceContactList *pFaceContactList;
   cPhysSubModelInst *pSubModel;
   cPhysModel *pModel2;
   mxs_vector ground_normal;
   mxs_real   best_z = -1.0;
   int j;

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if (pModel->GetFaceContacts(i, &pFaceContactList))
      {
         cFaceContact *pFaceContact;

         pFaceContact = pFaceContactList->GetFirst();
         while (pFaceContact != NULL)
         {
            if (pFaceContact->GetNormal().z > best_z)
            {
               mx_copy_vec(&ground_normal, &pFaceContact->GetNormal());
               best_z = ground_normal.z;
            }

            pFaceContact = pFaceContact->GetNext();
         }
      }

      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pSubModel))
      {
         while (pSubModel != NULL)
         {
            pModel2 = pSubModel->GetPhysicsModel();
            j = pSubModel->GetSubModId();

            if (pModel2->GetType(0) == kPMT_OBB)
            {
               cFaceContact faceContact((cPhysOBBModel *)pModel2, pSubModel->GetSubModId());

               if (faceContact.GetNormal().z > best_z)
               {
                  mx_copy_vec(&ground_normal, &faceContact.GetNormal());
                  best_z = ground_normal.z;
               }
            }
            else
            if ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 1))
            {
               cFaceContact faceContact((cPhysSphereModel *)pModel2, ((cPhysSphereModel *)pModel2)->GetRadius(0));

               if ((faceContact.GetPoly() != NULL) && (faceContact.GetNormal().z > 0))
               {
                  mx_copy_vec(&ground_normal, &faceContact.GetNormal());
                  best_z = ground_normal.z;
               }
            }
            else
            if ((pModel2->GetType(j) == kPMT_Sphere) ||
                ((pModel2->GetType(j) == kPMT_SphereHat) && (j == 0)))
            {
               mxs_vector obj_normal;
               PhysGetObjsNorm(pModel, i, pModel2, j, obj_normal);

               if (obj_normal.z > best_z)
               {
                  mx_copy_vec(&ground_normal, &obj_normal);
                  best_z = ground_normal.z;
               }
            }

            pSubModel = pSubModel->GetNext();
         }
      }
   }

   if (best_z <= 0)
      return;

   #ifndef SHIP
   if (config_is_defined("RestSpew"))
   {
      mprintf(" [%d]\n", pModel->GetObjID());
      mprintf("  ground_normal = %g %g %g\n", ground_normal.x, ground_normal.y, ground_normal.z);
      mprintf("  orientation = %d %d %d\n", pModel->GetRotation().tx, pModel->GetRotation().ty, pModel->GetRotation().tz);
   }
   #endif

   // find best axis
   int        best_axis_index = -1;
   mxs_vector best_axis;
   mxs_real   axis_val;
   mxs_real   max_axis_val;

   for (int i=0; i<6; i++)
   {
      if (pModel->GetRestAxes() & (1 << i))
      {
         mxs_vector axis;

         mx_copy_vec(&axis, &obj_rot.vec[i%3]);
         if (i >= 3)
            mx_scaleeq_vec(&axis, -1.0);

         axis_val = mx_dot_vec(&axis, &ground_normal);

         #ifndef SHIP
         if (config_is_defined("RestSpew"))
            mprintf("  axis %d with %g\n", i, axis_val);
         #endif

         if ((axis_val > max_axis_val) || (best_axis_index == -1))
         {
            max_axis_val = axis_val;
            mx_copy_vec(&best_axis, &axis);
            best_axis_index = i;
         }
      }
   }

   Assert_(best_axis_index != -1);

   #ifndef SHIP
   if (config_is_defined("RestSpew"))
      mprintf("  best_axis: %d (%g %g %g), val: %g\n", best_axis_index, best_axis.x, best_axis.y, best_axis.z, max_axis_val);
   #endif

   // find rotation amount
   mxs_vector rot_delta;
   mxs_vector rot_axis;
   mxs_real   scale;
   mxs_real   dot;

   mx_sub_vec(&rot_delta, &best_axis, &ground_normal);

   mx_cross_vec(&rot_axis, &rot_delta, &best_axis);

   if (IsZeroVector(rot_axis))
   {
      pModel->SetRest(TRUE);
      return;
   }

   mx_normeq_vec(&rot_axis);

   if (mx_mag2_vec(&rot_delta) < (dt * .3)) // || (mx_dot_vec(&rot_axis, rot_vel) < 0.0))// && (mx_mag2_vec(rot_vel) < 0.01))
   {
      mxs_matrix end_facing;
      mxs_angvec end_facing_ang;

      // slam position
      mx_copy_mat(&end_facing, &obj_rot);

      mx_copy_vec(&end_facing.vec[best_axis_index%3], &ground_normal);
      if (best_axis_index > 2)
         mx_scaleeq_vec(&end_facing.vec[best_axis_index%3], -1.0);

      switch (best_axis_index % 3)
      {
         case 0:
         {
            mx_cross_vec(&end_facing.vec[2], &end_facing.vec[0], &end_facing.vec[1]);
            mx_cross_vec(&end_facing.vec[1], &end_facing.vec[2], &end_facing.vec[0]);
            mx_normeq_vec(&end_facing.vec[2]);
            mx_normeq_vec(&end_facing.vec[1]);
            break;
         }
         case 1:
         {
            mx_cross_vec(&end_facing.vec[0], &end_facing.vec[1], &end_facing.vec[2]);
            mx_cross_vec(&end_facing.vec[2], &end_facing.vec[0], &end_facing.vec[1]);
            mx_normeq_vec(&end_facing.vec[0]);
            mx_normeq_vec(&end_facing.vec[2]);
            break;
         }
         case 2:
         {
            mx_cross_vec(&end_facing.vec[1], &end_facing.vec[2], &end_facing.vec[0]);
            mx_cross_vec(&end_facing.vec[0], &end_facing.vec[1], &end_facing.vec[2]);
            mx_normeq_vec(&end_facing.vec[1]);
            mx_normeq_vec(&end_facing.vec[0]);
            break;
         }
      }

      mx_mat2ang(&end_facing_ang, &end_facing);
      pModel->SetRotation(end_facing_ang);

      #ifndef SHIP
      if (config_is_defined("RestSpew"))
      {
         mprintf("  end_facing_ang = %d %d %d\n", end_facing_ang.tx, end_facing_ang.ty, end_facing_ang.tz);
         mprintf("\n");
      }
      #endif

      mx_scaleeq_vec(rot_vel, .3);

      pModel->SetRest(TRUE);
      return;
   }
   else
      pModel->SetRest(FALSE);

   scale = 20 * pModel->GetDynamics()->GetMass();
   dot = mx_dot_vec(&rot_axis, rot_vel);
   if (dot < -1)
      scale *= -dot;

   #ifndef SHIP
   if (config_is_defined("RestSpew"))
   {
      mprintf("  rot_delta = %g %g %g\n", rot_delta.x, rot_delta.y, rot_delta.z);
      mprintf("  rot_vel = %g %g %g\n", rot_vel->x, rot_vel->y, rot_vel->z);
      mprintf("  rot_axis = %g %g %g\n", rot_axis.x, rot_axis.y, rot_axis.z);
      mprintf("  scale = %g\n", scale);
      mprintf("\n");
   }
   #endif

   mx_scaleeq_vec(&rot_axis, scale);

   mx_addeq_vec(sum_forces, &rot_axis);
}

///////////////////////////////////////

static void UpdateModelRotDynamics(cPhysModel *pModel, cPhysDynData *pDynamics, mxs_real friction_amt, mxs_real dt)
{
   mxs_vector    rot_velocity;
   mxs_vector    rot_ideal_velocity;
   mxs_vector    rot_actual_velocity;

   mxs_vector    sum_forces;

   mxs_vector    friction;

   if (pDynamics->Frozen() || pModel->IsRotationControlled())
   {
      pModel->SetRest(TRUE);
      return;
   }

   if (pDynamics->GetMass() <= 0)
   {
      Warning(("obj %d has a mass of %g\n", pModel->GetObjID(), pDynamics->GetMass()));
      return;
   }

   friction_amt *= pDynamics->GetMass() * kGravityAmt / 3;

   // Get acceleration and velocity
   mx_copy_vec(&rot_velocity, &pDynamics->GetRotationalVelocity());
   mx_copy_vec(&sum_forces, &pDynamics->GetRotationalAcceleration());

   // Rest axes
   if ((pModel->GetRestAxes() == NoAxes) || (pModel->GetType(0) == kPMT_OBB))
      pModel->SetRest(TRUE);
   else
   {
      if ((friction_amt > 0) && PhysObjOnGround(pModel->GetObjID()))
      {
         if (!pModel->AtRest())
            AdjustForRestAxes(pModel, &rot_velocity, &sum_forces, friction_amt, dt);
      }
      else
         pModel->SetRest(FALSE);
   }

   mx_scale_add_vec(&rot_ideal_velocity, &rot_velocity, &sum_forces, dt / pModel->GetDynamics()->GetMass());

   int submod = pDynamics->GetSubModId();

   // Factor in friction
   if (!pModel->IsRotationalVelocityControlled() &&
       ((submod == -1) || !(pModel->GetControls(submod)->GetType() & kPCT_RotVelControl)))
   {
      if (!IsZeroVector(rot_velocity))
      {
         mx_norm_vec(&friction, &rot_velocity);
         mx_scaleeq_vec(&friction, -friction_amt);

         mx_addeq_vec(&sum_forces, &friction);
      }

      mx_scale_add_vec(&rot_actual_velocity, &rot_velocity, &sum_forces, dt / pDynamics->GetMass());

      if (mx_dot_vec(&rot_actual_velocity, &rot_ideal_velocity) < 0)
         mx_zero_vec(&rot_actual_velocity);
   }
   else
      mx_copy_vec(&rot_actual_velocity, &rot_ideal_velocity);

   mx_copy_vec(&rot_velocity, &rot_actual_velocity);

   pDynamics->SetRotationalVelocity(rot_velocity);
}

////////////////////////////////////////

static void UpdateModelDynamics(cPhysModel *pModel, mxs_real dt)
{
   mxs_vector velocity;

#if 0
   if (pModel->IsLocationControlled() && !pModel->IsRotationalVelocityControlled() && !pModel->IsRope())
   {
      mxs_vector zero;
      mx_zero_vec(&zero);

      pModel->GetDynamics()->SetVelocity(zero);
      pModel->SetSleep(TRUE);
      return;
   }
#endif

   ObjID objID = pModel->GetObjID();

   mxs_real friction;
   BOOL base_friction = PhysGetFriction(objID, &friction);

   // Update the model's dynamics
   if (pModel->IsRope())
      UpdateRopeDynamics(pModel);
   else
   {
      if (!pModel->GetDynamics()->Frozen() && !pModel->IsLocationControlled())
      {
         Assert_(pModel->GetDynamics()->GetMass() > 0.0);
         UpdateModelTransDynamics(pModel, base_friction, friction, dt);
      }
   }

   // Have we become non-physical?
   if (g_PhysModels.Get(objID) == NULL)
      return;

   UpdateModelRotDynamics(pModel, pModel->GetDynamics(), friction, dt);

   if (pModel->IsPlayer())
      UpdateModelRotDynamics(pModel, pModel->GetDynamics(PLAYER_HEAD), friction, dt);

   // Apply constriants
   mx_copy_vec(&velocity, &pModel->GetVelocity());
   pModel->ApplyConstraints(&velocity);
   pModel->GetDynamics()->SetVelocity(velocity);

#if 0
   if (pModel->IsPlayer())
   {
      mprintf("vel = %g %g %g\n", velocity.x, velocity.y, velocity.z);
      mprintf("pos = %g %g %g\n\n", pModel->GetLocationVec(PLAYER_HEAD).x, pModel->GetLocationVec(PLAYER_HEAD).y, pModel->GetLocationVec(PLAYER_HEAD).z);
   }
#endif
}

///////////////////////////////////////
//
// Update all models' dynamics
//
#ifdef PROFILE_UD
DECLARE_TIMER(UD_BuildConstraints, Average);
DECLARE_TIMER(UD_MovingTerr, Average);
DECLARE_TIMER(UD_Ballistic, Average);
DECLARE_TIMER(UD_MovingTerrPost, Average);
DECLARE_TIMER(UD_UpdateContacts, Average);
DECLARE_TIMER(UD_UpdateModel, Average);

#define UD_TIMER_Start(x) TIMER_Start(x)
#define UD_TIMER_Stop(x)  TIMER_MarkStop(x)

#else

#define UD_TIMER_Start(x)
#define UD_TIMER_Stop(x)

#endif

static void UpdateDynamics(mxs_real dt)
{
   cPhysModel *pModel;
   cPhysModel *pNextModel;
   int  i;

   // Build constraints
   UD_TIMER_Start(UD_BuildConstraints);
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {

      pNextModel = g_PhysModels.GetNextMoving(pModel);
      pModel->ClearConstraints();
      pModel = pNextModel;
   }

   BOOL allow_to_sleep;
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);

      allow_to_sleep = TRUE;
      for (i=0; i<pModel->NumSubModels(); i++)
      {
         allow_to_sleep &= !ConstrainFromObjects(pModel, i);
         ConstrainFromTerrain(pModel, i);
      }
      pModel->SetAllowedToSleep(allow_to_sleep);

      pModel = pNextModel;
   }
   UD_TIMER_Stop(UD_BuildConstraints);

   // Update moving terrain
   UD_TIMER_Start(UD_MovingTerr);
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);

      if (pModel->IsMovingTerrain() || pModel->IsPressurePlate())
      {
         UpdateModelDynamics(pModel, dt);

         if (pModel->NumAttachments() > 0)
         {
            cAutoLinkQuery query(g_pPhysAttachRelation, LINKOBJ_WILDCARD, pModel->GetObjID());

            for (; !query->Done(); query->Next())
            {
               cPhysModel *pAttachModel = g_PhysModels.Get(query.GetSource());

               if (pAttachModel)
                  UpdateAttachment(pAttachModel, dt);
            }
         }
      }

      pModel = pNextModel;
   }
   UD_TIMER_Stop(UD_MovingTerr);

   // Update ballistic dynamics
   UD_TIMER_Start(UD_Ballistic);
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);

      if (!pModel->IsMovingTerrain() && !pModel->IsPressurePlate() && !pModel->IsAttached())
         UpdateModelDynamics(pModel, dt);

      pModel = pNextModel;
   }
   UD_TIMER_Stop(UD_Ballistic);

   // Update based on object contacts
   UD_TIMER_Start(UD_UpdateContacts);
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);
      UpdateObjectContacts(pModel, dt);
      pModel = pNextModel;
   }
   UD_TIMER_Stop(UD_UpdateContacts);

   UD_TIMER_Start(UD_MovingTerrPost);
   pModel = g_PhysModels.GetFirstMoving();
   // Advance to end of list
   while (pModel && g_PhysModels.GetNextMoving(pModel) != NULL)
      pModel = g_PhysModels.GetNextMoving(pModel);
   // And go through backward
   while (pModel != NULL)
   {
      if (!pModel->IsMovingTerrain())
         PostMovingTerrainUpdate(pModel);
      pModel = g_PhysModels.GetPreviousMoving(pModel);
   }
   UD_TIMER_Stop(UD_MovingTerrPost);

   gInsideMT = FALSE;

   // And do the internal updating
   UD_TIMER_Start(UD_UpdateModel);
   pModel = g_PhysModels.GetFirstMoving();
   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);

      if (!pModel->IsAttached())
      {
         pModel->UpdateModel(dt);
         UpdateMantling(pModel, dt);
      }

      // Check if we should put to sleep
      if (!pModel->IsPlayer() &&
          !pModel->IsAvatar() &&
          pModel->AllowedToSleep())
      {
         BOOL put_to_sleep;

         if ((pModel->IsVelocityControlled() && !IsZeroVector(pModel->GetControls()->GetControlVelocity()) && !pModel->IsCreature()) ||
             (pModel->IsRotationalVelocityControlled() && !IsZeroVector(pModel->GetControls()->GetControlRotationalVelocity()) && !pModel->IsCreature()))
            put_to_sleep = FALSE;
         else
         {
            put_to_sleep = IsZeroVector(pModel->GetVelocity()) &&
                           IsZeroVector(pModel->GetDynamics()->GetRotationalVelocity());

            for (i=0; i<pModel->NumSubModels() && put_to_sleep; i++)
            {
               put_to_sleep &= (IsZeroVector(pModel->GetVelocity(i)) &&
                                IsZeroVector(pModel->GetDynamics(i)->GetRotationalVelocity()));
            }
            put_to_sleep &= !pModel->IsMovingTerrain() && !pModel->IsAttached() && pModel->AtRest();
         }

         pModel->SetSleep(put_to_sleep);
      }

      pModel = pNextModel;
   }
   UD_TIMER_Stop(UD_UpdateModel);

   #ifndef SHIP
   if (config_is_defined("phys_num_awake"))
   {
      int num_awake = 0;
      int num_creatures = 0;
      int num_ballistic = 0;

      pModel = g_PhysModels.GetFirstMoving();
      while (pModel)
      {
         if (pModel->IsCreature())
         {
            num_creatures++;
            if (!CreatureSelfPropelled(pModel->GetObjID()))
               num_ballistic++;
         }
         num_awake++;

         pModel = g_PhysModels.GetNextMoving(pModel);
      }
      mprintf("num awake = %d\n", num_awake);
      mprintf("num creatures = %d\n", num_creatures);
      mprintf("num ballistic = %d\n", num_ballistic);
      mprintf("\n");
   }
   #endif
}

////////////////////////////////////////////////////////////////////////////////

BOOL PressurePlateCallback(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   Assert_(pModel);
   Assert_(pModel->IsPressurePlate());
   Assert_(pModel->GetType(0) == kPMT_OBB);

   PhysPPlateListenerLock = TRUE;

   switch (pModel->GetPPlateState())
   {
      case kPPS_Activating:
      {
         pModel->GetControls()->StopControlVelocity();
         pModel->GetControls()->ControlLocation(pModel->GetLocationVec());

         pModel->SetPPlateState(kPPS_Active);

         PressurePlateMessage(objID, kPPS_Active);

         break;
      }

      case kPPS_Deactivating:
      {
         pModel->GetControls()->StopControlVelocity();
         pModel->GetControls()->ControlLocation(pModel->GetLocationVec());

         pModel->SetPPlateState(kPPS_Inactive);

         PressurePlateMessage(objID, kPPS_Inactive);

         break;
      }
   }

   PhysPPlateListenerLock = FALSE;

   return TRUE;
}

////////////////////////////////////////

static mxs_real FindRestingMass(cPhysModel *pModel)
{
   mxs_real accum_mass = 0;

   int submod = -1;

   switch (pModel->GetType(0))
   {
      case kPMT_Sphere:
      case kPMT_Point:
         return 0.0;

      case kPMT_SphereHat:
         submod = 1;
         break;

      case kPMT_OBB:
      {
         // Find closest "up" face
         mxs_vector normals[6];
         mxs_vector best_normal;
         int best_submod;

         ((cPhysOBBModel *)pModel)->GetNormals(normals);

         for (int i=0; i<6; i++)
         {
            if ((i == 0) || (normals[i].z > best_normal.z))
            {
               mx_copy_vec(&best_normal, &normals[i]);
               best_submod = i;
            }
         }

         submod = best_submod;
      }
   }

   Assert_(submod != -1);

   cPhysSubModelInst *pObjContact;

   if (GetObjectContacts(pModel->GetObjID(), submod, pModel, &pObjContact))
   {
      while (pObjContact)
      {
         accum_mass += pObjContact->GetPhysicsModel()->GetDynamics()->GetMass();
         accum_mass += FindRestingMass(pObjContact->GetPhysicsModel());

         pObjContact = pObjContact->GetNext();
      }
   }

   return accum_mass;
}

static void UpdatePressurePlate(cPhysModel *pModel, mxs_real dt)
{
   Assert_(pModel->IsPressurePlate());
   Assert_(pModel->GetType(0) == kPMT_OBB);

   PhysPPlateListenerLock = TRUE;

   switch (pModel->GetPPlateState())
   {
      case kPPS_Inactive:
      {
         // See if the weight tolerance has been exceeded
         if (FindRestingMass(pModel) >= pModel->GetPPlateActivationWeight())
         {
            mxs_vector limit;
            mxs_vector velocity;
            mxs_matrix orien;

            mx_ang2mat(&orien, &pModel->GetRotation());
            mx_scale_vec(&limit, &orien.vec[2], -pModel->GetPPlateTravel());
            mx_addeq_vec(&limit, &pModel->GetLocationVec());

            pModel->GetControls()->StopControlLocation();
            mx_scale_vec(&velocity, &orien.vec[2], -pModel->GetPPlateSpeed());
            pModel->GetControls()->SetControlVelocity(velocity);
            pModel->ClearTransLimits();
            pModel->AddTransLimit(limit, PressurePlateCallback);

            UpdatePhysProperty(pModel->GetObjID(), PHYS_ATTR | PHYS_CONTROL);

            pModel->SetPPlateCurPause(pModel->GetPPlatePause());
            pModel->SetPPlateState(kPPS_Activating);

            PressurePlateMessage(pModel->GetObjID(), kPPS_Activating);
         }

         break;
      }

      case kPPS_Active:
      {
         BOOL deactivate = FALSE;

         if (pModel->GetPPlatePause() == -1.0)
         {
            mxs_real accum_mass = 0;

            // See if the weight tolerance has been exceeded
            if (FindRestingMass(pModel) < pModel->GetPPlateActivationWeight())
               deactivate = TRUE;
         }
         else
         if (pModel->GetPPlatePause() != -2.0)
         {
            mxs_real delay;

            delay = pModel->GetPPlateCurPause() - dt;
            pModel->SetPPlateCurPause(delay);
            if (delay < 0)
               deactivate = TRUE;
         }

         if (deactivate)
         {
            mxs_vector limit;
            mxs_vector velocity;
            mxs_matrix orien;

            mx_ang2mat(&orien, &pModel->GetRotation());
            mx_scale_vec(&limit, &orien.vec[2], pModel->GetPPlateTravel());
            mx_addeq_vec(&limit, &pModel->GetLocationVec());

            pModel->GetControls()->StopControlLocation();
            mx_scale_vec(&velocity, &orien.vec[2], pModel->GetPPlateSpeed());
            pModel->GetControls()->SetControlVelocity(velocity);
            pModel->ClearTransLimits();
            pModel->AddTransLimit(limit, PressurePlateCallback);

            UpdatePhysProperty(pModel->GetObjID(), PHYS_ATTR | PHYS_CONTROL);

            pModel->SetPPlateState(kPPS_Deactivating);

            PressurePlateMessage(pModel->GetObjID(), kPPS_Deactivating);
         }

         break;
      }
   }

   PhysPPlateListenerLock = FALSE;
}

////////////////////////////////////////////////////////////////////////////////

static void UpdateModelControls(cPhysModel * pModel, mxs_real dt)
{
   cPhysCtrlData *pCtrl;
   int type;

   pCtrl = pModel->GetControls();
   type = pCtrl->GetType();

   if ((pModel->GetMantlingState() > 0) && (pModel->GetMantlingState() <= 5))
      return;

   if (pModel->IsRotationalVelocityControlled())
      pCtrl->ControlRotationalVelocity();

   if (pModel->IsVelocityControlled())
      pCtrl->ControlVelocity();

   if (pModel->IsPlayer() && (pModel->GetControls(PLAYER_HEAD)->GetType() & kPCT_RotVelControl))
   {
      pCtrl = pModel->GetControls(PLAYER_HEAD);
      pCtrl->ControlRotationalVelocity();
   }

   if (pModel->IsPressurePlate())
      UpdatePressurePlate(pModel, dt);
}

///////////////////////////////////////
//
// Update all models' controls
//

static void UpdateControls(mxs_real dt)
{
   cPhysModel *pModel = g_PhysModels.GetFirstMoving();
   cPhysModel *pNextModel;

   gInsideMT = TRUE;

   while (pModel != NULL)
   {
      pNextModel = g_PhysModels.GetNextMoving(pModel);
      PreMovingTerrainUpdate(pModel);
      UpdateModelControls(pModel, dt);
      pModel = pNextModel;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Stop, inform controller (if any), recalculate collisions for objs in ref list
// set the model state to frozen (it won't move for the rest of the frame unless collided with)
//

void PhysEmergencyStop(cPhysModel * pModel, tPhysSubModId subModId, mxs_real dt)
{
   if (CreatureSelfPropelled(pModel->GetObjID()))
      return;

   cPhysDynData *pDynamics = pModel->GetDynamics();
   int i;

   if (pModel->GetSpringTension(subModId) > 0.0)
   {
      pDynamics = pModel->GetDynamics(subModId);

      pDynamics->Stop();

      pModel->SetEndLocationVec(subModId, pModel->GetLocationVec(subModId));
   }
   else
   {
      pDynamics->Stop();
      pDynamics->SetFrozen(TRUE);

      pModel->SetEndLocationVec(pModel->GetLocationVec());

      for (i=0; i<pModel->NumSubModels(); i++)
      {
         pDynamics = pModel->GetDynamics(i);

         pDynamics->Stop();

         pModel->SetEndLocationVec(i, pModel->GetLocationVec(i));
      }
   }
}

///////////////////////////////////////
//
// Check terrain collisions for all sub-models of a model
//

static void CheckModelTerrainCollisions(cPhysModel * pModel, mxs_real t0, mxs_real dt)
{
   int i;
   cPhysClsn *pClsn;

   if (pModel->IsRemote() || pModel->IsSleeping() || !pModel->IsTranslating())
      return;
   for (i = 0; i < pModel->NumSubModels(); i++)
   {
      if (pModel->CheckTerrainCollision(i, t0, dt, &pClsn) && (pClsn != NULL))
      {
          g_PhysClsns.Insert(pClsn);
      }
   }
}

///////////////////////////////////////
//
// Check terrain collisions for all models
//

static void CheckTerrainCollisions(mxs_real t0, mxs_real dt)
{
   cPhysModel *pModel = g_PhysModels.GetFirstMoving();

   // iterate over all moveable models
   while (pModel != NULL)
   {
      if (g_PhysicsOn || !pModel->IsPlayer())
         CheckModelTerrainCollisions(pModel, t0, dt);
      pModel = g_PhysModels.GetNextMoving(pModel);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Collision detection for models
//
// Each function deals with a pair of model types
// Returns TRUE if a collision is detected and sets the collision pointer
// to a new collision structure
//
// @Note (toml 08-05-97): Although this is an abstraction violation, it is
//                        clean and fast.
//

struct sClsnCheckParms
{
   union
   {
      cPhysModel *       pModel1;
      cPhysSphereModel * pSphereModel1;
      cPhysBSPModel *    pBSPModel1;
      cPhysOBBModel *    pOBBModel1;
   };

   tPhysSubModId subModel1;

   union
   {
      cPhysModel *       pModel2;
      cPhysSphereModel * pSphereModel2;
      cPhysBSPModel *    pBSPModel2;
      cPhysOBBModel *    pOBBModel2;
   };
   tPhysSubModId  subModel2;

   mxs_real t0;
   mxs_real dt;
};

///////////////////////////////////////
//
// A collision checker
//
typedef BOOL (*tCheckModelsCollisionFunc)(sClsnCheckParms *pParms, cPhysClsn **ppClsn);

///////////////////////////////////////
//
// Sphere vs sphere
//

static BOOL CheckSphereVsSphereHat(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysSphereModel *pModel1 = pParms->pSphereModel1;
   cPhysSphereModel *pModel2 = pParms->pSphereModel2;

   ObjID objID1 = pModel1->GetObjID();
   ObjID objID2 = pModel2->GetObjID();

   tPhysSubModId submod1 = pParms->subModel1;
   tPhysSubModId submod2 = pParms->subModel2;

   ePhysModelType type1 = pModel1->GetType(submod1);
   ePhysModelType type2 = pModel2->GetType(submod2);

   cPhysSphereModel *pModel;
   cPhysSphereModel *pModelOther;
   tPhysSubModId submod;

   if ((type1 == kPMT_SphereHat) && (submod1 == 1))
   {
      if ((type2 == kPMT_SphereHat) && (submod2 == 1))
      {
        *ppClsn = NULL;
        return FALSE;
      }

      Assert_((type2 == kPMT_Sphere) || (type2 == kPMT_Point) || (type2 == kPMT_SphereHat));

      pModel = pModel2;
      submod = submod2;
      pModelOther = pModel1;
   }
   else
   if ((type2 == kPMT_SphereHat) && (submod2 == 1))
   {
      if ((type1 == kPMT_SphereHat) && (submod1 == 1) && (submod2 == 1))
      {
        *ppClsn = NULL;
        return FALSE;
      }

      Assert_((type1 == kPMT_Sphere) || (type1 == kPMT_Point) || (type1 == kPMT_SphereHat));

      pModel = pModel1;
      submod = submod1;
      pModelOther = pModel2;
   }
   else
   {
      CriticalMsg("SphereHat collision check with no sphere hats?\n");
      *ppClsn = NULL;
      return FALSE;
   }


   // Ghost only cares about its first submodel
   if (pModel->IsFancyGhost() && (submod1 != 0))
   {
      *ppClsn = NULL;
      return FALSE;;
   }

   // Adjust radius to be zero, if ghost
   mxs_real radius;
   mxs_real radius_other = pModelOther->GetRadius(0);

   if (pModel->IsFancyGhost() && (pModel->GetRadius(0) > 1.0) && !pModelOther->IsProjectile())
      radius = 1.0;
   else
      radius = pModel->GetRadius(0);

   // Do early-out check against radius of spheres
   mxs_real model_dist = mx_dist2_vec(&pModel->GetEndLocationVec(submod), &pModelOther->GetEndLocationVec());
   if (model_dist > ((5 * radius_other * radius_other) + (radius * radius)))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   if (pModelOther->IsPointVsNotSpecial() && !pModel->IsSpecial())
      radius_other = 0.0;

   cFaceContact faceContact(pModelOther, radius_other);
   cFaceContact faceEndContact(pModelOther, radius_other, TRUE);

   if ((faceContact.GetPoly() == NULL) || (faceEndContact.GetPoly() == NULL))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   mxs_real start_dist, end_dist;
   mxs_real face_start_dist, face_end_dist;
   mxs_real time, face_time;
   int      best_side = -1;

   face_start_dist = mx_dot_vec(&faceContact.GetNormal(), &pModel->GetLocationVec(submod)) -
                                 (faceContact.GetPlaneConst() + pModel->GetRadius(submod));
   face_end_dist   = mx_dot_vec(&faceEndContact.GetNormal(), &pModel->GetEndLocationVec(submod)) -
                                 (faceEndContact.GetPlaneConst() + pModel->GetRadius(submod));

   if (((face_start_dist * face_end_dist) > 0) ||
       ((face_start_dist < 0) && (face_end_dist > 0)))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   // Find time along line where intersection happens
   face_time = face_start_dist / (face_start_dist - face_end_dist);

   const cFacePoly *pPoly    = faceContact.GetPoly();
   const cFacePoly *pEndPoly = faceEndContact.GetPoly();
   BOOL valid_hit = TRUE;

   for (int i=0; (i<4) && valid_hit; i++)
   {
      //mxs_vector norm = pPoly->edgePlaneList[i]->normal;
      //mxs_vector end_norm = pEndPoly->edgePlaneList[i]->normal;
      mxs_real   d = pPoly->edgePlaneList[i]->d;
      mxs_real   end_d = pEndPoly->edgePlaneList[i]->d;


      start_dist = mx_dot_vec(&pPoly->edgePlaneList[i]->normal, &pModel->GetLocationVec(submod)) -
                              (pPoly->edgePlaneList[i]->d + pModel->GetRadius(submod));
      end_dist   = mx_dot_vec(&pEndPoly->edgePlaneList[i]->normal, &pModel->GetEndLocationVec(submod)) -
                              (pEndPoly->edgePlaneList[i]->d + pModel->GetRadius(submod));


      if ((start_dist <= 0.0001) && (end_dist <= 0.0001))
         continue;

      if ((start_dist >= -0.0001) && (end_dist > -0.0001))
         valid_hit = FALSE;

      time = start_dist / (start_dist - end_dist);

      // inside-out
      if ((start_dist < 0) && (end_dist > 0))
      {
         if (time < face_time)
            valid_hit = FALSE;
      }
      else
      {
         if (time > face_time)
            valid_hit = FALSE;
      }
   }

   if (valid_hit)
   {
      // Create collision
      *ppClsn = new cPhysClsn(kPC_SphereHatObject,
                              pParms->t0, (pParms->dt * face_time),
                              pModelOther, 1,
                              pModelOther->GetLocationVec(),
                              pModel, submod,
                              pModel->GetLocationVec(submod));
      return TRUE;
   }
   else
   {
      *ppClsn = NULL;
      return FALSE;
   }
}

static BOOL CheckSphereVsSphere(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   ePhysModelType type1 = pParms->pModel1->GetType(pParms->subModel1);
   ePhysModelType type2 = pParms->pModel2->GetType(pParms->subModel2);

   AssertMsg((type1 == kPMT_Sphere || type1 == kPMT_Point || type1 == kPMT_SphereHat) &&
             (type2 == kPMT_Sphere || type2 == kPMT_Point || type2 == kPMT_SphereHat),
             "Invalid types to CheckSphereVsSphere");

   cPhysSphereModel * pModel1 = pParms->pSphereModel1;
   cPhysSphereModel * pModel2 = pParms->pSphereModel2;

   ObjID objID1 = pModel1->GetObjID();
   ObjID objID2 = pModel2->GetObjID();

   mxs_vector     Vel1;
   mxs_vector     Vel2;

   mxs_vector     relativeMovement;
   mxs_vector     relativePosition;
   mxs_vector     relativeVelocity;
   mxs_real       time;
   mxs_vector *   pLoc1 = (mxs_vector *) & pModel1->GetLocationVec(pParms->subModel1);
   mxs_vector *   pLoc2 = (mxs_vector *) & pModel2->GetLocationVec(pParms->subModel2);
   mxs_vector *   pEnd1 = (mxs_vector *) & pModel1->GetEndLocationVec(pParms->subModel1);
   mxs_vector *   pEnd2 = (mxs_vector *) & pModel2->GetEndLocationVec(pParms->subModel2);
   mxs_vector     movement1, movement2;
   mxs_vector     colLoc1,
                  colLoc2;
   ePhysClsnType  type;
   mxs_real       radius1, radius2;
   mxs_real       sumRadii;

   if (pModel1->IsPointVsNotSpecial() && !pModel2->IsSpecial() &&
       pModel2->IsPointVsNotSpecial() && !pModel1->IsSpecial())
   {
      *ppClsn = NULL;
      return FALSE;
   }

   // Should we be doing sphere-hat checking?
   if ((type1 == kPMT_SphereHat) && (pParms->subModel1 == 1))
      return CheckSphereVsSphereHat(pParms, ppClsn);
   if ((type2 == kPMT_SphereHat) && (pParms->subModel2 == 1))
      return CheckSphereVsSphereHat(pParms, ppClsn);

   // Ghosts only care about submodel 0
   if ((pModel1->IsFancyGhost() && (pParms->subModel1 != 0) &&
        !(pModel2->IsProjectile() || pModel2->IsPlayer() || pModel2->IsWeapon(pParms->subModel2))) ||
       (pModel2->IsFancyGhost() && (pParms->subModel2 != 0) &&
        !(pModel1->IsProjectile() || pModel1->IsPlayer() || pModel1->IsWeapon(pParms->subModel1))))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   // Size the ghost models properly
   if (pModel1->IsFancyGhost() && (pModel1->GetRadius(pParms->subModel1) > 1.0) &&
       !(pModel2->IsProjectile() || pModel2->IsPlayer()))
      radius1 = 1.0;
   else
      radius1 = pModel1->GetRadius(pParms->subModel1);

   if (pModel2->IsFancyGhost() && (pModel2->GetRadius(pParms->subModel2) > 1.0) &&
       !(pModel1->IsProjectile() || pModel1->IsPlayer()))
      radius2 = 1.0;
   else
      radius2 = pModel2->GetRadius(pParms->subModel2);

   // Do point-vs-not-special stuff
   if (pModel1->IsPointVsNotSpecial() && !pModel2->IsSpecial())
      sumRadii = radius2;
   else
   if (pModel2->IsPointVsNotSpecial() && !pModel1->IsSpecial())
      sumRadii = radius1;
   else
      sumRadii = radius1 + radius2;

   mx_sub_vec(&movement1, pEnd1, pLoc1);
   mx_sub_vec(&movement2, pEnd2, pLoc2);

   mx_scale_vec(&Vel1, &movement1, 1 / pParms->dt);
   mx_scale_vec(&Vel2, &movement2, 1 / pParms->dt);

   mx_sub_vec(&relativeVelocity, &Vel1, &Vel2);
   mx_sub_vec(&relativeMovement, &movement1, &movement2);
   mx_sub_vec(&relativePosition, pLoc1, pLoc2);

   const BOOL fSolvePoint = PhysSolveLnPtDistVec(pLoc1,
                                                 &relativeMovement,
                                                 pLoc2,
                                                 sumRadii,
                                                 pParms->dt,
                                                 &time);

   if (fSolvePoint && (time <= pParms->dt) && (time > 0.0) &&
       (mx_dot_vec(&relativeVelocity, &relativePosition) <= -0.01))
   {
      mx_scale_add_vec(&colLoc1, pLoc1, &Vel1, time);
      if (pModel2->IsMoveable())
      {
         type = kPC_MoveableObject;
         // estimate collision location
         mx_scale_add_vec(&colLoc2, pLoc2, &Vel2, time);
      }
      else
          type = kPC_StationaryObject;

      // @TODO: These should be installed callbacks, and should be
      //        modified to use the physics system's collision result
      //        constants.
      //
      // Determine collision result
      mxs_vector momentum1, momentum2;
      mxs_vector rel_momentum;

      mx_scale_vec(&momentum1, &Vel1, pModel1->GetDynamics()->GetMass());
      mx_scale_vec(&momentum2, &Vel2, pModel2->GetDynamics()->GetMass());

      mx_sub_vec(&rel_momentum, &momentum1, &momentum2);

      // Create collision
      *ppClsn = new cPhysClsn(type, pParms->t0, time,
                              pModel1, pParms->subModel1, *pLoc1,
                              pModel2, pParms->subModel2, *pLoc2);
         return TRUE;
   }
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////

static BOOL CheckSphereVsBSP(sClsnCheckParms * /*pParms*/, cPhysClsn **ppClsn)
{
   Warning(("Don't know how to check for sphere vs BSP model collisions\n"));
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////


#define kSphereVsOBBEpsilon .001

static BOOL CheckSphereVsOBB(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysSphereModel *pSphereModel = pParms->pSphereModel1;
   cPhysOBBModel    *pOBBModel = pParms->pOBBModel2;

   ObjID objID1 = pSphereModel->GetObjID();
   ObjID objID2 = pOBBModel->GetObjID();

   int submodel = pParms->subModel1;

   if ((pSphereModel->GetType(0) == kPMT_Point) &&
       (pSphereModel->IsPlayer() || pSphereModel->IsAvatar()) &&
       (pOBBModel->GetClimbableSides() > 0))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   if ((submodel == 1) && (pSphereModel->GetType(submodel) == kPMT_SphereHat))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   if (((pSphereModel->GetRadius(submodel)) <= pOBBModel->GetPoreSize()) &&
       (pOBBModel->GetPoreSize()>0) &&
       !(pSphereModel->IsPlayer() || pSphereModel->IsAvatar())) //so you can't get your toes stuck in it
     {
#ifdef DBG
       ConfigSpew("PhysicsPoreSpew",("%d(%g) going through pore on %d(%g)\n",objID1,pSphereModel->GetRadius(submodel),objID2,pOBBModel->GetPoreSize()));
#endif
       *ppClsn = NULL;
       return FALSE;
     }

   mxs_real   radius = (pSphereModel->GetRadius(submodel)) + kSphereVsOBBEpsilon;
   mxs_real   time;
   int        best_side = -1;
   int        worst_side = -1;
   BOOL       outside_fail = FALSE;
   mxs_real   start_dist, end_dist;
   mxs_real   max_entry_time = -1000000.0;
   mxs_real   min_exit_time = 1000000.0;
   mxs_vector start_sphere = pSphereModel->GetLocationVec(submodel);
   mxs_vector end_sphere = pSphereModel->GetEndLocationVec(submodel);
   int i;

   // Useful for AI's pathing onto objects. TRUE if is sphere model that
   // supports this, and BBox model supports it.
   // Practically speaking, the AI physics spheres will be enabled for this,
   // and any object that is pathable.
   BOOL bDoDiskCheck = pSphereModel->DoDiskCheck() && pOBBModel->DoDiskCheck();


   static mxs_vector start_norm_list[6];
   static mxs_vector end_norm_list[6];
   static mxs_real   start_d_list[6];
   static mxs_real   end_d_list[6];

   // Ghosts only care about first submodel
   if (pSphereModel->IsFancyGhost())
   {
      if (submodel != 0)
      {
         *ppClsn = NULL;
         return FALSE;
      }
      else
      if (radius > 1.0)
         radius = 1.0;
   }
   // Adjust for sphere hat
   if (pSphereModel->IsPointVsNotSpecial() && !pOBBModel->IsSpecial())
      radius = 0.0;

   pOBBModel->GetNormals(start_norm_list, end_norm_list);
   pOBBModel->GetConstants(start_d_list, start_norm_list, end_d_list, end_norm_list);

   mxs_real   best_start, best_end;

   mxs_real   inside_epsilon = pOBBModel->IsEdgeTrigger() ? 0.01 : 0.0;

   for (i = 0; i < 6; ++i)
   {
      if (!bDoDiskCheck || (start_norm_list[i].z < 0.996) || (end_norm_list[i].z < 0.996)) // 0.996 is about cos(5) degrees.
      {
         start_d_list[i] += radius;
         end_d_list[i] += radius;
      }
      start_dist = mx_dot_vec(&start_norm_list[i], &start_sphere) - start_d_list[i];
      end_dist = mx_dot_vec(&end_norm_list[i], &end_sphere) - end_d_list[i];

      // We can ignore this face if we're completely inside it.
      if ((start_dist <= inside_epsilon) && (end_dist <= inside_epsilon))
         continue;

      // We can ignore the whole OBB if we're outside any one side.
      if ((start_dist > -inside_epsilon) && (end_dist > -inside_epsilon))
      {
         outside_fail = TRUE;
         break;
      }

      // Find time along line where intersection happens
      time = start_dist / (start_dist - end_dist);

      // Are we entering the OBB at this face, or exiting it?
      if (start_dist < 0.0)
      {
         if (time < min_exit_time)
         {
            min_exit_time = time;
            worst_side = i;
         }
      }
      else
      {
         if (time > max_entry_time)
         {
            max_entry_time = time;
            best_side = i;
            best_start = start_dist;
            best_end = end_dist;
         }
      }
   }

   if (outside_fail)
   {
      *ppClsn = NULL;
      return FALSE;
   }

   mxs_vector momentum1, momentum2;
   mxs_vector rel_momentum;

   mx_scale_vec(&momentum1, &pSphereModel->GetVelocity(submodel), pSphereModel->GetDynamics()->GetMass());
   mx_scale_vec(&momentum2, &pOBBModel->GetVelocity(), pOBBModel->GetDynamics()->GetMass());

   mx_sub_vec(&rel_momentum, &momentum1, &momentum2);

#if 0
   if (best_side != -1 || worst_side != -1)
   {
      mprintf("  transition\n");
      mprintf("  start = %g %g %g\n", start_sphere.x, start_sphere.y, start_sphere.z);
      mprintf("  end = %g %g %g\n", end_sphere.x, end_sphere.y, end_sphere.z);

      mprintf("  best_start = %g\n", best_start);
      mprintf("  best_end = %g\n", best_end);
      mprintf("  max_entry_time = %g\n", max_entry_time);
      mprintf("  dt = %g\n", pParms->dt);
      mprintf("  vel = %g %g %g\n", pSphereModel->GetVelocity(submodel).x, pSphereModel->GetVelocity(submodel).y, pSphereModel->GetVelocity(submodel).z);
   }
#endif

   mxs_vector coll_loc;

   // check for entry
   if ((best_side != -1) && (max_entry_time < 1.0) && (min_exit_time > max_entry_time))
   {
      mx_scale_add_vec(&coll_loc, &pSphereModel->GetLocationVec(submodel),
                                  &pSphereModel->GetVelocity(submodel),
                                   pParms->dt * max_entry_time);

      *ppClsn = new cPhysClsn(kPC_OBBObject,
                              pParms->t0, (pParms->dt * max_entry_time),
                              pSphereModel, submodel,
                              coll_loc,
                              pOBBModel, best_side,
                              pOBBModel->GetLocationVec());
      return TRUE;
   }
   else
   // check for exit
   if ((worst_side != -1) && (best_side == -1) && pOBBModel->IsEdgeTrigger())
   {
      mx_scale_add_vec(&coll_loc, &pSphereModel->GetLocationVec(submodel),
                                  &pSphereModel->GetVelocity(submodel),
                                   pParms->dt * min_exit_time);

      *ppClsn = new cPhysClsn(kPC_OBBObject,
                              pParms->t0, (pParms->dt * min_exit_time),
                              pSphereModel, submodel,
                              coll_loc,
                              pOBBModel, -(worst_side + 1),
                              pOBBModel->GetLocationVec());
      return TRUE;
   }
   else
   // no transition
   {
      *ppClsn = NULL;
      return FALSE;
   }
}


///////////////////////////////////////

static BOOL CheckBSPVsSphere(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysModel *temp;
   tPhysSubModId smtemp;

   temp = pParms->pModel1;
   pParms->pModel1 = pParms->pModel2;
   pParms->pModel2 = temp;

   smtemp = pParms->subModel1;
   pParms->subModel1 = pParms->subModel2;
   pParms->subModel2 = smtemp;

   return CheckSphereVsBSP(pParms, ppClsn);
}

///////////////////////////////////////

static BOOL CheckBSPVsBSP(sClsnCheckParms * /*pParms*/, cPhysClsn **ppClsn)
{
   Warning(("Don't know how to check for BSP vs BSP model collisions\n"));
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////

static BOOL CheckBSPVsPoint(sClsnCheckParms * /*pParms*/, cPhysClsn **ppClsn)
{
   Warning(("Don't know how to check for BSP vs Point model collisions\n"));
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////

static BOOL CheckBSPVsOBB(sClsnCheckParms * /*pParms*/, cPhysClsn **ppClsn)
{
   Warning(("Don't know how to check for BSP vs OBB model collisions\n"));
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////

static BOOL CheckPointVsBSP(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysModel *temp;
   tPhysSubModId smtemp;

   temp = pParms->pModel1;
   pParms->pModel1 = pParms->pModel2;
   pParms->pModel2 = temp;

   smtemp = pParms->subModel1;
   pParms->subModel1 = pParms->subModel2;
   pParms->subModel2 = smtemp;

   return CheckBSPVsPoint(pParms, ppClsn);
}

///////////////////////////////////////

static BOOL CheckPointVsPoint(sClsnCheckParms *, cPhysClsn **ppClsn)
{
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////

static BOOL CheckOBBVsSphere(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysModel *temp;
   tPhysSubModId smtemp;

   temp = pParms->pModel1;
   pParms->pModel1 = pParms->pModel2;
   pParms->pModel2 = temp;

   smtemp = pParms->subModel1;
   pParms->subModel1 = pParms->subModel2;
   pParms->subModel2 = smtemp;

   return CheckSphereVsOBB(pParms, ppClsn);
}

///////////////////////////////////////

static BOOL CheckOBBVsBSP(sClsnCheckParms *pParms, cPhysClsn **ppClsn)
{
   cPhysModel *temp;
   tPhysSubModId smtemp;

   temp = pParms->pModel1;
   pParms->pModel1 = pParms->pModel2;
   pParms->pModel2 = temp;

   smtemp = pParms->subModel1;
   pParms->subModel1 = pParms->subModel2;
   pParms->subModel2 = smtemp;

   return CheckBSPVsOBB(pParms, ppClsn);
}

///////////////////////////////////////

static BOOL CheckOBBVsOBB(sClsnCheckParms * /*pParms*/, cPhysClsn **ppClsn)
{
   *ppClsn = NULL;
   return FALSE;
}

///////////////////////////////////////
//
// Table of collision checkers
//

static tCheckModelsCollisionFunc g_CheckModelsCollisionFns[kPMT_NumTypes][kPMT_NumTypes] =
{
   {
      CheckSphereVsSphere,
      CheckSphereVsBSP,
      CheckSphereVsSphere,        // A point is just a sphere with radius 0
      CheckSphereVsOBB,
      CheckSphereVsSphere         // A sphere-hat is just a special-case sphere
   },
   {
      CheckBSPVsSphere,
      CheckBSPVsBSP,
      CheckBSPVsPoint,
      CheckBSPVsOBB,
      CheckBSPVsSphere
   },
   {
      CheckSphereVsSphere,
      CheckPointVsBSP,
      CheckPointVsPoint,
      CheckSphereVsOBB,
      CheckSphereVsSphere
   },
   {
      CheckOBBVsSphere,
      CheckOBBVsBSP,
      CheckOBBVsSphere,
      CheckOBBVsOBB,
      CheckOBBVsSphere
   },
   {
      CheckSphereVsSphere,
      CheckSphereVsBSP,
      CheckSphereVsSphere,
      CheckSphereVsOBB,
      CheckSphereVsSphere
   }
};

///////////////////////////////////////
//
// Check for collisions involving a single sub-model
//

void CheckSubModelCollisions(cPhysModel * pModel1, tPhysSubModId subModel1, cPhysModel * PotentialModels[], int numPotentialModels, mxs_real t0, mxs_real dt)
{
   #define CheckModelVsModel(pModel1, pModel2, pParms, ppClsn) \
      ((*(g_CheckModelsCollisionFns[type1][type2]))((pParms), (ppClsn)))

   sClsnCheckParms checkData;
   ePhysModelType  type1;
   ObjID           objID1;

   objID1 = pModel1->GetObjID();
   type1  = pModel1->GetType(subModel1);

   // get objects we're in contact with
   cPhysSubModelInst *pObjContacts1;
   cPhysSubModelInst *pObjContacts2;

   GetObjectContacts(pModel1->GetObjID(), subModel1, pModel1, &pObjContacts1);

   // Set up constant data
   checkData.t0 = t0;
   checkData.dt = dt;

   cPhysModel    *pModel2;
   ObjID          objID2;
   int            subModel2;
   ePhysModelType type2;

   cPhysClsn         *pClsn;
   cPhysSubModelInst *pIgnore;

   // Iterate over all potentially-colliding models
   while (numPotentialModels--)
   {
      pModel2 = PotentialModels[numPotentialModels];
      objID2 = pModel2->GetObjID();
      type2 = pModel2->GetType(0);

      int numSubmodels;

      // Handle OBBs
      if (type2 == kPMT_OBB)
         numSubmodels = 1;
      else
         numSubmodels = pModel2->NumSubModels();

      for (subModel2 = 0; subModel2 < numSubmodels; subModel2++)
      {
         // Submodel contact filtering
         if (pObjContacts1)
         {
            if (type1 == kPMT_OBB)
            {
               if (pObjContacts1->Find(pModel2->GetObjID(), -1, &pIgnore))
                  continue;
            }
            else
            if (type2 == kPMT_OBB)
            {
               if (!pModel1->IsPlayer())
               {
                  BOOL found_contact = FALSE;

                  for (int i=0; i<pModel2->NumSubModels(); i++)
                  {
                     GetObjectContacts(pModel2->GetObjID(), i, pModel2, &pObjContacts2);

                     if (pObjContacts2 && pObjContacts2->Find(pModel1->GetObjID(), -1, &pIgnore))
                     {
                        found_contact = TRUE;
                        break;
                     }
                  }

                  if (found_contact)
                     continue;
               }
            }
            else
            {
               if (pObjContacts1->Find(pModel2->GetObjID(), subModel2, &pIgnore))
                  continue;
            }
         }

         // Weapon/halo filtering
         ObjID weaponID = OBJ_NULL;
         ObjID ownerID = OBJ_NULL;

         if (pModel1->IsWeapon(subModel1))
         {
            weaponID = objID1;
            ownerID = GetWeaponOwnerObjID(weaponID);
         }
         else
         if (pModel2->IsWeapon(subModel2))
         {
            weaponID = objID2;
            ownerID = GetWeaponOwnerObjID(weaponID);
         }

         // Are we doing and owner-vs-weapon collision?
         if (((objID1 == weaponID) && (objID2 == ownerID)) ||
             ((objID1 == ownerID) && (objID2 == weaponID)))
            continue;

         // Don't let AIs hit each other
         if (weaponID == objID1)
         {
            if (ObjIsAI(ownerID) && ObjIsAI(objID2) && (AITeamCompare(ownerID, objID2) != kAI_Opponents))
               continue;

#if 0
            if (pModel2->IsWeapon(subModel2) && ObjIsAI(GetWeaponOwnerObjID(objID2)))
               continue;
#endif
         }
         else
         if (weaponID == objID2)
         {
            if (ObjIsAI(ownerID) && ObjIsAI(objID1) && (AITeamCompare(ownerID, objID1) != kAI_Opponents))
               continue;

#if 0
            if (pModel1->IsWeapon(subModel1) && ObjIsAI(GetWeaponOwnerObjID(objID1)))
               continue;
#endif
         }

         // Only have halos hit against weapons
         if ((!pModel1->IsHalo(subModel1) && !pModel2->IsHalo(subModel2)) ||
             (pModel1->IsHalo(subModel1) && pModel2->IsWeapon(subModel2)) ||
             (pModel1->IsWeapon(subModel1) && pModel2->IsHalo(subModel2)))
         {
            checkData.pModel1 = pModel1;
            checkData.pModel2 = pModel2;
            checkData.subModel1 = subModel1;
            checkData.subModel2 = subModel2;

            // Call the collision checker appropriate for the two types
            if (CheckModelVsModel(pModel1, pModel2, &checkData, &pClsn))
            {
               // a collision happened
               cPhysDynData *pDynamics1 = pModel1->GetDynamics();
               cPhysDynData *pDynamics2 = pModel2->GetDynamics();

               #ifndef SHIP
               if ((weaponID != OBJ_NULL) && (config_is_defined("WeaponHitSpew")))
               {
                  mprintf("weaponID = %d\n", weaponID);
                  mprintf("ownerID = %d\n", ownerID);

                  mprintf("objID1 = %d\n", pModel1->GetObjID());
                  mprintf("objID2 = %d\n", pModel2->GetObjID());
                  mprintf("\n");
               }
               #endif

               // check collision counts
               if (pDynamics1->CollisionCount() > kMaxFrameCollisions)
               {
                  #ifndef SHIP
                  if (config_is_defined("warn_collision_count"))
                     Warning(("Too many collisions: stopping object %d(%d). (from object)\n",
                              pModel1->GetObjID(), subModel1));
                  #endif
                  PhysEmergencyStop(pModel1, subModel1, dt);
               }
               if (pDynamics2->CollisionCount() > kMaxFrameCollisions)
               {
                  #ifndef SHIP
                  if (config_is_defined("warn_collision_count"))
                     Warning(("Too many collisions: stopping object %d(%d). (from object)\n",
                              pModel2->GetObjID(), checkData.subModel2));
                  #endif
                  PhysEmergencyStop(pModel2, subModel2, dt);
               }
               if ((pDynamics1->CollisionCount() > kMaxFrameCollisions) ||
                   (pDynamics2->CollisionCount() > kMaxFrameCollisions))
               {
                  delete pClsn;
                  return;
               }

               // add to collision list
               g_PhysClsns.Insert(pClsn);
            }
         }
      }
   }
}

///////////////////////////////////////
//
// Check for collisions involving a model
//

#ifdef OBJ_COLL_TIMING
DECLARE_TIMER(ObjColl, Average);
DECLARE_TIMER(ObjColl2, Average);
#endif

// @HACK: this should really be in physapi, but I don't feel like doing that
// recompile right now.
static BOOL IsOnMovingTerrainHack(cPhysModel *pModel)
{
   if (pModel->IsFancyGhost() || pModel->IsPlayer())
   {
      int submod;

      if (pModel->IsFancyGhost())
         submod = 0;
      else
         submod = PLAYER_FOOT;

      cPhysSubModelInst *pSubModel = NULL;

      if (GetObjectContacts(pModel->GetObjID(), submod, pModel, &pSubModel))
      {
         while (pSubModel != NULL)
         {
            if (pSubModel->GetPhysicsModel()->IsMovingTerrain())
               return TRUE;

            pSubModel = pSubModel->GetNext();
         }
      }
   }

   return FALSE;
}


void CheckModelObjectCollisions(cPhysModel * pModel, mxs_real t0, mxs_real dt, BOOL checkTest, tPhysSubModId subModId)
{
#ifdef OBJ_COLL_TIMING
   AUTO_TIMER(ObjColl);
#endif

   ObjID       objID;
   ObjID       ourObjID;

   ObjRef *    pInitialRef, *pCurrentRef;
   ObjRefID    CurrentBinID;
   ObjRef *    pCurrentBin;
   ObjRefID    OurID;
   cPhysModel *pCurModel;
   int         nPotential = 0;
   int         coll_type;

   const int   kInitialHashSize = 32;
   BOOL        dummy;

   static cHashTable<ObjID, BOOL, cHashTableFunctions<ObjID> > objectsSeen;
   static cDynArray<cPhysModel *> potentialModels;

   if (pModel->ObjectPassThru())
      return;

   if (pModel->IsCreature() && CreatureSelfPropelled(pModel->GetObjID()) && (subModId < 0))
      return;

   ObjGetCollisionType(pModel->GetObjID(), &coll_type);
   if (!((coll_type & COLLISION_BOUNCE) || (coll_type & COLLISION_NORESULT)))
      return;

   LGALLOC_PUSH_CREDIT();
   if (potentialModels.Size() < g_PhysModels.NumModels())
      potentialModels.SetSize(g_PhysModels.NumModels());
   LGALLOC_POP_CREDIT();

   objectsSeen.Clear(kInitialHashSize);

#if 0
   ////////////////
   // Deleted by Porges, optimization of algorithm declared bad

   mxs_real max_dist_2;

   cPhysClsn *pClsn = g_PhysClsns.GetFirst();
   BOOL found_clsn = FALSE;

   while (pClsn)
   {
      if ((pClsn->GetObjID() == objID) ||
          ((pClsn->GetType() & kPC_Object) && (pClsn->GetObjID2() == objID)))
      {
         found_clsn = TRUE;
         break;
      }
      pClsn = pClsn->GetNext();
   }

   if (found_clsn)
      max_dist_2 = mx_dist2_vec(&pModel->GetLocationVec(), &pClsn->GetClsnPt());

   ////////////////
#endif

   ourObjID = pModel->GetObjID();

   // Grab RefID of the pmodel
   OurID = OBJ_FIRST_REF(ourObjID, PhysRefSystemID);

   if (OurID == 0)
   {
      mxs_vector zero;
      mx_zero_vec(&zero);
#ifndef SHIP
      if (!HasObjectWarned(ourObjID) && !PhysObjValidPos(ourObjID,&zero))
      {
         SetObjectWarned(ourObjID);
         mprintf("ERROR: Invalid position for unrefed %s\n", ObjWarnName(ourObjID));
      }
#endif
      return;
   }

   BOOL is_AI = ObjIsAI(ourObjID);
   BOOL us_on_mt = IsOnMovingTerrainHack(pModel);

   AssertMsg1(OurID != 0, "Bad object& for ObjID %d!  Fell out of world?", ourObjID);

   // Keep track of initial pointer because list is circular
   pInitialRef = pCurrentRef = OBJREFID_TO_PTR(OurID);

   do
   {
      // We need the FIRST object& in the bin because this list isn't circular
      CurrentBinID = *(mRefSystems[pCurrentRef->refsys].ref_list_func((void *) pCurrentRef->bin, FALSE));
      do
      {
         pCurrentBin = OBJREFID_TO_PTR(CurrentBinID);
         objID = pCurrentBin->obj;

         // Don't add ourselves to the list
         if (objID != ourObjID)
         {
            // Does it even have physics?
            if ((pCurModel = g_PhysModels.GetActive(objID)) != NULL)
            {
               // Has this object already done its collision checking this frame?
               if (!pCurModel->ObjectPassThru())
               {
#if 0
                  // Deleted by Porges, optimization of algorithm declared bad
                  // Have we already collided with something closer than we
                  // possible could with this?
                  if (found_clsn)
                  {
                     mxs_real obj_dist_2 = mx_dist2_vec(&pModel->GetLocationVec(), &pCurModel->GetLocationVec());
                     obj_dist_2 -= sq(pCurModel->GetSize());

                     if (obj_dist_2 > max_dist_2)
                        goto next_coll_obj;
                  }
#endif

                  // Does this object collide with AIs if we are an AI?
                  if ((ObjIsAI(objID) && !pModel->AICollides()) ||
                      (is_AI && !pCurModel->AICollides()))
                     goto next_coll_obj;

                  // Players and FancyGhosts on moving terrain don't collide
                  if (((pCurModel->IsPlayer() && pModel->IsFancyGhost()) ||
                       (pCurModel->IsFancyGhost() && pModel->IsPlayer())) &&
                      (us_on_mt || IsOnMovingTerrainHack(pCurModel)))
                     goto next_coll_obj;

                  // Don't let us projectile and firer collide
                  if ((pCurModel->IsProjectile() && (GetProjectileFirer(objID) == ourObjID)) ||
                      (pModel->IsProjectile() && (GetProjectileFirer(ourObjID) == objID)))
                      goto next_coll_obj;

                  // Don't let ropes and doors collide
                  if ((pCurModel->IsRope() && pModel->IsDoor()) ||
                      (pCurModel->IsDoor() && pModel->IsRope()))
                     goto next_coll_obj;

                  // Don't let attachments collide with ojects in contact
                  if (pCurModel->IsAttached())
                  {
                     cPhysSubModelInst *pObjContacts;
                     cPhysSubModelInst *pIgnore;
                     int i;

                     for (i=0; i<pCurModel->NumSubModels(); i++)
                     {
                        GetObjectContacts(objID, i, pCurModel, &pObjContacts);

                        if (pObjContacts && pObjContacts->Find(ourObjID, -1, &pIgnore))
                           goto next_coll_obj;
                     }
                  }
                  if (pModel->IsAttached())
                  {
                     cPhysSubModelInst *pObjContacts;
                     cPhysSubModelInst *pIgnore;
                     int i;

                     for (i=0; i<pModel->NumSubModels(); i++)
                     {
                        GetObjectContacts(ourObjID, i, pModel, &pObjContacts);

                        if (pObjContacts && pObjContacts->Find(objID, -1, &pIgnore))
                           goto next_coll_obj;
                     }
                  }

                  // Have we already done this object?
                  if (!objectsSeen.Lookup(objID, &dummy))
                  {
                     objectsSeen.Insert(objID, TRUE);
                     ObjGetCollisionType(objID, &coll_type);

                     // Is it a good collision type?
                     if (coll_type & (COLLISION_BOUNCE | COLLISION_NORESULT))
                     {
                        if (!checkTest || !pCurModel->CollisionChecked())
                        {
                           potentialModels[nPotential] = pCurModel;
                           nPotential++;
                        }
                     }
                  }
               }
            }
         }

next_coll_obj:
         // Advance to next object in bin
         CurrentBinID = pCurrentBin->next_in_bin;
      } while (CurrentBinID != 0);

      // Advance to next bin we're intersecting
      pCurrentRef = OBJREFID_TO_PTR(pCurrentRef->next_of_obj);
   } while (pInitialRef != pCurrentRef);

   // Handle OBBs
   if (pModel->GetType(0) == kPMT_OBB)
      subModId = 0;

   if (nPotential)
   {
      #ifdef OBJ_COLL_TIMING
      AUTO_TIMER(ObjColl2);
      #endif

      // partially integrate all models potentially involved in collision
      if (t0 > 0)
      {
         for (int i=0; i<nPotential; i++)
            Integrate(potentialModels[i], t0);
      }

      if (subModId == -1)
      {
         for (int i = 0; i < pModel->NumSubModels(); i++)
            CheckSubModelCollisions(pModel, i, potentialModels, nPotential, t0, dt);
      }
      else
         CheckSubModelCollisions(pModel, subModId, potentialModels, nPotential, t0, dt);
   }
   pModel->SetCollisionChecked(checkTest);
}

///////////////////////////////////////
//
// Check all moving objects for collisions
//

static void CheckObjectCollisions(mxs_real t0, mxs_real dt)
{
   cPhysModel *pModel = g_PhysModels.GetFirstMoving();

   // iterate over all moveable models
   while (pModel != NULL)
   {
      if (g_PhysicsOn || !pModel->IsPlayer())
         CheckModelObjectCollisions(pModel, t0, dt, TRUE);
      pModel = g_PhysModels.GetNextMoving(pModel);
   }
}

///////////////////////////////////////
//
// Remove all collisions involving this object from the collision list
//

void RemoveObjectCollisions(ObjID objID)
{
   cPhysClsn *pClsn;
   cPhysClsn *pClsnNext;

   pClsn = g_PhysClsns.GetFirst();
   while (pClsn != NULL)
   {
      if ((objID == pClsn->GetObjID()) ||
          ((pClsn->GetType() & kPC_Object) && (objID == pClsn->GetObjID2())))
      {
         #ifndef SHIP
         if (config_is_defined("IntegrationSpew"))
         {
            if (pClsn->GetType() & kPC_Object)
               mprintf("removing collision with %d and %d\n", pClsn->GetObjID(), pClsn->GetObjID2());
            else
               mprintf("removing terrain collision with %d\n", pClsn->GetObjID());
         }
         #endif

         pClsnNext = pClsn->GetNext();
         delete g_PhysClsns.Remove(pClsn);
         pClsn = pClsnNext;
      }
      else
         pClsn = pClsn->GetNext();
   }
}

////////////////////////////////////////////////////////////////////////////////

#define MAX_ITERATIONS 16

struct sCretClsn
{
   cPhysClsn  *pClsn;
   mxs_vector  submod_movement;
};

BOOL UpdateCreaturePhysics(ObjID objID, mxs_vector *new_pos)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);
   mxs_vector joint_offset[8];
   int i, j, k;

   cCreature *pCreature = CreatureFromObj(objID);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   if (pModel == NULL)
   {
#ifndef SHIP
      mprintf("Attempt to update physics on %s, when non physical!\n", ObjWarnName(objID));
#endif
      return FALSE;
   }

   cPhysSubModelInst *pContacts;

   // Check if creatures have broken contact with anything.  This is because
   // they don't go through the standard physics code path.

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pContacts))
      {
         while (pContacts != NULL)
         {
            cPhysSubModelInst *pNextContacts = pContacts->GetNext();

            cPhysModel *pContactModel = pContacts->GetPhysicsModel();

            BOOL on_model = TRUE;

            if (pContactModel->GetType(0) == kPMT_OBB)
            {
               cFaceContact faceContact((cPhysOBBModel *)pContactModel, pContacts->GetSubModId());

               on_model = SubModOnPoly(pModel, i, &faceContact, FALSE);
            }

            mxs_vector normal;
            float objectDistance = PhysGetObjsNorm(pModel, i, pContactModel, pContacts->GetSubModId(), normal);

            if ((objectDistance > kBreakObjectContactDist) || !on_model)
            {
               #ifndef SHIP
               if (config_is_defined("ContactSpew"))
               {
                  mprintf("obj %d breaks contact with obj %d\n", pModel->GetObjID(), pContacts->GetObjID());
                  mprintf("  dist = %g, on_model = %d\n", objectDistance, on_model);
               }
               #endif

               DestroyObjectContact(pModel->GetObjID(), i, pModel,
                                    pContactModel->GetObjID(), pContacts->GetSubModId(), pContactModel);

            }

            pContacts = pNextContacts;
         }
      }
   }

   // @HACK: Check for squishing and apply here, since creatures don't go through
   // the normal squishing code-path.  We're going to assume that if they're
   // contacting the bottom of any moving terrain, that they're being squished.
   // Seems reasonable, but it's pretty brutally hacked.

   for (i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pContacts))
      {
         while (pContacts != NULL)
         {
            cPhysModel *pContactModel = pContacts->GetPhysicsModel();

            if (pContactModel->IsMovingTerrain() && (pContacts->GetSubModId() == 5))
            {
               pModel->Squish(50.0);

               // In case we've died and become non-physical inside Squish()
               if (g_PhysModels.Get(objID) == NULL)
                  return FALSE;

               break;
            }

            pContacts = pContacts->GetNext();
         }
      }
   }

   for (i=0; i<pModel->NumSubModels() && i<pCDesc->nPhysModels; i++)
   {
      GetSubModelPos(objID, i, &joint_offset[i]);
      mx_subeq_vec(&joint_offset[i], new_pos);
   }

   // Find our overall movement delta;
   mxs_vector movement_delta;
   mx_sub_vec(&movement_delta, new_pos, &pModel->GetLocationVec());

   if ((movement_delta.x == 0.0) && (movement_delta.y == 0.0) && (movement_delta.z == 0.0))
      return FALSE;

   cDynArray<sCretClsn> clsnList;
   cDynArray<ObjID> potentialCollisions;

   // Build list of potentially colliding objects
   ObjID    ourObjID    = pModel->GetObjID();
   ObjID    curObjID;

   ObjRefID ourRefID = OBJ_FIRST_REF(ourObjID, PhysRefSystemID);
   ObjRefID curRefID;

   ObjRef *pCurrentRef;
   ObjRef *pInitialRef;
   ObjRef *pCurrentBin;

   pInitialRef = pCurrentRef = OBJREFID_TO_PTR(ourRefID);

   do
   {
      if (ourRefID == 0)
         break;

      curRefID = *(mRefSystems[pCurrentRef->refsys].ref_list_func((void *) pCurrentRef->bin, FALSE));

      do
      {
         pCurrentBin = OBJREFID_TO_PTR(curRefID);
         curObjID = pCurrentBin->obj;

         if (curObjID != ourObjID)
         {
            cPhysModel *pCurModel = g_PhysModels.GetActive(curObjID);
            if (pCurModel != NULL)
            {
               if (!pCurModel->AICollides())
                  goto next_coll_obj;

               if ((pCurModel->IsProjectile() && (GetProjectileFirer(curObjID) == ourObjID)) ||
                   (pModel->IsProjectile() && (GetProjectileFirer(ourObjID) == curObjID)))
                  goto next_coll_obj;

               if (pCurModel->IsWeapon(0))
                  goto next_coll_obj;

               if (pCurModel->IsWeapon(0) &&
                   (AITeamCompare(ourObjID, GetWeaponOwnerObjID(curObjID)) != kAI_Opponents))
                  goto next_coll_obj;

               LGALLOC_AUTO_CREDIT();
               potentialCollisions.Append(curObjID);
            }
         }

next_coll_obj:

         curRefID = pCurrentBin->next_in_bin;
      } while (curRefID != 0);

      pCurrentRef = OBJREFID_TO_PTR(pCurrentRef->next_of_obj);
   } while (pInitialRef != pCurrentRef);

   BOOL sliding = TRUE;
   BOOL collided = FALSE;
   int  iterations = 0;

   while (sliding && (iterations < MAX_ITERATIONS))
   {
      for (i=0; i<pModel->NumSubModels() && i<pCDesc->nPhysModels; i++)
      {
         Location   start, end;
         mxs_vector submod_movement_delta;

         MakeHintedLocationFromVector(&start, &pModel->GetLocationVec(i), &pModel->GetLocation(i));
         MakeHintedLocationFromVector(&end, &pModel->GetLocationVec(), &pModel->GetLocation());

         mx_addeq_vec(&end.vec, &movement_delta);
         mx_addeq_vec(&end.vec, &joint_offset[i]);

         mx_sub_vec(&submod_movement_delta, &end.vec, &start.vec);

         pModel->SetEndLocationVec(i, end.vec);

         int contact_count;
         sSphrContact *pContact;

         // Spherecast our models
         if (((cPhysSphereModel *)pModel)->GetRadius(i) > 0)
         {
            if ((contact_count = SphrSpherecastStatic(&start, &end, ((cPhysSphereModel *)pModel)->GetRadius(i), 1)) > 0)
            {
               // Find the earliest contact time
               pContact = &gaSphrContact[0];
               for (j=1; j<contact_count; j++)
               {
                  if (gaSphrContact[j].time < pContact->time)
                     pContact = &gaSphrContact[j];
               }
            }
         }
         else
            contact_count = 0;

         // Do we already have a collision in our list that's earlier?
         BOOL use_collision = (contact_count > 0);

         for (j=0; j<clsnList.Size() && use_collision; j++)
         {
            // If we already have a collision that's earlier, and it's not an
            // Edge-triggered OBB, then ignore this collision
            if ((clsnList[j].pClsn->GetTime() < pContact->time) &&
                !((clsnList[j].pClsn->GetType() & kPC_Object) &&
                  (clsnList[j].pClsn->GetModel2()->GetType(0) == kPMT_OBB) &&
                  (((cPhysOBBModel *)clsnList[j].pClsn->GetModel2())->IsEdgeTrigger())))

            {
               use_collision = FALSE;
               break;
            }
         }

         if (use_collision)
         {
            // Build a collision structure
            ePhysClsnType type;

            switch (pContact->kind)
            {
               case kSphrPolygon:  type = kPC_TerrainFace;  break;
               case kSphrEdge:     type = kPC_TerrainEdge;  break;
               case kSphrVertex:   type = kPC_TerrainVertex;  break;
            }

            sCretClsn  cretClsn;
            mxs_vector collide_vec;
            mxs_vector move_vec;
            mxs_vector move_backup;
            mxs_real   move_len;

            mx_sub_vec(&move_vec, &pContact->point_on_ray, &pModel->GetLocationVec(i));

            move_len = mx_norm_vec(&move_backup, &move_vec);
            mx_scaleeq_vec(&move_backup, -(min(0.01, move_len)));

            mx_addeq_vec(&move_vec, &move_backup);

            mx_add_vec(&collide_vec, &pModel->GetLocationVec(i), &move_vec);

            cretClsn.submod_movement = submod_movement_delta;
            cretClsn.pClsn = new cPhysClsn(type, 0, pContact->time, pContact->point_in_world,
                                           pModel, i, collide_vec, gaSphrContactData,
                                           pContact->first_contact_data);
            LGALLOC_AUTO_CREDIT();
            clsnList.Append(cretClsn);
         }

         // Check object collisions
         sClsnCheckParms checkData;
         sCretClsn       cretClsn;

         cretClsn.submod_movement = submod_movement_delta;

         checkData.t0 = 0.0;
         checkData.dt = 1.0;

         checkData.pModel1 = pModel;
         checkData.subModel1 = i;

         for (j=0; j<potentialCollisions.Size(); j++)
         {
            cPhysModel *pCurModel = g_PhysModels.Get(potentialCollisions[j]);

            checkData.pModel2 = pCurModel;

            for (k=0; k<pCurModel->NumSubModels() && k<pCDesc->nPhysModels; k++)
            {
               checkData.subModel2 = k;

               ePhysModelType type1 = pModel->GetType(i);
               ePhysModelType type2 = pCurModel->GetType(k);

               if (CheckModelVsModel(pModel1, pModel2, &checkData, &cretClsn.pClsn))
               {
                  LGALLOC_AUTO_CREDIT();
                  clsnList.Append(cretClsn);
               }
            }
         }
      }

      int clsnIndex;

      while (clsnList.Size() > 0)
      {
         clsnIndex = 0;

         for (i=1; i<clsnList.Size(); i++)
         {
            if (clsnList[i].pClsn->GetTime() < clsnList[clsnIndex].pClsn->GetTime())
               clsnIndex = i;
         }

         // Check for edge-triggered OBBs
         if ((clsnList[clsnIndex].pClsn->GetType() & kPC_Object) &&
             (clsnList[clsnIndex].pClsn->GetModel2()->GetType(0) == kPMT_OBB) &&
             (((cPhysOBBModel *)clsnList[clsnIndex].pClsn->GetModel2())->IsEdgeTrigger()))
         {
            // Cause the edge-trigger to do its stuff...
            DetermineCollisionResult(clsnList[clsnIndex].pClsn);

            // ...and make the AI ignore it
            delete clsnList[clsnIndex].pClsn;
            clsnList.FastDeleteItem(clsnIndex);
         }
         else
            break;
      }

      while (clsnList.Size() > 0)
      {
         if (!(clsnList[clsnIndex].pClsn->GetType() & kPC_Terrain))
         {
            cPhysClsn *pClsn = clsnList[clsnIndex].pClsn;
            int reply1, reply2;

            PhysMessageCollision(pClsn, 0, &reply1, &reply2);
            if (reply2 == kPM_Slay)
            {
               clsnList.FastDeleteItem(clsnIndex);
               clsnIndex = 0;

               for (i=1; i<clsnList.Size(); i++)
               {
                  if (clsnList[i].pClsn->GetTime() < clsnList[clsnIndex].pClsn->GetTime())
                     clsnIndex = i;
               }
            }
            else
               break;
         }
         else
            break;
      }

      // If we've collided, we should try to slide
      if (clsnList.Size() > 0)
      {
         mxs_vector constrained_submod_movement;
         mxs_vector constraint_vec;
         mxs_vector normal;

         cPhysClsn *pClsn = clsnList[clsnIndex].pClsn;

         if (clsnList[clsnIndex].pClsn->GetType() & kPC_Terrain)
         {
            normal = clsnList[clsnIndex].pClsn->GetNormal();
         }
         else
         {
            cPhysClsn *pClsn = clsnList[clsnIndex].pClsn;
            PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(), pClsn->GetModel2(), pClsn->GetSubModId2(), normal);
         }

         normal.z = 0;
         if (mx_mag2_vec(&normal) > 0.0001)
            mx_normeq_vec(&normal);

         #ifndef SHIP
         if (config_is_defined("CreatureCollideSpew"))
         {
            mprintf("     %s\n", ObjWarnName(objID));
            mprintf("[%d] dot = %g\n", iterations, mx_dot_vec(&normal, &clsnList[clsnIndex].submod_movement));
            mprintf("     time = %g\n", clsnList[clsnIndex].pClsn->GetDT());
            mprintf("     move = %g %g %g\n", clsnList[clsnIndex].submod_movement.x,
                                              clsnList[clsnIndex].submod_movement.y,
                                              clsnList[clsnIndex].submod_movement.z);
            mprintf("     norm = %g %g %g\n", normal.x, normal.y, normal.z);
         }
         #endif

         mx_copy_vec(&constrained_submod_movement, &clsnList[clsnIndex].submod_movement);
         PhysRemNormComp(&constrained_submod_movement, normal);

         mx_sub_vec(&constraint_vec, &constrained_submod_movement, &clsnList[clsnIndex].submod_movement);

         // Do a little pushout, proportional to the dot product if necessary
         mxs_real dot = mx_dot_vec(&clsnList[clsnIndex].submod_movement, &normal);
         if (dot < 0.0)
            mx_scale_addeq_vec(&movement_delta, &normal, 0.02);
         else
            mx_scale_addeq_vec(&movement_delta, &normal, min(0.04, fabs(dot)));

         mx_scale_addeq_vec(&movement_delta, &constraint_vec, 1.03);

         for (i=0; i<clsnList.Size(); i++)
            delete clsnList[i].pClsn;
         clsnList.SetSize(0);

         collided = TRUE;

         iterations++;
      }
      else
         sliding = FALSE;
   }

   #ifndef SHIP
   if (config_is_defined("CreatureCollideSpew"))
   {
      if (collided)
         mprintf("     final movement delta = %g %g %g\n", movement_delta.x, movement_delta.y, movement_delta.z);
      mprintf("\n");
   }
   #endif

   if (iterations == MAX_ITERATIONS)
   {
      mx_copy_vec(new_pos, &pModel->GetLocationVec());
      return TRUE;
   }

   if ((movement_delta.x == 0.0) && (movement_delta.y == 0.0) && (movement_delta.z == 0.0))
      return FALSE;
   else
   {
      mx_add_vec(new_pos, &pModel->GetLocationVec(), &movement_delta);

      pModel->SetLocationVec(*new_pos, FALSE);

      mxs_vector submod_loc;
      for (i=0; i<pModel->NumSubModels() && i<pCDesc->nPhysModels; i++)
      {
         mx_add_vec(&submod_loc, new_pos, &joint_offset[i]);

         // Back up a bit to keep the spherecaster's epsilon happy
         mxs_vector move_vec;
         mxs_vector move_backup;
         mxs_real   move_len;

         mx_sub_vec(&move_vec, &submod_loc, &pModel->GetLocationVec(i));
         move_len = mx_norm_vec(&move_backup, &move_vec);

         if (move_len > 0.0)
         {
            mx_scaleeq_vec(&move_backup, -(min(0.01, move_len)));
            mx_addeq_vec(&move_vec, &move_backup);

            mx_add_vec(&submod_loc, &pModel->GetLocationVec(i), &move_vec);
         }

         pModel->SetLocationVec(i, submod_loc);

         // Back-compute the offset
         mxs_matrix rot;
         mxs_vector unrot_offset;

         mx_ang2mat(&rot, &ObjPosGet(objID)->fac);

         mx_mat_tmul_vec(&unrot_offset, &rot, &joint_offset[i]);
         pModel->SetSubModOffset(i, unrot_offset);
      }

      return collided;
   }
}


///////////////////////////////////////
//
// A collision has been resolved, update and continue
//

static void PostCollisionUpdate(ObjID objID, cPhysModel * pModel, mxs_real t0, mxs_real dt)
{
   if (pModel->IsCreature() && CreatureSelfPropelled(objID))
      return;

   RemoveObjectCollisions(objID);
   pModel->GetDynamics()->IncCollisionCount();
   ZeroAcceleration(pModel);

   pModel->ClearConstraints();
   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      ConstrainFromObjects(pModel, i);
      ConstrainFromTerrain(pModel, i);
   }

   gInsideMT = TRUE;
   PreMovingTerrainUpdate(pModel);

   UpdateModelControls(pModel, dt);
   UpdateModelDynamics(pModel, dt);
   UpdateObjectContacts(pModel, dt);

   PostMovingTerrainUpdate(pModel);
   gInsideMT = FALSE;

   pModel->UpdateModel(dt);

   CheckModelTerrainCollisions(pModel, t0, dt);
   CheckModelObjectCollisions(pModel, t0, dt, FALSE);
}

static void PostCollisionUpdate(ObjID objID1, cPhysModel *pModel1, ObjID objID2, cPhysModel *pModel2, mxs_real t0, mxs_real dt)
{
   RemoveObjectCollisions(objID1);
   pModel1->GetDynamics()->IncCollisionCount();
   RemoveObjectCollisions(objID2);
   pModel2->GetDynamics()->IncCollisionCount();

   ZeroAcceleration(pModel1);
   ZeroAcceleration(pModel2);

   pModel1->ClearConstraints();
   pModel2->ClearConstraints();

   int i;
   for (i=0; i<pModel1->NumSubModels(); i++)
   {
      ConstrainFromObjects(pModel1, i);
      ConstrainFromTerrain(pModel1, i);
   }
   for (i=0; i<pModel2->NumSubModels(); i++)
   {
      ConstrainFromObjects(pModel2, i);
      ConstrainFromTerrain(pModel2, i);
   }

   gInsideMT = TRUE;
   PreMovingTerrainUpdate(pModel1);
   PreMovingTerrainUpdate(pModel2);

   UpdateModelControls(pModel1, dt);
   UpdateModelControls(pModel2, dt);

   UpdateModelDynamics(pModel1, dt);
   UpdateModelDynamics(pModel2, dt);

   UpdateObjectContacts(pModel1, dt);
   UpdateObjectContacts(pModel2, dt);

   PostMovingTerrainUpdate(pModel1);
   PostMovingTerrainUpdate(pModel2);
   gInsideMT = FALSE;

   pModel1->UpdateModel(dt);
   pModel2->UpdateModel(dt);

   CheckModelTerrainCollisions(pModel1, t0, dt);
   CheckModelObjectCollisions(pModel1, t0, dt, FALSE);

   CheckModelTerrainCollisions(pModel2, t0, dt);
   CheckModelObjectCollisions(pModel2, t0, dt, FALSE);
}

///////////////////////////////////////
//
// Bounce an object off a surface
//

static void BounceObject(cPhysDynData * pDynamics, const mxs_vector &normal, mxs_real elasticity)
{
   mxs_real dampen;

   dampen = pDynamics->GetElasticity() * elasticity;

   if (pDynamics->GetModel()->IsLocationControlled())
   {
      mxs_vector rotvel;

      // Bounce rotation
      mx_copy_vec(&rotvel, &pDynamics->GetRotationalVelocity());
      mx_scaleeq_vec(&rotvel, -dampen);
      pDynamics->SetRotationalVelocity(rotvel);
   }
   else
   {
      mxs_vector norm_comp;
      mxs_vector reflected;
      mxs_real   dp;

      dp = mx_dot_vec((mxs_vector *) &pDynamics->GetVelocity(), (mxs_vector *) &normal);

      if (dp > 0)
         return;

      // Vector of component "into" the normal
      mx_scale_vec(&norm_comp, &normal, dp);

      // Subtract that out of the velocity (so we don't interpenetrate)
      mx_sub_vec(&reflected, &pDynamics->GetVelocity(), &norm_comp);

      // Add in normal component again (for bounce), but dampened by elasticity
      mx_scale_addeq_vec(&reflected, &norm_comp, -dampen);

      pDynamics->SetVelocity(reflected);
   }
}

static void BounceObjectTorque(cPhysDynData * pDynamics, const mxs_vector & normal, const mxs_vector & relVel)
{
   cPhysModel    *pModel   = pDynamics->GetModel();
   tPhysSubModId  subModId = pDynamics->GetSubModId();

   Assert_(pDynamics->GetMass() > 0);

   mxs_vector  velocity;
   mxs_vector  rot_velocity;
   mxs_vector  cur_rot_velocity;
   mxs_real    moment_arm_len;

   if (subModId == -1)
      subModId = 0;

   // Don't rotate non-sphere models
   if (pModel->GetType(subModId) != kPMT_Sphere)
      return;

   // Find distance to colliding poly
   moment_arm_len = ((cPhysSphereModel *)pModel)->GetRadius(subModId);

   // Remove component of velocity into contacting wall
   mx_copy_vec(&velocity, (mxs_vector *) &relVel);
   PhysRemNormComp(&velocity, normal);

   // And scale it backward proportional to the length of the moment arm
   mx_scaleeq_vec(&velocity, -moment_arm_len * 3.1415 / 20);

   // Scale based on mass
   if (pDynamics->GetMass() > 50)
      mx_scaleeq_vec(&velocity, 50 / pDynamics->GetMass());
   else
      mx_scaleeq_vec(&velocity, 1);

   // Find proper rotation axis in object coords
   mx_cross_vec(&rot_velocity, &velocity, (mxs_vector *) &normal);

   // Axis-limit rotation
   for (int i=0; i<3; i++)
   {
      if (!(pModel->GetRotAxes() & (1 << i)))
         rot_velocity.el[i] = 0;
   }

   mx_copy_vec(&cur_rot_velocity, (mxs_vector *) &pDynamics->GetRotationalVelocity());
   mx_addeq_vec(&cur_rot_velocity, &rot_velocity);
   pDynamics->SetRotationalVelocity(cur_rot_velocity);
}

///////////////////////////////////////
//
// check for terrain contact for a sub model and terrain poly
//

static BOOL CheckTerrainContact(cPhysModel * pModel, tPhysSubModId subModId, const cPhysDynData * pDynamics,
                                cFaceContact *pFaceContact)
{
   mxs_real dp;

   if (pModel->IsLocationControlled())
      return FALSE;

   if (pModel->IsRotationalVelocityControlled() &&
      !IsZeroVector(pModel->GetCOGOffset()) &&
      !pModel->IsPlayer())
      return FALSE;

   // check our relative velocity
   dp = mx_dot_vec(&pDynamics->GetVelocity(), &pFaceContact->GetNormal()) *
        pModel->GetDynamics()->GetElasticity();

   // Clear our dp if we're controlling into the surface
   if (pModel->IsVelocityControlled() &&
       (mx_dot_vec(&pModel->GetControls()->GetControlVelocity(), &pFaceContact->GetNormal()) < 0.0))
      dp = 0.0;

   // decide whether we bounce or stick
   if ((fabs(dp) < kBreakTerrainContactVel) || (pModel->GetSpringTension(subModId) > 0))
   {
      BOOL no_contacts = (!pModel->InObjectContact() && !pModel->InTerrainContact());

      pModel->CreateTerrainContact(subModId, pFaceContact->GetPoly(), pFaceContact->GetObjID());

      if (pModel->IsPlayer() && (subModId == PLAYER_FOOT))
      {
         BreakClimb(pModel->GetObjID(), FALSE, FALSE);

         if (g_pPlayerMovement->GetGroundObj() == OBJ_NULL)
         {
            if ((g_pPlayerMode->GetMode() == kPM_Jump) ||
                ((pModel->GetVelocity().z < -15.0) && (g_pPlayerMode->GetMode() != kPM_Swim)))
            {
               g_pPlayerMovement->LandOnGround(pFaceContact->GetObjID());
               if (g_pPlayerMode->GetMode() != kPM_Dead)
                  g_pPlayerMode->SetMode(kPM_Stand);
            }
            else
            {
               if ((g_pPlayerMode->GetMode() != kPM_Stand) &&
                   (g_pPlayerMode->GetMode() != kPM_Crouch) &&
                   (g_pPlayerMode->GetMode() != kPM_Swim) &&
                   (g_pPlayerMode->GetMode() != kPM_Dead))
                  g_pPlayerMode->SetMode(kPM_Stand);
               g_pPlayerMovement->SetGroundObj(pFaceContact->GetObjID());
            }
         }
      }

      if (no_contacts && !PhysObjInWater(pModel->GetObjID()) && (pFaceContact->GetNormal().z < 0.7071))
         CheckMantle(pModel->GetObjID());

      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) creating terrain face contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif

      return TRUE;
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) not creating face contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif
   }

   return FALSE;
}

static BOOL CheckTerrainContact(cPhysModel * pModel, tPhysSubModId subModId, const cPhysDynData * pDynamics,
                                cEdgeContact *pEdgeContact)
{
   mxs_real dp;

   if (pModel->IsLocationControlled())
      return FALSE;

   mxs_vector norm = pEdgeContact->GetNormal(pModel->GetLocationVec(subModId));

   // check our relative velocity
   dp = mx_dot_vec(&pDynamics->GetVelocity(), &norm) * pModel->GetDynamics()->GetElasticity();

   // Clear our dp if we're controlling into the surface
   if (pModel->IsVelocityControlled() &&
       (mx_dot_vec(&pModel->GetControls()->GetControlVelocity(), &norm) < 0.0))
      dp = 0.0;

   // decide whether we bounce or stick
   if ((fabs(dp) < kBreakTerrainContactVel) || (pModel->GetSpringTension(subModId) > 0))
   {
      if (pModel->GetType(subModId) == kPMT_Point)
         pModel->DestroyAllTerrainContacts(subModId);

      BOOL no_contacts = (!pModel->InObjectContact() && !pModel->InTerrainContact());

      pModel->CreateTerrainContact(subModId, pEdgeContact->GetStart(), pEdgeContact->GetEnd());

      if (no_contacts && !PhysObjInWater(pModel->GetObjID()) &&
          (pEdgeContact->GetNormal(pModel->GetLocationVec(subModId)).z < 0.0))
      {
         CheckMantle(pModel->GetObjID());
      }

      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) creating terrain edge contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif

      return TRUE;
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) not creating edge contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif
   }

   return FALSE;
}

static BOOL CheckTerrainContact(cPhysModel * pModel, tPhysSubModId subModId, const cPhysDynData * pDynamics,
                                cVertexContact *pVertexContact)
{
   mxs_real dp;

   if (pModel->IsLocationControlled())
      return FALSE;

   mxs_vector norm = pVertexContact->GetNormal(pModel->GetLocationVec(subModId));

   // check our relative velocity
   dp = mx_dot_vec(&pDynamics->GetVelocity(), &norm) * pModel->GetDynamics()->GetElasticity();

   // Clear our dp if we're controlling into the surface
   if (pModel->IsVelocityControlled() &&
       (mx_dot_vec(&pModel->GetControls()->GetControlVelocity(), &norm) < 0.0))
      dp = 0.0;

   // decide whether we bounce or stick
   if ((fabs(dp) < kBreakTerrainContactVel) || (pModel->GetSpringTension(subModId) > 0))
   {
      if (pModel->GetType(subModId) == kPMT_Point)
         pModel->DestroyAllTerrainContacts(subModId);

      BOOL no_contacts = (!pModel->InObjectContact() && !pModel->InTerrainContact());

      pModel->CreateTerrainContact(subModId, pVertexContact->GetPoint());

      if (no_contacts && !PhysObjInWater(pModel->GetObjID()) &&
          (pVertexContact->GetNormal(pModel->GetLocationVec(subModId)).z < 0.0))
      {
         CheckMantle(pModel->GetObjID());
      }

      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) creating terrain vertex contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif

      return TRUE;
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d (%d) not creating vertex contact (dp = %g)\n", pModel->GetObjID(), subModId, dp);
      #endif
   }

   return FALSE;
}

///////////////////////////////////////
//
// Check for contact between two spheres
//

static void CheckSpheresContact(cPhysSphereModel * pModel1, tPhysSubModId subModId1, const cPhysDynData * pDynamics1,
                                cPhysSphereModel * pModel2, tPhysSubModId subModId2, const cPhysDynData * pDynamics2)
{
   mxs_real   object_distance;
   mxs_real   dp;
   mxs_vector normal;
   mxs_vector relVel;

   if (pModel1->IsRotationalVelocityControlled() &&
      !IsZeroVector(pModel1->GetCOGOffset()) &&
      !pModel1->IsPlayer())
      return;

   if (pModel2->IsRotationalVelocityControlled() &&
      !IsZeroVector(pModel2->GetCOGOffset()) &&
      !pModel2->IsPlayer())
      return;

   mx_sub_vec(&normal,
              (mxs_vector *) &pModel1->GetLocation(subModId1),
              (mxs_vector *) &pModel2->GetLocation(subModId2));
   mx_normeq_vec(&normal);

   // check our relative velocity
   mx_sub_vec(&relVel, &pDynamics1->GetVelocity(), &pDynamics2->GetVelocity());

   dp = mx_dot_vec(&relVel, &normal) *
        pModel1->GetDynamics()->GetElasticity() *
        pModel2->GetDynamics()->GetElasticity();

   // Clear our dp if we're controlling into the surface
   if ((pModel1->IsVelocityControlled() &&
        (mx_dot_vec(&pModel1->GetControls()->GetControlVelocity(), &normal) < 0.0)) ||
       (pModel2->IsVelocityControlled() &&
        (mx_dot_vec(&pModel2->GetControls()->GetControlVelocity(), &normal) > 0.0)))
      dp = 0.0;

   // Get our distance
   object_distance = PhysGetObjsNorm(pModel1, subModId1, pModel2, subModId2, normal);

   // decide whether we bounce or stick
   if (fabs(dp) < kBreakObjectContactVel)
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
      {
         mprintf("obj %d (%d) creating object contact with %d (%d)\n", pModel1->GetObjID(), subModId1,
                                                                       pModel2->GetObjID(), subModId2);
         mprintf("   dp = %g, dist = %g\n", dp, object_distance);
      }
      #endif

      BOOL no_contacts1 = (!pModel1->InObjectContact() && !pModel1->InTerrainContact());
      BOOL no_contacts2 = (!pModel2->InObjectContact() && !pModel2->InTerrainContact());

      CreateObjectContact(pModel1->GetObjID(), subModId1, pModel1,
                          pModel2->GetObjID(), subModId2, pModel2);

      if ((pModel1->IsPlayer() && (subModId1 == PLAYER_FOOT)) ||
          (pModel2->IsPlayer() && (subModId2 == PLAYER_FOOT)))
      {
         if ((g_pPlayerMode->GetMode() != kPM_Stand) &&
             (g_pPlayerMode->GetMode() != kPM_Crouch) &&
             (g_pPlayerMode->GetMode() != kPM_Swim) &&
             (g_pPlayerMode->GetMode() != kPM_Dead))
            g_pPlayerMode->SetMode(kPM_Stand);
      }

      if (no_contacts1 && pModel2->IsRope())
         CheckClimb(pModel1->GetObjID());
      if (no_contacts2 && pModel1->IsRope())
         CheckClimb(pModel2->GetObjID());

      // Don't retain the contact if we don't end up climbing
      if (pModel1->IsRope())
      {
         if (pModel2->GetClimbingObj() != pModel1->GetObjID())
            DestroyObjectContact(pModel1->GetObjID(), subModId1, pModel1,
                                 pModel2->GetObjID(), subModId2, pModel2);

         #ifndef SHIP
         if (config_is_defined("ContactSpew"))
            mprintf("  destroying contact, rope without climbing\n");
         #endif
      }
      else
      if (pModel2->IsRope())
      {
         if (pModel1->GetClimbingObj() != pModel2->GetObjID())
            DestroyObjectContact(pModel1->GetObjID(), subModId1, pModel1,
                                 pModel2->GetObjID(), subModId2, pModel2);

         #ifndef SHIP
         if (config_is_defined("ContactSpew"))
            mprintf("  destroying contact, rope without climbing\n");
         #endif
      }
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
      {
         mprintf("obj %d (%d) not creating object contact with %d (%d)\n", pModel1->GetObjID(), subModId1,
                                                                           pModel2->GetObjID(), subModId2);
         mprintf("   dp = %g, dist = %g\n", dp, object_distance);
      }
      #endif
   }
}

static void CheckSphereOBBContact(cPhysSphereModel *pSphereModel, tPhysSubModId subModId,
                                   cPhysModel *pOBBModel, int side, cFaceContact *pFaceContact)
{
   mxs_real dp;
   mxs_vector rel_vel;

   // Check our relative velocity
   mx_sub_vec(&rel_vel, &pSphereModel->GetVelocity(subModId), &pOBBModel->GetVelocity());

   dp = mx_dot_vec(&rel_vel, &pFaceContact->GetNormal()) *
      pSphereModel->GetDynamics()->GetElasticity() *
      pOBBModel->GetDynamics()->GetElasticity();

   mxs_vector normal;
   mxs_real objectDistance = PhysGetObjsNorm(pSphereModel, subModId, pOBBModel, side, normal);
   mxs_real maxObjectDistance;

   if (pOBBModel->IsDoor())
      maxObjectDistance = kBreakObjectContactDist * 3;
   else
      maxObjectDistance = kBreakObjectContactDist;

   // Stick to sphere hat tops, moving terrain, and attached objects
   if (((pSphereModel->GetType(0) == kPMT_SphereHat) && (side == 1)) ||
       pOBBModel->IsMovingTerrain() || pOBBModel->IsAttached())
      dp = 0.0;

   // Clear our dp if we're controlling into the surface
   if ((pSphereModel->IsVelocityControlled() &&
        (mx_dot_vec(&pSphereModel->GetControls()->GetControlVelocity(), &normal) < 0.0)) ||
       (pOBBModel->IsVelocityControlled() &&
        (mx_dot_vec(&pOBBModel->GetControls()->GetControlVelocity(), &normal) > 0.0)))
      dp = 0.0;

   // Bounce or stick
   if ((fabs(dp) < kBreakObjectContactVel) && (objectDistance < maxObjectDistance))
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
      {
         mprintf("obj %d creates contact with obb %d, side %d\n", pSphereModel->GetObjID(), pOBBModel->GetObjID(), side);
         mprintf("  dp = %g\n", dp);
         mprintf("  obj_dist = %g\n", objectDistance);
      }
      #endif

      BOOL no_contacts = (!pSphereModel->InObjectContact() && !pSphereModel->InTerrainContact());

#ifdef THIEF
      BOOL was_climbing = (g_pPlayerMode->GetMode() == kPM_Climb);
#endif

      CreateObjectContact(pSphereModel->GetObjID(), subModId, pSphereModel,
                          pOBBModel->GetObjID(), side, pOBBModel);

      if (pSphereModel->IsPlayer() && (subModId == PLAYER_FOOT) && no_contacts)
         g_pPlayerMovement->LandOnGround(pOBBModel->GetObjID());

      if ((g_pPlayerMode->GetMode() != kPM_Stand) &&
          (g_pPlayerMode->GetMode() != kPM_Crouch) &&
          (g_pPlayerMode->GetMode() != kPM_Swim) &&
          (g_pPlayerMode->GetMode() != kPM_Dead))
         g_pPlayerMode->SetMode(kPM_Stand);

	  if (pSphereModel->GetObjID() == PlayerObject())
      {
		  char *str;	  	   
		  str = g_pInputBinder->ProcessCmd ("echo $climb_touch");	  	   
		  g_climb_on_touch = ((atof (str) != 0.0) ? TRUE : FALSE);	  
	  }

#ifdef THIEF
      if ((no_contacts) || (g_climb_on_touch) || (was_climbing))
#endif
         CheckClimb(pSphereModel->GetObjID());
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("ContactSpew"))
         mprintf("obj %d does not create contact with obb %d, side %d, dp = %g, dist = %g\n", pSphereModel->GetObjID(), pOBBModel->GetObjID(), side, dp, objectDistance);
      #endif
   }
}

void PhysInitializeContacts()
{
}

void WakeUpContacts(cPhysModel *pModel)
{
   pModel->SetSleep(FALSE);

   // Bail out if we can't wake it up
   if (pModel->IsSleeping())
      return;

   cPhysSubModelInst *pObjContact;

   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      if (GetObjectContacts(pModel->GetObjID(), i, pModel, &pObjContact))
      {
         while (pObjContact)
         {
            if (pObjContact->GetPhysicsModel()->IsSleeping())
               WakeUpContacts(pObjContact->GetPhysicsModel());

            pObjContact = pObjContact->GetNext();
         }
      }
   }
}

///////////////////////////////////////
//
// Bounce two objects off each other
//
// hmm.. this is still a little bit odd and could no doubt be improved
// the basic idea here is to transfer some energy from one object to the other
// assume we have two objects of mass M1 and M2, velocities V1 and V2
// now assume V1 M1 >= V2 M2, ie, object 1 has greater momentum
// assume a perfectly inelastic collision, then after the collision we want
// V1' = V1-A1 and V2' = V2-A2 where A2 = -(A1 M1)/M2
// one solution for this is
// A1 = (V1-V2)M2/(M1+M2) and A2 = -(V1-V2)M1/(M1+M2)
// that's what we use here
// of course, we also have to account for the constraints on each object
// it seems to work OK, we probably want to add a term to account for a more elastic collision
//

void BounceSpheres(const sPhysSubModInst & instance1, cPhysDynData * pDynamics1,
                   const sPhysSubModInst & instance2, cPhysDynData * pDynamics2,
                   mxs_real /* time */)
{
   cPhysModel * pModel1 = instance1.pModel;
   cPhysModel * pModel2 = instance2.pModel;
   mxs_vector   normal;
   mxs_vector   relVel;
   mxs_vector * pVel1 = (mxs_vector *) &pModel1->GetVelocity(instance1.subModId);
   mxs_vector * pVel2 = (mxs_vector *) &pModel2->GetVelocity(instance2.subModId);

   mxs_real     dp1, dp2;             // Portion of velocity along normal
   mxs_vector   vel1, vel2;           // Portion of velocity along normal
   mxs_vector   bvel1, bvel2;         // vel1 and vel2, backward and normalized
   mxs_vector   resvel1, resvel2;     // Resulting velocity
   mxs_vector   acc1, acc2;           // Bounce velocities
   mxs_real     mass1, mass2;
   mxs_vector   temp1, temp2;

   BOOL loc_ctrl1 = pModel1->IsLocationControlled();
   BOOL loc_ctrl2 = pModel2->IsLocationControlled();

   // -------------------
   // Transitional bounce
   // -------------------

   // Get "normal" between objects
   mx_sub_vec(&normal, (mxs_vector *) & instance1.locVec, (mxs_vector *) & instance2.locVec);
   mx_normeq_vec(&normal);

   // heavy if it's not going to move
   if (loc_ctrl1)
   {
      mass1 = 1000;
      mx_zero_vec(pVel1);
   }
   else
      mass1 = pModel1->GetDynamics()->GetMass();
   if (loc_ctrl2)
   {
      mass2 = 1000;
      mx_zero_vec(pVel2);
   }
   else
      mass2 = pModel2->GetDynamics()->GetMass();

   dp1 = mx_dot_vec(pVel1, &normal);
   dp2 = mx_dot_vec(pVel2, &normal);

   // Get velocity components relative to normal
   mx_scale_vec(&vel1, &normal, dp1);
   mx_scale_vec(&vel2, &normal, dp2);

   mx_scale_vec(&bvel1, &vel1, -1);
   if (!IsZeroVector(bvel1))
      mx_normeq_vec(&bvel1);

   mx_scale_vec(&bvel2, &vel2, -1);
   if (!IsZeroVector(bvel2))
      mx_normeq_vec(&bvel2);


   // If they're colliding really hard, break all object contacts
   mx_sub_vec(&relVel, &vel1, &vel2);
   if (mx_mag2_vec(&relVel) > kBreakAllObjectContactVel)
   {
#ifndef SHIP
      if ((config_is_defined("ropebouncecheck")) &&
	  ((pModel2->IsRope()) || (pModel1->IsRope())))
	AssertMsg(FALSE,"Very hard collision.  Object contacts broken!");
#endif
      DestroyAllObjectContacts(instance1.objID, instance1.subModId, instance1.pModel);
      DestroyAllObjectContacts(instance2.objID, instance2.subModId, instance2.pModel);
   }

   // Bounce these vectors, completely elastically, in one dimension
   // (along the line of the normal between the spheres)
   //
   // V1 = ((m1 - m2)*v1 / (m1 + m2)) + ((2 * m2 * v2) / (m1 + m2))
   // V2 = ((2 * m1 * v1) / (m1 + m2)) + ((m2 - m1)*v2 / (m1 + m2))

   mx_scale_vec(&temp1, &vel1, (mass1 - mass2) / (mass1 + mass2));
   mx_scale_vec(&temp2, &vel2, (2 * mass2) / (mass1 + mass2));
   mx_add_vec(&acc1, &temp1, &temp2);

   mx_scale_vec(&temp1, &vel1, (2 * mass1) / (mass1 + mass2));
   mx_scale_vec(&temp2, &vel2, (mass2 - mass1) / (mass1 + mass2));
   mx_add_vec(&acc2, &temp1, &temp2);

   mx_scaleeq_vec(&acc1, 0.5);
   mx_scaleeq_vec(&acc2, 0.5);

   // Replace each object's velocity component along the normal with
   // the new vector.  Note that we're not adding the bounce vector -
   // we're replacing with it.
   mx_copy_vec(&resvel1, pVel1);
   mx_copy_vec(&resvel2, pVel2);
   PhysRemNormComp(&resvel1, bvel1);
   PhysRemNormComp(&resvel2, bvel2);
   mx_addeq_vec(&resvel1, &acc1);
   mx_addeq_vec(&resvel2, &acc2);

   if (loc_ctrl1)
   {
      if (pModel1->IsRope())
         pModel1->GetDynamics(instance1.subModId)->SetVelocity(*pVel2);

      mx_zero_vec(&resvel1);
   }

   if (loc_ctrl2)
   {
      if (pModel2->IsRope())
         pModel2->GetDynamics(instance2.subModId)->SetVelocity(*pVel1);

      mx_zero_vec(&resvel2);
   }

   // Assign new velocity to object
   pModel1->GetDynamics()->SetVelocity(resvel1);
   pModel2->GetDynamics()->SetVelocity(resvel2);

   WakeUpContacts(pModel1);
   WakeUpContacts(pModel2);
}

void BounceSphereOBB(cPhysSphereModel *pSphereModel, tPhysSubModId subModId, cPhysModel *pOBBModel, cFaceContact *pFaceContact)
{
   cPhysDynData *pDynamics1 = pSphereModel->GetDynamics();
   cPhysDynData *pDynamics2 = pOBBModel->GetDynamics();

   mxs_vector * pVel1 = (mxs_vector *) &pSphereModel->GetVelocity(subModId);
   mxs_vector * pVel2 = (mxs_vector *) &pOBBModel->GetVelocity();

   mxs_vector   normal;
   mxs_real     dp1, dp2;             // Portion of velocity along normal
   mxs_vector   vel1, vel2;           // Portion of velocity along normal
   mxs_vector   bvel1, bvel2;         // vel1 and vel2, backward and normalized
   mxs_vector   resvel1, resvel2;     // Resulting velocity
   mxs_vector   acc1, acc2;           // Bounce velocities
   mxs_real     mass1, mass2;
   mxs_vector   temp1, temp2;

   BOOL loc_ctrl1 = pSphereModel->IsLocationControlled();
   BOOL loc_ctrl2 = pOBBModel->IsLocationControlled();

   if (loc_ctrl1 && loc_ctrl2)
      return;

   // "Stick" to location controlled or moving terrain obbs, don't bounce
   if (pOBBModel->IsMovingTerrain())
   {
      if (!loc_ctrl1)
      {
         mxs_vector vel;

         mx_copy_vec(&vel, &pDynamics2->GetVelocity());
         pDynamics1->SetVelocity(vel);
         for (int i=0; i<pSphereModel->NumSubModels(); i++)
            pSphereModel->GetDynamics(i)->SetVelocity(vel);
      }

      return;
   }

   if (pOBBModel->IsAttached())
   {
      LinkID linkID = g_pPhysAttachRelation->GetSingleLink(pOBBModel->GetObjID(), LINKOBJ_WILDCARD);
      sLink link;

      if (linkID)
      {
         g_pPhysAttachRelation->Get(linkID, &link);
         cPhysModel *pAttachModel = g_PhysModels.GetActive(link.dest);

         if (pAttachModel)
         {
            mxs_vector vel;
            mx_copy_vec(&vel, &pAttachModel->GetVelocity());

            pDynamics1->SetVelocity(vel);
            for (int i=0; i<pSphereModel->NumSubModels(); i++)
               pSphereModel->GetDynamics(i)->SetVelocity(vel);
         }
      }

      return;
   }

   // -------------------
   // Transitional bounce
   // -------------------

   // Get "normal" between objects
   mx_copy_vec(&normal, &pFaceContact->GetNormal());

   dp1 = mx_dot_vec(pVel1, &normal);
   dp2 = mx_dot_vec(pVel2, &normal);

   // Get velocity components relative to normal
   mx_scale_vec(&vel1, &normal, dp1);
   mx_scale_vec(&vel2, &normal, dp2);

   mx_scale_vec(&bvel1, &vel1, -1);
   if (!IsZeroVector(bvel1))
      mx_normeq_vec(&bvel1);

   mx_scale_vec(&bvel2, &vel2, -1);
   if (!IsZeroVector(bvel2))
      mx_normeq_vec(&bvel2);

   // Bounce these vectors, completely elastically, in one dimension
   // (along the line of the normal between the spheres)
   //
   // V1 = ((m1 - m2)*v1 / (m1 + m2)) + ((2 * m2 * v2) / (m1 + m2))
   // V2 = ((2 * m1 * v1) / (m1 + m2)) + ((m2 - m1)*v2 / (m1 + m2))

   // heavy if it's not going to move
   mass1 = (loc_ctrl1 && IsZeroVector(*pVel1)) ? 200 : pSphereModel->GetDynamics()->GetMass();
   mass2 = (loc_ctrl2 && IsZeroVector(*pVel2)) ? 200 : pOBBModel->GetDynamics()->GetMass();

   mx_scale_vec(&temp1, &vel1, (mass1 - mass2) / (mass1 + mass2));
   mx_scale_vec(&temp2, &vel2, (2 * mass2) / (mass1 + mass2));
   mx_add_vec(&acc1, &temp1, &temp2);

   mx_scale_vec(&temp1, &vel1, (2 * mass1) / (mass1 + mass2));
   mx_scale_vec(&temp2, &vel2, (mass2 - mass1) / (mass1 + mass2));
   mx_add_vec(&acc2, &temp1, &temp2);

   mx_scaleeq_vec(&acc1, 0.02);
   mx_scaleeq_vec(&acc2, 0.02);

   // Replace each object's velocity component along the normal with
   // the new vector.  Note that we're not adding the bounce vector -
   // we're replacing with it.
   mx_copy_vec(&resvel1, pVel1);
   mx_copy_vec(&resvel2, pVel2);
   PhysRemNormComp(&resvel1, bvel1);
   PhysRemNormComp(&resvel2, bvel2);
   mx_addeq_vec(&resvel1, &acc1);
   mx_addeq_vec(&resvel2, &acc2);

   if (loc_ctrl1)
      mx_zero_vec(&resvel1);

   if (loc_ctrl2 || pOBBModel->IsDoor())
      mx_zero_vec(&resvel2);

   // Assign new velocity to object
   pSphereModel->GetDynamics()->SetVelocity(resvel1);
   pSphereModel->GetDynamics(subModId)->SetVelocity(resvel1);
   pOBBModel->GetDynamics()->SetVelocity(resvel2);

   WakeUpContacts(pSphereModel);
   WakeUpContacts(pOBBModel);
}

///////////////////////////////////////
//
// Partial integration
// Integrate model to time t
//

inline static void Integrate(cPhysModel *pModel, mxs_real t)
{
   if (pModel->IsCreature() && !pModel->IsAvatar()) // && CreatureSelfPropelled(pModel->GetObjID()))
      return;

   if (pModel->IsLocationControlled() && pModel->IsRotationControlled())
      return;

   if (pModel->IsSquishing())
      return;

   mxs_vector old_model_pos;
   mxs_vector old_submod_pos[8];

   mxs_vector new_pos;
   mxs_real modelTime = pModel->GetDynamics()->GetCurrentTime();
   static mxs_real kPartialBackupAmt = .9;   // nasty hack to try to avoid epsilon issues

   if (t == modelTime)
      return;

   Assert_(t>=modelTime);

   // integrate model
   mx_scale_add_vec(&new_pos, &pModel->GetLocationVec(), &pModel->GetVelocity(), (t-modelTime)*kPartialBackupAmt);
   mx_copy_vec(&old_model_pos, &pModel->GetLocationVec());
   pModel->SetCurrentLocationVec(new_pos);

   #ifndef SHIP
   if (config_is_defined("IntegrationSpew"))
   {
      mprintf("Partially Integrating %d, to time %g, with %g already passed\n", pModel->GetObjID(), t, modelTime);
      mprintf("  model = %g %g %g\n", new_pos.x, new_pos.y, new_pos.z);
      mprintf("  vel = %g %g %g\n", pModel->GetVelocity().x, pModel->GetVelocity().y, pModel->GetVelocity().z);
      mprintf("  0vel = %g %g %g\n", pModel->GetVelocity(0).x, pModel->GetVelocity(0).y, pModel->GetVelocity(0).z);
   }
   #endif

   // integrate sub-models
   for (int i=0; i<pModel->NumSubModels(); i++)
   {
      mx_scale_add_vec(&new_pos, &pModel->GetLocationVec(i), &pModel->GetVelocity(i), (t-modelTime)*kPartialBackupAmt);
      mx_copy_vec(&old_submod_pos[i], &pModel->GetLocationVec(i));
      pModel->SetCurrentLocationVec(i, new_pos);

      #ifndef SHIP
      if (config_is_defined("IntegrationSpew") && ((pModel->GetType(0) == kPMT_Sphere) || (pModel->GetType(0) == kPMT_Point)))
         mprintf("  submod [%d] = %g %g %g\n", i, new_pos.x, new_pos.y, new_pos.z);
      #endif
   }

   pModel->UpdateMedium();

   #ifndef SHIP
   if (inworld_checks && !HasObjectWarned(pModel->GetObjID()) && !PhysObjValidPos(pModel->GetObjID(), NULL))
   {
      mprintf("Partially integrated %s to invalid position!\n", ObjEditName(pModel->GetObjID()));
      for (int i=0; i<pModel->NumSubModels(); i++)
      {
         if ((pModel->GetType(i) == kPMT_Sphere) || (pModel->GetType(i) == kPMT_SphereHat))
         {
            Location biteme;

            MakeLocationFromVector(&biteme, &pModel->GetLocationVec(i));
            if (!SphrSphereInWorld(&biteme, ((cPhysSphereModel *)pModel)->GetRadius(i), 0))
               mprintf("[%d] out! %g %g %g\n", i, pModel->GetLocationVec(i).x, pModel->GetLocationVec(i).y, pModel->GetLocationVec(i).z);
         }
         else
         {
            Location nudge;
            mx_copy_vec(&nudge.vec, &pModel->GetLocationVec(i));
            nudge.hint = nudge.cell = -1;
            if (CellFromLoc(&nudge) == CELL_INVALID)
               mprintf("[%d] out! %g %g %g\n", i, pModel->GetLocationVec(i).x, pModel->GetLocationVec(i).y, pModel->GetLocationVec(i).z);
         }
      }
   }
   #endif

   #ifndef SHIP
   if (config_is_defined("IntegrationSpew"))
      mprintf("\n");
   #endif

   // update model time
   pModel->GetDynamics()->SetCurrentTime(t);
}

///////////////////////////////////////
//
// Get object to calculated collision location
//

inline void IntegrateToCollision(cPhysModel * pModel, tPhysSubModId subModId,
                                  cPhysDynData * pDynamics, const mxs_vector & loc,
                                  mxs_real t0, mxs_real dt, mxs_vector &normal,
                                  mxs_real integration_backup)
{
   if (pModel->IsCreature() && !pModel->IsAvatar())
      return;

   if (pModel->IsLocationControlled() && pModel->IsRotationControlled())
      return;

   if (pModel->IsSquishing() || pModel->IsAttached())
      return;

   mxs_vector new_pos;

   mxs_vector old_model_pos;
   mxs_vector old_submod_pos[8];

   mxs_real model_time;
   mxs_real integration_time;

   mxs_real d = mx_dot_vec(&normal, &loc);

   if ((mx_dot_vec(&normal, &pModel->GetVelocity()) < 0) && ((mx_dot_vec(&normal, &pModel->GetLocationVec(subModId)) - d) < 0.001))
      integration_backup = 0;

   model_time = pModel->GetDynamics()->GetCurrentTime();
   integration_time = t0 + dt - model_time;

   if (integration_time <= 0.0)
      return;

   mx_scale_add_vec(&new_pos, &pModel->GetLocationVec(), &pModel->GetVelocity(),
                    integration_time * integration_backup);
   mx_copy_vec(&old_model_pos, &pModel->GetLocationVec());
   pModel->SetLocationVec(new_pos);

   #ifndef SHIP
   if (config_is_defined("IntegrationSpew"))
   {
      if (model_time > 0.0)
         mprintf("**");
      mprintf("Integrating %d, for %g time, with %g already passed\n", pModel->GetObjID(), integration_time, model_time);
      mprintf("  model = %g %g %g\n", new_pos.x, new_pos.y, new_pos.z);
      mprintf("  vel = %g %g %g\n", pModel->GetVelocity().x, pModel->GetVelocity().y, pModel->GetVelocity().z);
      mprintf("  0vel = %g %g %g\n", pModel->GetVelocity(0).x, pModel->GetVelocity(0).y, pModel->GetVelocity(0).z);
   }
   #endif

   if (pModel->GetType(0) != kPMT_OBB)
   {
      for (int i=0; i<pModel->NumSubModels(); i++)
      {
         if ((pModel->GetType(i) == kPMT_Point) && (i == subModId))
         {
            mxds_vector movement;

            movement.x = ((double)loc.x - (double)pModel->GetLocationVec(i).x) * integration_backup;
            movement.y = ((double)loc.y - (double)pModel->GetLocationVec(i).y) * integration_backup;
            movement.z = ((double)loc.z - (double)pModel->GetLocationVec(i).z) * integration_backup;

            new_pos.x = (double)pModel->GetLocationVec(i).x + movement.x;
            new_pos.y = (double)pModel->GetLocationVec(i).y + movement.y;
            new_pos.z = (double)pModel->GetLocationVec(i).z + movement.z;
         }
         else
         {
            mx_scale_add_vec(&new_pos, &pModel->GetLocationVec(i), &pModel->GetVelocity(i), integration_time * integration_backup);
         }
         mx_copy_vec(&old_submod_pos[i], &pModel->GetLocationVec(i));
         pModel->SetLocationVec(i, new_pos);
         #ifndef SHIP
         if (config_is_defined("IntegrationSpew"))
            mprintf("  submod [%d] = %g %g %g\n", i, new_pos.x, new_pos.y, new_pos.z);
         #endif
      }
   }

   #ifndef SHIP
   if (config_is_defined("IntegrationSpew"))
      mprintf("\n");
   #endif
   pDynamics->SetCurrentTime(t0 + dt);

   pModel->UpdateMedium();

   #ifndef SHIP
   if (inworld_checks && !HasObjectWarned(pModel->GetObjID()) && !PhysObjValidPos(pModel->GetObjID(), NULL))
   {
      mprintf("Integrated %s to invalid position!\n", ObjEditName(pModel->GetObjID()));
      for (int i=0; i<pModel->NumSubModels(); i++)
      {
         if ((pModel->GetType(i) == kPMT_Sphere) || (pModel->GetType(i) == kPMT_SphereHat))
         {
            Location biteme;

            MakeLocationFromVector(&biteme, &pModel->GetLocationVec(i));
            if (!SphrSphereInWorld(&biteme, ((cPhysSphereModel *)pModel)->GetRadius(i), 0))
               mprintf("[%d] out! %g %g %g\n", i, pModel->GetLocationVec(i).x, pModel->GetLocationVec(i).y, pModel->GetLocationVec(i).z);
         }
         else
         {
            Location nudge;
            mx_copy_vec(&nudge.vec, &pModel->GetLocationVec(i));
            nudge.hint = nudge.cell = -1;
            if (CellFromLoc(&nudge) == CELL_INVALID)
               mprintf("[%d] out! %g %g %g\n", i, pModel->GetLocationVec(i).x, pModel->GetLocationVec(i).y, pModel->GetLocationVec(i).z);
         }
      }
   }
   #endif

   pDynamics->SetCurrentTime(t0 + dt);
}

BOOL RayOBBCollideCheck(ObjID objID, Location *start, Location *end, Location *hit, int *side)
{
   static mxs_vector start_norm_list[6];
   static mxs_vector end_norm_list[6];
   static mxs_real   start_d_list[6];
   static mxs_real   end_d_list[6];

   mxs_real start_dist;
   mxs_real end_dist;
   mxs_real time;

   int best_side = -1;

   mxs_real   max_entry_time = -1000000.0;
   mxs_real   min_exit_time = 1000000.0;

   cPhysOBBModel *pOBBModel = (cPhysOBBModel *)g_PhysModels.Get(objID);

   Assert_(pOBBModel);
   Assert_(pOBBModel->GetType(0) == kPMT_OBB);

   pOBBModel->GetNormals(start_norm_list, end_norm_list);
   pOBBModel->GetConstants(start_d_list, start_norm_list, end_d_list, end_norm_list);

   for (int i=0; i<6; i++)
   {
      start_dist = mx_dot_vec(&start_norm_list[i], &start->vec) - start_d_list[i];
      end_dist   = mx_dot_vec(&start_norm_list[i], &end->vec) - start_d_list[i];

      if ((start_dist <= 0.0001) && (end_dist <= 0.0001))
         continue;

      if ((start_dist >= -0.0001) && (end_dist >= -0.0001))
      {
         best_side = -1;
         break;
      }

      time = start_dist / (start_dist - end_dist);

      if (start_dist < 0)
      {
         if (time < min_exit_time)
            min_exit_time = time;
      }
      else
      if (time > max_entry_time)
      {
         max_entry_time = time;
         best_side = i;
      }
   }

   if (best_side == -1)
      return TRUE;

   if ((max_entry_time < 1.0) && (min_exit_time > max_entry_time))
   {
      mxs_vector vec;

      mx_sub_vec(&vec, &end->vec, &start->vec);
      mx_scale_add_vec(&hit->vec, &start->vec, &vec, max_entry_time);
      *side = best_side;

      return FALSE;
   }
   else
      return TRUE;
}


///////////////////////////////////////
//
// Check for a step
//
static BOOL CheckStep(cPhysClsn * pClsn, ObjID coll_OBB)
{
   cPhysModel    *pModel;
   tPhysSubModId  subModId;

   mxs_vector     step_offset;
   mxs_real       step_up = 0.02;

   mx_mk_vec(&step_offset, 0, 0, 2);

   const mxs_real kMinZDelta = 0.3;

   mxs_vector movement_dir;
   Location start_loc, end_loc, hit_loc;
   BOOL collision;
   int  side;

   if (pClsn->GetType() & kPC_Terrain)
   {
      pModel = pClsn->GetModel();
      subModId = pClsn->GetSubModId();
   }
   else
   if (pClsn->GetType() & kPC_Object)
   {
      cPhysOBBModel *pOBBModel;

      if (pClsn->GetObjID() == coll_OBB)
      {
         pModel = pClsn->GetModel2();
         subModId = pClsn->GetSubModId2();

         pOBBModel = (cPhysOBBModel *)pClsn->GetModel();
      }
      else
      if (pClsn->GetObjID2() == coll_OBB)
      {
         pModel = pClsn->GetModel();
         subModId = pClsn->GetSubModId();

         pOBBModel = (cPhysOBBModel *)pClsn->GetModel2();
      }
      else
      {
         Warning(("CheckStep: OBB specified, but not in collision: %d (%d %d)\n", coll_OBB, pClsn->GetObjID(), pClsn->GetObjID2()));
         return FALSE;
      }

      if (pOBBModel->IsEdgeTrigger())
         return FALSE;
   }
   else
   {
      Warning(("CheckStep: unknown collision type: %d\n", pClsn->GetType()));
      return FALSE;
   }

   if (pModel->IsPlayer() || pModel->IsAvatar())
      subModId = PLAYER_FOOT;

   mx_copy_vec(&movement_dir, &pModel->GetVelocity(subModId));

   MakeHintedLocationFromVector(&start_loc, &pModel->GetLocationVec(subModId), &pModel->GetLocation(subModId));
   MakeHintedLocationFromVector(&end_loc, &start_loc.vec, &start_loc);

   mx_addeq_vec(&end_loc.vec, &step_offset);

   // Cast up
   #ifndef SHIP
   if (config_is_defined("StepSpew"))
   {
      mprintf("Casting up from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("             to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   if (coll_OBB != OBJ_NULL)
      collision = !RayOBBCollideCheck(coll_OBB, &start_loc, &end_loc, &hit_loc, &side);
   else
      collision = !PortalRaycast(&start_loc, &end_loc, &hit_loc, 0);

   if (collision && !pModel->IsAvatar())
   {
      #ifndef SHIP
      if (config_is_defined("StepSpew"))
         mprintf("Step up failed\n");
      #endif

      return FALSE;
   }

   MakeHintedLocationFromVector(&start_loc, &end_loc.vec, &start_loc);

   mx_scaleeq_vec(&movement_dir, 0.01);
   mx_addeq_vec(&end_loc.vec, &movement_dir);

   // Cast forward
   #ifndef SHIP
   if (config_is_defined("StepSpew"))
   {
      mprintf("Casting forward from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("                  to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   if (coll_OBB != OBJ_NULL)
      collision = !RayOBBCollideCheck(coll_OBB, &start_loc, &end_loc, &hit_loc, &side);
   else
      collision = !PortalRaycast(&start_loc, &end_loc, &hit_loc, 0);

   if (collision)
   {
      #ifndef SHIP
      if (config_is_defined("StepSpew"))
         mprintf("Step forward failed\n");
      #endif

      return FALSE;
   }

   MakeHintedLocationFromVector(&start_loc, &end_loc.vec, &start_loc);
   mx_subeq_vec(&end_loc.vec, &step_offset);
   mx_subeq_vec(&end_loc.vec, &step_offset);

   // Cast downward
   #ifndef SHIP
   if (config_is_defined("StepSpew"))
   {
      mprintf("Casting down from %g %g %g\n", start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
      mprintf("               to %g %g %g\n", end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
   }
   #endif

   if (coll_OBB != OBJ_NULL)
      collision = !RayOBBCollideCheck(coll_OBB, &start_loc, &end_loc, &hit_loc, &side);
   else
      collision = !PortalRaycast(&start_loc, &end_loc, &hit_loc, 0);

   if (collision)
   {
      mxs_vector new_pos, new_submod_pos;
      mxs_real   z_delta;

      int cell_id = -1;
      int poly_id = -1;

      if (coll_OBB == OBJ_NULL)
      {
         cell_id = PortalRaycastCell;
         poly_id = PortalRaycastFindPolygon();
      }

      // Find the z-delta between our point and the hit location
      z_delta = hit_loc.vec.z - pModel->GetLocationVec(subModId).z;

      if (z_delta < kMinZDelta)
         z_delta = kMinZDelta;

      z_delta += step_up;

      #ifndef SHIP
      if (config_is_defined("StepSpew"))
         mprintf("  z-delta = %g\n", z_delta);
      #endif

      mx_copy_vec(&new_pos, &pModel->GetLocationVec());
      new_pos.z += z_delta;

      int i;

      for (i=0; i<pModel->NumSubModels(); i++)
      {
         mx_copy_vec(&new_submod_pos, &pModel->GetLocationVec(i));
         new_submod_pos.z += z_delta;

         if (pModel->GetSpringTension(i) > 0)
            pModel->SetEndLocationVec(i, pModel->GetLocationVec(i));
         else
            pModel->SetEndLocationVec(i, new_submod_pos);
      }

      cPhysClsn *pDummyClsn;

      for (i=0; i<pModel->NumSubModels(); i++)
      {
         if (pModel->GetType(i) != kPMT_Sphere)
            continue;

         if (((cPhysSphereModel *)pModel)->CheckTerrainCollision(i, 0, 1, &pDummyClsn))
         {
            #ifndef SHIP
            if (config_is_defined("StepSpew"))
               mprintf("  submod %d failed\n", i);
            #endif

            return FALSE;
         }
      }

      #ifndef SHIP
      if (config_is_defined("StepSpew"))
      {
         mprintf("  all submodels passed\n");
         mprintf("  new pos = %g %g %g\n", new_pos.x, new_pos.y, new_pos.z);
      }
      #endif

      pModel->SetLocationVec(new_pos);

      for (i=0; i<pModel->NumSubModels(); i++)
         pModel->SetLocationVec(i, pModel->GetEndLocationVec(i));

      BreakClimb(pModel->GetObjID(), FALSE, FALSE);

      if (coll_OBB == OBJ_NULL)
      {
         cFaceContact new_contact(cell_id, poly_id);

         if (pModel->IsPlayer())
         {
            pModel->DestroyAllTerrainContacts(PLAYER_KNEE);
            pModel->DestroyAllTerrainContacts(PLAYER_SHIN);
            pModel->DestroyAllTerrainContacts(PLAYER_FOOT);

            pModel->CreateTerrainContact(PLAYER_FOOT, new_contact.GetPoly(), new_contact.GetObjID());
         }
         else
         {
            pModel->DestroyAllTerrainContacts(subModId);

            pModel->CreateTerrainContact(subModId, new_contact.GetPoly(), new_contact.GetObjID());
         }

         g_pPlayerMovement->SetGroundObj(new_contact.GetObjID());

         if ((g_pPlayerMode->GetMode() != kPM_Stand) &&
             (g_pPlayerMode->GetMode() != kPM_Crouch) &&
             (g_pPlayerMode->GetMode() != kPM_Swim) &&
             (g_pPlayerMode->GetMode() != kPM_Dead))
            g_pPlayerMode->SetMode(kPM_Stand);

         #ifndef SHIP
         if (config_is_defined("ContactSpew"))
            mprintf("step creating terrain face contact\n");
         #endif
      }
      else
      {
         cPhysOBBModel *pOBBModel;

         if (pModel == pClsn->GetModel())
            pOBBModel = (cPhysOBBModel *)pClsn->GetModel2();
         else
            pOBBModel = (cPhysOBBModel *)pClsn->GetModel();

         if (pModel->IsPlayer())
         {
            CreateObjectContact(pModel->GetObjID(), PLAYER_FOOT, pModel,
                                pOBBModel->GetObjID(), side, pOBBModel);
            g_pPlayerMovement->SetGroundObj(pOBBModel->GetObjID());

            if ((g_pPlayerMode->GetMode() != kPM_Stand) &&
                (g_pPlayerMode->GetMode() != kPM_Crouch) &&
                (g_pPlayerMode->GetMode() != kPM_Swim) &&
                (g_pPlayerMode->GetMode() != kPM_Dead))
               g_pPlayerMode->SetMode(kPM_Stand);
         }
         else
         {
            CreateObjectContact(pModel->GetObjID(), subModId, pModel,
                                pOBBModel->GetObjID(), side, pOBBModel);
         }

         WakeUpContacts(pOBBModel);

         #ifndef SHIP
         if (config_is_defined("ContactSpew"))
            mprintf("step creating OBB face contact\n");
         #endif
      }

      return TRUE;
   }
   else
   {
      #ifndef SHIP
      if (config_is_defined("StepSpew"))
         mprintf("Step down failed\n");
      #endif

      return FALSE;
   }
}

///////////////////////////////////////
//
// resolve a bouncing collision
//
static void ResolveBounce(cPhysClsn * pClsn, mxs_real dt)
{
   cPhysDynData *pDynamics1;

   if (pClsn->GetModel()->GetSpringTension(pClsn->GetSubModId()) > 0.01)
      pDynamics1 = pClsn->GetModel()->GetDynamics(pClsn->GetSubModId());
   else
      pDynamics1 = pClsn->GetModel()->GetDynamics();

   switch (pClsn->GetType())
   {
      case kPC_SphereHatObject:
      {
         cPhysDynData *pDynamics2 = pClsn->GetModel2()->GetDynamics();
         cFaceContact  faceContact((cPhysSphereModel *)pClsn->GetModel(),
                                   ((cPhysSphereModel *)pClsn->GetModel())->GetRadius(0));

         CheckSphereOBBContact((cPhysSphereModel *)pClsn->GetModel(), pClsn->GetSubModId(),
                                pClsn->GetModel2(), pClsn->GetSubModId2(), &faceContact);

         BounceSphereOBB((cPhysSphereModel *)pClsn->GetModel(), pClsn->GetSubModId(),
                          pClsn->GetModel2(), &faceContact);

         if (!(pClsn->GetModel()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID())))
            pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());
         if (!(pClsn->GetModel2()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID2())))
            pClsn->GetModel2()->UpdateModel(dt - pClsn->GetTime());

         break;
      }

      case kPC_OBBObject:
      {
         cPhysDynData *pDynamics2 = pClsn->GetModel2()->GetDynamics();
         cFaceContact  faceContact((cPhysOBBModel *)pClsn->GetModel2(), pClsn->GetSubModId2());

         CheckSphereOBBContact((cPhysSphereModel *)pClsn->GetModel(), pClsn->GetSubModId(),
                               (cPhysOBBModel *)pClsn->GetModel2(), pClsn->GetSubModId2(), &faceContact);

         BounceSphereOBB((cPhysSphereModel *)pClsn->GetModel(), pClsn->GetSubModId(),
                         (cPhysOBBModel *)pClsn->GetModel2(), &faceContact);

         if (!(pClsn->GetModel()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID())))
            pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());
         if (!(pClsn->GetModel2()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID2())) &&
             !pClsn->GetModel2()->IsAttached())
            pClsn->GetModel2()->UpdateModel(dt - pClsn->GetTime());

         break;
      }

      case kPC_MoveableObject:
      {

         cPhysDynData *pDynamics2 = pClsn->GetModel2()->GetDynamics();


         CheckSpheresContact((cPhysSphereModel *) pClsn->GetModel(), pClsn->GetSubModId(), pDynamics1,
                              (cPhysSphereModel *) pClsn->GetModel2(), pClsn->GetSubModId2(), pDynamics2);

         if (!pClsn->GetModel()->IsPlayer() && !pClsn->GetModel2()->IsPlayer())
         {
            mxs_vector    norm;
            mxs_vector    relvel;

            mx_sub_vec(&relvel, &pDynamics1->GetVelocity(), &pDynamics2->GetVelocity());
            PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(), pClsn->GetModel2(), pClsn->GetSubModId2(), norm);

            if (!pClsn->GetModel()->IsPlayer() && !pClsn->GetModel()->IsRotationControlled())
               BounceObjectTorque(pDynamics1, norm, relvel);

            mx_scaleeq_vec(&relvel, -1.0);
            mx_scaleeq_vec(&norm, -1.0);

            if (!pClsn->GetModel2()->IsPlayer() && !pClsn->GetModel2()->IsRotationControlled())
               BounceObjectTorque(pDynamics2, norm, relvel);

         }

         BounceSpheres(pClsn->GetInstance(), pDynamics1,
                        pClsn->GetInstance2(), pDynamics2, pClsn->GetTime());

         if (!(pClsn->GetModel()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID())))
            pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());
         if (!(pClsn->GetModel2()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID2())))
            pClsn->GetModel2()->UpdateModel(dt - pClsn->GetTime());

         break;
      }

      case kPC_TerrainVertex:
      case kPC_TerrainEdge:
      case kPC_TerrainFace:
      {
         switch (pClsn->GetType())
         {
            case kPC_TerrainVertex:
            {
               mxs_vector clsn_pt;

               mx_copy_vec(&clsn_pt, &pClsn->GetClsnPt());
               cVertexContact vertexContact(clsn_pt);

               if (CheckTerrainContact(pClsn->GetModel(), pClsn->GetSubModId(), pDynamics1, &vertexContact))
               {
                  pClsn->GetModel()->ClearConstraints();
                  for (int i=0; i<pClsn->GetModel()->NumSubModels(); i++)
                  {
                     ConstrainFromObjects(pClsn->GetModel(), i);
                     ConstrainFromTerrain(pClsn->GetModel(), i);
                  }
               }

               const mxs_vector *p_submod_loc = &pClsn->GetModel()->GetLocationVec(pClsn->GetSubModId());

               // Bounce and spin object (as long as it's not the player)
               if (!pClsn->GetModel()->IsPlayer() && ! pClsn->GetModel()->IsCreature() &&
                   !pClsn->GetModel()->IsRotationControlled())
                  BounceObjectTorque(pDynamics1, vertexContact.GetNormal(*p_submod_loc), pDynamics1->GetVelocity());

               BounceObject(pDynamics1, vertexContact.GetNormal(*p_submod_loc), kTerrainBounce);

               pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());

               break;
            }

            case kPC_TerrainEdge:
            {
               cPhysTerrPoly *pTerrPoly;
               int cell_id;
               int poly_id;
               int offset, offset_max;
               int i, j;
               mxs_vector *start, *end;
               mxs_vector clsn_pt;
               BOOL found = FALSE;

               for (int edge_num=0; edge_num<2 && !found; edge_num++)
               {
                  pTerrPoly = pClsn->GetTerrainPoly(edge_num);
                  cell_id = pTerrPoly->GetCellID();
                  poly_id = pTerrPoly->GetPolyID();

                  // Find starting offset of poly
                  offset = 0;
                  for (i=0; i<poly_id; i++)
                      offset += WR_CELL(cell_id)->poly_list[i].num_vertices;

                  offset_max = offset + WR_CELL(cell_id)->poly_list[poly_id].num_vertices;

                  // Find an edge that contains the collision point
                  for (i=offset; i<offset_max; i++)
                  {
                     j = ((i - offset + 1) % (offset_max - offset)) + offset;
                     start = (mxs_vector *) &WR_CELL(cell_id)->vpool[WR_CELL(cell_id)->vertex_list[i]];
                     end   = (mxs_vector *) &WR_CELL(cell_id)->vpool[WR_CELL(cell_id)->vertex_list[j]];

                     cEdgeContact edgeContact(*start, *end);
                     mx_copy_vec(&clsn_pt, &pClsn->GetClsnPt());
                     if (edgeContact.GetDist(clsn_pt) < 0.04)
                     {
                        found = TRUE;
                        break;
                     }
                  }
               }

               AssertMsg(found, "Unable to find edge containing collision point");

               // Ugly evil bad
               cEdgeContact edgeContact(*start, *end);

               if (CheckTerrainContact(pClsn->GetModel(), pClsn->GetSubModId(), pDynamics1, &edgeContact))
               {
                  pClsn->GetModel()->ClearConstraints();
                  for (int i=0; i<pClsn->GetModel()->NumSubModels(); i++)
                  {
                     ConstrainFromObjects(pClsn->GetModel(), i);
                     ConstrainFromTerrain(pClsn->GetModel(), i);
                  }
               }

               const mxs_vector *p_submod_loc = &pClsn->GetModel()->GetLocationVec(pClsn->GetSubModId());

               // Bounce and spin object (as long as it's not the player)
               if (!pClsn->GetModel()->IsPlayer() && !pClsn->GetModel()->IsCreature() &&
                   !pClsn->GetModel()->IsRotationControlled())
                  BounceObjectTorque(pDynamics1, edgeContact.GetNormal(*p_submod_loc), pDynamics1->GetVelocity());

               BounceObject(pDynamics1, edgeContact.GetNormal(*p_submod_loc), kTerrainBounce);

               pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());

               break;
            }

            case kPC_TerrainFace:
            {
               cFaceContact faceContact(pClsn->GetCellID(), pClsn->GetPolyID());

               if (CheckTerrainContact(pClsn->GetModel(), pClsn->GetSubModId(), pDynamics1, &faceContact))
               {
                  pClsn->GetModel()->ClearConstraints();
                  for (int i=0; i<pClsn->GetModel()->NumSubModels(); i++)
                  {
                     ConstrainFromObjects(pClsn->GetModel(), i);
                     ConstrainFromTerrain(pClsn->GetModel(), i);
                  }
               }

               if (!pClsn->GetModel()->IsPlayer() && !pClsn->GetModel()->IsCreature() &&
                   !pClsn->GetModel()->IsRotationControlled())
                  BounceObjectTorque(pDynamics1, pClsn->GetNormal(), pDynamics1->GetVelocity());

               BounceObject(pDynamics1, pClsn->GetNormal(), kTerrainBounce);

               pClsn->GetModel()->UpdateModel(dt - pClsn->GetTime());

               break;
            }
         }

         break;
      }
      default:
         Warning(("ResolveCollision: unknown collision type %d\n",
                  pClsn->GetType()));
         return;
   }
}

///////////////////////////////////////
//
// Resolve a collision
//

int g_CollisionObj1;     // holy crap this is a hack
int g_CollisionObj2;
int g_CollisionSubmod1;
int g_CollisionSubmod2;
EXTERN mxs_vector g_terr_coll_normal;
mxs_vector g_terr_coll_normal;
mxs_vector g_collision_location;

DECLARE_TIMER(PHYS_CollideEvent, Average);

ePhysClsnResult DetermineCollisionResult(cPhysClsn * pClsn)
{
   int bits, flags;
   int result;

   mxs_vector momentum;
   mxs_real  dot;

   if (pClsn->GetType() & kPC_Terrain)
   {
      mxs_vector normal = pClsn->GetNormal();

      dot = mx_dot_vec(&pClsn->GetModel()->GetVelocity(pClsn->GetSubModId()), &normal);
      dot = 0.5 * pClsn->GetModel()->GetDynamics()->GetMass() * dot * dot;

      g_CollisionObj1 = pClsn->GetObjID();
      g_CollisionSubmod1 = pClsn->GetSubModId();
      g_CollisionObj2 = GetTextureObj(pClsn->GetTexture());
      g_CollisionSubmod2 = -1;
      g_terr_coll_normal = normal;

      ObjID texture = GetTextureObj(pClsn->GetTexture());
      bits = CollideTest(pClsn->GetObjID(), OBJ_NULL);

      PosPropLock = TRUE;
      if (!(pClsn->GetModel()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID())))
      {
         if (((pClsn->GetModel()->GetType(0) == kPMT_Sphere || pClsn->GetModel()->GetType(0) == kPMT_Point) &&
              (pClsn->GetModel()->NumSubModels() == 1)) ||
             (pClsn->GetModel()->GetType(0) == kPMT_SphereHat))
         {
            ObjPosUpdate(pClsn->GetObjID(), &pClsn->GetModel()->GetLocationVec(), &pClsn->GetModel()->GetRotation());
         }
      }
      PosPropLock = FALSE;

      AUTO_TIMER(PHYS_CollideEvent);
      flags = CollideEvent(pClsn->GetObjID(), texture, bits, fabs(dot), pClsn);
   }
   else
   {
      mxs_vector normal;
      mxs_real dot;
      mxs_real kinetic_energy = 0;

      if (pClsn->GetSubModId2() >= 0)
      {
         PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(),
                         pClsn->GetModel2(), pClsn->GetSubModId2(), normal);
      }
      else
      {
         PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(),
                         pClsn->GetModel2(), -(pClsn->GetSubModId2() + 1), normal);
      }

      dot = max(0, -mx_dot_vec(&pClsn->GetModel()->GetVelocity(), &normal));
      kinetic_energy += 0.5 * pClsn->GetModel()->GetDynamics()->GetMass() * dot * dot;

      dot = max(0, mx_dot_vec(&pClsn->GetModel2()->GetVelocity(), &normal));
      kinetic_energy += 0.5 * pClsn->GetModel2()->GetDynamics()->GetMass() * dot * dot;

      if (pClsn->GetModel()->IsAttached() || pClsn->GetModel2()->IsAttached())
         kinetic_energy = 0.0;

      if (!(pClsn->GetType() & kPC_OBBObject) || !((cPhysOBBModel *)pClsn->GetModel2())->IsEdgeTrigger())
      {
         g_CollisionObj1 = pClsn->GetObjID();
         g_CollisionSubmod1 = pClsn->GetSubModId();
         g_CollisionObj2 = pClsn->GetObjID2();
         g_CollisionSubmod2 = pClsn->GetSubModId2();

         bits = CollideTest(pClsn->GetObjID(), pClsn->GetObjID2());
         if (bits != COLLISION_NONE)
         {
            PosPropLock = TRUE;
            if (!(pClsn->GetModel()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID())))
            {
               if (((pClsn->GetModel()->GetType(0) == kPMT_Sphere || pClsn->GetModel()->GetType(0) == kPMT_Point) &&
                    (pClsn->GetModel()->NumSubModels() == 1)) ||
                   (pClsn->GetModel()->GetType(0) == kPMT_SphereHat))
               {
                  ObjPosUpdate(pClsn->GetObjID(), &pClsn->GetModel()->GetLocationVec(),
                               &pClsn->GetModel()->GetRotation());
               }
            }

            if (!(pClsn->GetModel2()->IsCreature() && CreatureSelfPropelled(pClsn->GetObjID2())))
            {
               if (((pClsn->GetModel2()->GetType(0) == kPMT_Sphere || pClsn->GetModel2()->GetType(0) == kPMT_Point) &&
                    (pClsn->GetModel2()->NumSubModels() == 1)) ||
                   (pClsn->GetModel2()->GetType(0) == kPMT_SphereHat))
               {
                  ObjPosUpdate(pClsn->GetObjID2(), &pClsn->GetModel2()->GetLocationVec(),
                               &pClsn->GetModel2()->GetRotation());
               }
            }
            PosPropLock = FALSE;

            AUTO_TIMER(PHYS_CollideEvent);
            flags = CollideEvent(pClsn->GetObjID(), pClsn->GetObjID2(), bits, kinetic_energy, pClsn);
         }
      }
      else
      {
         flags = 0;
         bits = 0;
      }
   }

   result = kPCR_Nothing;
   if (flags & COLLISION_NON_PHYS_US)
      result = kPCR_NonPhys1;
   if (flags & COLLISION_NON_PHYS_THEM)
   {
      if (result == kPCR_NonPhys1)
         result = kPCR_NonPhysBoth;
      else
         result = kPCR_NonPhys2;
   }

   if (result != kPCR_Nothing)
      return result;

   static int count = 0;

   // Set flags if either should die in collision or we should hit at all
   if ((pClsn->GetType() & kPC_OBBObject) && ((cPhysOBBModel *)pClsn->GetModel2())->IsEdgeTrigger())
   {
      result = kPCR_Nothing;

      if (pClsn->GetSubModId2() < 0)
         PhysMessageExit(pClsn->GetModel2(), pClsn->GetObjID(), pClsn->GetSubModId());
      else
         PhysMessageEnter(pClsn->GetModel2(), pClsn->GetObjID(), pClsn->GetSubModId());
   }
   else
   {
      if ((flags & COLLISION_NOTHING_US) || (bits & COLLISION_NOTHING_US) ||
          (flags & COLLISION_NOTHING_THEM) || (bits & COLLISION_NOTHING_THEM))
         result = kPCR_Nothing;
      else
         result = kPCR_Bounce;

      if ((flags & COLLISION_KILL_US) || (bits & COLLISION_KILL_US) ||
          (flags & COLLISION_SLAY_US) || (bits & COLLISION_SLAY_US))
         result = kPCR_Die1;
      if ((flags & COLLISION_KILL_THEM) || (bits & COLLISION_KILL_THEM) ||
          (flags & COLLISION_SLAY_THEM) || (bits & COLLISION_SLAY_THEM))
      {
         if (result == kPCR_Die1)
            result = kPCR_DieBoth;
         else
            result = kPCR_Die2;
      }
   }

   // Collision messaging
   if ((result != kPCR_Nothing) && (result != kPCR_NonPhys1) &&
       (result != kPCR_NonPhys2) && (result != kPCR_NonPhysBoth))
   {
      int reply1, reply2;

      if (pClsn->GetType() & kPC_Object)
         PhysMessageCollision(pClsn, mx_mag_vec(&momentum), &reply1, &reply2);
      else
         PhysMessageCollision(pClsn, dot, &reply1, &reply2);

      switch(reply1)
      {
         case kPM_StatusQuo: break;
         case kPM_Nothing:   result = kPCR_Nothing; break;
         case kPM_Bounce:    result = kPCR_Bounce; break;
         case kPM_Slay:
         {
            if (reply2 == kPM_Slay)
               result = kPCR_DieBoth;
            else
               result = kPCR_Die1;
            break;
         }
         case kPM_NonPhys:
         {
            if (reply2 == kPM_NonPhys)
               result = kPCR_NonPhysBoth;
            else
               result = kPCR_NonPhys1;
            break;
         }
      }
      switch(reply2)
      {
         case kPM_StatusQuo: break;
         case kPM_Nothing:   result = kPCR_Nothing; break;
         case kPM_Bounce:    result = kPCR_Bounce; break;
         case kPM_Slay:
         {
            if (reply1 == kPM_Slay)
               result = kPCR_DieBoth;
            else
               result = kPCR_Die2;
            break;
         }
         case kPM_NonPhys:
         {
            if (reply1 == kPM_NonPhys)
               result = kPCR_NonPhysBoth;
            else
               result = kPCR_NonPhys2;
            break;
         }
      }
   }

   return result;
}

static void ResolveCollision(cPhysClsn * pClsn, mxs_real dt)
{
   cPhysModel *pModel = pClsn->GetModel();
   cPhysModel *pModel2 = NULL;

   if (!(pClsn->GetType()&kPC_Terrain))
      pModel2 = pClsn->GetModel2();

   // This is gross, but it keeps doors from popping off their hinges
   if (pClsn->GetType() & kPC_Object)
   {
      if (pModel->IsDoor())
      {
         sDoorProp *pDoorProp = GetDoorProperty(pModel->GetObjID());

         if (pModel2->GetDynamics()->GetMass() > pDoorProp->push_mass)
            pModel->SetEndRotationVec(pModel->GetRotation());
      }
      else
      if (pModel2->IsDoor())
      {
         sDoorProp *pDoorProp = GetDoorProperty(pModel2->GetObjID());
         AssertMsg1(pDoorProp, "ResolveCollision: Object %d passed IsDoor() and is missing door property.", pModel2->GetObjID());
         if (pModel->GetDynamics()->GetMass() > pDoorProp->push_mass)
            pModel2->SetEndRotationVec(pModel2->GetRotation());
      }
   }

   // Clear velocity changed flags
   pModel->GetDynamics()->ClearVelocityChanged();
   if (!(pClsn->GetType()&kPC_Terrain))
      pModel2->GetDynamics()->ClearVelocityChanged();

   // Integrate our position to the collision
   mxs_vector normal;

   if (pClsn->GetType() & kPC_Terrain)
      normal = pClsn->GetNormal();
   else
      mx_zero_vec(&normal);

   mxs_real integration_backup;

   if (((pModel->GetType(0) == kPMT_OBB) && ((cPhysOBBModel *)pModel)->IsEdgeTrigger()) ||
       ((pClsn->GetType() & kPC_Object) &&
        (pClsn->GetModel2()->GetType(0) == kPMT_OBB) && ((cPhysOBBModel *)pClsn->GetModel2())->IsEdgeTrigger()))
      integration_backup = 1.0;
   else
      integration_backup = .9;

   IntegrateToCollision(pModel,
      pClsn->GetSubModId(),
      pModel->GetDynamics(),
      pClsn->GetLoc(),
      pClsn->GetT0(),
      pClsn->GetDT(), normal,
      integration_backup);
   // integrate second model, if needed
   if (pClsn->GetType() & kPC_Object)
      IntegrateToCollision(pClsn->GetModel2(),
         pClsn->GetSubModId2(),
         pClsn->GetModel2()->GetDynamics(),
         pClsn->GetLoc2(),
         pClsn->GetT0(),
         pClsn->GetDT(), normal,
         integration_backup);

   // Set the collision result
   pClsn->SetResult(DetermineCollisionResult(pClsn));

   if (pModel->IsMantling())
   {
      BreakMantle(pClsn->GetObjID());
      delete pClsn;
      return;
   }
   if ((pClsn->GetType() & kPC_Object) && pClsn->GetModel2()->IsMantling())
   {
      BreakMantle(pClsn->GetObjID2());
      delete pClsn;
      return;
   }

   // Check if we've become non-physical
   switch (pClsn->GetResult())
   {
      case kPCR_NonPhys1:
      case kPCR_NonPhys2:
      case kPCR_NonPhysBoth:
         delete pClsn;
         return;
   }

   // Check if we're stepping
   if ((pClsn->GetType() == kPC_TerrainFace) && (fabs(pClsn->GetNormal().z) < 0.4))
   {
      if ((pModel->IsPlayer() || pModel->IsAvatar()) &&
          ((pClsn->GetSubModId() == PLAYER_FOOT) ||
           (pClsn->GetSubModId() == PLAYER_SHIN) ||
           (pClsn->GetSubModId() == PLAYER_KNEE)))
      {
         if (CheckStep(pClsn, OBJ_NULL))
         {
            PostCollisionUpdate(pClsn->GetObjID(), pModel, pClsn->GetTime(), dt - pClsn->GetTime());
            delete pClsn;
            return;
         }
      }
   }
   else
   if (pClsn->GetType() & kPC_Object)
   {
      if ((pModel->IsPlayer() || pModel->IsAvatar()) &&
          ((pClsn->GetSubModId() == PLAYER_FOOT) ||
           (pClsn->GetSubModId() == PLAYER_SHIN) ||
           (pClsn->GetSubModId() == PLAYER_KNEE)) &&
          (pClsn->GetModel2()->GetType(0) == kPMT_OBB))
      {

         if (((cPhysOBBModel *)pClsn->GetModel2())->GetClimbableSides() == 0)
         {
            mxs_vector normal;

            ((cPhysOBBModel *)pClsn->GetModel2())->GetNormal(pClsn->GetSubModId2(), &normal);
            if (normal.z < 0.4)
            {
               if (CheckStep(pClsn, pClsn->GetObjID2()))
               {
                  PostCollisionUpdate(pClsn->GetObjID(), pModel, pClsn->GetTime(), dt - pClsn->GetTime());
                  delete pClsn;
                  return;
               }
            }
         }
         else
            pClsn->SetResult(kPCR_Nothing);
      }
      else
      if ((pClsn->GetModel2()->IsPlayer() || pClsn->GetModel2()->IsAvatar()) &&
          ((pClsn->GetSubModId2() == PLAYER_FOOT) ||
           (pClsn->GetSubModId2() == PLAYER_SHIN) ||
           (pClsn->GetSubModId2() == PLAYER_KNEE)) &&
          (pModel->GetType(0) == kPMT_OBB))
      {
         mxs_vector normal;

         ((cPhysOBBModel *)pClsn->GetModel2())->GetNormal(pClsn->GetSubModId2(), &normal);
         if (normal.z < 0.4)
         {
            if (CheckStep(pClsn, pClsn->GetObjID()))
            {
               PostCollisionUpdate(pClsn->GetObjID2(), pClsn->GetModel2(), pClsn->GetTime(), dt - pClsn->GetTime());
               delete pClsn;
               return;
            }
         }
      }
   }

   switch (pClsn->GetResult())
   {
      case kPCR_Bounce:
      case kPCR_Die1:
      case kPCR_Die2:
         ResolveBounce(pClsn, dt);
         break;
      case kPCR_Nothing:
      case kPCR_DieBoth:
         break;
      default:
         Warning(("ResolveCollision: unknown collision result %d\n",
                  pClsn->GetResult()));
         delete pClsn;
         return;
   }

   switch (pClsn->GetResult())
   {
      case kPCR_Die1:
         PhysDeregisterModel(pClsn->GetObjID());
         break;
      case kPCR_Die2:
         PhysDeregisterModel(pClsn->GetObjID2());
         break;
      case kPCR_DieBoth:
         PhysDeregisterModel(pClsn->GetObjID());
         PhysDeregisterModel(pClsn->GetObjID2());
         break;
   }

   BOOL want_to_update_1 = pModel->GetDynamics()->VelocityChanged();
   BOOL want_to_update_2 = (pClsn->GetType() & kPC_Object) &&
      (pModel2->GetDynamics()->VelocityChanged());

   BOOL is_physical_1 = PhysObjHasPhysics(pClsn->GetObjID());
   BOOL is_physical_2 = (pClsn->GetType() & kPC_Object) && PhysObjHasPhysics(pClsn->GetObjID2());

   switch (pClsn->GetType())
   {
      case kPC_SphereHatObject:
      case kPC_OBBObject:
      case kPC_MoveableObject:
      {
         if (is_physical_1 && want_to_update_1 && is_physical_2 && want_to_update_2)
            PostCollisionUpdate(pClsn->GetObjID(), pModel, pClsn->GetObjID2(), pModel2, pClsn->GetTime(), dt - pClsn->GetTime());
         else
         if (is_physical_1 && want_to_update_1)
            PostCollisionUpdate(pClsn->GetObjID(), pModel, pClsn->GetTime(), dt - pClsn->GetTime());
         else
         if (is_physical_2 && want_to_update_2)
            PostCollisionUpdate(pClsn->GetObjID2(), pModel2, pClsn->GetTime(), dt - pClsn->GetTime());
         break;
      }

      case kPC_TerrainVertex:
      case kPC_TerrainEdge:
      case kPC_TerrainFace:
      {
         if (PhysObjHasPhysics(pClsn->GetObjID()))
            PostCollisionUpdate(pClsn->GetObjID(), pModel, pClsn->GetTime(), dt - pClsn->GetTime());
         break;
      }
   }

   delete pClsn;
}

///////////////////////////////////////
//
// Resolve all collisions for the frame
//

#ifdef CLSN_COUNT
int clsn_count = 0;
#endif

static void ResolveCollisions(mxs_real dt)
{
   cPhysClsn * pClsn;
   int i=0;

   pClsn = g_PhysClsns.GetFirst();
   while (pClsn != NULL)
   {
      g_PhysClsns.Remove(pClsn);
      if (pClsn->GetDT() > 0)
      {
         #ifdef CLSN_COUNT
         clsn_count++;
         #endif

         ResolveCollision(pClsn, dt);
      }
      else
      {
         if ((pClsn->GetType())&kPC_Object)
            Warning(("ResolveCollisions: non-positive dt (%g) (%s vs %s)\n",
                     pClsn->GetDT(), ObjWarnName(pClsn->GetObjID()), ObjWarnName(pClsn->GetObjID2())));
         else
            Warning(("ResolveCollisions: non-positive dt (%g) (%s)\n",
                     pClsn->GetDT(), ObjWarnName(pClsn->GetObjID())));
         delete pClsn;
      }
      pClsn = g_PhysClsns.GetFirst();
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// High level flow
//

//
// Zero acceleration, always do before updating controls & dynamics
//

static void ZeroAcceleration(cPhysModel *pModel)
{
   pModel->GetDynamics()->ZeroAcceleration();

   for (int i=0; i<pModel->NumSubModels(); i++)
      pModel->GetDynamics(i)->ZeroAcceleration();
}

//
// Run frame startup
//

static void StartFrame(mxs_real dt)
{
   int i;
   cPhysModel   *pModel = g_PhysModels.GetFirstMoving();
   cPhysDynData *pDyn;

   #ifdef CLSN_COUNT
   mprintf("collisions last frame = %d\n", clsn_count);
   clsn_count = 0;
   #endif

   // iterate over all moveable models
   while (pModel != NULL)
   {
      pDyn = pModel->GetDynamics();

      pModel->SetCollisionChecked(FALSE);
      pModel->SetSquishingState(FALSE);

      pDyn->SetFrozen(FALSE);
      pDyn->ZeroAcceleration();

      if (pModel->ObjectPassThru())
         pModel->DecObjectPassThru(dt);

      for (i=0; i<pModel->NumSubModels(); i++)
      {
         pDyn = pModel->GetDynamics(i);

         pDyn->ZeroCollisionCount();
         pDyn->ZeroAcceleration();
      }
      pModel = g_PhysModels.GetNextMoving(pModel);
   }


   pModel = g_PhysModels.GetFirstActive();
   while (pModel)
   {
      pDyn = pModel->GetDynamics();

      pDyn->ZeroCollisionCount();
      pDyn->SetCurrentTime(0);

      pModel = g_PhysModels.GetNextActive(pModel);
   }
}

///////////////////////////////////////
//
// End the frame by updating all positions to calculated end positions
//

static void UpdatePositions()
{
   cPhysModel   *pModel = g_PhysModels.GetFirstMoving();
   cPhysDynData *pDynamics;
   int i;

   static mxs_vector me = {0, 0, 0};

   PosPropLock++;

   // iterate over all moveable models
   while (pModel != NULL)
   {
      pDynamics = pModel->GetDynamics();

      // Objects created after the dynamics update will not have an
      //  endlocation, so ignore them this time around
      if (!IsZeroVector(pModel->GetEndLocationVec()) || !pModel->IsAttached())
      {
         mxs_angvec ang_limit;
         mxs_vector trans_limit;

         pModel->UpdateMedium();

         if (pModel->FacesVel())
         {
            // Projectiles should always face their direction of motion
            mxs_vector vel;

            mx_copy_vec(&vel, &pModel->GetVelocity());

            if (mx_mag2_vec(&vel) < 0.0001)
               pModel->SetRotation(pModel->GetEndRotationVec());
            else
            {
               mxs_matrix orien;
               mxs_angvec rot;

               mx_norm_vec(&orien.vec[0], &vel);
               mx_unit_vec(&orien.vec[2], 2);
               mx_cross_vec(&orien.vec[1], &orien.vec[2], &orien.vec[0]);
               mx_cross_vec(&orien.vec[2], &orien.vec[0], &orien.vec[1]);

               mx_mat2ang(&rot, &orien);
               pModel->SetRotation(rot);
            }
         }
         else
         {
            // Check for rotation limits
            ang_limit = pModel->GetEndRotationVec();
            pModel->CheckAngleLimits(pModel->GetRotation(), pModel->GetEndRotationVec(), &ang_limit);
            pModel->SetRotation(ang_limit, TRUE);
         }

         // Back up a bit to keep the spherecaster's epsilon happy
         mxs_vector move_vec;
         mxs_vector end_vec;
         mxs_vector move_backup;
         mxs_real   move_len;

         mx_sub_vec(&move_vec, &pModel->GetEndLocationVec(), &pModel->GetLocationVec());
         move_len = mx_norm_vec(&move_backup, &move_vec);

         if ((move_len > 0.0) && !pModel->IsDoor() && !pModel->IsGhost())
         {
            mx_scaleeq_vec(&move_backup, -(min(0.01, move_len)));
            mx_addeq_vec(&move_vec, &move_backup);

            mx_add_vec(&end_vec, &pModel->GetLocationVec(), &move_vec);

            pModel->SetEndLocationVec(end_vec);
         }
         else
            mx_zero_vec(&move_backup);

         // Check for translation limits
         mx_copy_vec(&trans_limit, &pModel->GetEndLocationVec());
         pModel->CheckTransLimits(pModel->GetLocationVec(), pModel->GetEndLocationVec(), &trans_limit);
         pModel->SetLocationVec(trans_limit, TRUE);

         if (pModel->NumAttachments() > 0)
         {
            cAutoLinkQuery query(g_pPhysAttachRelation, pModel->GetObjID());

            for (; !query->Done(); query->Next())
            {
               cPhysModel *pAttachModel = g_PhysModels.GetActive(query.GetDest());

               if (pAttachModel)
               {
                  mxs_vector loc;
                  mx_add_vec(&loc, &trans_limit, &(((sPhysAttachData *)query->Data())->offset));
                  pAttachModel->SetLocationVec(loc, TRUE);
               }
            }
         }

         // Check for media transitions
         pModel->SetEndLocationVec(trans_limit);

         // Update submodel locations
         for (i=0; i<pModel->NumSubModels(); i++)
         {
            if (pModel->GetSpringTension(i) == 0.0)
            {
               mxs_vector submod_loc;
               mx_add_vec(&submod_loc, &pModel->GetEndLocationVec(i), &move_backup);

               pModel->SetLocationVec(i, submod_loc);
            }
            else
               pModel->SetLocationVec(i, pModel->GetEndLocationVec(i));

            if (pModel->GetType(0) != kPMT_OBB)
               pModel->SetRotation(i, pModel->GetEndRotationVec(i));
         }

         ObjPosUpdate(pModel->GetObjID(), &pModel->GetLocationVec(), &pModel->GetRotation());

         if (pModel->IsRopeClimbing())
            pModel->ComputeRopeSegPct();
      }
      pDynamics->SetCurrentTime(0); // Ever used?

      pModel = g_PhysModels.GetNextMoving(pModel);
   }

   PosPropLock--;
}

////////////////////////////////////////

// We might want to validate the positions of all moving models, but for
// now, we'll just validate the player and creatures

static void ValidatePositions()
{
   cPhysModel *pModel;

   // The player
   if (PlayerObjectExists())
   {
      pModel = g_PhysModels.Get(PlayerObject());

      if (pModel)
      {
         mxs_real radius = ((cPhysSphereModel *)pModel)->GetRadius(PLAYER_HEAD);

         if (!SphrSphereInWorld((Location *)&pModel->GetLocation(PLAYER_HEAD), radius, 0))
         {
            Location test_loc;

            // First try its target location
            MakeHintedLocationFromVector(&test_loc, &pModel->GetTargetLocation(PLAYER_HEAD),
                                                    &pModel->GetLocation(PLAYER_HEAD));

            if (SphrSphereInWorld(&test_loc, radius, 0))
            {
               pModel->SetLocationVec(PLAYER_HEAD, test_loc.vec);
               return;
            }

            // Otherwise, try moving along the major axes
            float dist;
            int x, y, z;

            mxs_vector test_vec;
            mxs_matrix ident;
            mx_identity_mat(&ident);

            for (dist = 0.2; dist < 1.01; dist += 0.2)
            {
               for (x=0; x<2; x++)
               {
                  for (y=0; y<2; y++)
                  {
                     for (z=0; z<2; z++)
                     {
                        test_vec = pModel->GetTargetLocation(PLAYER_HEAD);

                        mx_scale_addeq_vec(&test_vec, &ident.vec[0], dist * ((x==0) ? 1 : -1));
                        mx_scale_addeq_vec(&test_vec, &ident.vec[1], dist * ((y==0) ? 1 : -1));
                        mx_scale_addeq_vec(&test_vec, &ident.vec[2], dist * ((z==0) ? 1 : -1));

                        MakeHintedLocationFromVector(&test_loc, &test_vec,
                                                     &pModel->GetLocation(PLAYER_HEAD));

                        if (SphrSphereInWorld(&test_loc, radius, 0))
                        {
                           pModel->SetLocationVec(PLAYER_HEAD, test_vec);
                           goto player_done;
                        }
                     }
                  }
               }
            }

            // That didn't work, so just slam the player's head to their center model
            pModel->SetLocationVec(PLAYER_HEAD, pModel->GetLocationVec(PLAYER_BODY));
         }
      }
   }

 player_done:

   // Creatures

   static int creature_and = 0x00;
   const int creature_mask = 0x0F;

   creature_and = (creature_and + 1) & creature_mask;

   int max_chandle = max_chandle_id();
   for (int i=0; i<max_chandle; i++)
   {
      sCreatureHandle *pCHandle = CreatureHandle(i);
      if (!pCHandle)
         continue;

      cCreature *pCreature = pCHandle->pCreature;
      if (!pCreature)
         continue;

      if (IsRemoteGhost(pCreature->GetObjID()))
         continue;

      if ((pCreature->GetObjID() & creature_mask) == creature_and)
      {
         // check each submodel
         cPhysModel *pModel = g_PhysModels.Get(pCreature->GetObjID());
         if (!pModel)
            continue;

         for (int j=0; j<pModel->NumSubModels(); j++)
         {
            const Location *pLoc = &pModel->GetLocation(j);

            float radius = ((cPhysSphereModel *)pModel)->GetRadius(j);

            if (!SphrSphereInWorld((Location *)pLoc, radius, 0))
            {
#ifndef SHIP
               if ( config_is_defined( "BadCretPosSpew" ) ) {
                  mprintf("Correcting %s for bad cret pos, submod %d\n", ObjEditName(pModel->GetObjID()), j);
               }
#endif
               // we can just move the submodel a little, because the next creature
               // update will spherecast from where it is to where it wants to go, and
               // should detect a collision and recover

               Location test_loc;
               mxs_vector test_vec;

               mxs_matrix ident;
               mx_identity_mat(&ident);

               float dist;
               int x, y, z;

               for (dist = 0.2; dist < 1.01; dist += 0.2)
               {
                  for (x=0; x<2; x++)
                  {
                     for (y=0; y<2; y++)
                     {
                        for (z=0; z<2; z++)
                        {
                           test_vec = pModel->GetLocationVec(j);

                           mx_scale_addeq_vec(&test_vec, &ident.vec[0], dist * ((x==0) ? 1 : -1));
                           mx_scale_addeq_vec(&test_vec, &ident.vec[1], dist * ((y==0) ? 1 : -1));
                           mx_scale_addeq_vec(&test_vec, &ident.vec[2], dist * ((z==0) ? 1 : -1));

                           MakeHintedLocationFromVector(&test_loc, &test_vec,
                                                        &pModel->GetLocation(j));

                           if (SphrSphereInWorld(&test_loc, radius, 0))
                           {
                              pModel->SetLocationVec(j, test_vec);
                              goto submodel_fixed;
                           }
                        }
                     }
                  }
               }
            }
         submodel_fixed:
            ;
         }
      }
   }
}

///////////////////////////////////////
//
// Run one frame of specified length, in seconds
//

DECLARE_TIMER(PHYS_ValidatePos, Average);
DECLARE_TIMER(PHYS_StartFrame, Average);
DECLARE_TIMER(PHYS_UpdateControls, Average);
DECLARE_TIMER(PHYS_UpdateDynamics, Average);
DECLARE_TIMER(PHYS_CheckObjColls, Average);
DECLARE_TIMER(PHYS_CheckTerrColls, Average);
DECLARE_TIMER(PHYS_ResColls, Average);
DECLARE_TIMER(PHYS_UpdatePos, Average);


static void PhysicsFrame(mxs_real dt)
{
   #ifndef SHIP
   if (config_is_defined("SphereCheckSpew") || config_is_defined("PointCheckSpew"))
      mprintf("\n");
   #endif

   AutoAppIPtr_(ObjectSystem, pObjSys);
   pObjSys->Lock();
   CreaturePosPropLock();

   g_PhysModels.ApplyDeferredVelocities();

   TIMER_Start(PHYS_ValidatePos);
   ValidatePositions();
   TIMER_MarkStop(PHYS_ValidatePos);

   TIMER_Start(PHYS_StartFrame);
   StartFrame(dt);
   TIMER_MarkStop(PHYS_StartFrame);

   TIMER_Start(PHYS_UpdateControls);
   UpdateControls(dt);
   TIMER_MarkStop(PHYS_UpdateControls);

   TIMER_Start(PHYS_UpdateDynamics);
   UpdateDynamics(dt);
   TIMER_MarkStop(PHYS_UpdateDynamics);

   TIMER_Start(PHYS_CheckTerrColls);
   CheckTerrainCollisions(0, dt);
   TIMER_MarkStop(PHYS_CheckTerrColls);

   TIMER_Start(PHYS_CheckObjColls);
   CheckObjectCollisions(0, dt);
   TIMER_MarkStop(PHYS_CheckObjColls);

   TIMER_Start(PHYS_ResColls);
   ResolveCollisions(dt);
   TIMER_MarkStop(PHYS_ResColls);

   TIMER_Start(PHYS_UpdatePos);
   UpdatePositions();
   TIMER_MarkStop(PHYS_UpdatePos);

#ifdef PER_FRAME_TIMING
   mprintf("------------------------------------\n");

   TIMER_OutputResult(PHYS_ValidatePos);
   TIMER_OutputResult(PHYS_StartFrame);
   TIMER_OutputResult(PHYS_UpdateControls);
   TIMER_OutputResult(PHYS_UpdateDynamics);
   TIMER_OutputResult(PHYS_CheckObjColls);
   TIMER_OutputResult(PHYS_CheckTerrColls);
   TIMER_OutputResult(PHYS_ResColls);
   TIMER_OutputResult(PHYS_UpdatePos);

   TIMER_Clear(PHYS_StartFrame);
   TIMER_Clear(PHYS_UpdateControls);
   TIMER_Clear(PHYS_UpdateDynamics);
   TIMER_Clear(PHYS_CheckObjColls);
   TIMER_Clear(PHYS_CheckTerrColls);
   TIMER_Clear(PHYS_ResColls);
   TIMER_Clear(PHYS_UpdatePos);
#endif

   pObjSys->Unlock();
   CreaturePosPropUnlock();
}

void PhysClearTimers()
{
   TIMER_Clear(PHYS_ValidatePos);
   TIMER_Clear(PHYS_StartFrame);
   TIMER_Clear(PHYS_UpdateControls);
   TIMER_Clear(PHYS_UpdateDynamics);
   TIMER_Clear(PHYS_CheckObjColls);
   TIMER_Clear(PHYS_CheckTerrColls);
   TIMER_Clear(PHYS_ResColls);
   TIMER_Clear(PHYS_UpdatePos);

   TIMER_Clear(PHYS_CollideEvent);
#ifdef OBJ_COLL_TIMING
   TIMER_Clear(ObjColl);
   TIMER_Clear(ObjColl2);
#endif
}

///////////////////////////////////////

DECLARE_TIMER(PHYS_Total, Average);

static LazyAggMember(INetManager) gpNetMan;

void PhysUpdate(long dt_msec)
{
   AUTO_TIMER(PHYS_Total);

   static int accum_ms = 0;

   sim_frames++;

   if (dt_msec == 0)
      return;

   int frame_count;
   int max_frame_count;

   if (net_cap_physics && gpNetMan->Networking())
   {
      frame_count = 0;
      max_frame_count = 10;
   }
   else
      max_frame_count = 0;

   accum_ms += dt_msec;

   if (accum_ms > kMaxFrameLen)
   {
      while ((accum_ms > kMaxFrameLen) && ((max_frame_count == 0) || (frame_count < max_frame_count)))
      {
         frame_count++;
         phys_frames++;
         PhysicsFrame((mxs_real) kMaxFrameLen / 1000);
         accum_ms -= kMaxFrameLen;
      }
      if ((max_frame_count > 0) && (frame_count == max_frame_count))
         accum_ms = 0;
   }
   else
   {
      phys_frames++;
      PhysicsFrame((mxs_real) accum_ms / 1000);
      accum_ms = 0;
   }

#if 0
   cAutoLinkQuery query(g_pPhysAttachRelation);

   for (; !query->Done(); query->Next())
   {
      sLink link;
      sPhysAttachData *pAttachData;
      query->Link(&link);
      pAttachData = (sPhysAttachData *)query->Data();

      mxs_vector delta;
      mx_sub_vec(&delta, &ObjPosGet(link.dest)->loc.vec, &ObjPosGet(link.source)->loc.vec);

      mprintf("delta: %g %g %g  -  offset: %g %g %g\n", delta.x, delta.y, delta.z,
              pAttachData->offset.x, pAttachData->offset.y, pAttachData->offset.z);
   }
#endif
}

///////////////////////////////////////////////////////////////////////////////



