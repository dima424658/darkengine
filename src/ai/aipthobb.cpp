/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthobb.cpp,v 1.6 2000/03/06 17:19:27 toml Exp $
//
// AI OBB pathable cells
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <dynarray.h>
#include <matrix.h>

#include <linkman.h>
#include <linkbase.h>
#include <relation.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <phmtprop.h>
#include <phmterr.h>
#include <phprop.h>
#include <prcniter.h>

#include <objpos.h>

#include <aipathdb.h>
#include <aipthobb.h>
#include <aiprops.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

IBoolProperty *g_pObjPathableProperty = NULL;

static sPropertyDesc _g_ObjPathablePropertyDesc =
{
  "AI_ObjPathable",
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Pathable object" },
   kPropertyChangeLocally,  // net_flags
};

void LGAPI ObjectPathableListener(sPropertyListenMsg * msg, PropListenerData data)
{
   // Find our object in the obj->cell mapping list
   for (int i=0; i<g_AIPathDB.m_nCellObjMaps; i++)
   {
      sAIPathCellObjMap *map = &g_AIPathDB.m_CellObjMap[i];

      if (map->objID == msg->obj)
      {
         if (map->prevPropState != msg->value.intval)
         {
            // mprintf ("cell %d was %d\n", map->cellID, g_AIPathDB.m_Cells[map->cellID].unpathable);

            if (msg->value.intval == TRUE)
               g_AIPathDB.m_Cells[map->cellID].pathFlags &= ~kAIPF_Unpathable;
            else
               g_AIPathDB.m_Cells[map->cellID].pathFlags |= kAIPF_Unpathable;

            map->prevPropState = msg->value.intval;

            // mprintf("setting cell %d to %d\n", map->cellID, g_AIPathDB.m_Cells[map->cellID].unpathable); 
         }

         break;
      }
   }
}

////////////////////////////////////////

void AIInitObjectPathable(IAIManager *)
{
   Assert_(g_pObjPathableProperty == NULL);
   
   g_pObjPathableProperty = CreateBoolProperty(&_g_ObjPathablePropertyDesc, kPropertyImplVerySparse);
   g_pObjPathableProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset,
                                 ObjectPathableListener, NULL);
}

void AITermObjectPathable()
{
   Assert_(g_pObjPathableProperty != NULL);

   SafeRelease(g_pObjPathableProperty);
}

////////////////////////////////////////////////////////////////////////////////

void GenerateOBBPathCell(sAIObjCell *pNewObjCell, ObjID obj, mxs_vector &size, mxs_vector &offset)
{
   int i, j;
   sAIExternCell *pNewCell;

   pNewObjCell->objID = obj;
   pNewObjCell->pAIExternCell = AINewExternCell(4);
   pNewCell = pNewObjCell->pAIExternCell;
               
   mxs_matrix mat;
   mx_ang2mat(&mat, &ObjPosGet(obj)->fac);
   
   // Find the most upward-facing side
   int best_side = 0;
   
   for (i=1; i<3; i++)
   {
      if (fabs(mat.vec[i].z) > fabs(mat.vec[best_side].z))
         best_side = i;
   }     

   // Find the centerpoint of that face
   mxs_vector face_center;
   
   mx_scale_vec(&face_center, &mat.vec[best_side], size.el[best_side] / 2.0 + offset.el[best_side]);
   
   if (mat.vec[best_side].z < 0)
      mx_scaleeq_vec(&face_center, -1.0);

   mx_addeq_vec(&face_center, &ObjPosGet(obj)->loc.vec);
   
   // Fill in plane of our cell
   if (mat.vec[best_side].z < 0)
   {
      pNewCell->plane.normal = mat.vec[best_side];
      mx_scaleeq_vec(&pNewCell->plane.normal, -1.0);
   }
   else
      pNewCell->plane.normal = mat.vec[best_side];

   pNewCell->plane.constant = -mx_dot_vec(&pNewCell->plane.normal, &face_center);
   pNewCell->nVertices = 0;
   
   // Find the vertices of that face
   int edge_1 = -1;
   int edge_2 = -1;

   for (i=0; i<3; i++)
   {
      if (i != best_side)
      {
         if (edge_1 == -1)
            edge_1 = i;
         else
         if (edge_2 == -1)
            edge_2 = i;
      }
   }

   Assert_((edge_1 != -1) && (edge_2 != -1));
               
   for (i=0; i<2; i++)
   {
      for (j=0; j<2; j++)
      {
         mxs_vector edge_offset_1;
         mxs_vector edge_offset_2;

         mx_scale_vec(&edge_offset_1, &mat.vec[edge_1], size.el[edge_1] / 2.0);
         mx_scale_vec(&edge_offset_2, &mat.vec[edge_2], size.el[edge_2] / 2.0);

         // this looks wierd but causes us to wind
         // correctly usually (--, -+, ++, +-)
         if (i == 0)
         {
            mx_scaleeq_vec(&edge_offset_1, -1.0);
            if (j == 1)
               mx_scaleeq_vec(&edge_offset_2, -1.0);
         }       
         else
         if (j == 0)
            mx_scaleeq_vec(&edge_offset_2, -1.0);

         mxs_vector vertex_offset;
         mxs_vector vertex_loc;
                     
         mx_add_vec(&vertex_offset, &edge_offset_1, &edge_offset_2);
         mx_add_vec(&vertex_loc, &face_center, &vertex_offset);

         pNewCell->vertices[pNewCell->nVertices++] = vertex_loc;
      }
   }

   // See if we wound the wrong way
   mxs_vector vec1;
   mxs_vector vec2;
   mxs_vector cross_12;
   
   mx_sub_vec(&vec1, &pNewCell->vertices[1], &pNewCell->vertices[0]);
   mx_sub_vec(&vec2, &pNewCell->vertices[2], &pNewCell->vertices[0]);

   mx_cross_vec(&cross_12, &vec1, &vec2);

   if (mx_dot_vec(&cross_12, &pNewCell->plane.normal) > 0.0)
   {
      // We did, swap vertex order
      mxs_vector temp_vec;
                  
      temp_vec = pNewCell->vertices[0];
      pNewCell->vertices[0] = pNewCell->vertices[3];
      pNewCell->vertices[3] = temp_vec;

      temp_vec = pNewCell->vertices[1];
      pNewCell->vertices[1] = pNewCell->vertices[2];
      pNewCell->vertices[2] = temp_vec;
   }
}

////////////////////////////////////////

void AIGetAllPathableOBBCells(cAIObjCells * pCellList)
{
   sAIObjCell newObjCell;
   ObjID obj;

   // Iterate over all pathable objects
   cConcreteIter iter(g_pObjPathableProperty);
   iter.Start();

   while (iter.Next(&obj))
   {
      if (IsMovingTerrain(obj))
         continue;

      cPhysTypeProp *pTypeProp;
      if (g_pPhysTypeProp->Get(obj, &pTypeProp))
      {
         if (pTypeProp->type == kOBBProp)
         {
            cPhysDimsProp *pDimsProp;
            if (g_pPhysDimsProp->Get(obj, &pDimsProp))
            {
               GenerateOBBPathCell(&newObjCell, obj, pDimsProp->size, pDimsProp->offset[0]);

               pCellList->Append(newObjCell);
               newObjCell.pAIExternCell = NULL; // memory handed off to cell list (toml 03-06-00)
            }
            else
               Warning(("Obj %d marked as pathable, but no dimensions property!\n", obj));
         }
         else
               Warning(("Obj %d marked as pathable, but not OBB!\n", obj));
      }
      else
         Warning(("Obj %d marked as pathable, but no physics!\n", obj));
   }
   iter.Stop();
}

////////////////////////////////////////

void AIGetAllMovingTerrainCells(cAIObjCells *pCellList)
{
   sAIObjCell newObjCell;
   ObjID obj;

   AutoAppIPtr_(LinkManager, pLinkMan);

   IRelation *pTPNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);
   IRelation *pTPInitRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_INIT_NAME);

   // Iterate over all moving terrain objects
   cDynArray<ObjID> movingTerrainObjs;
   AIGetAllMovingTerrainObjs(&movingTerrainObjs);

   for (int i=0; i<movingTerrainObjs.Size(); i++)
   {
      obj = movingTerrainObjs[i];

      cPhysDimsProp *pDimsProp;
      g_pPhysDimsProp->Get(obj, &pDimsProp);

      LinkID init_link_id = pTPInitRel->GetSingleLink(obj, LINKOBJ_WILDCARD);
      sLink  init_link;

      pTPInitRel->Get(init_link_id, &init_link);

      ObjID start_obj = init_link.dest; 

      // Scan back to the beginning of the moving terrain links
      ObjID cur_obj = start_obj;
      LinkID cur_link_id;
      sLink  cur_link;

      cur_link_id = pTPNextRel->GetSingleLink(LINKOBJ_WILDCARD, cur_obj);
      while (cur_link_id != LINKID_NULL)
      {
         pTPNextRel->Get(cur_link_id, &cur_link);
         cur_obj = cur_link.source;

         if (cur_obj == start_obj)
         {
            // Circular, bail
            break;
         }

         cur_link_id = pTPNextRel->GetSingleLink(LINKOBJ_WILDCARD, cur_obj);
      }

      // Now iterate forward, generating cells as we go
      cur_link_id = pTPNextRel->GetSingleLink(cur_obj, LINKOBJ_WILDCARD);
      while (cur_link_id != LINKID_NULL)
      {
         // Move moving terrain object to waypoint and build cell
         ObjPosSetLocation(obj, &ObjPosGet(cur_obj)->loc);
         GenerateOBBPathCell(&newObjCell, obj, pDimsProp->size, pDimsProp->offset[0]);

         newObjCell.data = (void *)cur_obj;

         mprintf(".");
         pCellList->Append(newObjCell);
         newObjCell.pAIExternCell = NULL; // memory handed off to cell list (toml 03-06-00)

         pTPNextRel->Get(cur_link_id, &cur_link);
         cur_obj = cur_link.dest;

         if (cur_obj == start_obj)
         {
            // Wrapped, bail
            break;
         }

         cur_link_id = pTPNextRel->GetSingleLink(cur_obj, LINKOBJ_WILDCARD);
      }
   }

   movingTerrainObjs.SetSize(0);
}

////////////////////////////////////////

void AIGetAllMovingTerrainObjs(cDynArray<ObjID> *pObjList)
{
   ResetMovingTerrain();

   ObjID obj;

   cConcreteIter iter(g_pMovingTerrainProperty);
   iter.Start();
   
   while (iter.Next(&obj))
      pObjList->Append(obj);

   iter.Stop();

}

////////////////////////////////////////////////////////////////////////////////
