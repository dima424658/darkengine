/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodobb.cpp,v 1.26 2000/01/09 18:10:00 adurant Exp $
//
//
//

#include <lg.h>
#include <matrixs.h>
#include <objshape.h>
#include <objscale.h>
#include <wrtype.h>
#include <portal.h>

#include <doorphys.h>

#include <phcore.h>
#include <phmodobb.h>
#include <phclsn.h>

// must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysOBBModel
//

cPhysOBBModel::cPhysOBBModel(ObjID objID, tPhysSubModId numSubModels, unsigned flags)
 : cPhysModel(objID, kPMT_OBB, numSubModels, flags)
{
   mxs_vector shape;
   mxs_vector scale;
   int i;

   ObjGetUnscaledDims(objID, &shape);
   if (ObjGetScale(objID, &scale))
   {
      for (i=0; i<3; i++)
         m_EdgeLen.el[i] = shape.el[i] * scale.el[i];
   }
   else
      mx_copy_vec(&m_EdgeLen, &shape);

   mx_scaleeq_vec(&m_EdgeLen, 0.999);  // a little epsilon

   m_EdgeTrigger = FALSE;
   m_ClimbableSides = 0;
   m_PoreSize = 0.0;

   mx_zero_vec(&m_Offset);

   // Setup submodel offsets
   for (i=0; i<6; i++)
   {
      mxs_vector offset;

      mx_unit_vec(&offset, i%3);
      mx_scaleeq_vec(&offset, 0.5);
      mx_elmuleq_vec(&offset, &m_EdgeLen);
      if (i > 2)
         mx_scaleeq_vec(&offset, -1.0);

         SetSubModOffset(i, offset);
   }
}

///////////////////////////////////////

cPhysOBBModel::cPhysOBBModel(PhysReadWrite func) : cPhysModel(func)
{
   // Load OBB-model specific stuff
   if (g_PhysVersion >= 24)
      func((void *)&m_Offset, sizeof(mxs_vector), 1);
   else
      mx_zero_vec(&m_Offset);

   func((void *)&m_EdgeLen, sizeof(mxs_vector), 1);

   if (g_PhysVersion >= 16)
      func((void *)&m_EdgeTrigger, sizeof(BOOL), 1);
   else
      m_EdgeTrigger = FALSE;

   if (g_PhysVersion >= 20)
      func((void *)&m_ClimbableSides, sizeof(int), 1);
   else
      m_ClimbableSides = 0;

   if (g_PhysVersion >= 32)
      func((void *)&m_PoreSize, sizeof(mxs_real),1);
   else
      m_PoreSize = 0.0;

   // Setup submodel offsets
   for (int i=0; i<6; i++)
   {
      mxs_vector offset;

      mx_unit_vec(&offset, i%3);
      mx_scaleeq_vec(&offset, 0.5);
      mx_elmuleq_vec(&offset, &m_EdgeLen);
      if (i > 2)
         mx_scaleeq_vec(&offset, -1.0);

         SetSubModOffset(i, offset);
   }
}

///////////////////////////////////////

cPhysOBBModel::~cPhysOBBModel()
{
}

///////////////////////////////////////

void cPhysOBBModel::Write(PhysReadWrite func) const
{
   // Write model-common data
   cPhysModel::Write(func);

   // Write OBB-model specific stuff 
   func((void *)&m_Offset, sizeof(mxs_vector), 1);
   func((void *)&m_EdgeLen, sizeof(mxs_vector), 1);

   func((void *)&m_EdgeTrigger, sizeof(BOOL), 1);

   func((void *)&m_ClimbableSides, sizeof(int), 1);
   func((void *)&m_PoreSize, sizeof(mxs_real), 1);
}

///////////////////////////////////////

BOOL cPhysOBBModel::CheckTerrainCollision(tPhysSubModId subModId, mxs_real t0, mxs_real dt, cPhysClsn ** ppClsn)
{

   *ppClsn = NULL;
   return FALSE;

#if 0
   mxs_matrix orien;
   mxs_vector unit, normal;
   mxs_vector start_pt, end_pt;
   mxs_vector offset;
   Location   start_loc, end_loc, hit_loc;
   mxs_real   best_time = 1.0;
   int        best_side = -1;
   int        best_cell;
   int        best_poly;
   int i;

   // Doors can't terrain collide
   if (IsDoor(GetObjID()))
   {
      *ppClsn = NULL;
      return FALSE;
   }

   mx_ang2mat(&orien, &GetRotation());

   mx_sub_vec(&offset, &GetEndLocationVec(), &GetLocationVec());

   for(i=0; i<6; i++)
   {
      mx_unit_vec(&unit, i % 3);
      mx_mat_mul_vec(&normal, &orien, &unit);
      mx_scaleeq_vec(&normal, mx_dot_vec(&unit, &GetEdgeLengths()) / 2);

      if (i >= 3)
         mx_scaleeq_vec(&normal, -1.0);

      mx_add_vec(&start_pt, &GetLocationVec(), &normal);
      mx_add_vec(&end_pt, &start_pt, &offset);

      MakeHintedLocationFromVector(&start_loc, &start_pt, &GetLocation());
      MakeLocationFromVector(&end_loc, &end_pt);

      if (!PortalRaycast(&start_loc, &end_loc, &hit_loc, TRUE))
      {
         mxs_vector inside, outside;
         mxs_real   time;

         mx_sub_vec(&inside, &hit_loc.vec, &start_loc.vec);
         mx_sub_vec(&outside, &end_loc.vec, &hit_loc.vec);
         time = sqrt(mx_mag2_vec(&inside) / (mx_mag2_vec(&inside) + mx_mag2_vec(&outside)));
         if (time < best_time)
         {
            AssertMsg1(PortalRaycastCell != -1, "Invalid cell from raycast for obj %d", GetObjID());
            AssertMsg1(PortalRaycastFindPolygon() != -1, "Invalid poly from raycast for obj %d", GetObjID());

            if ((PortalRaycastCell != -1) &&
                (PortalRaycastFindPolygon() != -1))
            {
               best_time = time;
               best_side = i;
               best_cell = PortalRaycastCell;
               best_poly = PortalRaycastFindPolygon();
            }
         }
      }
   }

   if (best_side == -1)
   {
      *ppClsn = NULL;
      return FALSE;
   }

   *ppClsn = new cPhysClsn(kPC_TerrainFace, t0, (best_time * dt), hit_loc.vec, 
                           this, best_side, GetLocationVec(), 
                           best_cell, best_poly);
   return TRUE;

#endif
}

///////////////////////////////////////

mxs_real cPhysOBBModel::TerrainDistance(tPhysSubModId subModId, cFaceContact *pFaceContact) const
{
   mxs_matrix orien;
   mxs_vector unit, normal;
   mxs_vector point;
   mxs_real   dist;
   mxs_real   best_dist = 100000;
   int i;   

   mx_ang2mat(&orien, &GetRotation());

   for (i=0; i<6; i++)
   {
      mx_unit_vec(&unit, i % 3);
      mx_mat_mul_vec(&normal, &orien, &unit);
      mx_scaleeq_vec(&normal, mx_dot_vec(&unit, &GetEdgeLengths()) / 2);

      if (i >= 3)
         mx_scaleeq_vec(&normal, -1.0);

      mx_add_vec(&point, &GetLocationVec(), &normal);
      
      dist = mx_dot_vec(&point, &pFaceContact->GetNormal()) + pFaceContact->GetPlaneConst();
      if (dist < best_dist)
         best_dist = dist;
   }

   return best_dist;
}

mxs_real cPhysOBBModel::TerrainDistance(tPhysSubModId subModId, cEdgeContact *pEdgeContact) const
{
   return 100.0;
}

mxs_real cPhysOBBModel::TerrainDistance(tPhysSubModId subModId, cVertexContact *pVertexContact) const
{
   return 100.0;
}

///////////////////////////////////////

ePhysIntersectResult cPhysOBBModel::TestLocationVec(int subModId, const mxs_vector & Location) const
{
   CriticalMsg("cPhysOBBModel::TestLocation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////

ePhysIntersectResult cPhysOBBModel::TestRotation(int subModId, const mxs_vector & Location) const
{
   CriticalMsg("cPhysOBBModel::TestRotation: not yet implemented\n");
   return kPCR_Nothing;
}

///////////////////////////////////////

void cPhysOBBModel::UpdateRefs(void)
{
   CriticalMsg("cPhysOBBModel::UpdateRefs: not yet implemented\n");
}

////////////////////////////////////////////////////////////////////////////////

void cPhysOBBModel::GetNormals(mxs_vector *start_list, mxs_vector *end_list)
{
   mxs_matrix mat, end_mat;
   BOOL same;

   mx_ang2mat(&mat, &GetRotation());

   same = (GetRotation().tx == GetEndRotationVec().tx) && 
          (GetRotation().ty == GetEndRotationVec().ty) &&
          (GetRotation().tz == GetEndRotationVec().tz);

   if (end_list && !same)
      mx_ang2mat(&end_mat, &GetEndRotationVec());

   int i;

   for (i=0; i<3; i++)
   {
      mx_copy_vec(&start_list[i], &mat.vec[i]);
      if (end_list)
      {
         if (same)
            mx_copy_vec(&end_list[i], &start_list[i]);
         else
            mx_copy_vec(&end_list[i], &end_mat.vec[i]);
      }
   }

   for (i=3; i<6; i++)
   {
      mx_copy_vec(&start_list[i], &mat.vec[i % 3]);
      mx_scaleeq_vec(&start_list[i], -1.0);
      if (end_list)
      {
         if (same)
            mx_copy_vec(&end_list[i], &start_list[i]);
         else
         {
            mx_copy_vec(&end_list[i], &end_mat.vec[i % 3]);
            mx_scaleeq_vec(&end_list[i], -1.0);
         }
      }
   }
}

void cPhysOBBModel::GetConstants(mxs_real *start_list, mxs_vector *start_norm_list, mxs_real *end_list, mxs_vector *end_norm_list)
{
   mxs_matrix start_rot_mat;
   mxs_matrix end_rot_mat;
   mxs_vector start_pt_on_plane;
   mxs_vector end_pt_on_plane;
   mxs_vector start_loc_vec;
   mxs_vector end_loc_vec;
   mxs_real edge_len;
   int i;

   // Build rotation matrices
   for (i=0; i<3; i++)
   {
      mx_copy_vec(&start_rot_mat.vec[i], &start_norm_list[i]);
      if (end_norm_list)
         mx_copy_vec(&end_rot_mat.vec[i], &end_norm_list[i]);
   }

   mxs_vector start_offset;
   mxs_vector end_offset;

   mx_mat_mul_vec(&start_offset, &start_rot_mat, &GetOffset());
   mx_add_vec(&start_loc_vec, &GetLocationVec(), &start_offset);

   if (end_norm_list)
   {
      mx_mat_mul_vec(&end_offset, &end_rot_mat, &GetOffset());
      mx_add_vec(&end_loc_vec, &GetEndLocationVec(), &end_offset);
   }

   for (i=0; i<6; i++)
   {
      edge_len = GetEdgeLengths().el[i % 3] / 2;

      mx_scale_add_vec(&start_pt_on_plane, &start_loc_vec, &start_norm_list[i], edge_len);
      start_list[i] = mx_dot_vec(&start_pt_on_plane, &start_norm_list[i]);

      if (end_norm_list)
      {
         mx_scale_add_vec(&end_pt_on_plane, &end_loc_vec, &end_norm_list[i], edge_len);
         end_list[i] = mx_dot_vec(&end_pt_on_plane, &end_norm_list[i]);
      }
   }
}

////////////////////////////////////////

void cPhysOBBModel::GetNormal(int side, mxs_vector *normal, BOOL end_of_frame)
{
   mxs_matrix mat;

   if (end_of_frame)
      mx_ang2mat(&mat, &GetEndRotationVec());
   else
      mx_ang2mat(&mat, &GetRotation());

   mx_copy_vec(normal, &mat.vec[side % 3]);

   if (side > 2)
      mx_scaleeq_vec(normal, -1.0);
}

mxs_real cPhysOBBModel::GetConstant(int side, mxs_vector &normal, BOOL end_of_frame)
{
   mxs_real edge_len;
   mxs_vector pt_on_plane;
   mxs_vector loc_vec;
   mxs_matrix obb_rot;
   mxs_vector obb_offset;

   mx_ang2mat(&obb_rot, &GetRotation());
   mx_mat_mul_vec(&obb_offset, &obb_rot, &GetOffset());

   if (end_of_frame)
      mx_add_vec(&loc_vec, &GetEndLocationVec(), &obb_offset);
   else
      mx_add_vec(&loc_vec, &GetLocationVec(), &obb_offset);

   edge_len = GetEdgeLengths().el[side % 3] / 2;

   mx_scale_add_vec(&pt_on_plane, &loc_vec, &normal, edge_len);

   return mx_dot_vec(&normal, &pt_on_plane);
}

////////////////////////////////////////

#ifndef SHIP

long cPhysOBBModel::InternalSize() const
{
   long model_size = cPhysModel::InternalSize();
   long obb_size = 0;

   return (model_size + obb_size);
}

#endif

////////////////////////////////////////







