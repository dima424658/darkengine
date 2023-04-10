// $Header: x:/prj/tech/libsrc/md/RCS/hitdet.c 1.3 1997/07/17 10:53:07 CCAROLLO Exp $
//
// Model hit detection.
// Current implementation is straightforward, but doofy.
// Could be optimised by using bsp info and being smart.

#include <matrix.h>
#include <mds.h>

// Uncomment to use old brute-force (non-BSP) intersection methods
// #define USE_OLD_SEGMENT_HIT_DETECT
// #define USE_OLD_SPHERE_HIT_DETECT

static bool check_point_in_poly(mxs_vector *test, mds_pgon *p, mxs_vector *vlist, mxs_vector *norm)
{
   int i,n = p->num;
   ushort *v_index = p->verts;
   mxs_vector *v0, *v1;

   v0 = vlist + v_index[n-1];
   for (i=0; i<n; i++) {
      mxs_vector v10, vt0, cp;
      v1 = vlist + v_index[i];
      mx_sub_vec(&vt0, test, v0);
      mx_sub_vec(&v10, v1, v0);
      mx_cross_vec(&cp, &vt0, &v10);
      if (mx_dot_vec(&cp, norm) < 0.0)
         return FALSE;
      v0 = v1;
   }
   return TRUE;
}

static void check_pgon(mds_pgon *p, mds_segment_hit_detect_info *hdi)
{
   mxs_vector *v, *norm;
   mxs_vector temp, midp;
   mxs_real r, da, db;

   v = hdi->verts + p->verts[0]; // first point in poly
   norm = hdi->norms + p->norm;

   // test a and b on appropriate side of poly plane
   mx_sub_vec(&temp, hdi->a, v);
   if ((da = mx_dot_vec(&temp, norm))<0)
      return;
   mx_sub_vec(&temp, v, hdi->b);
   if ((db = mx_dot_vec(&temp, norm))<0)
      return;

   r = da/(db + da);       // guaranteed 0<r<1
   mx_interpolate_vec(&midp, hdi->a, hdi->b, r);

   if (check_point_in_poly(&midp, p, hdi->verts, norm) == FALSE)
      return;

   if (hdi->hit==FALSE) {
      hdi->hit = TRUE;
      hdi->hit_normal = norm;
      hdi->r = r;
      return;
   }
   if (hdi->r < r)
      return;

   hdi->hit_normal = norm;
   hdi->r = r;
}

static bool check_sphere_pgon(mds_pgon *p, mds_sphere_hit_detect_info *hdi)
{
   mxs_vector *v, *norm;
   mxs_vector *center;
   mxs_vector projection_point;
   mxs_vector a, b, v1, v2;
   mxs_vector v1c, v2c;
   mxs_vector edge_norm;
   mxs_real   dist;
   mxs_real   a2, edge_point_dist2;
   mxs_real   rad2;
   int        v1c_sign, v2c_sign;

   int i, n = p->num;

   v = hdi->verts + p->verts[0]; // first point in poly
   norm = hdi->norms + p->norm;
   center = &hdi->sphere.cen;

#ifndef BACKFACE_INTERSECTION
   // Determine which side the sphere is on
   mx_sub_vec(&temp, center, v);
   if (mx_dot_vec(&temp, norm) < 0)
      return FALSE;
#endif

   // Check if the sphere is close enough to the plane to intersect
   dist = mx_dot_vec(norm, center) + p->d;
   if (dist > hdi->sphere.rad || dist < -hdi->sphere.rad)
      return FALSE;

   // Check if any vertex lies within the sphere
   rad2 = hdi->sphere.rad * hdi->sphere.rad;
   for (i=0; i<n; i++) {
      v = hdi->verts + p->verts[i];
      if (mx_dist2_vec(v, center) <= rad2)
         return TRUE;
   }

   // Check if the sphere penetrates the face (if the center of the
   // sphere projects onto the interior of the polygon)
   mx_scale_add_vec(&projection_point, center, norm, -dist);
   if (check_point_in_poly(&projection_point, p, hdi->verts, norm)) 
      return TRUE;

   // Check if any edges intersect the sphere
   for (i=0; i<n; i++) {
      mx_copy_vec(&a, hdi->verts + p->verts[i]);
      mx_copy_vec(&b, hdi->verts + p->verts[(i+1)%n]);
      mx_sub_vec(&v1, center, &a);
      mx_sub_vec(&v2, &b, &a);

      // Check if edge vertices are on opposite sides of normal to sphere
      // by seeing if vertex->center vectors point different directions
      // with respect to the edge vector
      mx_sub_vec(&v1c, center, &a);
      mx_sub_vec(&v2c, center, &b);

      v1c_sign = (mx_dot_vec(&v1c, &v2) > 0) ? 1 : -1;
      v2c_sign = (mx_dot_vec(&v2c, &v2) > 0) ? 1 : -1;
      if (v1c_sign == v2c_sign)
         continue;

      // Determine distance from edge through the center of the
      // sphere via the area of the triangle (a->b->center->a)
      // length of the edge (see Graphics Gems II)
      mx_cross_vec(&edge_norm, &v1, &v2);
      a2 = mx_mag2_vec(&edge_norm);
      edge_point_dist2 = a2 / (mx_dist2_vec(&a, &b));

      if (edge_point_dist2 < rad2) 
         return TRUE;
   }

   return FALSE;
}


static mds_pgon *get_next(mds_pgon *p)
{
   int offset, list_size;

   // adjust for header, vlist, light list, and uv list
   list_size = p->num * sizeof(ushort);
   offset = sizeof(mds_pgon) + 2 * list_size;

   if ((p->type&MD_PGON_PRIM_MASK) == MD_PGON_PRIM_TMAP)
      offset += list_size;

   return (mds_pgon *)( ((uchar *)p) + offset );
}

// Fills in hit_normal with the normal to the polygon closest to a that
// intesects the segment a-b.  Fills in r with the fraction of the 
// distance from a to b at the intersection.
// Returns false if there are no intersecting polys, true otherwise.

#define n_split(x) ((mds_node_split *)(x))
#define n_raw(x)   ((mds_node_raw *)(x))

// Prototype these so forward-referencing doesn't produce warnings
void intersect_segment_node(mds_segment_hit_detect_info *hdi, uchar *n);
void intersect_sphere_node(mds_sphere_hit_detect_info *hdi, uchar *n);


#ifdef USE_OLD_SEGMENT_HIT_DETECT

void md_segment_hit_detect(mds_model *m, mds_segment_hit_detect_info *hdi)
{
   mds_pgon *p = (mds_pgon *)((uchar *)m+m->pgon_off);
   int i;

   hdi->verts = (mxs_vector*)((uchar *)m+m->point_off);
   hdi->norms = (mxs_vector*)((uchar *)m+m->norm_off);
   hdi->hit = FALSE;

   for (i=0; i<m->pgons; i++) {
      check_pgon(p, hdi);
      if(hdi->hit && hdi->early_out) 
         return;
      p = get_next(p);
   }
}

#else

void md_segment_hit_detect(mds_model *m, mds_segment_hit_detect_info *hdi)
{
   // Set up structure pointers
   hdi->verts = (mxs_vector*)((uchar *)m+m->point_off);
   hdi->norms = (mxs_vector*)((uchar *)m+m->norm_off);
   hdi->pgons = (uchar *)((uchar *)m+m->pgon_off);
   hdi->nodes = (uchar *)((uchar *)m+m->node_off);
   hdi->hit = FALSE;

   // Start recursion
   intersect_segment_node(hdi, (uchar *)hdi->nodes);
  
}

#endif

void intersect_segment_node(mds_segment_hit_detect_info *hdi, uchar *n)
{
   static int i;               // static to cut down on stack thrashing
   static mxs_real a_side, b_side;

   switch (*n) {
      case MD_NODE_SPLIT:
      {
         // Intersect with "after" polygons first because they're more
         // likely to be near the exterior of the model (for early-out)
         for (i=0; i<n_split(n)->pgons_after; ++i) {
            check_pgon((mds_pgon *)(hdi->pgons + n_split(n)->polys[i+n_split(n)->pgons_before]), hdi);
            if (hdi->hit && hdi->early_out)
               return;
         }

         // Determine which side of split plane each endpoint lies on
         a_side = mx_dot_vec((mxs_vector *)(hdi->norms + n_split(n)->norm), hdi->a) + n_split(n)->d;
         b_side = mx_dot_vec((mxs_vector *)(hdi->norms + n_split(n)->norm), hdi->b) + n_split(n)->d;

         // Completely in front, so only intersect with "in front" nodes
         if ((a_side > 0) && (b_side > 0)) 
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));

         // Completely behind, so only intersect with "behind" nodes
         else if ((a_side < 0) && (b_side < 0))
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));

         // Overlapping, so intersect with both.  Intersect first with side
         // on which majority of segment lies -- better for early-out?
         else if (a_side + b_side > 0) {
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));
            if (hdi->hit && hdi->early_out)
               return;
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));
         } else {
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));
            if (hdi->hit && hdi->early_out)
               return;
            intersect_segment_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));
         }
         
         // If recursion bailed, continue bailing, as done above
         if (hdi->hit && hdi->early_out)
            return;

         // Finally intersect with "before" polygons
         for (i=0; i<n_split(n)->pgons_before; ++i) {
            check_pgon((mds_pgon *)(hdi->pgons + n_split(n)->polys[i]), hdi);
            if (hdi->hit && hdi->early_out)
               return;
         }

         break;
      }

      case MD_NODE_RAW:
      {
         // BSP leaf, simply intersect with polygons
         for (i=0; i<n_raw(n)->num; i++) {
            check_pgon((mds_pgon *)((uchar *)hdi->pgons + n_raw(n)->polys[i]), hdi);
            if (hdi->hit && hdi->early_out)
               return;
         }

         break;
      }

      case MD_NODE_SUBOBJ:
      {
         // Continue checking at the subobject's root node
         intersect_segment_node(hdi, (uchar *)(n + sizeof(mds_node_subobj)));

         break;
      }

      // Ignore other node types
      default:
         break;
   }
}

#ifdef USE_OLD_SPHERE_HIT_DETECT

void md_sphere_hit_detect_old(mds_model *m, mds_sphere_hit_detect_info *hdi)
{
   mds_pgon *p = (mds_pgon *)((uchar *)m+m->pgon_off);
   int i;

   // First check against model's bounding sphere
   if(mx_mag2_vec(&(hdi->sphere.cen)) > ((m->radius + hdi->sphere.rad) * (m->radius + hdi->sphere.rad))) {
      hdi->hit = FALSE;
      hdi->num = 0;
   }

   hdi->verts = (mxs_vector*)((uchar *)m+m->point_off);
   hdi->norms = (mxs_vector*)((uchar *)m+m->norm_off);
   hdi->num = 0;
   hdi->hit = FALSE;

   for (i=0; i<m->pgons; i++) {
      if(check_sphere_pgon(p, hdi)) {
         hdi->hit = TRUE;
         if(hdi->early_out)
            return;
         hdi->polys[hdi->num] = i;
         hdi->num++;
      }
      p = get_next(p);
   }
}

#else

void md_sphere_hit_detect(mds_model *m, mds_sphere_hit_detect_info *hdi)
{
   // First check against model's bounding sphere
   if(mx_mag2_vec(&(hdi->sphere.cen)) > ((m->radius + hdi->sphere.rad) * (m->radius + hdi->sphere.rad))) {
      hdi->hit = FALSE;
      hdi->num = 0;
   }

   // Set up structure pointers
   hdi->verts = (mxs_vector*)((uchar *)m+m->point_off);
   hdi->norms = (mxs_vector*)((uchar *)m+m->norm_off);
   hdi->pgons = (uchar *)((uchar *)m+m->pgon_off);
   hdi->nodes = (uchar *)((uchar *)m+m->node_off);
   hdi->num = 0;
   hdi->hit = FALSE;

   // Start recursion
   intersect_sphere_node(hdi, (uchar *)hdi->nodes);
  
}

#endif

void intersect_sphere_node(mds_sphere_hit_detect_info *hdi, uchar *n)
{
   static int i;            // static to cut down on stack thrashing
   static mxs_real dist;

   switch (*n) {
      case MD_NODE_SPLIT:
      {
         // Intersect with "after" polygons first because they're more
         // likely to be near the exterior of the model (for early-out)
         for (i=0; i<n_split(n)->pgons_after; ++i) {
            if (check_sphere_pgon((mds_pgon *)(hdi->pgons + n_split(n)->polys[i+n_split(n)->pgons_before]), hdi)) {
               hdi->hit = TRUE;
               if (hdi->early_out)
                  return;
               hdi->polys[hdi->num] = n_split(n)->polys[i + n_split(n)->pgons_before];
               hdi->num++;
            }
         }

         // Determine where the sphere lies wrt to the split plane
         dist = mx_dot_vec((mxs_vector *)(hdi->norms + n_split(n)->norm), &hdi->sphere.cen) + n_split(n)->d;

         // Completely in front, so only intersect with "in front" nodes
         if (dist > hdi->sphere.rad) 
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));

         // Completely behind, so only intersect with "behind" nodes
         else if (dist < -hdi->sphere.rad)
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));

         // Overlapping, so intersect with both.  Intersect first with side
         // on which majority of sphere lies -- better for early-out?
         else if (dist > 0) {
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));
            if (hdi->hit && hdi->early_out)
               return;
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));
         } else {
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_behind));
            if (hdi->hit && hdi->early_out)
               return;
            intersect_sphere_node(hdi, ((uchar *)hdi->nodes + n_split(n)->node_front));
         }
         
         // If recursion bailed, continue bailing, as done above
         if (hdi->hit && hdi->early_out)
            return;

         // Finally intersect with "before" polygons
         for (i=0; i<n_split(n)->pgons_before; ++i) {
            if (check_sphere_pgon((mds_pgon *)(hdi->pgons + n_split(n)->polys[i]), hdi)) {
               hdi->hit = TRUE;
               if (hdi->early_out)
                  return;
               hdi->polys[hdi->num] = n_split(n)->polys[i];
               hdi->num++;
            }
         }

         break;
      }

      case MD_NODE_RAW:
      {
         // BSP leaf, simply intersect with polygons
         for (i=0; i<n_raw(n)->num; i++) {
            if (check_sphere_pgon((mds_pgon *)((uchar *)hdi->pgons + n_raw(n)->polys[i]), hdi)) {
               hdi->hit = TRUE;
               if (hdi->early_out)
                  return;
               hdi->polys[hdi->num] = n_raw(n)->polys[i];
               hdi->num++;
            }
         }

         break;
      }

      case MD_NODE_SUBOBJ:
      {
         // Continue checking at the subobject's root node
         intersect_sphere_node(hdi, (uchar *)(n + sizeof(mds_node_subobj)));

         break;
      }

      // Ignore other node types
      default:
         break;
   }
}






