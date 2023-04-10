/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phterr.cpp,v 1.12 2000/02/19 12:32:23 toml Exp $
//
//
//

#include <lg.h>
#include <matrix.h>
#include <math.h>
#include <wrfunc.h>

#include <phcontct.h>
#include <phterr.h>

class cPhysSubModelInst;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPolyList
//

cPhysTerrPolyList::~cPhysTerrPolyList(void)
{
   cPhysTerrPoly *pPoly = GetFirst();
   while (pPoly != NULL)
   {
      delete pPoly;
      pPoly = GetFirst();
   }
}

///////////////////////////////////////

BOOL cPhysTerrPolyList::GetNormal(mxs_vector & normal, const mxs_vector &sphere_loc, mxs_real sphere_radius) const
{
   cPhysTerrPoly *pPoly = GetFirst();

   // count the number of polygons
   int poly_count = 0;
   while (pPoly != NULL)
   {
      poly_count++;
      pPoly = pPoly->GetNext();
   }

   switch (poly_count)
   {
      case 0: 
      {
         Warning(("Zero-length poly list?\n"));
         return FALSE;
      }

      case 1: 
      {
         pPoly = GetFirst();
         mx_copy_vec(&normal, &pPoly->GetNormal());
         break;
      }

      case 2:
      {
         mxs_vector *start, *end;
         int cell_id, poly_id;
         int offset, offset_max;
         int i, j;

         mxs_real best_edge_dist = -1.0;

         pPoly = GetFirst();
         while (pPoly != NULL)
         {
            cell_id = pPoly->GetCellID();
            poly_id = pPoly->GetPolyID();

            // Find starting offset of poly
            offset = 0;
            for (i=0; i<poly_id; i++)
               offset += WR_CELL(cell_id)->poly_list[i].num_vertices;

            offset_max = offset + WR_CELL(cell_id)->poly_list[poly_id].num_vertices;

            // Find an edge that contains the collision point
            for (i=offset; i<offset_max; i++)
            {
               j = ((i - offset + 1) % (offset_max - offset)) + offset;
               start = &WR_CELL(cell_id)->vpool[WR_CELL(cell_id)->vertex_list[i]];
               end   = &WR_CELL(cell_id)->vpool[WR_CELL(cell_id)->vertex_list[j]];

               cEdgeContact edgeContact(*start, *end);
               mxs_real edge_dist = fabs(edgeContact.GetDist(sphere_loc) - sphere_radius);
               if ((best_edge_dist < 0.0) || (edge_dist < best_edge_dist))
               {
                  best_edge_dist = edge_dist;
                  mx_copy_vec(&normal, &edgeContact.GetNormal(sphere_loc));
               }
            }

            pPoly = pPoly->GetNext();
         }

         if (best_edge_dist < 0.0)
         {
            Warning(("No edge found for normal computation!\n"));
            mx_unit_vec(&normal, 0); // uh, why not
            return FALSE;
         }

         break;
      }

      default:
      {
         if ((poly_count < 0) || (poly_count > 20))
         {
            Warning(("Suspicious number of polys in list: %d\n", poly_count));
            return FALSE;
         }

         mxs_vector *vertex;
         int cell_id, poly_id;
         int offset, offset_max;
         int i;
         mxs_real best_vertex_dist = -1.0;

         pPoly = GetFirst();
         while (pPoly != NULL)
         {
            cell_id = pPoly->GetCellID();
            poly_id = pPoly->GetPolyID();

            // Find starting offset of poly
            offset = 0;
            for (i=0; i<poly_id; i++)
               offset += WR_CELL(cell_id)->poly_list[i].num_vertices;

            offset_max = offset + WR_CELL(cell_id)->poly_list[poly_id].num_vertices;

            // Find a vertex that is the right distance from the collision point
            for (i=offset; i<offset_max; i++)
            {
               vertex = &WR_CELL(cell_id)->vpool[WR_CELL(cell_id)->vertex_list[i]];

               mxs_real vertex_dist = fabs(mx_dist_vec(vertex, &sphere_loc) - sphere_radius);

               if ((best_vertex_dist < 0.0) || (vertex_dist < best_vertex_dist))
               {
                  best_vertex_dist = vertex_dist;
                  mx_sub_vec(&normal, &sphere_loc, vertex);
               }
            }

            pPoly = pPoly->GetNext();
         }

         if (best_vertex_dist < 0.0)
         {
            Warning(("No edge found for normal computation!\n"));
            mx_unit_vec(&normal, 0); // uh, why not
            return FALSE;
         }

         if (mx_mag2_vec(&normal) > 0.0001)
            mx_normeq_vec(&normal);

         break;
      }
   }

   return TRUE;
}

///////////////////////////////////////
//
// Find a poly in a list
//

BOOL cPhysTerrPolyList::Find(int cellID, int polyID, cPhysTerrPoly ** ppTerrPoly) const
{
   cPhysTerrPoly *pPoly = GetFirst();

   while (pPoly != NULL)
   {
      if ((pPoly->GetCellID() == cellID) && (pPoly->GetPolyID() == polyID))
      {
         *ppTerrPoly = pPoly;
         return TRUE;
      }
      pPoly = pPoly->GetNext();
   }

   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPoly
//

#include <wr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

int cPhysTerrPoly::GetPlaneID(void) const
{
   return WR_CELL(m_cellID)->poly_list[m_polyID].planeid;
}

///////////////////////////////////////

const mxs_vector & cPhysTerrPoly::GetNormal(void) const
{
   return WR_CELL(m_cellID)->plane_list[GetPlaneID()].normal;
}

///////////////////////////////////////

mxs_real cPhysTerrPoly::GetPlaneConstant(void) const
{
   return WR_CELL(m_cellID)->plane_list[GetPlaneID()].plane_constant;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPolyArray
//

///////////////////////////////////////////////////////////////////////////////

