/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodsph.cpp,v 1.96 1999/08/17 17:31:38 ccarollo Exp $
//
//
//

//#define PROFILE_ON

#include <lg.h>
#include <config.h>
#include <timings.h>
#include <allocapi.h>

#include <matrixs.h>
#include <matrix.h>

#include <portal.h>  // For raycast
#include <wr.h>      // For PortalRaycastCell

#include <phcore.h>
#include <phmodsph.h>
#include <phmodobb.h>
#include <phdyn.h>
#include <phclsn.h>
#include <phutils.h>
#include <phconst.h>
#include <phutils.h>
#include <phmodutl.h>
#include <phflist.h>
#include <phref.h>

#include <sphrcst.h>
#include <sphrcsts.h>

// Must be last header
#include <dbmem.h>

int inworld_checks;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysSphereModel
//
// @TBD (toml 08-05-97): why only one radius, with many submodels?

cPhysSphereModel::cPhysSphereModel(ObjID objID, tPhysSubModId numSubModels, BOOL hat, unsigned flags,
                                 mxs_real radius, mxs_real elasticity)
 : cPhysModel(objID, hat ? kPMT_SphereHat : kPMT_Sphere, numSubModels, flags)
{
   cPhysDynData * pDynamics;
   int            i;

   if (hat)
      numSubModels = 2;

   LGALLOC_PUSH_CREDIT();
   m_radius.SetSize(numSubModels);
   LGALLOC_POP_CREDIT();

   for (i = 0; i < NumSubModels(); i++)
   {
      if (hat)
      {
         m_radius[i] = (i == 1) ? -1 : radius;
         SetType(i, kPMT_SphereHat);
      }
      else
      {
         m_radius[i] = radius;
         SetType(i, (radius < 0.0001) ? kPMT_Point : kPMT_Sphere);
      }

      pDynamics = GetDynamics(i);
      pDynamics->SetTranslatable(TRUE);
      pDynamics->SetRotatable(TRUE);
      // @TBD (toml 08-05-97): should we set the dynamics elasticity here?
   }
}

///////////////////////////////////////

cPhysSphereModel::cPhysSphereModel(PhysReadWrite func) : cPhysModel(func)
{

   // Read sphere-specific data
   LGALLOC_PUSH_CREDIT();
   m_radius.SetSize(NumSubModels());
   LGALLOC_POP_CREDIT();
   func((void *)&m_radius[0], sizeof(mxs_real), NumSubModels());

   if ((NumSubModels() == 2) && (m_radius[1] == -1))
   {
      for (int i=0; i<NumSubModels(); i++)
         SetType(i, kPMT_SphereHat);
   }
   else
   {
      for (int i=0; i<NumSubModels(); i++)
         SetType(i, (m_radius[i] < 0.0001) ? kPMT_Point : kPMT_Sphere);
   }
}

///////////////////////////////////////

cPhysSphereModel::~cPhysSphereModel()
{
   m_radius.SetSize(0);
}

///////////////////////////////////////

void cPhysSphereModel::Write(PhysReadWrite func) const
{
   // Write model-common data
   cPhysModel::Write(func);

   // Write sphere-specific data
   func((void *)&m_radius[0], sizeof(mxs_real), NumSubModels());
}


///////////////////////////////////////
//
// Check terrain collisions for a sub-model
//

BOOL cPhysSphereModel::CheckTerrainCollision(tPhysSubModId i, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn)
{
   cPhysDynData *  pDynamics = GetDynamics(i);
   ePhysClsnType   type;
   sSphrContact *  pContact;
   sSphrContactData * pContactData;
   int             contactCount;
   mxs_real        time;
   Location        start_loc;
   Location        end_loc;

   if ((GetType(i) == kPMT_SphereHat) && (i == 1))
   {
      ppClsn = NULL;
      return FALSE;
   }

   ComputeCellForLocation((Location *)&GetLocation(i));
   MakeHintedLocationFromVector(&start_loc, &GetLocationVec(i), &GetLocation(i));
   MakeHintedLocationFromVector(&end_loc, &GetEndLocationVec(i), &GetLocation(i));
      
   if ((start_loc.hint == CELL_INVALID) && HasObjectWarned(GetObjID()))
   {
      *ppClsn = NULL;
      return FALSE;
   } 

   mxs_real radius = m_radius[i];

   // Ghosts only care about submodel 0, and even then vs terrain, they are points
   if (IsFancyGhost())
   {
      if (i != 0)
      {
         *ppClsn = NULL;
         return FALSE;
      }
      else
         radius = (m_radius[i] > 1.0) ? 1.0 : m_radius[i];
   }

   // Special-case Point models
   if ((GetType(i) == kPMT_Point) || IsPointVsTerrain())
   {
      Location   hit;
      mxs_vector total_segment;
      mxs_vector inside_segment;
      mxs_real   mag2_total, mag2_inside;
      mxs_real   coll_time;

      #ifndef SHIP
      if (inworld_checks && !IsLocationControlled())
      {
         Location backup_loc;

         MakeHintedLocationFromVector(&backup_loc, &start_loc.vec, &start_loc);
         if (config_is_defined("PointCheckSpew"))
            mprintf("[%d] Checking pt start loc %g %g %g\n", GetObjID(), start_loc.vec.x, start_loc.vec.y, start_loc.vec.z);
         if (!HasObjectWarned(GetObjID()) && CellFromLoc(&start_loc) == CELL_INVALID)
         {
            SetObjectWarned(GetObjID());
            mprintf("Point (%s - %d) out of world!\n", ObjWarnName(GetObjID()), i);
         }
      }
      #endif

      if (!PortalRaycast(&start_loc, &end_loc, &hit, TRUE))
      {
         #ifndef SHIP
         if (inworld_checks && !IsLocationControlled())
         {
            if (config_is_defined("PointCheckSpew"))
               mprintf("[%d] Checking pt hit loc %g %g %g\n", GetObjID(), hit.vec.x, hit.vec.y, hit.vec.z);
            if (!HasObjectWarned(GetObjID()) && CellFromLoc(&hit) == CELL_INVALID) 
            {
               SetObjectWarned(GetObjID());
               mprintf("Point (%s - %d) out of world!\n", ObjWarnName(GetObjID()), i);
            }
         }
         #endif

         // Calculate time by distance along segment
         mx_sub_vec(&total_segment, &end_loc.vec, (mxs_vector *) &GetLocation(i).vec);
         mx_sub_vec(&inside_segment, &hit.vec, (mxs_vector *) &GetLocation(i).vec);
         mag2_total = mx_mag2_vec(&total_segment);
         mag2_inside = mx_mag2_vec(&inside_segment);
         if (fabs(mag2_total) > 0)
            coll_time = dt * (sqrt(mag2_inside / mag2_total));
         else
            coll_time = 0;

         // Make sure raycast is okay
         if ((PortalRaycastCell == -1) || (PortalRaycastFindPolygon() == -1))
         {
            if (!HasObjectWarned(GetObjID()))
            {
               Warning(("Invalid cell (%d) or polygon (%d) after raycast\n         (%g %g %g) to (%g %g %g)\n", 
                        PortalRaycastCell, PortalRaycastFindPolygon(), 
                        GetLocation(i).vec.x, GetLocation(i).vec.y, GetLocation(i).vec.z, 
                        end_loc.vec.x, end_loc.vec.y, end_loc.vec.z));
            }
            *ppClsn = NULL;
            return FALSE;
         }

         // And create the collision
         *ppClsn = new cPhysClsn(kPC_TerrainFace, t0, coll_time, hit.vec,
                                 this, i, hit.vec, 
                                 PortalRaycastCell, PortalRaycastFindPolygon());

         return TRUE;
      }
      else
      {
         #ifndef SHIP
         if (inworld_checks && !IsLocationControlled())
         {
            Location temp;

            if (config_is_defined("PointCheckSpew"))
               mprintf("[%d] Checking pt end loc %g %g %g\n", GetObjID(), end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);

            MakeHintedLocationFromVector(&temp, &end_loc.vec, &end_loc);
            if (!HasObjectWarned(GetObjID()) && CellFromLoc(&end_loc) == CELL_INVALID)
            {
               SetObjectWarned(GetObjID());
               mprintf("Point (%s - %d) out of world!\n", ObjWarnName(GetObjID()), i);
            }
         }
         #endif

         *ppClsn = NULL;
         return FALSE;
      }
   }

   #ifndef SHIP
   if (inworld_checks && !IsLocationControlled())
   {
      if (config_is_defined("SphereCheckSpew"))
         mprintf("[%d] %d - Checking start loc %g %g %g\n", GetObjID(), i, GetLocationVec(i).x, GetLocationVec(i).y, GetLocationVec(i).z);
      if (!HasObjectWarned(GetObjID()) && !SphrSphereInWorld((Location *)&GetLocation(i), radius, 0))
      {
         SetObjectWarned(GetObjID());
         mprintf("Sphere (%s - %d) starts out of world!\n", ObjWarnName(GetObjID()), i);
      }
   }
   #endif

   // Do the spherecast
   contactCount = (SphrSpherecastStatic((Location *) &GetLocation(i), &end_loc, radius, 0));

   if ((contactCount == 0) || (pDynamics->TerrainPassThru()))
   {
      #ifndef SHIP     
      if (inworld_checks && !IsLocationControlled())
      {
         if (config_is_defined("SphereCheckSpew"))
            mprintf("[%d] %d - Checking end loc %g %g %g\n", GetObjID(), i, end_loc.vec.x, end_loc.vec.y, end_loc.vec.z);
         MakeHintedLocationFromVector(&end_loc, &end_loc.vec, (Location *) &GetLocation(i));
         if (!HasObjectWarned(GetObjID()) && !SphrSphereInWorld(&end_loc, radius, 0))
         {
            mxs_vector thestart = GetLocationVec(i);

            SetObjectWarned(GetObjID());
            mprintf("Sphere (%s - %d) out of world! (%x %x %x) to (%x %x %x) (%x radius)\n", ObjWarnName(GetObjID()), i,
                    *(int *)&thestart.x, *(int *)&thestart.y, *(int *)&thestart.z,
                    *(int *)&end_loc.vec.x, *(int *)&end_loc.vec.y, *(int *)&end_loc.vec.z,
                    *(int *)&radius);
         }
      }
      #endif
 
      *ppClsn = NULL;
      return FALSE;
   }

   // Find the collision with the lowest timestamp
   pContact = &gaSphrContact[0];
   for (int j=1; j<contactCount; j++)   
   {
      if (gaSphrContact[j].time < pContact->time)
         pContact = &gaSphrContact[j];
   }
   pContactData = &gaSphrContactData[pContact->first_contact_data];

#if 0
   if (pContact->time < 0.00001)
   {
      mprintf("%s collision too early in frame!\n", ObjWarnName(GetObjID()));
      *ppClsn = NULL;
      return FALSE;
   }
#endif

   if (!IsLocationControlled()) 
   {
      if (GetDynamics()->CollisionCount() > kMaxFrameCollisions)
      {
         cFaceContact newContact(pContactData->cell_index, pContactData->polygon_index);

         if (config_is_defined("warn_collision_count"))
            Warning(("Too many collisions: stopping object %d(%d)\n", GetObjID(), i));

         CreateTerrainContact(i, newContact.GetPoly(), newContact.GetObjID());
         PhysEmergencyStop(this, i, dt);
         *ppClsn = NULL;
         return TRUE;
      }
   }

   // Create a new collision and add to list
   switch (pContact->kind)
   {
      case kSphrPolygon:
         type = kPC_TerrainFace;
         break;
      case kSphrEdge:
         type = kPC_TerrainEdge;
         break;
      case kSphrVertex:
         type = kPC_TerrainVertex;
         break;
      default:
         CriticalMsg4("Unrecognised collision type: %d, on %d contacts, %s (%d)", 
                      pContact->kind, contactCount, ObjWarnName(GetObjID()), i);
   }

   // Back up a little, to avoid touching the surface...
   // this should really be done by distance from the surface not distance along
   // the spherecast, but is it really worth the extra math?
   // the real problem is that we can't back up any further than we've moved this
   // frame.
   mxs_vector collide_vec;
   mxs_vector move_vec;
   mxs_vector move_backup;
   mxs_real   move_len;

   mx_sub_vec(&move_vec, &pContact->point_on_ray, (mxs_vector *) &GetLocationVec(i));

   move_len = mx_norm_vec(&move_backup, &move_vec);

   if (move_len > 0.0)
   {
      mx_scaleeq_vec(&move_backup, -min(0.01, move_len));
      mx_addeq_vec(&move_vec, &move_backup);
   }
   else
      mx_zero_vec(&move_vec);

   time = pContact->time;

   mx_add_vec(&collide_vec, (mxs_vector *) &GetLocationVec(i), &move_vec);

#if 0
   #ifndef SHIP     
   if (inworld_checks && !IsLocationControlled())
   {
      if (config_is_defined("SphereCheckSpew"))
         mprintf("[%d] %d - Checking post end loc %g %g %g\n", GetObjID(), i, collide_vec.x, collide_vec.y, collide_vec.z);
 
      Location post_loc;

      MakeHintedLocationFromVector(&post_loc, &collide_vec, (Location *) &GetLocation(i));
      if (!HasObjectWarned(GetObjID()) && !SphrSphereInWorld(&post_loc, radius, 0))
      {
         mxs_vector thestart = GetLocationVec(i);

         SetObjectWarned(GetObjID());
         mprintf("Sphere (%s - %d) out of world (post-backup)! (%x %x %x) to (%x %x %x) (%x radius)\n", 
                 ObjWarnName(GetObjID()), i,
                 *(int *)&thestart.x, *(int *)&thestart.y, *(int *)&thestart.z,
                 *(int *)&end_loc.vec.x, *(int *)&end_loc.vec.y, *(int *)&end_loc.vec.z,
                 *(int *)&radius);

      }
   }
   #endif
#endif

   Location post_loc;
   MakeHintedLocationFromVector(&post_loc, &collide_vec, (Location *) &GetLocation(i));

   if (!SphrSphereInWorld(&post_loc, radius, 0))
   {
#ifndef SHIP
      if (config_is_defined("post_backup_spew"))
         mprintf("%s stopping (out of world post-backup)\n", ObjWarnName(GetObjID()));
#endif
      PhysEmergencyStop(this, i, dt);
      *ppClsn = NULL;
      return FALSE;
   }

   *ppClsn = new cPhysClsn(type, t0, (time * dt), pContact->point_in_world, 
                           this, i, collide_vec, gaSphrContactData,
                           pContact->first_contact_data);

   return TRUE;
}

///////////////////////////////////////

ePhysIntersectResult cPhysSphereModel::TestLocationVec(int subModId, const mxs_vector & position) const
{
   CriticalMsg("cPhysSphereModel::TestLocation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////

ePhysIntersectResult cPhysSphereModel::TestRotation(int subModId, const mxs_vector & rotation) const
{
   CriticalMsg("cPhysSphereModel::TestRotation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////
//
// return distance from model to terrain poly
//

mxs_real cPhysSphereModel::TerrainDistance(tPhysSubModId subModId, cFaceContact *pFaceContact) const
{
   float radius = (IsFancyGhost() && m_radius[subModId] > 1.0) ? 1.0 : m_radius[subModId];

   return mx_dot_vec((mxs_vector *) &GetLocationVec(subModId), (mxs_vector *) &pFaceContact->GetNormal()) +
          pFaceContact->GetPlaneConst() - radius;
}

mxs_real cPhysSphereModel::TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const
{
   float radius = (IsFancyGhost() && m_radius[subModId] > 1.0) ? 1.0 : m_radius[subModId];

   return pEdgeContact->GetDist(GetLocationVec(subModId)) - radius;
}

mxs_real cPhysSphereModel::TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const
{
   float radius = (IsFancyGhost() && m_radius[subModId] > 1.0) ? 1.0 : m_radius[subModId];

   return mx_dist_vec((mxs_vector *) &GetLocationVec(subModId), (mxs_vector *) &pVertexContact->GetPoint()) - radius;
}

///////////////////////////////////////


//DECLARE_TIMER(PHYS_ApplyForces, Average);
DECLARE_TIMER(PHYS_AF_CollTerrCheck, Average);
DECLARE_TIMER(PHYS_AF_CollObjCheck, Average);
DECLARE_TIMER(PHYS_AF_DetRes, Average);

BOOL cPhysSphereModel::ApplyForces(tPhysSubModId subModId, sPhysForce *flist, int nforces, mxs_real dt, 
                                   mxs_vector &start_loc, mxs_vector *end_loc, mxs_vector *force, 
                                   mxs_real *ctime, BOOL terrain_check, BOOL object_check)
{
   //AUTO_TIMER(PHYS_ApplyForces);

   cPhysCtrlData  *pControls = GetControls(subModId);
   cPhysDynData   *pDynamics = GetDynamics(subModId);
   cPhysClsn  *pClsn;
   eForceListResult forcelist_result = kFLR_Success;;
   ePhysClsnResult result;
   Position    pos;
   mxs_vector  accel;
   mxs_vector  velocity;
   mxs_real    radius; 
   int i;

   // Accumulate forces
   mx_zero_vec(&accel);
   for (i=0; i<nforces; i++)
      mx_addeq_vec(&accel, &(flist[i].vector));

   // Integrate constant acceleration and update end location
   mx_scale_vec(&velocity, &accel, dt / 2);
   pDynamics->SetVelocity(velocity);
   mx_copy_vec(&pos.loc.vec, &start_loc);
   mx_mk_angvec(&pos.fac, GetRotation(subModId).tx,
                          GetRotation(subModId).ty,
                          GetRotation(subModId).tz);
   SetEndLocationVec(subModId, pos.loc.vec);
   SetEndRotationVec(subModId, pos.fac);

   // Calculate the actual submodel velocity
   mx_sub_vec(&velocity, &GetEndLocationVec(subModId), &GetLocationVec(subModId));
   if (dt != 0)
      mx_scaleeq_vec(&velocity, 1 / dt);
   else
      mx_zero_vec(&velocity);
   pDynamics->SetVelocity(velocity);

   if (GetType(subModId) == kPMT_Sphere)
      radius = ((cPhysSphereModel *)this)->GetRadius(subModId);
   else 
      radius = 0;

   if (terrain_check)
   {
      AUTO_TIMER(PHYS_AF_CollTerrCheck);
      if (CheckTerrainCollision(subModId, 0, dt, &pClsn) && (pClsn != NULL))
         g_PhysClsns.Insert(pClsn);
   }
   if (object_check)
   {
      AUTO_TIMER(PHYS_AF_CollObjCheck);
      SetSleep(FALSE);
      CheckModelObjectCollisions(this, 0, dt, FALSE, subModId);
   }

   pClsn = g_PhysClsns.GetFirst();
   while (pClsn != NULL)
   {
      TIMER_Start(PHYS_AF_DetRes);
      result = DetermineCollisionResult(pClsn);  
      TIMER_MarkStop(PHYS_AF_DetRes);

      if ((result == kPCR_NonPhys1) || (result == kPCR_NonPhys2) || (result == kPCR_NonPhysBoth))
      {
         forcelist_result |= kFLR_MadeNonPhysical;
         // Um, do nothing
      }
      else
      if ((pClsn->GetType() == kPC_MoveableObject) || (pClsn->GetType() == kPC_SphereHatObject))
      {
         BounceSpheres(pClsn->GetInstance(), 
                       pClsn->GetModel()->GetDynamics(pClsn->GetSubModId()),
                       pClsn->GetInstance2(), 
                       pClsn->GetModel2()->GetDynamics(pClsn->GetSubModId2()),
                       pClsn->GetTime());
         
         // Let biped resolve terrain collisions
         mx_copy_vec(end_loc, (mxs_vector *) &GetEndLocationVec(subModId));
         *ctime = pClsn->GetTime();

         PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(),
                         pClsn->GetModel2(), pClsn->GetSubModId2(), *force);

         // Clear remaining collisions
         pClsn = g_PhysClsns.GetFirst();
         while (pClsn != NULL)
         {
            g_PhysClsns.Remove(pClsn);
            delete pClsn;
            pClsn = g_PhysClsns.GetFirst();
         }

         forcelist_result |= kFLR_ObjectCollision;

         // Hit an ojbect
         return forcelist_result;
      }
      else
      if (pClsn->GetType() == kPC_OBBObject)
      {
         // Check if it's an edge trigger
         if (!((cPhysOBBModel *)pClsn->GetModel2())->IsEdgeTrigger())
         {
            // Let biped resolve terrain collisions
            mx_copy_vec(end_loc, (mxs_vector *) &GetEndLocationVec(subModId));
            *ctime = pClsn->GetTime();

            PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(),
                            pClsn->GetModel2(), pClsn->GetSubModId2(), *force);


            // Clear remaining collisions
            pClsn = g_PhysClsns.GetFirst();
            while (pClsn != NULL)
            {
               g_PhysClsns.Remove(pClsn);
               delete pClsn;
               pClsn = g_PhysClsns.GetFirst();
            }

            forcelist_result |= kFLR_ObjectCollision;

            return forcelist_result;
         }
         else
         {
            g_PhysClsns.Remove(pClsn);
            delete pClsn;
            pClsn = g_PhysClsns.GetFirst();
         }
      }
      else
      {
         // Let biped resolve terrain collisions
         mx_copy_vec(end_loc, (mxs_vector *) &GetEndLocationVec(subModId));
         *force = pClsn->GetNormal();
         *ctime = pClsn->GetTime();

         pClsn = g_PhysClsns.GetFirst();
         // Clear remaining collisions
         while (pClsn != NULL)
         {
            g_PhysClsns.Remove(pClsn);
            delete pClsn;
            pClsn = g_PhysClsns.GetFirst();
         }

         forcelist_result |= kFLR_TerrainCollision;

         // Hit terrain
         return forcelist_result;
      }

      pClsn = g_PhysClsns.GetFirst();
   }

   if (forcelist_result == kFLR_Success)
   {
      // No terrain collisions
      mx_copy_vec(end_loc, (mxs_vector *)&GetEndLocationVec(subModId));
      mx_zero_vec(force);
      *ctime = 0.0;

      AssertMsg1(IsControllable(), "Attempt to apply forces to uncontrollable object: %d", GetObjID());

      pControls->ControlLocation(*end_loc);
   }
   
   if ((result != kPCR_NonPhys1) && (result != kPCR_NonPhys2) && (result != kPCR_NonPhysBoth))
      PhysUpdateRefs(this);

   return forcelist_result;
}


void cPhysSphereModel::UpdateRefs(void)
{
   CriticalMsg("cPhysSphereModel::UpdateRefs: not yet implemented\n");
}

///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP

long cPhysSphereModel::InternalSize() const
{
   long model_size = cPhysModel::InternalSize();
   long sphere_size = m_radius.Size() * sizeof(mxs_real);

   return (model_size + sphere_size);
}

#endif

///////////////////////////////////////////////////////////////////////////////

