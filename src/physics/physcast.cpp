/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/physcast.cpp,v 1.13 2000/03/01 16:23:38 toml Exp $
//
// Raycasting through physics objects
//

//#define PROFILE_ON
//#define PROFILE_MORE

#include <matrix.h>
#include <timings.h>
#include <config.h>
#include <port.h>

#include <osysbase.h>
#include <refsys.h>
#include <objtype.h>

#include <textarch.h>
#include <wrdbrend.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmodsph.h>
#include <phmodobb.h>
#include <phref.h>
#include <phutils.h>

#include <physcast.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////////////////////////

#define MAX_PHYSCAST_OBJS    (512)
#define MAX_PHYSCAST_CELLS   (128)
#define MAX_PHYSCAST_OBJIDS HACK_MAX_OBJ

#ifndef SHIP
static BOOL m_spew;
#endif

////////////////////////////////////////

void GatherPhysObjs(cPhysModel **pModelList, int *nModels, 
                    Location &start, Location &end, int filter);

BOOL RaycastVsSphere(cPhysModel *pModel, int submod, const mxs_vector &start, const mxs_vector &end, 
                     mxs_real radius, mxs_vector *hit, mxs_real* hit_time);
BOOL RaycastVsSphereHat(cPhysModel *pModel, const mxs_vector &start, const mxs_vector &end, 
                        mxs_real radius, mxs_vector *hit, mxs_real* hit_time);
BOOL RaycastVsOBB(cPhysModel *pModel, const mxs_vector &start, const mxs_vector &end, 
                  mxs_real radius, mxs_vector *hit, mxs_real* hit_time);

////////////////////////////////////////

DECLARE_TIMER(PR_All, Average);
DECLARE_TIMER(PR_Gather, Average);
DECLARE_TIMER(PR_Terrain, Average);
DECLARE_TIMER(PR_ObjColl, Average);

static BOOL   gObjListSet = FALSE;
static ObjID *gObjList = NULL;
static sbool  gObjListLookup[HACK_MAX_OBJ];
static int    gObjListSize = 0;
#define GatherWantsObj(obj) (!gObjListSet || gObjListLookup[obj])

static tPhyscastObjTest gObjCallback = NULL;
void* g_pData = NULL;

int PhysRaycast(Location &start, Location &end, Location *hit, ObjID *hit_obj, mxs_real radius, int filter)
{
   AUTO_TIMER(PR_All);

   cPhysModel *pModelList[MAX_PHYSCAST_OBJS];
   int         nModels = 0;
   int i, j;
   
   mxs_vector min_hit;
   mxs_real   min_hit_time = -1.0;
   int        min_hit_type;
   ObjID      min_hit_obj;

   mxs_vector cur_hit;
   mxs_real   cur_hit_time;

   mxs_real   hit_time_modifier = 1.0;

   Location   used_end = end;

   #ifndef SHIP
   if (config_is_defined("physcast_spew"))
   {
      mprintf("[] PhysRaycast from %g %g %g to %g %g %g\n", start.vec.x, start.vec.y, start.vec.z, 
              used_end.vec.x, used_end.vec.y, used_end.vec.z);
      m_spew = TRUE;
   }
   #endif

   #ifdef PROFILE_MORE
   TIMER_Start(PR_Terrain);
   #endif

   // Initialize hit
   hit->cell = hit->hint = CELL_INVALID;

   if (CellFromLoc(&start) == CELL_INVALID)
   {
      mx_zero_vec(&hit->vec);
      *hit_obj = OBJ_NULL;
      return kCollideInvalid;
   }

   // Do the terrain raycast if we're set to
   if (filter & kCollideTerrain)
   {
      Location l_start, l_end, l_hit;

      l_start = start;
      l_end = used_end;

      if (!PortalRaycast(&start, &end, &l_hit, TRUE))
      {
         min_hit = l_hit.vec;

         mxs_vector hit_delta;
         mxs_vector cast_delta;

         mx_sub_vec(&hit_delta, &min_hit, &start.vec);
         mx_sub_vec(&cast_delta, &end.vec, &start.vec);

         min_hit_time = sqrt(mx_mag2_vec(&hit_delta) / mx_mag2_vec(&cast_delta));
         min_hit_type = kCollideTerrain;

         if (PortalRaycastCell != -1)
         {
            ubyte texture = WR_CELL(PortalRaycastCell)->render_list[PortalRaycastFindPolygon()].texture_id;
            min_hit_obj = GetTextureObj(texture);
          
            used_end = l_hit;

            hit_time_modifier = sqrt(mx_dist2_vec(&start.vec, &used_end.vec) / mx_dist2_vec(&start.vec, &end.vec)); 
         }
         else
            min_hit_obj = OBJ_NULL;
      }
   }

   #ifdef PROFILE_MORE
   TIMER_MarkStop(PR_Terrain);
   TIMER_Start(PR_Gather);
   #endif

   // Gather the physics objects to cast against
   GatherPhysObjs(pModelList, &nModels, start, used_end, filter);

   #ifdef PROFILE_MORE
   TIMER_MarkStop(PR_Gather);
   AUTO_TIMER(PR_ObjColl);
   #endif

   // Cast against each object (@TBD: is it worth sorting?)
   for (i=0; i<nModels; i++)
   {
      if (min_hit_time == 0)
         break;

      switch (pModelList[i]->GetType(0))
      {
         case kPMT_Sphere:
         {
            for (j=0; j<pModelList[i]->NumSubModels(); j++)
            {
               if (pModelList[i]->GetType(j) != kPMT_Sphere)
                  continue;

               if (RaycastVsSphere(pModelList[i], j, start.vec, used_end.vec, radius, &cur_hit, &cur_hit_time))
               {
                  cur_hit_time *= hit_time_modifier;

                  if ((cur_hit_time < min_hit_time) || (min_hit_time < 0))
                  {
                     min_hit_time = cur_hit_time;
                     min_hit = cur_hit;
                     min_hit_type = kCollideSphere;
                     min_hit_obj = pModelList[i]->GetObjID();
                  }
               }
            }

            break;
         }

         case kPMT_SphereHat:
         {
            if (RaycastVsSphereHat(pModelList[i], start.vec, used_end.vec, radius, &cur_hit, &cur_hit_time))
            {
               cur_hit_time *= hit_time_modifier;
                  
               if ((cur_hit_time < min_hit_time) || (min_hit_time < 0))
               {
                  min_hit_time = cur_hit_time;
                  min_hit = cur_hit;
                  min_hit_type = kCollideSphereHat;
                  min_hit_obj = pModelList[i]->GetObjID();
               }
            }

            break;
         }

         case kPMT_OBB:
         {
            if (RaycastVsOBB(pModelList[i], start.vec, used_end.vec, radius, &cur_hit, &cur_hit_time))
            {
               if (cur_hit_time == 0)
               {
                  if (filter & kInsideOBB)
                  {
                     min_hit_time = cur_hit_time;
                     min_hit = cur_hit;
                     min_hit_type = kInsideOBB;
                     min_hit_obj = pModelList[i]->GetObjID();
                  }
               }
               else
               {
                  cur_hit_time *= hit_time_modifier;
               
                  if ((cur_hit_time < min_hit_time) || (min_hit_time < 0))
                  {
                     min_hit_time = cur_hit_time;
                     min_hit = cur_hit;
                     min_hit_type = kCollideOBB;
                     min_hit_obj = pModelList[i]->GetObjID();
                  }
               }
            }

            break;
         }
      }
   }

   if (min_hit_time >= 0)
   {
      hit->vec = min_hit;
      *hit_obj = min_hit_obj;
      return min_hit_type;
   }
   else
   {
      mx_zero_vec(&hit->vec);
      *hit_obj = OBJ_NULL;
      return kCollideNone;
   }
}

////////////////////////////////////////

BOOL RaycastVsSphere(cPhysModel *pModel, int submod, const mxs_vector &start, const mxs_vector &end, 
                     mxs_real radius, mxs_vector *hit, mxs_real *hit_time)
{
   mxs_real   cur_hit_time;
   mxs_vector delta;
   mx_sub_vec(&delta, &end, &start);

   if (PhysSolveLnPtDist(&start, &delta, &pModel->GetLocationVec(submod), 
                         ((cPhysSphereModel *)pModel)->GetRadius(submod) + radius, 1.0, &cur_hit_time))
   {
      if ((cur_hit_time <= 1.0) && (cur_hit_time > 0.0))
      {
         *hit_time = cur_hit_time;
         mx_scale_add_vec(hit, &start, &delta, cur_hit_time);
         return TRUE;
      }
      else
         return FALSE;
   }
   else
      return FALSE;
}

////////////////////////////////////////

BOOL RaycastVsSphereHat(cPhysModel *pModel, const mxs_vector &start, const mxs_vector &end, 
                        mxs_real radius, mxs_vector *hit, mxs_real *hit_time)
{
   // Do an eary out to see if we're even within range
   mxs_vector delta;
   mxs_real   sphere_radius;
   mxs_real   eo_hit_time;
   BOOL       eo;

   // assumes a 0.5r sphere hat edge length
   sphere_radius = ((cPhysSphereModel *)pModel)->GetRadius(0) * 1.25;
   mx_sub_vec(&delta, &end, &start);

   eo = !PhysSolveLnPtDist(&start, &delta, &pModel->GetLocationVec(), sphere_radius + radius, 1.0, &eo_hit_time);

   if (eo || eo_hit_time > 1.0)
      return FALSE;

   // First check if we've hit the hat
   mxs_vector hat_hit;
   mxs_real   hat_hit_time = -1.0;

   cFaceContact faceContact((cPhysSphereModel *)pModel, ((cPhysSphereModel *)pModel)->GetRadius(0));
   const cFacePoly *pPoly;
   const cFacePoly *pEdgePoly;

   // Project our point onto the hat
   pPoly = faceContact.GetPoly();

   if (pPoly)
   {
      mxs_real face_start_dist = mx_dot_vec(&pPoly->normal, &start) - pPoly->d;
      mxs_real face_end_dist = mx_dot_vec(&pPoly->normal, &end) - pPoly->d;

      // Check for both on same side of plane
      if ((face_start_dist * face_end_dist) < 0)
      {
         // Find hit location with plane
         mxs_vector delta;
         mx_sub_vec(&delta, &end, &start);

         hat_hit_time = face_start_dist / (fabs(face_start_dist - face_end_dist));
         mx_scale_add_vec(&hat_hit, &start, &delta, hat_hit_time);
   
         // Is this hit point on the surface of the hat?
         for (int i=0; i<4; i++)
         {
            pEdgePoly = pPoly->edgePlaneList[i];

            if ((mx_dot_vec(&pEdgePoly->normal, &hat_hit) - pEdgePoly->d) > 0)
            {
               hat_hit_time = -1.0;
               break;
            }
         }
      }
   }

   mxs_vector sphere_hit;
   mxs_real   sphere_hit_time;

   if (!RaycastVsSphere(pModel, 0, start, end, radius, &sphere_hit, &sphere_hit_time))
      sphere_hit_time = -1.0;

   // Either hit?
   if ((hat_hit_time == -1.0) && (sphere_hit_time == -1.0))
      return FALSE;

   // Just one hit?
   if ((hat_hit_time != -1.0) && (sphere_hit_time == -1.0))
   {
      *hit = hat_hit;
      *hit_time = hat_hit_time;
      return TRUE;
   }
   if ((hat_hit_time == -1.0) && (sphere_hit_time != 1.0))
   {
      *hit = sphere_hit;
      *hit_time = sphere_hit_time;
      return TRUE;
   }

   // Determine which hit first
   if (hat_hit_time < sphere_hit_time)
   {
      *hit = hat_hit;
      *hit_time = hat_hit_time;
      return TRUE;
   }
   else
   {
      *hit = sphere_hit;
      *hit_time = sphere_hit_time;
      return TRUE;
   }
}

////////////////////////////////////////

BOOL RaycastVsOBB(cPhysModel *pModel, const mxs_vector &start, const mxs_vector &end, 
                  mxs_real radius, mxs_vector *hit, mxs_real *hit_time)
{
   // Do an early out to see if we're even within range
   mxs_vector delta;
   mxs_real   bounding_radius;
   mxs_real   eo_hit_time;
   BOOL       eo;

#if 1
   mxs_vector edge_len = ((cPhysOBBModel *)pModel)->GetEdgeLengths();
   bounding_radius = edge_len.x + edge_len.y + edge_len.z;
#else
   bounding_radius = mx_mag_vec(&((cPhysOBBModel *)pModel)->GetEdgeLengths());
#endif
   mx_sub_vec(&delta, &end, &start);

   eo = !PhysSolveLnPtDist(&start, &delta, &pModel->GetLocationVec(), bounding_radius + radius, 1.0, &eo_hit_time);

   if (eo || (eo_hit_time > 1.0))
      return FALSE;
   
   mxs_vector start_norm_list[6];
   mxs_vector end_norm_list[6];
   mxs_real   start_d_list[6];
   mxs_real   end_d_list[6];

   mxs_real   max_entry_time = -1000000.0;
   mxs_real   min_exit_time =   1000000.0;
   mxs_real   cur_time;

   ((cPhysOBBModel *)pModel)->GetNormals(start_norm_list, end_norm_list);
   ((cPhysOBBModel *)pModel)->GetConstants(start_d_list, start_norm_list, end_d_list, end_norm_list);

   int inside_count = 0;

   for (int i=0; i<6; i++)
   {
      mxs_real start_dist = mx_dot_vec(&start_norm_list[i], &start) - start_d_list[i] - radius;
      mxs_real end_dist = mx_dot_vec(&start_norm_list[i], &end) - start_d_list[i] - radius;

      if (start_dist <= 0.0001)
         inside_count++;

      // We can ignore this face if we're completely inside it.
      if ((start_dist <= 0.0001) && (end_dist <= 0.0001))
         continue;

      // We can ignore the whole OBB if we're outside any one side.
      if ((start_dist >= -0.0001) && (end_dist >= -0.0001)) 
      {
         max_entry_time = -1.0;
         break;
      }

      // Find time along line where intersection happens
      cur_time = start_dist / (start_dist - end_dist);

      // Are we entering the OBB at this face, or exiting it?
      if (start_dist > 0.0)  
      {
         if (cur_time > max_entry_time) 
            max_entry_time = cur_time;
      }
      else
      {
         if (cur_time < min_exit_time)
            min_exit_time = cur_time;
      }
   }

   if (inside_count == 6)
   {
      mx_copy_vec(hit, &start);
      *hit_time = 0;
      
      return TRUE;
   }

   if ((max_entry_time > 0) && (min_exit_time > max_entry_time))
   {
      mxs_vector delta;
      mx_sub_vec(&delta, &end, &start);

      mx_scale_add_vec(hit, &start, &delta, max_entry_time);
      *hit_time = max_entry_time;

      return TRUE;
   }
   else
      return FALSE;
}

////////////////////////////////////////

#define OBJ_WAS_ADDED(x)  (obj_added[(x) >> 3] & (1 << ((x) & 0x0007)))
#define SET_OBJ_ADDED(x)  (obj_added[(x) >> 3] |= (1 << ((x) & 0x0007)))

static bool obj_added[MAX_PHYSCAST_OBJIDS / 8];

void GatherPhysObjs(cPhysModel **pModelList, int *nModels, 
                    Location &start, Location &end, int filter)
{
   int cell_list[MAX_PHYSCAST_CELLS];
   int num_cells;
   int i;

   mxs_vector min;
   mxs_vector max;

   Assert_(gMaxObjID < MAX_PHYSCAST_OBJIDS);

   // Blast our obj_added array
   memset(obj_added, 0, sizeof(bool) * MAX_PHYSCAST_OBJIDS / 8);

   // Build min/max of our bounding box
   for (i=0; i<3; i++)
   {
      if (start.vec.el[i] > end.vec.el[i])
      {
         min.el[i] = end.vec.el[i];
         max.el[i] = start.vec.el[i];
      }
      else
      {
         min.el[i] = start.vec.el[i];
         max.el[i] = end.vec.el[i];
      }
   }

   // Gather cells intersecting our bbox
   num_cells = PortalCellsInArea(cell_list, MAX_PHYSCAST_CELLS, &min, &max, &start);

   // Gather objects with the physics ref system
   for (i=0; i<num_cells; i++)
   {
      ObjRefID current_bin_id;
      ObjRef  *current_bin_ptr;
      ObjID    current_obj;

      #ifndef SHIP
      if (m_spew)
         mprintf("Examining cell %d\n", cell_list[i]);
      #endif

      current_bin_id = *(mRefSystems[PhysRefSystemID].ref_list_func((void *)&cell_list[i], FALSE));
      while(current_bin_id)
      {
         cPhysModel *pModel;

         current_bin_ptr = OBJREFID_TO_PTR(current_bin_id);
         current_obj = current_bin_ptr->obj;

         if (!OBJ_WAS_ADDED(current_obj))
         {
            SET_OBJ_ADDED(current_obj);
            if (GatherWantsObj(current_obj))
            {
               #ifndef SHIP
               if (m_spew)
                  mprintf("  adding obj %d (pending filter)\n", current_obj);
               #endif

               pModel = g_PhysModels.Get(current_obj);
               if (pModel)
               {
                  switch (pModel->GetType(0))
                  {
                     case kPMT_Point:
                     {
                        // Rays can't hit points
                        break;
                     }

                     case kPMT_Sphere:
                     {
                        if (!gObjCallback || gObjCallback(current_obj, pModel, g_pData))
                        {
                           if (filter & kCollideSphere)
                              pModelList[(*nModels)++] = pModel;
                        }

                        break;
                     }

                     case kPMT_SphereHat:
                     {
                        if (!gObjCallback || gObjCallback(current_obj, pModel, g_pData))
                        {
                           if (filter & kCollideSphereHat)
                              pModelList[(*nModels)++] = pModel;
                        }

                        break;
                     }

                     case kPMT_OBB:
                     {
                        if (!gObjCallback || gObjCallback(current_obj, pModel, g_pData))
                        {
                           if (((cPhysOBBModel *)pModel)->IsEdgeTrigger())
                           {
                              if (filter & kCollideEdgeTrigger)
                                 pModelList[(*nModels)++] = pModel;
                           }
                           else
                           {
                              if (filter & (kCollideOBB | kInsideOBB))
                                 pModelList[(*nModels)++] = pModel;
                           }
                        }
                        break;
                     }

                     default:
                     {
                        Warning(("GatherPhysObjs: unknown model type on %d\n", current_obj));
                        break;
                     }
                  }
               }
            }
         }

         current_bin_id = current_bin_ptr->next_in_bin;
      }
   }
}

////////////////////////////////////////

void PhysRaycastSetObjlist(ObjID *obj_list, int num_objects)
{
   gObjListSet = TRUE;
   gObjList = obj_list;
   gObjListSize = num_objects;
   memset(gObjListLookup, 0, sizeof(gObjListLookup));
   for (int i = 0; i < num_objects; i++)
   {
      gObjListLookup[gObjList[i]] = TRUE;
   }
}

void PhysRaycastClearObjlist()
{
   gObjListSet = FALSE;
   gObjList = NULL;
   gObjListSize = 0;

}

void PhysRaycastSetCallback(tPhyscastObjTest callback, void* pData)
{
   gObjCallback = callback;
   g_pData = pData;
}

void PhysRaycastClearCallback()
{
   gObjCallback = NULL;
}

////////////////////////////////////////







