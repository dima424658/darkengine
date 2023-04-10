/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomutil.cpp,v 1.8 1998/09/10 21:31:45 CCAROLLO Exp $
//
// Room System utilities
//

#include <lg.h>

#include <matrixs.h>
#include <matrix.h>
#include <math.h>

#include <wrtype.h>

#include <roombase.h>
#include <roomutil.h>

// Must be last header
#include <dbmem.h>


////////////////////////////////////////////////////////////////////////////////
//
// Returns signed distance from the plane to the point
//
mxs_real PointPlaneDist(const tPlane &plane, const mxs_vector &point)
{
   return mx_dot_vec((mxs_vector *)&plane.normal, (mxs_vector *)&point) + plane.d;
}

////////////////////////////////////////
//
// Finds intersection point of ray and plane.  Returns FALSE if no 
//  intersection.
//
BOOL RayPlaneIntersection(const tPlane &plane, const mxs_vector &from, const mxs_vector &dir, mxs_vector *intersection)
{
   mxs_vector to;
   mxs_real   dir_on_norm;
   mxs_real   from_dist;
   mxs_real   to_dist;

   dir_on_norm = fabs(mx_dot_vec(&dir, &plane.normal));

   if (dir_on_norm == 0)
      return FALSE;

   mx_add_vec(&to, &from, &dir);

   from_dist = PointPlaneDist(plane, from);
   to_dist = PointPlaneDist(plane, to);

   if ((from_dist * to_dist) > 0)
   {
      if (fabs(from_dist) < fabs(to_dist))
         mx_scale_add_vec(intersection, &from, &dir, -fabs(from_dist) / dir_on_norm);
      else
         mx_scale_add_vec(intersection, &to, &dir, fabs(to_dist) / dir_on_norm);
   }
   else
      return LinePlaneIntersection(plane, from, to, intersection);

   return TRUE;
}

////////////////////////////////////////
//
// Finds intersection of line segment and plane.  Returns FALSE if no
//  intersection.
//
BOOL LinePlaneIntersection(const tPlane &plane, const mxs_vector &from, const mxs_vector &to, mxs_vector *intersection)
{
   mxs_vector slope;
   mxs_real   from_dist, to_dist;

   from_dist = PointPlaneDist(plane, from);
   to_dist   = PointPlaneDist(plane, to);

   if (from_dist * to_dist > 0)
      return FALSE;

   from_dist = fabs(from_dist);
   to_dist   = fabs(to_dist);

   if ((from_dist + to_dist) < ON_PLANE_EPSILON)
   {
      mx_add_vec(intersection, &from, &to);
      mx_scaleeq_vec(intersection, 0.5);
      return TRUE;
   }

   mx_sub_vec(&slope, &to, &from);
   mx_scale_add_vec(intersection, (mxs_vector *)&from, &slope, (from_dist) / (from_dist + to_dist));

   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Calculates the list of planes that make up the OBB
//
void GetOBBPlanes(const tOBB &obb, tPlane plane_list[])
{
   mxs_matrix M;
   mxs_vector pt_on_plane;


   mx_ang2mat(&M, (mxs_angvec *)&obb.pos.fac);

   for (int i=0; i<6; i++)
   {
      // Calc normal
      mx_copy_vec(&plane_list[i].normal, &M.vec[i%3]);
      if (i >= 3)
         mx_scaleeq_vec(&plane_list[i].normal, -1);
 
      // Calc plane constant (dot of normal and any point on plane)
      mx_scale_vec(&pt_on_plane, &plane_list[i].normal, obb.scale.el[i%3]);
      mx_addeq_vec(&pt_on_plane, (mxs_vector *)&obb.pos.loc.vec);
      
      plane_list[i].d = -mx_dot_vec(&plane_list[i].normal, &pt_on_plane);
   }
}

////////////////////////////////////////
//
// Calculates the vertices that make up the OBB
// 
const int x_sign_table[4] = { 1, -1, -1, 1 };

void GetOBBVertices(const tOBB &obb, mxs_vector vertex_list[])
{
   mxs_matrix M;
   mxs_vector obb_vertex;


   mx_ang2mat(&M, (mxs_angvec *)&obb.pos.fac);

   for (int i=0; i<8; i++)
   {
      int x_sign = x_sign_table[(i & 3)];
      int y_sign = (i & 2) ? -1 : 1;
      int z_sign = (i & 4) ? -1 : 1;

      // Generate object-space vertex locations
      mx_mk_vec(&obb_vertex, obb.scale.x * x_sign, 
                             obb.scale.y * y_sign, 
                             obb.scale.z * z_sign);

      // Rotate into world space
      mx_mat_mul_vec(&vertex_list[i], &M, &obb_vertex);

      // Translate into world space
      mx_addeq_vec(&vertex_list[i], (mxs_vector *)&obb.pos.loc.vec);
   }
}

////////////////////////////////////////
//
// Returns bitfield representing which planes in the plane list it is
//  "in front" of.
//
short GetBits(const mxs_vector &pt, const tPlane plane_list[])
{
   short bits;


   bits = 0x0000;
   for (int i=0; i<6; i++)
   {
      if (PointPlaneDist(plane_list[i], pt) > ON_PLANE_EPSILON)
         bits |= (0x0001 << i);
   }

   return bits;
}

////////////////////////////////////////////////////////////////////////////////
//
// Adds an point to the list, but only if it is unique.
//
void AddIntersectionPoint(const mxs_vector &int_pt, mxs_vector pt_list[], int *num_in_list)
{
   int        i;
   mxs_vector delta;

   for (i=0; i<*num_in_list; i++)
   {
      mx_sub_vec(&delta, (mxs_vector *)&int_pt, &pt_list[i]);
      if (mx_mag2_vec(&delta) < SAME_POINT_EPSILON_2)
         return;
   }

   mx_copy_vec(&pt_list[*num_in_list], (mxs_vector *)&int_pt);
   (*num_in_list)++;
}

////////////////////////////////////////
//
// Calculates intersection points of edges between an OBBs vertices and
//  another OBBs list of planes.
//
void BuildIntersections(const short bits[], const mxs_vector vertices[], const tPlane planes[], mxs_vector int_list[], int *num_ints)
{
   mxs_vector  intersection;
   short       clip_planes;
   int         i, j;

   // Check each edge's intersection
   for (i=0; i<12; i++)
   {
      // If they have common bits, then they both lie on the same side of
      //  a plane and can be ignored
      if (bits[OBBEdge[i].from] & bits[OBBEdge[i].to])
         continue;

      // Find all the clipping planes
      clip_planes = (bits[OBBEdge[i].from] | bits[OBBEdge[i].to]);

      // If both are inside (no clipping planes), ignore them
      if (clip_planes == 0x0000)
         continue;

      // Find the intersection point
      for (j=0; j<6; j++)
      {
         if (clip_planes & (0x0001 << j))
         {
            LinePlaneIntersection(planes[j], vertices[OBBEdge[i].from],
                                  vertices[OBBEdge[i].to], &intersection);
       
            if (GetBits(intersection, planes) == 0x0000)
               AddIntersectionPoint(intersection, int_list, num_ints);
         }
      }
   }
}

////////////////////////////////////////
//
// Culls a list of points (based on distance) down to four.
//
void CullIntPts(const mxs_vector &center, mxs_vector int_pts[], int *num_ints)
{
   mxs_vector closest[4];
   mxs_real   closest_dist[4];
   mxs_real   dist;
   int        next_out;
   int        i, j;

   AssertMsg1(*num_ints > 4, "Attempting to cull %d points", *num_ints);

   // Init with the first four
   for (i=0; i<4; i++)
   {
      mx_copy_vec(&closest[i], &int_pts[i]);
      closest_dist[i] = mx_dist2_vec((mxs_vector *)&center, &int_pts[i]);
   }

   // Find the largest of the first four
   next_out = 0;
   for (i=1; i<4; i++)
   {
      if (closest_dist[i] > closest_dist[next_out])
         next_out = i;
   }

   // Check each remaining against the first four
   for (i=4; i<*num_ints; i++)
   {
      dist = mx_dist2_vec((mxs_vector *)&center, &int_pts[i]);
      if (dist < closest_dist[next_out])
      {
         mx_copy_vec(&closest[next_out], &int_pts[i]);
         closest_dist[next_out] = dist;
         for (j=0; j<4; j++)
         {
            if (closest_dist[j] > closest_dist[next_out])
               next_out = j;
         }
      }
   }

   // And copy closest 4 back into array
   for (i=0; i<4; i++)
      mx_copy_vec(&int_pts[i], &closest[i]);

   *num_ints = 4;
}

////////////////////////////////////////////////////////////////////////////////
//
// Calculates the plane that a list of point lie in.  Returns FALSE if the 
//  points are not coplanar.
//
BOOL PlaneFromPoints(const mxs_vector points[], int numPoints, tPlane *newPlane)
{
   mxs_vector v1, v2;

   AssertMsg1(numPoints > 2, "Attempt to create plane from %d points", numPoints);

   // Calc normal
   mx_sub_vec(&v1, (mxs_vector *)&points[1], (mxs_vector *)&points[0]);
   mx_sub_vec(&v2, (mxs_vector *)&points[2], (mxs_vector *)&points[0]);

   mx_cross_vec(&newPlane->normal, &v1, &v2);
   mx_normeq_vec(&newPlane->normal);

   // And plane constant
   newPlane->d = -mx_dot_vec(&newPlane->normal, (mxs_vector *)&points[0]);

   // Now check any further points for coplanarity
   for (int i=3; i<numPoints; i++)
   {
      if (fabs(PointPlaneDist(*newPlane, points[i])) > ON_PLANE_EPSILON)
         return FALSE;
   }

   return TRUE;
}        

////////////////////////////////////////
//
// Calculates the outward-facing planes of a portal based on its plane and
//  its (non-wound) vertex list.
//
void PortalEdgePlanes(const mxs_vector points[], int numPoints, const tPlane &portalPlane, tPlane portalEdges[])
{
   mxs_vector  edge;
   int         num_edges;
   BOOL        first_test;
   BOOL        bad_edge;
   int  i, j, k;

   // Make an edge plane from each point, pairwise
   num_edges = 0;
   for (i=0; i<numPoints; i++)
   {
      for (j=0; j<i; j++)
      {
        // Make the normal
        mx_sub_vec(&edge, (mxs_vector *)&points[i], (mxs_vector *)&points[j]);
        mx_cross_vec(&portalEdges[num_edges].normal, &edge, (mxs_vector *)&portalPlane.normal);
        mx_normeq_vec(&portalEdges[num_edges].normal);

        // And the plane constant
        portalEdges[num_edges].d = -mx_dot_vec(&portalEdges[num_edges].normal, (mxs_vector *)&points[i]);

        // Check that all remaining points are behind the new plane.  You are 
        //  allowed to flip the plane for the first point tested (in case you 
        //  guessed the normal incorrectly), but not after that because that 
        //  means the plane is interior to the portal.
        first_test = TRUE;
        bad_edge = FALSE;

        for (k=0; k<numPoints && !bad_edge; k++)
        {
           if ((k == i) || (k == j))
              continue;

           if (PointPlaneDist(portalEdges[num_edges], points[k]) > ON_PLANE_EPSILON)
           {
              if (first_test)
              {
                 mx_scaleeq_vec(&portalEdges[num_edges].normal, -1.0);
                 portalEdges[num_edges].d *= -1.0;
              }
              else
                 bad_edge = TRUE;
           }
           first_test = FALSE;
        }

        // If it's a valid edge, increment the number of edges so we keep it
        if (!bad_edge)
           num_edges++;
      }
   }

   AssertMsg2(num_edges == numPoints, "Different number of portal edges (%d) than points (%d)", num_edges, numPoints);
}
 
////////////////////////////////////////
//
// Calculates the center point of a portal from its vertex list.
//
void PortalCenter(const mxs_vector portalPoints[], int numPortalPoints, mxs_vector *portalCenter)
{
   AssertMsg(numPortalPoints != 0, "Attempt to find center of 0 points");

   mx_zero_vec(portalCenter);
   for (int i=0; i<numPortalPoints; i++)
      mx_addeq_vec(portalCenter, (mxs_vector *)&portalPoints[i]);
   mx_scaleeq_vec(portalCenter, 1.0 / numPortalPoints);
}

////////////////////////////////////////////////////////////////////////////////
//
// Determines whether two OBBs intersect.  From "Fast Overlap Test for
//  OBBs", by ???.  I got the paper excerpt from Doug.
//

// First index removes an index, second index returns the other index
const int index_lookup[3][3] = 
{ { -1,  2,  1},
  {  2, -1,  0},
  {  1,  0, -1} };

BOOL OBBsIntersect(const tOBB &b1, const tOBB &b2)
{
   mxs_vector T;           // Translation vector between brushes
   mxs_vector L;           // Sparating Axis
   mxs_real T_L;           // Length of projection of T onto L
   mxs_matrix M, M_abs;    // Rotation matrix between brushes, and 
                           //  absolute value version
   mxs_matrix M_1, M_2;    // Rotation matrices for each brush
   mxs_real   sum;         // Accumulation of summation elements
   int i, j, k;

   // Calculate T vector 
   mx_sub_vec(&T, (mxs_vector *)&b2.pos.loc.vec, (mxs_vector *)&b1.pos.loc.vec); 

   // Calculate orientation matrices for each obb
   mx_ang2mat(&M_1, (mxs_angvec *)&(b1.pos.fac));
   mx_ang2mat(&M_2, (mxs_angvec *)&(b2.pos.fac));

   // Transform to obb1-relative rotation
   mx_mul_mat(&M, &M_1, &M_2);

   // Calculate absolute valued M matrix
   for (i=0; i<9; ++i) 
      M_abs.el[i] = (M.el[i] > 0) ? M.el[i] : -M.el[i];

   // Use first brush's normals for separating axis generation
   for (i=0; i<3; ++i) 
   {
      // Calculate distance between projected centers
      T_L = fabs(mx_dot_vec(&T, &(M_1.vec[i])));

      // Calculate length of maximum radius of brush, projected
      sum = b1.scale.el[i];
      for (j=0; j<3; ++j)
         sum += b2.scale.el[j] * fabs(mx_dot_vec(&M_2.vec[j], &M_1.vec[i]));

      if (sum < T_L)
         return FALSE;
   }

   // Use second brush's normals for separating axis generation
   for (i=0; i<3; ++i) 
   {
      // Calculate distance between projected centers
      T_L = fabs(mx_dot_vec(&T, &(M_2.vec[i])));

      // Calculate length of maximum radius of brush, projected
      sum = b2.scale.el[i];
      for (j=0; j<3; ++j)
         sum += b1.scale.el[j] * fabs(mx_dot_vec(&M_1.vec[j], &M_2.vec[i]));

      if (sum < T_L)
         return FALSE;
   }

   // Use edge-pairs for separating axis generation
   for (i=0; i<3; ++i) 
   {
      for (j=0; j<3; ++j) 
      {
         mx_cross_vec(&L, &(M_1.vec[i]), &(M_2.vec[j]));
         if (mx_mag2_vec(&L) > 0.0001)
            mx_normeq_vec(&L);
         T_L = fabs(mx_dot_vec(&T, &L));

         // Do summations
         sum = 0;

         for (k=0; k<3; k++)
            sum += b1.scale.el[k] * fabs(mx_dot_vec(&M_1.vec[k], &L));

         for (k=0; k<3; k++)
            sum += b2.scale.el[k] * fabs(mx_dot_vec(&M_2.vec[k], &L));

         // If radius is smaller than the distance from center to center,
         //  then they must not intersect
         if (sum < T_L)
            return FALSE;
      }
   }

   // All separating axis tests fell through, must intersect
   return TRUE;
}

////////////////////////////////////////
//
// Determine if a point in inside an obb
//
BOOL PointInOBB(const tOBB &obb, const mxs_vector &pt)
{
   static tPlane planes[6];

   GetOBBPlanes(obb, planes);
   return (GetBits(pt, planes) == 0x0000);
}


////////////////////////////////////////
//
// Given two OBBS, determines if they intersect and calculates the portal's
//  plane, the planes of the portal's edges, and the center point of the 
//  portal.
//
void FindOBBPortal(const tOBB &b1, const tOBB &b2, tPlane *portalPlane, tPlane portalEdges[], int *portalEdgeSize, mxs_vector *center)
{
   mxs_vector  b1_int_pt[24];
   mxs_vector  b2_int_pt[24];
   int         b1_num_int_pts; 
   int         b2_num_int_pts; 

   tPlane      b1_planes[6];
   tPlane      b2_planes[6];
   mxs_vector  b1_vertices[8];
   mxs_vector  b2_vertices[8];
   short       b1_bits[8];
   short       b2_bits[8];

   int         i;


   b1_num_int_pts = 0;
   b2_num_int_pts = 0;

   // Find the planes of the OBBs
   GetOBBPlanes(b1, b1_planes);
   GetOBBPlanes(b2, b2_planes);

   // Find the vertices of the OBBs
   GetOBBVertices(b1, b1_vertices);
   GetOBBVertices(b2, b2_vertices);

   // Find relative octants of each point
   for (i=0; i<8; i++)
   {
      b1_bits[i] = GetBits(b1_vertices[i], b2_planes);
      b2_bits[i] = GetBits(b2_vertices[i], b1_planes);
   }

   // Find the edge intersections
   BuildIntersections(b1_bits, b1_vertices, b2_planes, b1_int_pt, &b1_num_int_pts);
   BuildIntersections(b2_bits, b2_vertices, b1_planes, b2_int_pt, &b2_num_int_pts);

   if (b1_num_int_pts > 4)
      CullIntPts(b1.pos.loc.vec, b1_int_pt, &b1_num_int_pts);
   if (b2_num_int_pts > 4)
      CullIntPts(b2.pos.loc.vec, b2_int_pt, &b2_num_int_pts);

   // @TODO: Remove this when room brushes actually exist
   if ((b1_num_int_pts == 0) && (b2_num_int_pts == 0))
   {
      *portalEdgeSize = 0;      
      return;
   }

   // Try using b1 if it has 4 points
   if (b1_num_int_pts == 4)
   {
      if (PlaneFromPoints(b1_int_pt, b1_num_int_pts, portalPlane))
      {
         PortalEdgePlanes(b1_int_pt, b1_num_int_pts, *portalPlane, portalEdges);
         PortalCenter(b1_int_pt, b1_num_int_pts, center);
         *portalEdgeSize = b1_num_int_pts;
         return;
      }
   }

   // Try using b2 if it has 4 points
   if (b2_num_int_pts == 4)
   {
      if (PlaneFromPoints(b2_int_pt, b2_num_int_pts, portalPlane))
      { 
         PortalEdgePlanes(b2_int_pt, b2_num_int_pts, *portalPlane, portalEdges);
         PortalCenter(b2_int_pt, b2_num_int_pts, center);
         *portalEdgeSize = b2_num_int_pts;
         return;
      }
   }

   if (b1_num_int_pts == 3)
   {
      PlaneFromPoints(b1_int_pt, b1_num_int_pts, portalPlane);
      PortalEdgePlanes(b1_int_pt, b1_num_int_pts, *portalPlane, portalEdges);
      PortalCenter(b1_int_pt, b1_num_int_pts, center);
      *portalEdgeSize = b1_num_int_pts;
      return;
   }

   if (b2_num_int_pts == 3)
   {
      PlaneFromPoints(b2_int_pt, b2_num_int_pts, portalPlane);
      PortalEdgePlanes(b2_int_pt, b2_num_int_pts, *portalPlane, portalEdges);
      PortalCenter(b2_int_pt, b2_num_int_pts, center);
      *portalEdgeSize = b2_num_int_pts;
      return;
   }

   *portalEdgeSize = 0;
}






