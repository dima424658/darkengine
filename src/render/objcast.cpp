/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objcast.cpp,v 1.13 2000/02/01 18:23:06 bfarquha Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   objcast.cpp

   raycaster which checks for intersections with both terrain and
   objects

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


extern "C" {
#include <r3d.h>
#include <2d.h>
#include <mm.h>
#include <mdutil.h>
};

#include <cfgdbg.h>
#include <dynarray.h>

#include <port.h>
#include <objmodel.h>
#include <mnumprop.h>
#include <objshape.h>
#include <refsys.h>
#include <objpos.h>

#include <objcast.h>

// For ObjRaycast IBoolProperty parm:
#include <propface.h>

// Must be last header
#include <dbmem.h>


extern "C" r3s_context *sim_context;    // from r3app.c

static Location *g_pStartLoc;
static Location *g_pEndLoc;
static Location *g_pHitLoc;
static Position *g_pObjPos;
static int g_iCell;
static int g_iModelIdx;
static float g_fEarliestTime;

// what we hit
ObjID g_ObjCastObjID;
mms_ray_hit_detect_info g_ObjCastMeshHit;
mds_segment_hit_detect_info g_ObjCastMDHit;


// This only returns TRUE if we not only hit the model but did so at a
// time before our earliest contact.
static BOOL MDDetect()
{
   // We need our ray to be in object space.
   mxs_vector StartObjRel, EndObjRel;
   BOOL rv = FALSE;

   r3_start_object_angles(&g_pObjPos->loc.vec, &g_pObjPos->fac, R3_DEFANG);
   r3_start_block();
   r3_transform_w2o(&StartObjRel, &g_pStartLoc->vec);
   r3_transform_w2o(&EndObjRel, &g_pEndLoc->vec);

   g_ObjCastMDHit.a = &StartObjRel;
   g_ObjCastMDHit.b = &EndObjRel;

   mds_model *pModel = (mds_model *) objmodelGetModel(g_iModelIdx);
   md_segment_hit_detect(pModel, &g_ObjCastMDHit);

   if (g_ObjCastMDHit.hit && g_ObjCastMDHit.r < g_fEarliestTime) {
      g_fEarliestTime = g_ObjCastMDHit.r;

      // find intersection point in world space
      mx_interpolate_vec(&g_pHitLoc->vec, &g_pStartLoc->vec, &g_pEndLoc->vec,
                         g_ObjCastMDHit.r);
      g_pHitLoc->cell = g_iCell;
      rv = TRUE;
   }

   r3_end_block();
   r3_end_object();

   return rv;
}


// This returns whether we hit the given object, and slams the time if
// we did.  Like MDDetect it ignores any hit later than the earliest
// we've found.
static BOOL MeshDetect()
{
   BOOL rv = FALSE;

   // The mesh model will put itself into world space using its joint
   // callbacks.  So we can use our ray's endpoints unaltered.
   g_ObjCastMeshHit.bound_radius = ObjGetRadius(g_ObjCastObjID);
   g_ObjCastMeshHit.bound_center = &g_pObjPos->loc.vec;

   mms_model *pModel = (mms_model *) objmodelGetModel(g_iModelIdx);
   int iBuffSize = mm_buffsize(pModel, 0);
   void *pBuff = Malloc(iBuffSize);
   mm_set_buff(pModel, 0, pBuff);

   if (mm_ray_hit_detect(pModel, &g_ObjCastMeshHit)
    && g_ObjCastMeshHit.hit_time < g_fEarliestTime) {
      g_fEarliestTime = g_ObjCastMeshHit.hit_time;
      g_pHitLoc->vec = g_ObjCastMeshHit.hit_point;
      g_pHitLoc->cell = g_iCell;
      rv = TRUE;
   }

   Free(pBuff);
   return rv;
}


// given the index of a world rep cell, pull the list of ObjIDs;
// we return the number of objects added to the list
static void AddRefsForCellID(cDynArray_<ObjID, 128> *pObjList)
{
   int i;
   ObjRef *pRef;
   ObjID NewObj;
   PortalCell *pCell = wr_cell[g_iCell];
   ObjRefID RefID = * (int *) (&pCell->refs);
   int iOldSize = pObjList->Size();

   while (RefID) {
      pRef = OBJREFID_TO_PTR(RefID);
      NewObj = pRef->obj;

      // Do we already have this object?
      for (i = 0; i < iOldSize; ++i)
         if ((*pObjList)[i] == NewObj)
            break;

      if (i == iOldSize)
         pObjList->Append(pRef->obj);

      RefID = pRef->next_in_bin;
   }
}


// This is for when we're only casting against a given set of objects.
inline BOOL ObjInList(ObjID *pObjList, uint nObjCount)
{
   for (int i = 0; i < nObjCount; ++i) {
      if (*pObjList == g_ObjCastObjID)
         return TRUE;
      ++pObjList;
   }
   return FALSE;
}


eObjCastResult ObjRaycastC(Location *pStartLoc, Location *pEndLoc, Location *pHitLoc,
                           BOOL ShortCircuit, ObjID *pObjList, uint nNumObjs, IBoolProperty *pObjProp, BOOL UseZeroEpsilon)
{
   if (pObjList == NULL)
      return ObjRaycast(pStartLoc, pEndLoc, pHitLoc, ShortCircuit, 0, kObjCastAllObjs, pObjProp, UseZeroEpsilon);
   else
      return ObjRaycast(pStartLoc, pEndLoc, pHitLoc, ShortCircuit, pObjList, nNumObjs, pObjProp, UseZeroEpsilon);
}

eObjCastResult ObjRaycast(Location *pStartLoc, Location *pEndLoc,
                          Location *pHitLoc, BOOL ShortCircuit,
                          ObjID *pObjList, uint nNumObjs, IBoolProperty *pObjProp,
                          BOOL UseZeroEpsilon)
{
   int i, j;
   int iNewSize, iOldSize;
   BOOL bVal;
   r3s_context *pOldContext;
   r3e_space nOldSpace;

   g_fEarliestTime = 1.0;
   eObjCastResult rv = kObjCastNone;
   cDynArray_<ObjID, 128> ObjsCompared;

   // This is global since it's used for both kinds of object
   // intersection.
   g_pHitLoc = pHitLoc;
   g_pStartLoc = pStartLoc;
   g_pEndLoc = pEndLoc;

   // set up global struct for md objects
   g_ObjCastMeshHit.early_out = ShortCircuit;

   // set up global struct for mesh objects
   g_ObjCastMeshHit.start = &pStartLoc->vec;
   g_ObjCastMeshHit.end = &pEndLoc->vec;
   g_ObjCastMeshHit.early_out = ShortCircuit;

   // our terrain cast gives us a ref list
   if (!PortalRaycastRefs(pStartLoc, pEndLoc, g_pHitLoc, UseZeroEpsilon)) {
      rv = kObjCastTerrain;
      g_fEarliestTime = PortalRaycastTime;
      g_pHitLoc->cell = g_iCell;
   }

   pOldContext = r3d_state.cur_con;
   if (pOldContext)
   {
      nOldSpace = r3_get_space();
      r3d_state.cur_con = 0;
   }

   r3_use_context(sim_context);
   r3_set_view_canvas(grd_canvas);
   r3_set_space(R3_PROJECT_SPACE);

   for (i = 0; i < PortalRaycastRefCount; ++i) {
      g_iCell = PortalRaycastRef[i];
      iOldSize = ObjsCompared.Size();
      AddRefsForCellID(&ObjsCompared);
      iNewSize = ObjsCompared.Size();

      // process the objects from our new cell
      for (j = iOldSize; j < iNewSize; ++j) {
         g_ObjCastObjID = ObjsCompared[j];

//if our list is 0, and we aren't supposed to check everything, then, for
//gods sake, don't check everything!.
	 if (nNumObjs != kObjCastAllObjs && (!pObjList))
	   continue;

         if (nNumObjs != kObjCastAllObjs && pObjList && !ObjInList(pObjList, nNumObjs))
            continue;

         if (pObjProp && (!pObjProp->Get(g_ObjCastObjID, &bVal) || !bVal))
            continue; // val is FALSE. skip this object.

         g_pObjPos = ObjPosGet(g_ObjCastObjID);

         if (ObjGetModelNumber(g_ObjCastObjID, &g_iModelIdx)) {

            ConfigSpew("RaycastSpew",
                       ("test against object %d w/model idx %d\n",
                        g_ObjCastObjID, g_iModelIdx));

            switch (objmodelGetModelType(g_iModelIdx)) {
               case OM_TYPE_MD:
               {
                  if (MDDetect()) {
                     rv = kObjCastMD;
                     if (ShortCircuit)
                        goto Done;
                  }
                  break;
               }

               case OM_TYPE_MESH:
               {
                  if (MeshDetect()) {
                     rv = kObjCastMesh;
                     if (ShortCircuit)
                        goto Done;
                  }
                  break;
               }

               default:
                  break;

            }
         }
      }

      // Even if we're not short-circuiting in the normal sense, we're
      // following the order of the cells along the ray, so if we've hit
      // something in this cell it's earlier than any hits in cells we
      // haven't tried yet--so it's safe to exit.
      if (rv != kObjCastNone && rv != kObjCastTerrain)
         break;
   }

Done:
   r3_done_with_context();
   if (pOldContext)
   {
      r3d_state.cur_con = pOldContext;
      r3_set_space(nOldSpace);
   }
   return rv;
}
