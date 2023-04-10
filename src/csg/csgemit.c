/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgemit.c,v 1.46 2000/02/19 12:54:59 toml Exp $

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <mprintf.h>

#include <r3d.h>
#include <wr.h>
#include <wrlimit.h>
#include <wrdbrend.h>
#include <portal.h>

#include <csg.h>
#include <bspdata.h>
#include <hep.h>
#include <bsppinfo.h>
#include <csgbrush.h>
#include <csgmerge.h>
#include <csgcheck.h>
#include <csgalloc.h>
#include <csgutil.h>
#include <missrend.h>
#include <rand.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


extern PortalPolyEdge *AddEdgeToList(PortalPolyEdge *list, PortalPolyEdge *edge);
extern void RegisterFace(ulong brface, int cell, int surface, int vertex);
extern PortalPolygon *PortalClipPolygonByPlane(PortalPolygon *source, BspPlane *clip, PortalPolyhedron *ph, bool merge);
extern int PortalPolygonPlaneCompare(PortalPolygon *p, BspPlane *plane);
extern void split_polyhedron(PortalPolyhedron *ph);
extern void cross_prod(BspVertex *cp, BspVertex *p1, BspVertex *p2, BspVertex *p3);

//extern void build_polygon_decals(PortalLightMap *p, int brush, int face);

//extern bool decal_hack;
extern int *wr_brfaces[MAX_REGIONS];

extern PortalPolygon *PortalPolyCopy(PortalPolygon *source);

///////////////////////////////////////////////////////////////////////////////////////
//
//     malloc data structures for the world rep (self-freeing)
//
//     !!! *** No longer used in due to memory leaks and code ugliness *** !!!

///////////////////////////////////////////////////////////////////////////////////////
//
//    find a point inside the polyhedron (so we can determine poly facing correctly)
//

BspVertex nnside;

int num_inside_pts;

void compute_ph_center_init(void)
{
   nnside.x = 0;
   nnside.y = 0;
   nnside.z = 0;

   num_inside_pts = 0;
}

void compute_ph_center_more(PortalPolygon *poly)
{
   PortalPolyEdge *edge = poly->edge, *first = edge;

   do {
      nnside.x += edge->data->start.x + edge->data->end.x;
      nnside.y += edge->data->start.y + edge->data->end.y;
      nnside.z += edge->data->start.z + edge->data->end.z;
      num_inside_pts += 2;
      edge = edge->poly_next;
   } while (edge != first);
}

void compute_ph_center_finish(void)
{
   nnside.x /= num_inside_pts;
   nnside.y /= num_inside_pts;
   nnside.z /= num_inside_pts;
}

void compute_center(PortalPolyhedron *ph)
{
   PortalPolygon *first, *poly;

   compute_ph_center_init();

   first = poly = ph->poly;
   do {
      compute_ph_center_more(poly);
      poly = GetPhNext(poly, ph);
   } while (poly != first);

   compute_ph_center_finish();
}

////////////////////////////
//
//  generic linear algebra
//
//   Mx = b
//
//  M is expressed as three row vectors
//
//  We use Cramer's rule.

double mat_det(double m00, double m01, double m02,
               double m10, double m11, double m12,
               double m20, double m21, double m22)
{
   return m00*(m11*m22-m12*m21)
        + m10*(m21*m02-m22*m01)
        + m20*(m01*m12-m02*m11);
}

void compute_3_linear_eq(mxs_vector *x, mxs_vector *M1,
     mxs_vector *M2, mxs_vector *M3, double bx, double by, double bz)
{
   double divisor =
      mat_det(M1->x,M1->y,M1->z, M2->x,M2->y,M2->z, M3->x,M3->y,M3->z);

   x->x = mat_det(bx,M1->y,M1->z, by,M2->y,M2->z, bz,M3->y,M3->z) / divisor;
   x->y = mat_det(M1->x,bx,M1->z, M2->x,by,M2->z, M3->x,bz,M3->z) / divisor;
   x->z = mat_det(M1->x,M1->y,bx, M2->x,M2->y,by, M3->x,M3->y,bz) / divisor;
}


///////////////////////////////////////////////////////////////////////////////////////
//
//
//  output a portalized database into world-rep format
//
//

#define MAX_POINTS  8192

#define MAX_PALETTE  2048
#define MAX_POLYGONS 1024
#define MAX_PPP      128

#define MAX_PORTAL_POLYGONS max_portal_polygons
#define MAX_PORTAL_VERTICES 128

int max_portal_polygons=64;
int num_cells;
int num_points;
int num_poly;
int num_portal;

// Build up the data for a single polyhedron

//   vertex palette
int num_vertices;
BspVertex vpal[MAX_PALETTE];

static int vertex_palette(BspVertex *point)
{
   int i;
   for (i=0; i < num_vertices; ++i) {
      if (!VERTEX_COMPARE(vpal[i], *point))
         return i;
   }
   if (num_vertices == MAX_PALETTE)
      Error(1, "Too many vertices in cell (MAX_PALETTE).\n");
   vpal[num_vertices] = *point;
   return num_vertices++;
}

int poly_nv[MAX_POLYGONS];
int poly_dest[MAX_POLYGONS];
int poly_brface[MAX_POLYGONS];
int poly_clut[MAX_POLYGONS];
BspPlane *poly_plane[MAX_POLYGONS];
uchar poly_render[MAX_POLYGONS];
int vertices[MAX_POLYGONS][MAX_PPP];
int cur_poly;
mxs_vector this_norm;

static bool check_poly_reversed(int k)
{
   BspVertex cp, dir, best_cp;
   int g,h,i, best=-1;
   Real best_sz=0, sz;
   mxs_vector norm;
   h = poly_nv[k]-1;
   norm.x = norm.y = norm.z = 0;
   for (i=0; i < poly_nv[k]; ++i) {
      norm.x +=  (vpal[vertices[k][h]].y - vpal[vertices[k][i]].y)
                *(vpal[vertices[k][h]].z + vpal[vertices[k][i]].z);
      norm.y +=  (vpal[vertices[k][h]].z - vpal[vertices[k][i]].z)
                *(vpal[vertices[k][h]].x + vpal[vertices[k][i]].x);
      norm.z +=  (vpal[vertices[k][h]].x - vpal[vertices[k][i]].x)
                *(vpal[vertices[k][h]].y + vpal[vertices[k][i]].y);
      h = i;
   }
   this_norm = norm;

   // grab three non-co-linear points

   // take two points and then find a non-colinear third

   for (g=0; g < poly_nv[k]; ++g) {
      h = g+1; if (h == poly_nv[k]) h = 0;
      i = h+1; if (i == poly_nv[k]) i = 0;
      cross_prod(&cp, &vpal[vertices[k][g]], &vpal[vertices[k][h]], &vpal[vertices[k][i]]);
      sz = cp.x * cp.x + cp.y * cp.y + cp.z * cp.z;
      if (sz > best_sz) { best_cp = cp; best_sz = sz; best = i; }
   }
   i = best;
   cp = best_cp;

   if (i == -1)
      Error(1, "check_poly_reversed: Couldn't find non-colinear points.\n");

     // shift the points down until the first three points aren't colinear
   while (i != 2) {
      int n,j;
      n = vertices[k][0];
      for (j=0; j < poly_nv[k]-1; ++j)
         vertices[k][j] = vertices[k][j+1];
      vertices[k][j] = n;
      if (--i < 0) i = poly_nv[k]-1;
   }

   // take vector from center to first point
   dir = vpal[vertices[k][0]];

   dir.x -= nnside.x;
   dir.y -= nnside.y;
   dir.z -= nnside.z;

   // check the dot product of the two
   return (cp.x * dir.x + cp.y * dir.y + cp.z * dir.z < 0);
}


extern int stat_max_vertices;

// plane_count is passed in, maybe incremented
void emit_polygon(int k, PortalCell *cell, PortalPolygonCore *pcore, int *vcount, int *plane_count, int surface)
{
   int i, v, vc = *vcount, n;
   bool reversed;

   PortalPlane this_plane;
   mxs_vector this_normal;      // the plane of this polygon
   mxs_vector bnorm;

   n = pcore->num_vertices = poly_nv[k];
   pcore->clut_id = poly_clut[k];
   pcore->flags = 0;
   pcore->motion_index = 0;

   if (n > stat_max_vertices)
      stat_max_vertices = n;

   if (pcore->clut_id)
      pcore->flags |= PORTAL_SPLITS_OBJECT;

   // copy vertices

   vc = *vcount;

   reversed = check_poly_reversed(k);

   if (reversed) {
      for (i=2; i >= 0; --i)
         cell->vertex_list[vc++] = vertices[k][i];
      for (i=poly_nv[k]-1; i >= 3; --i)
         cell->vertex_list[vc++] = vertices[k][i];
   } else
      for (i=0; i < poly_nv[k]; ++i)
         cell->vertex_list[vc++] = vertices[k][i];

   v = *vcount;
   vc = *vcount;

   // load index of first vertex into v, then update count

   *vcount += poly_nv[k];

   // now compute the plane
   {
      // stick some values in local so the code is readable

      Vertex *pool = cell->vpool;          // all the vertices for this cell
      uchar *vlist = cell->vertex_list + v;  // the vertex list for this poly

      mxs_vector a,b;

      mx_sub_vec(&a, &pool[vlist[1]], &pool[vlist[0]]);
      mx_sub_vec(&b, &pool[vlist[2]], &pool[vlist[1]]);
      mx_cross_norm_vec(&this_normal, &b, &a);

      // for a vertex, ax + by + cz + d = 0, so d = -(ax + by + cz)
      this_plane.plane_constant = -mx_dot_vec(&this_normal, &pool[vlist[0]]);
   }

   // now use that plane to determine whether or not to reverse
   // the original bsp plane

   bnorm.x = poly_plane[k]->a;
   bnorm.y = poly_plane[k]->b;
   bnorm.z = poly_plane[k]->c;
   
   if (!reversed) {
      this_norm.x = -this_norm.x;
      this_norm.y = -this_norm.y;
      this_norm.z = -this_norm.z;
   }
   if (mx_dot_vec(&this_norm, &bnorm) < 0) {
      this_normal.x = -poly_plane[k]->a;
      this_normal.y = -poly_plane[k]->b;
      this_normal.z = -poly_plane[k]->c;
      this_plane.plane_constant = -poly_plane[k]->d;
   } else {
      this_normal.x = poly_plane[k]->a;
      this_normal.y = poly_plane[k]->b;
      this_normal.z = poly_plane[k]->c;
      this_plane.plane_constant = poly_plane[k]->d;
   }

   // now see if this plane is coplanar to another plane
   for (i = 0; i < *plane_count; i++)
   {
      if ((mx_dot_vec (&this_normal,
                       &cell->plane_list[i].normal) >= VEC_DOT_ONE) &&
          (fabs (this_plane.plane_constant - cell->plane_list[i].plane_constant)
               <= PLANE_CONST_EPSILON))
      {
         // These are basically the same plane, make us point to it
         cell->poly_list[surface].planeid = i;
         return;
      }
   }

   // We're a new plane, woo hoo
   cell->poly_list[surface].planeid = i;
   cell->plane_list[i].normal = this_normal;
   cell->plane_list[i].plane_constant = this_plane.plane_constant;

   ++(*plane_count);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  compute texture information
//

float tex_scale=4;

// baseaxis[x][0] = u axis
// baseaxis[x][1] = v axis
// baseaxis[x][2] = normal
mxs_vector baseaxis[6][3] =
{
 { {1,0,0}, {0,1,0}, {0,0,-1} },   // west wall
 { {0,1,0}, {-1,0,0}, {0,0,-1} },   // south wall
 { {0,0,1}, {1,0,0}, {0,-1,0} },   // floor
 { {-1,0,0}, {0,-1,0}, {0,0,-1} },  // east wall
 { {0,-1,0}, {1,0,0}, {0,0,-1} },  // north wall
 { {0,0,-1}, {1,0,0}, {0,1,0} }  // ceiling
};

  // solve for:
  //    dest . row1 = 1
  //    dest . row2 = 0
  //    dest . row3 = 0

// e.g. dest == u
//      row1 = primary u axis
//      row2 = primary v axis
//      row3 = poly norm

// if row3 != baseaxis, then we may end up not of unit length.
// This is the RIGHT behavior to make textures match up!

  // since so many entries are 0, we just use cramer's rule
void linear_eq_vec(mxs_vector *dest, mxs_vector *row1, mxs_vector *row2, mxs_vector *row3)
{
   mxs_real numer;
   numer = row1->x * (row2->y * row3->z - row3->y * row2->z)
         + row2->x * (row3->y * row1->z - row1->y * row3->z)
         + row3->x * (row1->y * row2->z - row2->y * row1->z);
   dest->x = (row2->y * row3->z - row3->y * row2->z) / numer;
   dest->y = (row2->z * row3->x - row3->z * row2->x) / numer;
   dest->z = (row2->x * row3->y - row3->x * row2->y) / numer;
}

ushort compute_uv_align(mxs_vector *uv, Vertex *base,
     float tex_scale, float align)
{
   mxs_real core_loc = mx_dot_vec(uv, base) / tex_scale + align;
   return fmod(core_loc, 4) * (16*256);
}

float compute_uv_at_loc(mxs_vector *uv, Vertex *loc, float tex_scale, float align)
{
   return mx_dot_vec(uv, loc) / tex_scale + align;
}

static mxs_vector tex_axis[2], tex_norm, tex_origin;
static float this_tex_scale, this_tex_align_u, this_tex_align_v;
static float this_plane_constant;

static mxs_vector brush_axis[2];
static float brush_scale, brush_align_u, brush_align_v;

float compute_dist_from_u(float this_u)
{
   // this_u = mx_dot_vec(uv, pt) / this_tex_scale + this_tex_align_u
   // then make the plane equation ax + by + cz + d, such that
   // points with u of this_u are on the plane.
   //    mx_dot_vec(uv, pt) + d = 0
   //    d = -mx_dot_vec(uv,pt)
   // but uv_at_loc = mx_dot_vec(uv,pt) / tex_scale + align
   // so mx_dot_vec(uv,pt) = (uv_at_loc - align) * tex_scale
   //    d = -(this_u - this_tex_align_u)*this_tex_scale;

   return -(this_u - this_tex_align_u) * this_tex_scale;
}

float compute_dist_from_v(float this_v)
{
   return -(this_v - this_tex_align_v) * this_tex_scale;
}

void compute_brush_uv_from_loc(mxs_vector *p, float *u, float *v)
{
   *u = compute_uv_at_loc(&brush_axis[0], p, brush_scale, brush_align_u);
   *v = compute_uv_at_loc(&brush_axis[1], p, brush_scale, brush_align_v);
}

void compute_loc_from_brush_uv(mxs_vector *p, float u, float v)
{
   // invert the above code:

   // mx_dot_vec(brush_axis[0],p) / brush_scale + brush_align_u = u
   // mx_dot_vec(brush_axis[1],p) / brush_scale + brush_align_v = v
   // mx_dot_vec(tex_norm,p) + this_plane_constant = 0

   // brush_axis[0] . p = (u - brush_align_u) * brush_scale
   // brush_axis[1] . p = (v - brush_align_v) * brush_scale
   // tex_norm      . p = -this_plane_constant

   compute_3_linear_eq(p, &brush_axis[0], &brush_axis[1], &tex_norm,
            (u - brush_align_u) * brush_scale,
            (v - brush_align_v) * brush_scale, -this_plane_constant);
}

void compute_poly_uv_from_loc(mxs_vector *p, float *u, float *v)
{
   // poly uv is expressed as tex_axis[0], tex_axis[1], tex_origin
   //   (x,y,z) = tex_origin + u*tex_axis[0] + v*tex_axis[1] + d*tex_norm
   //
   // To make this a matrix equation, we have to regroup terms:
   //    x = tex_origin.x + u*tex_axis[0].x + v*tex_axis[1].x + d*tex_norm.x
   //    ...etc...

   mxs_vector temp,mat_row1,mat_row2,mat_row3;

   mat_row1.x = tex_axis[0].x;
   mat_row1.y = tex_axis[1].x;
   mat_row1.z = tex_norm.x;

   mat_row2.x = tex_axis[0].y;
   mat_row2.y = tex_axis[1].y;
   mat_row2.z = tex_norm.y;

   mat_row3.x = tex_axis[0].z;
   mat_row3.y = tex_axis[1].z;
   mat_row3.z = tex_norm.z;

   compute_3_linear_eq(&temp, &mat_row1, &mat_row2, &mat_row3,
        p->x - tex_origin.x, p->y - tex_origin.y, p->z - tex_origin.z);
   *u = temp.x;
   *v = temp.y;
}

void compute_poly_texture_info(mxs_vector *norm, int brush, int face)
{
   int best=-1,i;
   mxs_real best_sz=0, sz;
   mxs_matrix rot_m;
   fixang rot_a;

   if (brush < 0 ||
         !CB_FACE_BRUSH_ALIGN_TEX(brush, face, &tex_axis[0], &tex_axis[1])) {
      for (i=0; i < 6; ++i) {
         sz = mx_dot_vec(&baseaxis[i][0], norm);
         if (sz > best_sz)
            best_sz = sz, best = i;
      }
      if (best == -1)
         Error(1, "Couldn't find good tmap mapping for plane.\n");

      if (brush >= 0)
         rot_a = CB_FACE_TEX_ROT(brush, face);
      else
         rot_a = 0;

      if (best == 0 || best == 3)
         mx_mk_rot_x_mat(&rot_m, best >= 3 ? -rot_a : rot_a);
      else if (best == 1 || best == 4)
         mx_mk_rot_y_mat(&rot_m, best >= 3 ? -rot_a : rot_a);
      else
         mx_mk_rot_z_mat(&rot_m, best >= 3 ? -rot_a : rot_a);

      mx_mat_mul_vec(&tex_axis[0], &rot_m, &baseaxis[best][1]);   
      mx_mat_mul_vec(&tex_axis[1], &rot_m, &baseaxis[best][2]);   
   }

   if (brush >= 0) {
      this_tex_scale = CB_FACE_TEX_SCALE(brush, face);
      this_tex_align_u = CB_FACE_TEX_ALIGN_U(brush, face);
      this_tex_align_v = CB_FACE_TEX_ALIGN_V(brush, face);
   } else {
      this_tex_scale = tex_scale;
      this_tex_align_u = this_tex_align_v = 0;
   }
}

void compute_point_texture_info(BspVertex *vec, float *u, float *v)
{
   mxs_vector pt;
   pt.x = vec->x;
   pt.y = vec->y;
   pt.z = vec->z;
   *u = compute_uv_at_loc(&tex_axis[0], &pt, this_tex_scale, this_tex_align_u);
   *v = compute_uv_at_loc(&tex_axis[1], &pt, this_tex_scale, this_tex_align_v);
}

extern void wr_alloc_light_map(PortalCell *p, int surface, int vc);
//extern r3s_texture hacked_decal();

void compute_poly_texture(PortalPolygonRenderInfo *render, Vertex *base,
  PortalPlane *plane, int brush, int face, PortalCell *p, int s, int vc)
{
   double m1,m2;
   mxs_vector *norm = &plane->normal;

   // compute brush space info for decals
   compute_poly_texture_info(norm, brush, face);

   brush_axis[0] = tex_axis[0];
   brush_axis[1] = tex_axis[1];
   brush_scale = this_tex_scale;
   brush_align_u = this_tex_align_u;
   brush_align_v = this_tex_align_v;

   tex_norm = *norm;
   this_plane_constant = plane->plane_constant;

   // compute (u,v) values at 'base'
   render->u_base = compute_uv_align(&tex_axis[0], base, this_tex_scale, this_tex_align_u);
   render->v_base = compute_uv_align(&tex_axis[1], base, this_tex_scale, this_tex_align_v);

   linear_eq_vec(&render->tex_u, &tex_axis[0], &tex_axis[1], norm);
   linear_eq_vec(&render->tex_v, &tex_axis[1], &tex_axis[0], norm);

   mx_scaleeq_vec(&render->tex_u, this_tex_scale);
   mx_scaleeq_vec(&render->tex_v, this_tex_scale);

   m1 = mx_mag_vec(&render->tex_u);
   m2 = mx_mag_vec(&render->tex_v);
   render->texture_mag = m1 > m2 ? m1 : m2;

   tex_axis[0] = render->tex_u;
   tex_axis[1] = render->tex_v;
   tex_origin.x = base->x;
   tex_origin.y = base->y;
   tex_origin.z = base->z;
   mx_scale_addeq_vec(&tex_origin, &tex_axis[0], -(render->u_base / (16*256.0)));
   mx_scale_addeq_vec(&tex_origin, &tex_axis[1], -(render->v_base / (16*256.0)));

   render->cached_surface = 0;

   wr_alloc_light_map(p, s, vc);

#if 0
   if (decal_hack) {
      p->light_list[s].decal = Malloc(16);
      p->light_list[s].decal[0].decal = hacked_decal();      
      p->light_list[s].decal[0].offset =
          (Rand() % p->light_list[s].h) * 256 +
          (Rand() % p->light_list[s].w);
      p->light_list[s].decal[1].offset = 65536;
   } else
      build_polygon_decals(&p->light_list[s], brush, face);
#endif
}

void reset_cell_lighting(PortalCell *p)
{
   int i, n = p->num_render_polys;
   PortalLightMap *lm;

   int ambient, r, g, b;
   mxs_vector rgb;
   mx_copy_vec(&rgb, &GetMissionRenderParams()->ambient_light);

#ifdef RGB_LIGHTING
#ifdef RGB_888
   r = rgb.x * 256;
   g = rgb.y * 256;
   b = rgb.z * 256;
   ambient = r + (g << 8) + (b << 16);
#else // RGB_888
   r = rgb.x * 32;
   g = rgb.y * 32;
   b = rgb.z * 32;
   ambient = r + (g << 5) + (b << 10);
#endif // RGB_888

#else
   ambient = mx_mag_vec(&rgb);
#endif

   if (p->light_list) {
      for (i=0; i < n; ++i) {
         if (p->light_list[i].data) {
            lm = &p->light_list[i];

            lm->data = Realloc(lm->data, lm->h * lm->w * sizeof(LightmapEntry));
#ifndef RGB_LIGHTING
            memset(lm->data, ambient, lm->h * lm->w);
#else
            {
               int j;
#ifdef RGB_888
               for (j = 0; j < lm->h * lm->w; ++j)
                  lm->data[j].I = ambient;
#else // RGB_888
               for (j=0; j < lm->h * lm->w; ++j)
                  lm->data[j] = ambient;
#endif // RGB_888
            }
#endif
         }
         p->light_list[i].anim_light_bitmask = 0;
      }
   }

   if (p->light_indices) {
      p->light_indices = Realloc(p->light_indices, sizeof(ushort));
      *p->light_indices = 0;
   }

   if (p->anim_light_index_list) {
      Free(p->anim_light_index_list);
      p->anim_light_index_list = NULL;
   }
   p->num_anim_lights = 0;
}


void reset_lighting(void)
{
   int i;
   for (i=0; i < wr_num_cells; ++i)
       reset_cell_lighting(WR_CELL(i));
}

// The center point of the polygon is in the render info instead of
// the vertex palette since it's only used for finding the MIP
// level and we do that in world space.
void set_polygon_center_point(PortalPolygonCore *p, 
                              int vertex_offset,
                              PortalPolygonRenderInfo *render,
                              int cell)
{
   mxs_vector *point;
   mxs_vector *vertex_pool = wr_cell[cell]->vpool;
   uchar *vertex_list = wr_cell[cell]->vertex_list;
   mxs_vector centerpiece = {0, 0, 0};
   int i, n = p->num_vertices;

   for (i = 0; i < n; i++) {
      point = &vertex_pool[vertex_list[vertex_offset + i]];

      centerpiece.x += point->x;
      centerpiece.y += point->y;
      centerpiece.z += point->z;
   }

   centerpiece.x /= n;
   centerpiece.y /= n;
   centerpiece.z /= n;

   render->center = centerpiece;
}

BOOL no_lightmap_assert = FALSE;

static void emit_cell(int cell, int medium)
{
   // read the vertex palette
   int num_v, num_s, num_p, i, vcount, vl, surface, vl_r;
   int planes;                  // number of planes

   PortalCell *p;
   if (cell >= MAX_REGIONS)
      Error(1, "Too many cells.\n");

   vl = 0;   // vertex list count
   vl_r = 0; // vertex list rendered count
   num_p = 0;
   num_s = 0;
   for (i=0; i < cur_poly; ++i) {
      vl += poly_nv[i];

      if (poly_dest[i])
         ++num_p;

      if (poly_render[i]) {
         ++num_s;
         vl_r += poly_nv[i];
      }
   }

   num_v = num_vertices;

   num_portal += num_p;
   num_poly += cur_poly - num_p;
   num_points += num_v;

   p = wr_cell[cell] = Malloc(sizeof(PortalCell));
   p->num_vertices = num_v;
   p->num_polys = cur_poly;
   p->num_render_polys = num_s;
   p->num_portal_polys = num_p;
   p->num_full_bright = 0;

   wr_brfaces[cell] = Malloc(sizeof(int) * num_s);  // num render polys

   p->medium = medium;
   p->flags = 0;
   p->motion_index = 0;
  
   p->vpool = Malloc(sizeof(Vertex) * num_v);
   p->poly_list = Malloc(sizeof(PortalPolygonCore) * p->num_polys);
   p->portal_poly_list = p->poly_list + p->num_polys - num_p;
   p->render_list = Malloc(sizeof(PortalPolygonRenderInfo) * num_s);
   p->light_list = Malloc(sizeof(PortalLightMap) * num_s);
   memset(p->light_list, 0, sizeof(PortalLightMap) * num_s);

   p->vertex_list = Malloc(vl);
   //   if (vl_r)
   //      p->vertex_list_lighting = Malloc(vl_r);
   //   else
   //      p->vertex_list_lighting = 0;
   //   p->vertex_list_dynamic = 0;
   p->num_vlist = vl;

   p->num_anim_lights = 0;
   p->changed_anim_light_bitmask = 0;
   p->anim_light_index_list = 0;
   p->light_indices = Malloc(sizeof(p->light_indices[0]));
   p->light_indices[0] = 0;

   reset_cell_lighting(p);

   p->plane_list = Malloc(sizeof(PortalPlane) * p->num_polys);
   p->render_data = 0;
   p->refs = 0;

   for (i=0; i < num_v; ++i) {
      p->vpool[i].x = vpal[i].x;
      p->vpool[i].y = vpal[i].y;
      p->vpool[i].z = vpal[i].z;
   }

   vcount = 0;

   // process rendered surfaces
   surface = 0;
   planes = 0;

   for (i=0; i < cur_poly; ++i) {
      if (poly_render[i] && !poly_dest[i]) {
         int k, vc = vcount;
         int plane;

         emit_polygon(i, p, &p->poly_list[surface], &vcount, &planes, surface);

         if (poly_brface[i] != -1) {
            p->render_list[surface].texture_id =
                  CB_FACE_TEXTURE(poly_brface[i] >> 8, poly_brface[i] & 255);
            if (p->render_list[surface].texture_id == 249 ||   // doug
                CB_FACE_IS_SELF_LUMINOUS(poly_brface[i] >> 8, poly_brface[i] & 255))
               p->poly_list[surface].flags |= RENDER_DOESNT_LIGHT;
            RegisterFace(poly_brface[i], cell, surface, vc);
         } else
            p->render_list[surface].texture_id = 0;

         set_polygon_center_point(&p->poly_list[surface], vc, 
                                  &p->render_list[surface], cell);

         k = p->render_list[surface].texture_anchor = 0;
         plane = p->poly_list[surface].planeid;
         wr_brfaces[cell][surface] = poly_brface[i];
         compute_poly_texture(&p->render_list[surface], &p->vpool[p->vertex_list[vc+k]], &p->plane_list[plane], poly_brface[i] >> 8, poly_brface[i] & 255, p, surface, vc);

         ++surface;
      }
   }

   // rendered portals

   no_lightmap_assert = TRUE;

   p->portal_vertex_list = vcount;

   for (i=0; i < cur_poly; ++i) {
      if (poly_dest[i] && poly_render[i]) {
         int k, vc = vcount;
         int plane;

         emit_polygon(i, p, &p->poly_list[surface], &vcount, &planes, surface);

         p->poly_list[surface].flags |= PORTAL_SPLITS_OBJECT;

         p->render_list[surface].texture_id = poly_brface[i];

         set_polygon_center_point(&p->poly_list[surface], vc, 
                                  &p->render_list[surface], cell);

         k = p->render_list[surface].texture_anchor = 0;
         plane = p->poly_list[surface].planeid;
         compute_poly_texture(&p->render_list[surface], &p->vpool[p->vertex_list[vc+k]], &p->plane_list[plane], -1, -1, p, surface, vc);

         p->poly_list[surface].destination = poly_dest[i]-1;
         p->poly_list[surface].flags |= RENDER_DOESNT_LIGHT;

         wr_brfaces[cell][surface] = -1;  // it's a special surface!
         ++surface;
      }
   }

   no_lightmap_assert = FALSE;

      // process non-rendered portals
   for (i=0; i < cur_poly; ++i) {
      if (poly_dest[i] && !poly_render[i]) {
         emit_polygon(i, p, &p->poly_list[surface], &vcount, &planes, surface);
         p->poly_list[surface].destination = poly_dest[i]-1;
         ++surface;
      }
   }
   if (vcount != vl)
      Error(1, "vcount != vl\n");
   if (surface != cur_poly)
      Error(1, "surface != cur_poly\n");

   p->num_planes = planes;
   PortalComputeBoundingSphere(p);
}

extern int get_texture_for_medium_transition(int, int);
extern int get_clut_for_medium_transition(int, int);

uchar verts[MAX_PPP*2];

void emit_vertex_list(PortalPolygon *poly)
{
   PortalPolyEdge *edge = poly->edge, *first = edge;
   int n, a, b, c, i, j, p1,p2;

     // collect all edges together in terms of vertex palette
   n = 0;
   do {
      if (n == MAX_PPP*2)
         Error(1, "Too many points per polygon, increase MAX_PPP\n");
      verts[n++] = vertex_palette(&edge->data->start);
      verts[n++] = vertex_palette(&edge->data->end);
      edge = edge->poly_next;
   } while (edge != first);

     // build final vertex list by searching edge list
   a = verts[0];
   p1 = 0;
   p2 = -1;
   b = 0;
   c = 0;
   vertices[cur_poly][c++] = a;

   j = 0;
   while (c < n/2) {
      for (i=0; i < n; i += 2) {
         if (i == b) continue;
         if (verts[i] == vertices[cur_poly][c-1]) {
            b = i;
            vertices[cur_poly][c++] = verts[i+1];
            p2 = p1;
            p1 = i+1;
            break;
         }
         if (verts[i+1] == vertices[cur_poly][c-1]) {
            b = i;
            vertices[cur_poly][c++] = verts[i];
            p2 = p1;
            p1 = i;
            break;
         }
      }
      if (++j >= 2*n) {
         //dump_portal_poly(poly);
         Error(1, "emit_vertex_list: Bad polygon\n");
      }
#if 0
      // ack, this is STILL WRONG
      // make sure there are no duplicates
      for(i+=2; i < n; ++i) {
         if (verts[i] == vertices[cur_poly][c-2] && i != p1 && i != p2) 
            Error(1, "emit_vertex_list: Vertex appeared more than once!\n");
      }
#endif
   }
   poly_nv[cur_poly] = n/2;
}

// we've output everything about this poly into poly_foo[cur_poly]
// except the vertex list

// now, due to light maps etc., we may need to split this into more
// polys.  The way we do this is to split it perpendicular to U and V
// axes until it's small enough

// we make the code recursive for simplicity

// this doesn't permanently affect the database; if we make a
// new pass, we'll have these fragments sitting around in memory,
// and the original unsplit poly still there in memory

#define TEXTURE_SIZE    64
#define LIGHT_MAP_SIZE  16

int subdivide;

void copy_poly_foo(void)
{
   poly_plane[cur_poly] = poly_plane[cur_poly-1];
   poly_dest[cur_poly] = poly_dest[cur_poly-1];
   poly_render[cur_poly] = poly_render[cur_poly-1];
   poly_clut[cur_poly] = poly_clut[cur_poly-1];
   poly_brface[cur_poly] = poly_brface[cur_poly-1];
}

#ifdef BIG_LIGHTMAPS
  #define MAX_LM_SIZE     (1024-4*LIGHT_MAP_SIZE)
#else
  #define MAX_LM_SIZE     (256-2*LIGHT_MAP_SIZE)
#endif

static void subdivide_poly(PortalPolygon *poly)
{
   float min_u, max_u, min_v, max_v;
   PortalPolyEdge *edge = poly->edge;

   min_u = min_v = 1e20;
   max_u = max_v = -1e20;

   do {
      float u,v;
      compute_point_texture_info(&edge->data->start, &u, &v);
      if (u < min_u) min_u = u; else if (u > max_u) max_u = u;
      if (v < min_v) min_v = v; else if (v > max_v) max_v = v;
      compute_point_texture_info(&edge->data->end, &u, &v);
      if (u < min_u) min_u = u; else if (u > max_u) max_u = u;
      if (v < min_v) min_v = v; else if (v > max_v) max_v = v;
      edge = edge->poly_next;
   } while (edge != poly->edge);

     // rescale them into light map scale
   min_u *= TEXTURE_SIZE; max_u *= TEXTURE_SIZE;
   min_v *= TEXTURE_SIZE; max_v *= TEXTURE_SIZE;

     // do we need to subdivide in u?
   if (max_u - min_u > MAX_LM_SIZE) {
      BspPlane clip;
      PortalPolygon *res;
      poly = PortalPolyCopy(poly);

      ++subdivide;

      clip.a = tex_axis[0].x;      
      clip.b = tex_axis[0].y;      
      clip.c = tex_axis[0].z;      
      clip.d = compute_dist_from_u((min_u + MAX_LM_SIZE - LIGHT_MAP_SIZE)
                                 / TEXTURE_SIZE);

#ifdef DBG_ON
      if (PortalPolygonPlaneCompare(poly, &clip) != CROSS) {
         Error(1, "Tried to subdivide in u but plane compared invalidly.\n");
      }
#endif
      res = PortalClipPolygonByPlane(poly, &clip, 0, 0);
#ifdef DBG_ON
      CheckPoly2(res, " (split poly res)");
      CheckPoly2(poly, " (split poly poly)");
#endif
      // this computes res on one side, and outputs poly on the other side
      subdivide_poly(poly);
      ++cur_poly;
      if (cur_poly == MAX_POLYGONS)
         Error(1, "Too many polygons per polyhedron; increase MAX_POLYGONS\n");
      copy_poly_foo();
      subdivide_poly(res);
   } else if (max_v - min_v > MAX_LM_SIZE) {
      BspPlane clip;
      PortalPolygon *res;
      poly = PortalPolyCopy(poly);

      ++subdivide;

      clip.a = tex_axis[1].x;      
      clip.b = tex_axis[1].y;      
      clip.c = tex_axis[1].z;      
      clip.d = compute_dist_from_v((min_v + MAX_LM_SIZE - LIGHT_MAP_SIZE)
                                 / TEXTURE_SIZE);
#ifdef DBG_ON
      if (PortalPolygonPlaneCompare(poly, &clip) != CROSS) {
         Error(1, "Tried to subdivide in v but plane compared invalidly.\n");
      }
#endif
      res = PortalClipPolygonByPlane(poly, &clip, 0, 0);
#ifdef DBG_ON
      CheckPoly2(res, " (split poly res)");
      CheckPoly2(poly, " (split poly poly)");
#endif
      // this computes res on one side, and outputs poly on the other side
      subdivide_poly(poly);
      ++cur_poly;
      if (cur_poly == MAX_POLYGONS)
         Error(1, "Too many polygons per polyhedron; increase MAX_POLYGONS\n");
      copy_poly_foo();
      subdivide_poly(res);
   } else {
#ifdef DBG_ON
      CheckPoly2(poly, " (subdivide leaf)");
#endif
      emit_vertex_list(poly);
   }
}

bool subdivide_large_poly=TRUE;
static void emit_portal_poly(PortalPolygon *poly, PortalPolyhedron *ph)
{
   int n, i;

   poly_plane[cur_poly] = poly->plane;

     // check if it's a wall or a portal
   n = (poly->ph[0] == ph);
   if (poly->ph[n])
      poly_dest[cur_poly] = poly->ph[n]->leaf->cell_id;
   else
      poly_dest[cur_poly] = 0;

     // check if it should be rendered regardless
   if (!poly_dest[cur_poly]) {
      poly_render[cur_poly] = 1;
      poly_brface[cur_poly] = poly->brface;
      poly_clut[cur_poly] = 0;
   } else if (poly->ph[n]->leaf->medium != ph->leaf->medium) {
      i = get_texture_for_medium_transition(ph->leaf->medium,
              poly->ph[n]->leaf->medium);
      if (i == -1) {
         poly_render[cur_poly] = 0;
      } else {
         poly_render[cur_poly] = 1;
         poly_brface[cur_poly] = i;
      }
      poly_clut[cur_poly] = get_clut_for_medium_transition(ph->leaf->medium,
                 poly->ph[n]->leaf->medium);
   } else {
      poly_render[cur_poly] = 0;
      poly_brface[cur_poly] = 0;
      poly_clut[cur_poly] = 0;
   }

   // add check for Self-Lit and Sky texture before allowing splitting
   if (poly_dest[cur_poly] == 0 &&
       CB_FACE_IS_SELF_LUMINOUS(poly_brface[cur_poly] >> 8, poly_brface[cur_poly] & 255)==0 &&
       CB_FACE_TEXTURE(poly_brface[cur_poly] >> 8, poly_brface[cur_poly] & 255) != 249) {
      mxs_vector norm, inside_vec;
      norm.x = poly_plane[cur_poly]->a;
      norm.y = poly_plane[cur_poly]->b;
      norm.z = poly_plane[cur_poly]->c;
      // now the normal may be backfacing, damnit
      // so what we gotsta to do is check if it is,
      // and reverse it if so
      // sadly, this is a pain in the butt
      // compute vector from point on poly into thing

      inside_vec.x = nnside.x - poly->edge->data->start.x;
      inside_vec.y = nnside.y - poly->edge->data->start.y;
      inside_vec.z = nnside.z - poly->edge->data->start.z;

      if (mx_dot_vec(&inside_vec, &norm) < 0) {
         norm.x = -norm.x;
         norm.y = -norm.y;
         norm.z = -norm.z;
      }

      compute_poly_texture_info(&norm, poly_brface[cur_poly] >> 8, poly_brface[cur_poly] & 255);

      if (subdivide_large_poly) { // && !poly_dest[cur_poly])
         subdivide_poly(poly);
      } else {
         emit_vertex_list(poly);
      }
   } else {
#ifdef DBG_ON
      CheckPoly2(poly, " (portal)");
#endif
      emit_vertex_list(poly);
      if (post_edge_merge && poly_nv[cur_poly] > 32) {
         mprintf("Portal has too many vertices during emit.\n");
         dump_portal_poly(poly);
      }
   }
}

extern bool resplit_cell;
void free_pinfo(PortalPolyhedron *ph);

void emit_portal_polyhedron(PortalPolyhedron *ph)
{
   PortalPolygon *first, *poly;

#ifdef DBG_ON
   // visit all polygons
   cur_poly = 0;
   first = poly = ph->poly;
   do {
      ++cur_poly;
      poly = GetPhNext(poly, ph);
   } while (poly != first);
   if (cur_poly < 4) {
     Error(1, "Infinitely small cell!");
   }
#endif

   compute_center(ph);

     // clear count of info
   num_vertices = cur_poly = 0;

     // visit all polygons
   first = poly = ph->poly;
   do {
      if (cur_poly == MAX_POLYGONS)
         break;
      emit_portal_poly(poly, ph);
      ++cur_poly;
      poly = GetPhNext(poly, ph);
   } while (poly != first);

   if (cur_poly > MAX_PORTAL_POLYGONS || num_vertices > MAX_PORTAL_VERTICES) {
Warning(("Split large cell: %d polys, %d vertices\n", cur_poly, num_vertices));
      resplit_cell = TRUE;
      free_pinfo(ph);
       // free pinfo before the split, so we don't have
       // to deal with it during the split
      split_polyhedron(ph);
   } else {
      emit_cell(ph->leaf->cell_id-1, ph->leaf->medium);
      free_pinfo(ph);
   }
}

/////////////////////////////////////////////////////////////////////
//
//  T-JOINT FIXUP

#define TJOINT_NORMAL_EPS   0.001
#define TJOINT_PROJECT_EPS  0.001

#define MAX_TJOINT_EDGES   65536     // jeez, have 1.5M of overhead
#define MAX_TJOINT_DIRS    16384

// direction an edge is pointing (normalized)
static mxs_vector dirs[MAX_TJOINT_DIRS];

static int edge_num_pts[MAX_TJOINT_EDGES];
static int num_edges, num_dirs;

// projection of this edge onto plane passing through origin
static mxs_vector edge_prj[MAX_TJOINT_EDGES];
static int edge_dir[MAX_TJOINT_EDGES];

static mxs_vector *edge_pts[MAX_TJOINT_EDGES];

static void cleanup_tjoint_storage(void)
{
   int i;
   for (i=0; i < num_edges; ++i)
      Free(edge_pts[i]);
   num_edges = 0;
   num_dirs = 0;
}

#define DIR_EQ(a,b)   (fabs((a)-(b)) < TJOINT_NORMAL_EPS)
#define PROJ_EQ(a,b)  (fabs((a)-(b)) < TJOINT_PROJECT_EPS)

static int find_dir(mxs_vector *norm)
{
   int i;
   for (i=0; i < num_dirs; ++i) {
      if (DIR_EQ(norm->x,dirs[i].x) &&
          DIR_EQ(norm->y,dirs[i].y) &&
          DIR_EQ(norm->z,dirs[i].z))
         return i;
   }
   if (num_dirs >= MAX_TJOINT_DIRS)
      Error(1, "Increase MAX_TJOINT_DIRS.\n");
   dirs[num_dirs] = *norm;
   return num_dirs++;
}

static double dir_eval_point(int dir, mxs_vector *loc)
{
   mxs_vector *n = &dirs[dir];
   return (double) n->x * loc->x
        + (double) n->y * loc->y
        + (double) n->z * loc->z;

}

static double edge_eval_point(int edge, mxs_vector *loc)
{
   return dir_eval_point(edge_dir[edge], loc);
}

static int find_projected_dir(int dir, mxs_vector *loc)
{
   int e,i;
   for (i=0; i < num_edges; ++i) {
      if (edge_dir[i] == dir) {
         // compare projected point
         if (PROJ_EQ(loc->x,edge_prj[i].x) &&
             PROJ_EQ(loc->y,edge_prj[i].y) &&
             PROJ_EQ(loc->z,edge_prj[i].z))
            return i;
      }
   }

   // add a new edge
   if (num_edges >= MAX_TJOINT_EDGES)
      Error(1, "Increase MAX_TJOINT_EDGES");

   e = num_edges++;

   edge_dir[e] = dir;
   edge_prj[e] = *loc;
   edge_num_pts[e] = 0;
   edge_pts[e] = Malloc(sizeof(mxs_vector)*8);   // initial allocation
   return e;
}

static int find_edge(mxs_vector *p0, mxs_vector *p1)
{
   mxs_vector v;
   float val;
   int dir;

   mx_sub_vec(&v, p1, p0);
   mx_normeq_vec(&v);
   // convert to canonical ordering
   val = 0;
   if (fabs(v.x) > fabs(val)) val = v.x;
   if (fabs(v.y) > fabs(val)) val = v.y;
   if (fabs(v.z) > fabs(val)) val = v.z;
   if (val < 0) {
      v.x = - v.x;
      v.y = - v.y;
      v.z = - v.z;
   }

   dir = find_dir(&v);
   // now find projected point on plane through origin
   val = dir_eval_point(dir, p0);
   mx_scale_add_vec(&v, p0, &v, -val);  // compute:  p - (p.n)*n
   return find_projected_dir(dir, &v);
}

static int add_point_to_edge(int edge, mxs_vector *loc)
{
   int e;
   e = edge_num_pts[edge]++;
   edge_pts[edge] = Realloc(edge_pts[edge], sizeof(mxs_vector) * edge_num_pts[edge]);
   edge_pts[edge][e] = *loc;
   return e;
}

static void record_point_on_edge(int edge, mxs_vector *loc)
{
   int i;
   for (i=0; i < edge_num_pts[edge]; ++i)
      if (edge_pts[edge][i].x == loc->x &&
          edge_pts[edge][i].y == loc->y &&
          edge_pts[edge][i].y == loc->z)
         return;
   add_point_to_edge(edge, loc);
}

static BOOL record_edges(PortalCell *p, int poly, int v0, int v1)
{
   int e = find_edge(&p->vpool[p->vertex_list[v0]],
                     &p->vpool[p->vertex_list[v1]]);
   record_point_on_edge(e, &p->vpool[p->vertex_list[v0]]);
   record_point_on_edge(e, &p->vpool[p->vertex_list[v1]]);
   return FALSE;
}

int num_vpool_added;
static int find_vertex_in_palette(PortalCell *p, mxs_vector *loc)
{
   int i;
   for (i=0; i < p->num_vertices; ++i) {
      if (p->vpool[i].x == loc->x &&
          p->vpool[i].y == loc->y &&
          p->vpool[i].z == loc->z)
         return i;
   }

   if (p->num_vertices == 255) {
      Warning(("Couldn't fix tjoint--too many vertices in vpool.\n"));
      return -1;
   }
      
   ++p->num_vertices;
   p->vpool = Realloc(p->vpool, sizeof(Vertex) * p->num_vertices);
   p->vpool[p->num_vertices-1] = *loc;

   ++num_vpool_added;
   return p->num_vertices-1;
}

static int first_v;

static int num_vertices_added, max_vertices;
static BOOL add_vertex(PortalCell *p, int poly, int vert, mxs_vector *loc)
{
   int n = find_vertex_in_palette(p, loc);
   int i;

   if (n == -1)
      return FALSE;

   if (p->num_vlist == 255) {
      Warning(("Couldn't fix tjoint--too many vertices in vlist.\n"));
      return FALSE;
   }

   // check if this point is already somewhere in the polygon
   for (i=0; i < p->poly_list[poly].num_vertices; ++i) {
      if (p->vertex_list[first_v+i] == n) {
         Warning(("Tried to add vertex that was already present!\n"));
         return FALSE;
      }
   }

   ++p->num_vlist;
   p->vertex_list = Realloc(p->vertex_list, p->num_vlist);
   for (i=p->num_vlist-1; i > vert; --i)
      p->vertex_list[i] = p->vertex_list[i-1];
   p->vertex_list[vert] = n;
   ++p->poly_list[poly].num_vertices;
   if (p->poly_list[poly].num_vertices > max_vertices) max_vertices = p->poly_list[poly].num_vertices;
   ++num_vertices_added;

   return TRUE;
}

static BOOL find_tjoint(int edge, mxs_vector *p0, mxs_vector *p1, mxs_vector *res)
{
   double d0,d1, d, best_d;
   mxs_vector *loc;
   int i,n, best;

   d0 = edge_eval_point(edge, p0);
   d1 = edge_eval_point(edge, p1);

   // iterate through all points on this edge, looking for
   // the one between d0 and d1 closest to d0
   n = edge_num_pts[edge];
   best = -1;
   best_d = d1;

   for (i=0; i < n; ++i) {
      BOOL test;
      loc = &edge_pts[edge][i];
      d = edge_eval_point(edge, loc);
      if (d0 < d1)
         test = (d0 < d && d < best_d);
      else
         test = (d0 > d && d > best_d);
      if (test) {
         best = i;
         best_d = d;
      }
   }
   if (best == -1)
      return FALSE;


   *res = edge_pts[edge][best];

   Assert_(p0->x != res->x || p0->y != res->y || p0->z != res->z);
   Assert_(p1->x != res->x || p1->y != res->y || p1->z != res->z);

   return TRUE;
}

static BOOL edge_fix_tjoints(PortalCell *p, int poly, int v0, int v1)
{
   // find the closest point to v0 between v0 and v1
   int e = find_edge(&p->vpool[p->vertex_list[v0]],
                     &p->vpool[p->vertex_list[v1]]);
   mxs_vector loc;
   if (find_tjoint(e,
        &p->vpool[p->vertex_list[v0]],
        &p->vpool[p->vertex_list[v1]], &loc)) {
      // if the add fails, pass along FALSE indicating failure
      return add_vertex(p, poly, v0+1, &loc);
   } else
      return FALSE;
}

void process_cell_edges(PortalCell *p, BOOL (*func)(PortalCell *p, int n,
    int v0, int v1))
{
   int n,v;
   v = 0;
   for (n=0; n < p->num_render_polys; ++n) {
      int start_v = v;
      int j = v+p->poly_list[n].num_vertices-1;
      first_v = v;
      while (v < start_v + p->poly_list[n].num_vertices) {
         if (func(p, n, j, v)) {
            // everything right of j got shifted, and j advances:
            if (v > j) ++v;
            ++j;
            // update the index into the middle of the vlist for portal polys
            if (n < p->num_polys - p->num_portal_polys)
               ++p->portal_vertex_list;
         } else
            j = v++;
      }
   }
}

void fix_tjoints(void)
{
   int i;
   num_vertices_added = num_vpool_added = 0;
   for (i=0; i < wr_num_cells; ++i)
      process_cell_edges(wr_cell[i], record_edges);
   for (i=0; i < wr_num_cells; ++i)
      process_cell_edges(wr_cell[i], edge_fix_tjoints);
   cleanup_tjoint_storage();
   mprintf("Added %d vertices\n", num_vertices_added);
   mprintf("Max vertices post tjoint: %d\n", max_vertices);
}
