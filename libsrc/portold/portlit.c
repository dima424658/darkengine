/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


//  $Header: r:/t2repos/thief2/libsrc/portold/portlit.c,v 1.37 1998/03/12 09:39:33 MAT Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <mprintf.h>

#include <portal_.h>
#include <portclip.h>
#include <portdraw.h>
#include <pt.h>

#include <animlit.h>

// Have you heard?  This has to be the last header.
#include <dbmem.h>

//////////////////////////////////////////////////////////////////////////////

//  The following sections of code are variations on
//  draw_region and draw_surface which are used for doing
//  lighting.  Lighting works by rendering from the point
//  of view of the light; when we get to actually needing
//  to "draw" a surface, we paint light onto it.


////////////////////////////////////////
//
// dynamic light map lighting system

typedef struct dyn_lm_bits dyn_lm_bits;
struct dyn_lm_bits {
   PortalLightMap *lm;
   dyn_lm_bits *next;
   uchar first_byte;
};


// This points to the first element of a linked list.
static dyn_lm_bits *dyn_lm_base;


void reset_dynamic_lm(void)
{
   dyn_lm_bits *next;

   while (dyn_lm_base) {
      dyn_lm_base->lm->dynamic = 0;
      next = dyn_lm_base->next;
      Free(dyn_lm_base);
      dyn_lm_base = next;
   }
}


void reset_dynamic_lights(void)
{
   reset_dynamic_lm();
}


uchar *get_dynamic_lm(PortalLightMap *lm)
{
   int size = lm->row * lm->h;
   dyn_lm_bits *new_lm = Malloc(sizeof(dyn_lm_bits) - sizeof(uchar) + size);

   AssertMsg1(new_lm, "Could not allocate dynamic lightmap of size %d.", size);

   new_lm->next = dyn_lm_base;
   dyn_lm_base = new_lm;

   new_lm->lm = lm;
   lm->dynamic = &new_lm->first_byte;
   memcpy(lm->dynamic, lm->bits, size);
   return lm->dynamic;
}


void unget_dynamic_lm(PortalLightMap *lm)
{
   dyn_lm_bits *next;

   AssertMsg1(dyn_lm_base, "Failed to unget dynamic lightmap for %x", lm);

   dyn_lm_base->lm->dynamic = 0;
   next = dyn_lm_base->next;
   Free(dyn_lm_base);
   dyn_lm_base = next;
}

extern bool PortalTestLocationInsidePlane(PortalPlane *p, Location *loc);

// information about the current light

#define MAX_DIST 8.0
float max_dist_plain = MAX_DIST;
float max_dist_2 = (MAX_DIST * MAX_DIST);
static Location *light_loc;
static float bright;
static unsigned char start_medium;

void PortalSetLightInfo(Location *l, float br, uchar start)
{
   light_loc = l;
   bright = br;
   start_medium = start;
}

int num_bad;

extern bool debug_raycast;

void portal_raycast_light_poly(PortalCell *r, PortalPolygonCore *p, Location *lt, int voff)
{
   int i,n = p->num_vertices;
   Location temp;

   for (i=0; i < n; ++i) {
      int k = r->vertex_list[voff+i];   // get which vertex pool entry it is
      MakeLocationFromVector(&temp, &r->vpool[k]);
   }
}

bool dynamic_light;

float ambient_weight;

#if 0
void light_surface(PortalPolygonCore *poly, PortalPlane *plane, int voff, void *clip)
{
   int i,n = poly->num_vertices;
   mxs_vector *norm;

   norm = &plane->norm->raw;

   // now iterate over all of the vertices
   for (i=0; i < n; ++i) {
      int k = r_vertex_list[voff+i];
        // check if this vertex is visible
      if (!cur_ph[k].ccodes  && PortClipTestPoint(clip, cur_ph[k].grp.sx, cur_ph[k].grp.sy))
      {
         // compute the lighting on this vertex
         //   lighting is:  let L = light - point
         //                     N = surface normal
         //          light = intensity * (L . N) / (L . L)
         mxs_vector lvec;

         if (dynamic_light) {
            float result2;
            int result;
            mx_sub_vec(&lvec, &light_loc->vec, &cur_pool[k]);
            result2 = mx_dot_vec(&lvec, norm) / mx_mag2_vec(&lvec);
            if (result2 > 0) {
               result = bright * result2;
               result += r_vertex_lighting[voff+i];
               if (result > 255) result = 255;
               if (result < 0) result = 0;
               r_vertex_lighting[voff+i] = result;
            }
         } else {
            float result, result2;
            mx_sub_vec(&lvec, &light_loc->vec, &cur_pool[k]);
            result = mx_dot_vec(&lvec, norm) / mx_mag2_vec(&lvec);
            if (result < 0) result = 0; else result = result * bright;
            result2 = bright*0.25 / sqrt(mx_mag_vec(&lvec));
            result = result + ambient_weight * (result2 - result);
            if (result > 255) result = 255;
//            if (result < -1 || result > 1)
//               LightAtVertex(voff+i, result);
         }
      }
   }
}
#endif

// If the light level is less than this then we pretend our raycast
// didn't reach.
static int illumination_cutoff;
static bool illumination_reached_polygon;

float compute_light_at_point(mxs_vector *pt, mxs_vector *norm, mxs_vector *lt)
{
   float result,len;
   mxs_vector lvec;
   lvec.x = lt->x - pt->x;
   lvec.y = lt->y - pt->y;
   lvec.z = lt->z - pt->z;
   result = lvec.x * norm->x + lvec.y * norm->y + lvec.z * norm->z;

   if (result < 0) return 0;

   len = mx_mag_vec(&lvec);
   if (max_dist_plain != 0.0 && len > max_dist_plain)
      return 0.0;
   result = (result/len/2 + 0.5);

      // copy quake's angle remapping
     // bright * result / length(vec)^2 / 2, bright / length(vec)

   result = result * bright / len;

   if (result > illumination_cutoff) {
      illumination_reached_polygon = TRUE;
      return result;
   } else
      return FALSE;
}

// now, for any point on the plane, the _unnormalized dot product_
// above (the first result) is just the distance of the light from
// the plane; it's a constant!

float fast_compute_light_at_point(mxs_vector *pt, mxs_vector *lt, float dist)
{
   float result;
   mxs_vector lvec;
   lvec.x = lt->x - pt->x;
   lvec.y = lt->y - pt->y;
   lvec.z = lt->z - pt->z;

   result = dist * bright /
        (lvec.x*lvec.x + lvec.y*lvec.y + lvec.z*lvec.z);

   return result;
}

float fast_precompute_light(mxs_vector *pt, mxs_vector *norm, mxs_vector *lt)
{
   mxs_vector lvec;
   lvec.x = lt->x - pt->x;
   lvec.y = lt->y - pt->y;
   lvec.z = lt->z - pt->z;
   return lvec.x * norm->x + lvec.y * norm->y + lvec.z * norm->z;
}

float fast_compute_light_at_center(float dist)
{
   return bright / dist;
}

float dynamic_light_min = 16.0;

float fast_compute_dynamic_light_at_point(mxs_vector *pt, mxs_vector *lt, float dist)
{
   float result;
   mxs_vector lvec;
   lvec.x = lt->x - pt->x;
   lvec.y = lt->y - pt->y;
   lvec.z = lt->z - pt->z;

   result = lvec.x * lvec.x + lvec.y * lvec.y + lvec.z * lvec.z;
   // we want to clamp it so things disappear by the distance max_dist_2
   if (result > max_dist_2) return 0;

   result = (dist * bright) * (max_dist_2 - result) / (max_dist_2 * result);
   return result;
}

float fast_compute_dynamic_light_at_center(float dist)
{
   float result = dist * dist;
   if (result > max_dist_2) return 0;
   return bright * (max_dist_2 - dist) / (dist * max_dist_2);
}

float fast_compute_dynamic_light_at_dist(float dist, float plane_dist)
{
   float result = dist * dist;
   if (result > max_dist_2) return 0;
   result = plane_dist * bright * (max_dist_2 - result) / (max_dist_2 * result);
   return result;
}

#if 0
void light_region(int cell)
{
   PortalCell *r = WR_CELL(cell);
   int n = r->num_render_polys;
   int voff=0;
   PortalPolygonCore *poly = r->poly_list;
   PortalPolygonRenderInfo *render = r->render_list;

     // copy common data into globals for efficient communicating

   cur_ph = POINTS(r);
   cur_pool = r->vpool;

#if 0
   r_vertex_list = r->vertex_list;
   if (dynamic_light) {
      r_vertex_lighting = r->vertex_list_dynamic ?
                          r->vertex_list_dynamic :
                          get_dynamic_vertex_lighting(r);
   } else {
      LightCellStart(cell);
   }
#endif

   r_clip = CLIP_DATA(r);

   if (start_medium != r->medium)
      bright /= 2;

   while (n--) {
      if (check_surface_visible(r, poly, voff))
         light_surface(poly, &r->plane_list[poly->planeid], voff, r_clip);
      voff += poly->num_vertices;
      ++poly;
      ++render;
   }

   if (start_medium != r->medium)
      bright *= 2;

//   if (!dynamic_light)
//      LightCellEnd();
}
#endif

void portal_light_poly(int r, int p)
{
#if 0
   bool dyn;
#endif
   int i, voff=0;
   PortalPolygonCore *poly = WR_CELL(r)->poly_list;

   for (i=0; i < p; ++i)
      voff += poly++->num_vertices;

#if 0
   dyn = WR_CELL(r)->vertex_list_dynamic ? WR_CELL(r)->vertex_list_dynamic : get_dynamic_vertex_lighting(WR_CELL(r));

   for (i=0; i < poly->num_vertices; ++i)
      dyn[voff+i] = 127;
#endif
}

#define SPOTSCALE_SHIFT   3
#define SPOTSCALE_MAX (256 >> SPOTSCALE_SHIFT)

float spotscale[SPOTSCALE_MAX][SPOTSCALE_MAX];
bool underwater_light;

#if 0
void spotlight_surface(PortalPolygonCore *poly, PortalPlane *plane, int voff, void *clip)
{
   int i,n = poly->num_vertices;
   mxs_vector *norm;

   norm = &plane->norm->raw;

   // now iterate over all of the vertices
   for (i=0; i < n; ++i) {
      int k = r_vertex_list[voff+i];
        // check if this vertex is visible
      if (!cur_ph[k].ccodes  && PortClipTestPoint(clip, cur_ph[k].grp.sx, cur_ph[k].grp.sy))
      {
         mxs_vector lvec;
         int result, x,y;
         float len,sp;

         mx_sub_vec(&lvec, &light_loc->vec, &cur_pool[k]);

         // scale by screen values
         x = cur_ph[k].grp.sx >> (16 + SPOTSCALE_SHIFT); // equals x/32
         y = cur_ph[k].grp.sy >> (16 + SPOTSCALE_SHIFT); // equals y/32

         { float fx,fy,d;
           d = (float) (1 << (16 + SPOTSCALE_SHIFT));
           fx = (float) (cur_ph[k].grp.sx & ((1 << (16 + SPOTSCALE_SHIFT))-1))/d;
           fy = (float) (cur_ph[k].grp.sy & ((1 << (16 + SPOTSCALE_SHIFT))-1))/d;
           { float yx, y1x, yx1, y1x1;
              yx = spotscale[y][x];
              y1x = spotscale[y+1][x];
              yx1 = spotscale[y][x+1];
              y1x1 = spotscale[y+1][x+1];

              sp = yx + (y1x-yx)*fy + ((yx - y1x - yx1 + y1x1)*fy + yx1-yx)*fx;
           }
         }

         if (underwater_light) {
            len = mx_mag2_vec(&lvec);
         } else {
            len = mx_mag_vec(&lvec); // mag or mag2
            len *= sqrt(len);
         }

         result = bright * mx_dot_vec(&lvec, norm) / len * sp;
         if (result < 0) result = 0;

         if (dynamic_light) {
            result += r_vertex_lighting[voff+i];
            if (result > 255)
               result = 255;
            r_vertex_lighting[voff+i] = result;
         } else {
            if (result > 255) result = 255;
//            LightAtVertex(voff+i, result);
         }
      }
   }
}

void spotlight_region(int cell)
{
   PortalCell *r = WR_CELL(cell);
   int n = r->num_render_polys;
   int voff=0;
   PortalPolygonCore *poly = r->poly_list;
   PortalPolygonRenderInfo *render = r->render_list;

     // copy common data into globals for efficient communicating

   cur_ph = POINTS(r);
   cur_pool = r->vpool;

#if 0
   r_vertex_list = r->vertex_list;
   if (dynamic_light)
      r_vertex_lighting = r->vertex_list_dynamic ? r->vertex_list_dynamic : get_dynamic_vertex_lighting(r);
   else
      r_vertex_lighting = r->vertex_list_lighting;
#endif

   r_clip = CLIP_DATA(r);

   underwater_light = (start_medium != r->medium);
   if (r->medium > 1) bright /= 2;

   while (n--) {
      if (check_surface_visible(r, poly, voff))
         spotlight_surface(poly, &r->plane_list[poly->planeid], voff, r_clip);
      voff += poly->num_vertices;
      ++poly;
      ++render;
   }

   if (r->medium > 1) bright *= 2;
}
#endif

#define LIGHT_MAP_SIZE  0.25

bool record_movement;

extern int cur_raycast_cell;

uchar *portal_anim_light_intensity;
int num_anim_lights;

void (*failed_light_callback)(Location *hit, Location *dest);
void (*lightmap_point_callback)(mxs_vector *loc, bool lit);
void (*lightmap_callback)(PortalLightMap *lightmap);


#define DIST_IN_FROM_POLYGON .025


// This finds the level of illumination on a point from a light
// source.  If our point is outside the world rep, we approximate it
// using the point we reach when we raycast to our intended point
// from the middle of the polygon.
static int portal_illumination_from_light(Location *point_being_lit,
                                          Location *light,
                                          Location *point_in_poly,
                                          PortalCell *cell,
                                          int polygon_index)
{
   Location dest;
   Location dummy;

   if (PortalRaycast(light, point_being_lit, &dummy, 0))
      return compute_light_at_point(&point_being_lit->vec,
        &cell->plane_list[cell->poly_list[polygon_index].planeid].norm->raw, 
                                  &light->vec);

   // We know that our cast was blocked.  But was it by some actual
   // obstacle, or was our lightmap point simply outside our polygon
   // and cut off by an inside corner?  We cast a ray from a point known
   // to be inside it to our lightmap point, and test from there instead.
   // @TODO: We'd rather use some inside point close to the lightmap
   // point, rather than the middle of the polygon.
   if (!PortalRaycast(point_in_poly, point_being_lit, &dest, 1))
      if (PortalRaycast(light, &dest, &dummy, 0))
        return compute_light_at_point(&point_being_lit->vec,
          &cell->plane_list[cell->poly_list[polygon_index].planeid].norm->raw,
                                    &light->vec);

   return 0;
}


void portal_raycast_light_poly_lightmap(PortalCell *r, int s, int vc,
                                        Location *lt, uchar *bits,
                                        bool quadruple_lighting)
{
   // iterate over all of the points in the light map
   int i, j, lux;
   uchar *light_point;
   float u,v;
   Location dest, source;
   mxs_vector where, src, step;
   mxs_vector quarter_offset_plus;      // offset to upper-right quadrant
   mxs_vector quarter_offset_minus;     // offset to upper-left quadrant
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor

   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].u->raw, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].v->raw, -r->render_list[s].v_base / (16*256.0));
   mx_scale_addeq_vec(&src, 
                      &r->plane_list[r->poly_list[s].planeid].norm->raw,
                      DIST_IN_FROM_POLYGON);

   // start with a point inside the polygon
   where = r->render_list[s].center;
   mx_scale_addeq_vec(&where, 
                      &r->plane_list[r->poly_list[s].planeid].norm->raw,
                      DIST_IN_FROM_POLYGON);

   // We're using the point inside the poly to check whether each
   // light point really exists in its poly, rather than being outside
   // the world rep.  For now, I'm moving this check inside the cell
   // by a hair so the raycaster can have a smaller epsilon.
   MakeLocationFromVector(&source, &where);
   source.cell = source.hint = cur_raycast_cell;

   if (lightmap_callback)
      lightmap_callback(&r->light_list[s]);

   if (quadruple_lighting) {
      mx_scale_vec(&quarter_offset_plus, 
                   &r->render_list[s].u->raw, .25);
      quarter_offset_minus = quarter_offset_plus;

      mx_scale_addeq_vec(&quarter_offset_plus, 
                         &r->render_list[s].v->raw, .25);

      mx_scale_addeq_vec(&quarter_offset_minus,
                         &r->render_list[s].v->raw, -.25);
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].u->raw, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].v->raw, v);
      mx_scale_vec(&step, &r->render_list[s].u->raw, LIGHT_MAP_SIZE);

      for (i = 0; i < r->light_list[s].w; ++i) {
         MakeLocationFromVector(&dest, &where);
         if (quadruple_lighting) {
            mx_add_vec(&dest.vec, &where, &quarter_offset_plus);
            lux = portal_illumination_from_light(&dest, lt, &source, r, s);

            mx_add_vec(&dest.vec, &where, &quarter_offset_minus);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_plus);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_minus);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s);

            lux /= 4;
         } else
            lux = portal_illumination_from_light(&dest, lt, &source, r, s);

         light_point = &(bits[j * r->light_list[s].row + i]);
         lux += *light_point;
         if (lux > 255) 
            lux = 255;
         *light_point = lux;

         mx_addeq_vec(&where, &step);
         u += LIGHT_MAP_SIZE;
      }
      v += LIGHT_MAP_SIZE;
   }
}

void portal_light_poly_lightmap(PortalCell *r, int s, int vc, Location *lt,
                                uchar *bits)
{
   // iterate over all of the points in the light map
   int i,j;
   float u,v, dist;
   mxs_vector where, src, step;
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor

   mxs_vector *norm = &r->plane_list[r->poly_list[s].planeid].norm->raw;
   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].u->raw, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].v->raw, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].norm->raw, &lt->vec);

   if (fast_compute_light_at_center(dist) < 2.0)
      return;

   if (lightmap_callback)
      lightmap_callback(&r->light_list[s]);

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      uchar *output = &bits[j*r->light_list[s].row];

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].u->raw, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].v->raw, v);
      mx_scale_vec(&step, &r->render_list[s].u->raw, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = compute_light_at_point(&where, norm, &lt->vec);
         if (amt > 1) {
            illumination_reached_polygon = TRUE;
            amt += *output;
            if (amt > 255) amt = 255;
            *output = amt;
         }
         mx_addeq_vec(&where, &step);
         ++output;
      }
      v += LIGHT_MAP_SIZE;
   }
}

bool keep_all_lit;

void portal_dynamic_light_lightmap(PortalCell *r, int s, int vc, Location *lt,
                                   mxs_real min_dist)
{
   // iterate over all of the points in the light map
   int i,j;
   float u,v, dist;
   mxs_vector where, src, step;
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor
   bool lit = keep_all_lit;
   float max_bright;

   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].u->raw, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].v->raw, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].norm->raw, &lt->vec);

#if 1
   if (dist > min_dist)
      // nearest possible point on polygon is further than min distance
      // to the cell, so test that directly
      max_bright = fast_compute_dynamic_light_at_center(dist);
   else
      // nearest possible point on polygon is min_dist
      max_bright = fast_compute_dynamic_light_at_dist(min_dist, dist);

   if (max_bright < dynamic_light_min)
      return;
#endif

   if (!r->light_list[s].dynamic)
      get_dynamic_lm(&r->light_list[s]);
   else {
      lit=1;
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      uchar *output = &r->light_list[s].dynamic[j*r->light_list[s].row];

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].u->raw, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].v->raw, v);
      mx_scale_vec(&step, &r->render_list[s].u->raw, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = fast_compute_dynamic_light_at_point(&where, &lt->vec, dist);
         if (amt > 8) {
            amt += *output;
            if (amt > 255) amt = 255;
            *output = amt;
            lit = TRUE;
         }
         mx_addeq_vec(&where, &step);
         ++output;
      }
      v += LIGHT_MAP_SIZE;
   }
   
   if (!lit)
      unget_dynamic_lm(&r->light_list[s]);
}


void portal_dynamic_dark_lightmap(PortalCell *r, int s, int vc, Location *lt,
                                   mxs_real min_dist)
{
   // iterate over all of the points in the light map
   int i,j;
   float u,v, dist;
   mxs_vector where, src, step;
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor
   bool lit = keep_all_lit;
   float max_bright;

   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].u->raw, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].v->raw, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].norm->raw, &lt->vec);

#if 1
   if (dist > min_dist)
      // nearest possible point on polygon is further than min distance
      // to the cell, so test that directly
      max_bright = fast_compute_dynamic_light_at_center(dist);
   else
      // nearest possible point on polygon is min_dist
      max_bright = fast_compute_dynamic_light_at_dist(min_dist, dist);

   if (max_bright < dynamic_light_min)
      return;
#endif

   if (!r->light_list[s].dynamic)
      get_dynamic_lm(&r->light_list[s]);
   else {
      lit=1;
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      uchar *output = &r->light_list[s].dynamic[j*r->light_list[s].row];

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].u->raw, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].v->raw, v);
      mx_scale_vec(&step, &r->render_list[s].u->raw, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = fast_compute_dynamic_light_at_point(&where, &lt->vec, dist);
         if (amt > 52) {
            amt = *output - amt;
            if (amt < 0) amt = 0;
            *output = amt;
            lit = TRUE;
         }
         mx_addeq_vec(&where, &step);
         ++output;
      }
      v += LIGHT_MAP_SIZE;
   }
   
   if (!lit)
      unget_dynamic_lm(&r->light_list[s]);
}


// We need one height * row for each animated light (that is, each on bit)
// plus one for the static lightmap data.
static int bit_count(int i)
{
   int size = 0;

   while (i) {
      if (i & 1)
         size++;
      i >>= 1;
   }
   return size;
}


#define ANIM_LIGHT_CUTOFF 15

bool portal_raycast_light(PortalCell *r, Location *lt, uchar perm)
{
   PortalPolygonCore *poly = r->poly_list;
   int voff=0,i,n = r->num_render_polys;
   bool quad = FALSE;
   bool illumination_reached_cell = FALSE;

   if (perm & LIGHT_ANIMATED) {
      illumination_cutoff = ANIM_LIGHT_CUTOFF;
   } else {
      illumination_cutoff = 0;
   }
   
   for (i = 0; i < n; ++i) {
      PortalLightMap *lm = &r->light_list[i];

      if (perm & LIGHT_ANIMATED)
         lm->anim_light_bitmask <<= 1;

      if (perm & LIGHT_QUAD)
         quad = TRUE;

      // The bitmask for a polygon's animated lights maps into its
      // cell's light_indices.  So we advance the bitmask even if this
      // polygon is not reached by this light, since the light still
      // appears in the list.
      if (check_surface_visible(r, poly, voff)) {
         uchar *bits = lm->bits;

         portal_raycast_light_poly(r, poly, lt, voff);

         // Light from static lights is all combined into one
         // lightmap.  Light from each animated light is stored
         // separately in the memory right after that, with the first
         // at the end of the list.  So we expand the bits field of
         // the lightmap on the fly to hold the new data.
         if (bits) {
            if (perm & LIGHT_ANIMATED) {
               // We have one static image and let's-see-how-many others.
               int num_images = 1 + bit_count(lm->anim_light_bitmask);
               int area = lm->h * lm->row;

               lm->bits = Realloc(lm->bits, area * (num_images + 1));
               memmove(lm->bits + area * 2, lm->bits + area,
                       area * (num_images - 1));

               // point to new second image in lightmap bits & clear image
               bits = lm->bits + area;
               memset (bits, 0, area);

               // We don't bother to keep the separate lightmap if the
               // light doesn't reach this surface.
               illumination_reached_polygon = FALSE;

               portal_raycast_light_poly_lightmap(r, i, voff, lt, bits,
                                                  quad);

               if (illumination_reached_polygon) {
                  lm->anim_light_bitmask |= 1;
                  illumination_reached_cell = TRUE;
               } else {
                  memmove(lm->bits + area, lm->bits + area * 2,
                          area * (num_images - 1));

                  lm->bits = Realloc(lm->bits, area * num_images);
               }
            } else {
               portal_raycast_light_poly_lightmap(r, i, voff, lt, bits,
                                                  quad);
               if (illumination_reached_polygon)
                  illumination_reached_cell = TRUE;
            }
         }
      }
      voff += poly->num_vertices;
      ++poly;
   }

   if (perm & LIGHT_ANIMATED && !illumination_reached_cell)
      for (i = 0; i < n; ++i) {
         PortalLightMap *lm = &r->light_list[i];
         lm->anim_light_bitmask >>= 1;
      }

   return illumination_reached_cell;
}


bool portal_nonraycast_light(PortalCell *r, Location *lt, uchar perm)
{
   PortalPolygonCore *poly = r->poly_list;
   int voff=0,i,n = r->num_render_polys;
   bool illumination_reached_cell = FALSE;

   for (i=0; i < n; ++i) {
      PortalLightMap *lm = &r->light_list[i];

      if (perm & LIGHT_ANIMATED)
         lm->anim_light_bitmask <<= 1;

      // The bitmask for a polygon's animated lights maps into its
      // cell's light_indices.  So we advance the bitmask even if this
      // polygon is not reached by this light, since the light still
      // appears in the list.
      if (check_surface_visible(r, poly, voff)) {
         uchar *bits = lm->bits;

         portal_raycast_light_poly(r, poly, lt, voff);

         // Light from static lights is all combined into one
         // lightmap.  Light from each animated light is stored
         // separately in the memory right after that, with the first
         // at the end of the list.  So we expand the bits field of
         // the lightmap on the fly to hold the new data.
         if (bits) {
            if (perm & LIGHT_ANIMATED) {
               // We have one static image and let's-see-how-many others.
               int num_images = 1 + bit_count(lm->anim_light_bitmask);
               int area = lm->h * lm->row;

               lm->bits = Realloc(lm->bits, area * (num_images + 1));
               memmove(lm->bits + area * 2, lm->bits + area,
                       area * (num_images - 1));

               bits = lm->bits + area;
               memset (bits, 0, area);

               // We don't bother to keep the separate lightmap if the
               // light doesn't reach this surface.
               illumination_reached_polygon = FALSE;

               portal_light_poly_lightmap(r, i, voff, lt, bits);

               if (illumination_reached_polygon) {
                  lm->anim_light_bitmask |= 1;
                  illumination_reached_cell = TRUE;
               } else {
                  memmove(lm->bits + area, lm->bits + area * 2,
                          area * (num_images - 1));

                  lm->bits = Realloc(lm->bits, area * num_images);
               }
            } else {
               portal_light_poly_lightmap(r, i, voff, lt, bits);
               if (illumination_reached_polygon)
                  illumination_reached_cell = TRUE;
            }
         }
      }
      voff += poly->num_vertices;
      ++poly;
   }

   if (perm & LIGHT_ANIMATED && !illumination_reached_cell)
      for (i = 0; i < n; ++i) {
         PortalLightMap *lm = &r->light_list[i];
         lm->anim_light_bitmask >>= 1;
      }

   return illumination_reached_cell;
}

void portal_dynamic_light(PortalCell *r, Location *lt)
{
   PortalPolygonCore *poly = r->poly_list;
   int voff=0,i,n = r->num_render_polys;
   // compute distance to nearest point on sphere
   //    1) compute distance to sphere
   //    2) subtract sphere radius
   mxs_real dist = mx_dist_vec(&r->sphere_center, &lt->vec);
   dist -= r->sphere_radius;

   if (dist > 0
    && fast_compute_dynamic_light_at_center(dist) < dynamic_light_min)
      return;

   for (i=0; i < n; ++i) {
      if (check_surface_visible(r, poly, voff)) {
         if (r->light_list[i].bits)
            portal_dynamic_light_lightmap(r, i, voff, lt, dist);
      }
      voff += poly->num_vertices;
      ++poly;
   }
}


void portal_dynamic_dark(PortalCell *r, Location *lt)
{
   PortalPolygonCore *poly = r->poly_list;
   int voff=0,i,n = r->num_render_polys;
   PortalPlane *plane;

   // compute distance to nearest point on sphere
   //    1) compute distance to sphere
   //    2) subtract sphere radius
   mxs_real dist = mx_dist_vec(&r->sphere_center, &lt->vec);
   dist -= r->sphere_radius;

   if (dist > 0
    && fast_compute_dynamic_light_at_center(dist) < dynamic_light_min)
      return;

   for (i=0; i < n; ++i) {
      plane = &r->plane_list[poly->planeid];

      if (plane->norm->raw.z > 0.7071067811865  // cos 45
       && check_surface_visible(r, poly, voff)
       && r->light_list[i].bits)
         portal_dynamic_dark_lightmap(r, i, voff, lt, dist);
      voff += poly->num_vertices;
      ++poly;
   }
}


void init_portal_light(void)
{
   int i,j;
   for (j=0; j < SPOTSCALE_MAX; ++j) {
      for (i=0; i < SPOTSCALE_MAX; ++i) {
         // decay the lighting depending on distance from center
         int x = (i << SPOTSCALE_SHIFT) - 120;
         int y = (j << SPOTSCALE_SHIFT) - 120;

         float d = sqrt(x*x + y*y) / 100;
         if (d > 1) d = 1;

         spotscale[j][i] = 1-d*d;
      }
   }
}




#if 0
/////////////////////////////////////
//
// dynamic vertex lighting system

PortalCell *first_dynamic;

void reset_dynamic_vertex_lights(void)
{
   PortalCell *next;
   while (first_dynamic) {
      next =  *(PortalCell **) (first_dynamic->vertex_list_dynamic-4);
      Free(first_dynamic->vertex_list_dynamic-4);
      first_dynamic->vertex_list_dynamic = 0;
      first_dynamic = next;
   }
}

  // allocate a dynamic lighting slot for this
uchar *get_dynamic_vertex_lighting(PortalCell *r)
{
   uchar *p = Malloc(r->num_vlist+4);

   * (PortalCell **) p = first_dynamic;
   first_dynamic = r;

   p += 4;

   memcpy(p, r->vertex_list_lighting, r->num_vlist);
   return (r->vertex_list_dynamic = p);
}
#endif

