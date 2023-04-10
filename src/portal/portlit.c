/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portlit.c,v 1.51 2000/01/26 16:11:27 BFarquha Exp $
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
#include <wrdbrend.h>
#include <objcast.h>

#include <animlit.h>

// Have you heard?  This has to be the last header.
#include <dbmem.h>

#define LMSIZE    sizeof(LightmapEntry)

//////////////////////////////////////////////////////////////////////////////

//  The following sections of code are variations on
//  draw_region and draw_surface which are used for doing
//  lighting.  Lighting works by rendering from the point
//  of view of the light; when we get to actually needing
//  to "draw" a surface, we paint light onto it.

#ifdef RGB_LIGHTING
#ifdef RGB_888

extern int hack_red;
extern int hack_green;
extern int hack_blue;

LightmapEntry rgb_add_clamp(LightmapEntry cur, int add)
{
   int c;

   c = cur.A[0] + ((add * hack_red) >> 8);
   if (c > 255)
      c = 255;
   cur.A[0] = c;

   c = cur.A[1] + ((add * hack_green) >> 8);
   if (c > 255)
      c = 255;
   cur.A[1] = c;

   c = cur.A[2] + ((add * hack_blue) >> 8);
   if (c > 255)
      c = 255;
   cur.A[2] = c;

   return cur;
}


// @TODO: If we want to use this for real we need to know the ambient
// light level rather than clamping to 0.
LightmapEntry rgb_sub_clamp(LightmapEntry cur, int add)
{
   int c;

   c = cur.A[0] - ((add * hack_red) >> 8);
   if (c < 0)
      c = 0;
   cur.A[0] = c;

   c = cur.A[1] - ((add * hack_green) >> 8);
   if (c < 0)
      c = 0;
   cur.A[1] = c;

   c = cur.A[2] - ((add * hack_blue) >> 8);
   if (c < 0)
      c = 0;
   cur.A[2] = c;

   return cur;
}

#else // RGB_888

// for each light we process, we write out the following tables
// so we can compute things fast
ulong pl_red_color[256], pl_green_color[256], pl_blue_color[256];
ulong pl_red_mask, pl_green_mask, pl_blue_mask;
LightmapEntry rgb_add_clamp(LightmapEntry cur, int add)
{
   int r = pl_red_color[add] + (cur & pl_red_mask);
   int g = pl_green_color[add] + (cur & pl_green_mask);
   int b = pl_blue_color[add] + (cur & pl_blue_mask);
   if (r > pl_red_mask) r = pl_red_mask; else r &= pl_red_mask;
   if (g > pl_green_mask) g = pl_green_mask; else g &= pl_green_mask;
   if (b > pl_blue_mask) b = pl_blue_mask; else b &= pl_blue_mask;
   return r | g | b;
}

LightmapEntry rgb_sub_clamp(LightmapEntry cur, int add)
{
   int r = (cur & pl_red_mask) - pl_red_color[add];
   int g = (cur & pl_green_mask) - pl_green_color[add];
   int b = (cur & pl_blue_mask) - pl_blue_color[add];
   if (r < 0) r = 0; else r &= pl_red_mask;
   if (g < 0) g = 0; else g &= pl_green_mask;
   if (b < 0) b = 0; else b &= pl_blue_mask;
   return r | g | b;
}
#endif // RGB_888

#endif

////////////////////////////////////////
//
// dynamic light map lighting system

typedef struct dyn_lm_bits dyn_lm_bits;
struct dyn_lm_bits {
   PortalLightMap *lm;
   dyn_lm_bits *next;
   LightmapEntry first_byte[1];
};


// This points to the first element of a linked list.
static dyn_lm_bits *dyn_lm_base;


void reset_dynamic_lm(void)
{
   dyn_lm_bits *next;

   while (dyn_lm_base) {
      dyn_lm_base->lm->dynamic_light = NULL;
      next = dyn_lm_base->next;
      Free(dyn_lm_base);
      dyn_lm_base = next;
   }
}


void reset_dynamic_lights(void)
{
   reset_dynamic_lm();
}


void get_dynamic_lm(PortalLightMap *lm)
{
   int size = lm->pixel_row * lm->h * LMSIZE;
   dyn_lm_bits *new_lm = Malloc(sizeof(dyn_lm_bits) - LMSIZE + size);

   AssertMsg1(new_lm, "Could not allocate dynamic lightmap of size %d.", size);

   new_lm->next = dyn_lm_base;
   dyn_lm_base = new_lm;

   new_lm->lm = lm;
   lm->dynamic_light = new_lm->first_byte;
   memcpy(lm->dynamic_light, lm->data, size);
}


void unget_dynamic_lm(PortalLightMap *lm)
{
   dyn_lm_bits *next;

   AssertMsg1(dyn_lm_base, "Failed to unget dynamic lightmap for %x", lm);

   dyn_lm_base->lm->dynamic_light = 0;
   next = dyn_lm_base->next;
   Free(dyn_lm_base);
   dyn_lm_base = next;
}

extern bool PortalTestLocationInsidePlane(PortalPlane *p, Location *loc);

// information about the current light

#define MAX_DIST 8.0
float max_dist = MAX_DIST;
float max_dist_2 = (MAX_DIST * MAX_DIST);
float inner_dist, dist_diff_recip;
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

   norm = &plane->normal;

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
   if (max_dist != 0.0 && len > max_dist)
      return 0.0;
   result = (result/len/2 + 0.5);

      // copy quake's angle remapping
     // bright * result / length(vec)^2 / 2, bright / length(vec)

   if (inner_dist && len > inner_dist)
      result *= (max_dist - len) * dist_diff_recip;

   result = result * bright / len;

   if (result > illumination_cutoff) {
      illumination_reached_polygon = TRUE;
      return result;
   } else
      return 0;
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

void portal_light_poly(int r, int p)
{
   int i, voff=0;
   PortalPolygonCore *poly = WR_CELL(r)->poly_list;

   for (i=0; i < p; ++i)
      voff += poly++->num_vertices;
}

#define LIGHT_MAP_SIZE  0.25

bool record_movement;

extern int cur_raycast_cell;

uchar *portal_anim_light_intensity;
int num_anim_lights;

void (*failed_light_callback)(Location *hit, Location *dest);
void (*lightmap_point_callback)(mxs_vector *loc, bool lit);
void (*lightmap_callback)(PortalLightMap *lightmap);


#define DIST_IN_FROM_POLYGON .025


#ifdef EDITOR
extern ObjID *g_pObjCastList;
extern int    g_ObjCastListSize;
#endif

// This finds the level of illumination on a point from a light
// source.  If our point is outside the world rep, we approximate it
// using the point we reach when we raycast to our intended point
// from the middle of the polygon.
static int portal_illumination_from_light(Location *point_being_lit,
                                          Location *light,
                                          Location *point_in_poly,
                                          PortalCell *cell,
                                          int polygon_index,
                                          bool objcast)
{
   Location dest;
   Location dummy;

#ifdef EDITOR
   if (objcast)
   {
      if (!PortalRaycast(point_in_poly, point_being_lit, &dest, 1))
         point_being_lit = &dest;

      if (ObjRaycastC(light, point_being_lit, &dummy, TRUE,
                     g_pObjCastList, g_ObjCastListSize, 0, FALSE) == kObjCastNone)
      {
         return compute_light_at_point(&point_being_lit->vec,
                  &cell->plane_list[cell->poly_list[polygon_index].planeid].normal,
                  &light->vec);
      }
   }
   else
#endif
   {
      if (!PortalRaycast(point_in_poly, point_being_lit, &dest, 1))
         point_being_lit = &dest;
      if (PortalRaycast(light, point_being_lit, &dummy, 0))
      {
         return compute_light_at_point(&point_being_lit->vec,
                  &cell->plane_list[cell->poly_list[polygon_index].planeid].normal,
                  &light->vec);
      }
   }

   return 0;
}


// large vector in direction of sunlight
mxs_vector portal_sunlight;
// unit vector in opposite direction from sunlight
mxs_vector portal_sunlight_norm;

float compute_sunlight_at_point(mxs_vector *norm)
{
   float result = -mx_dot_vec(&portal_sunlight, norm) * ambient_weight;

   if (result < 0)
      return 0;

   if (result > illumination_cutoff) {
      illumination_reached_polygon = TRUE;
      return result;
   } else
      return 0;
}


static BOOL TouchedTheSky()
{
   int iPoly = PortalRaycastFindPolygon();
   int iTex = (WR_CELL(PortalRaycastCell)->render_list + iPoly)->texture_id;

   return iTex >= 249;  // @HACK: value is from texmem.h: BACKHACK_IDX
}


BOOL portal_loc_has_sunlight(Location *point_being_lit)
{
   Location light, dummy;
   mx_sub_vec(&light.vec, &point_being_lit->vec, &portal_sunlight);

   if (PortalRaycast(point_being_lit, &light, &dummy, 0)) {
      Warning(("portlit: sunlight cast hit nothing!.\n"));
      return 0;
   }

   return TouchedTheSky();
}


static BOOL sunlight_move_axis_in(float fDiff, float *pAxis)
{
   if (fDiff > DIST_IN_FROM_POLYGON) {
      *pAxis -= DIST_IN_FROM_POLYGON;
      return TRUE;
   }

   if (fDiff < -DIST_IN_FROM_POLYGON) {
      *pAxis += DIST_IN_FROM_POLYGON;
      return TRUE;
   }

   return FALSE;
}


// This finds the level of illumination on a point from sunlight.
static int portal_illumination_from_sunlight(Location *point_being_lit,
                                             Location *point_in_poly,
                                             PortalCell *cell,
                                             int polygon_index,
                                             bool objcast)
{
   Location source;
   Location dummy;
   Location light;
   mxs_vector diff;

   ComputeCellForLocation(point_being_lit);

   // if necessary, move in from lightmap point
   if (!PortalRaycast(point_in_poly, point_being_lit, &source, 1)) {
      source.cell = source.hint = point_in_poly->cell;
      point_being_lit = &source;
      ComputeCellForLocation(point_being_lit);
   }

   // We need to make sure our point is in the world before we raycast
   // from it.
   if (point_being_lit->cell == CELL_INVALID) {
      // move contact point towards middle of polygon to try to get it
      // into the world--if we can't move it we use the middle of the
      // polygon since that's *got* to be in the world
      BOOL moved_point = FALSE;
      mx_sub_vec(&diff, &point_being_lit->vec, &point_in_poly->vec);

      moved_point |= sunlight_move_axis_in(diff.x, &point_being_lit->vec.x);
      moved_point |= sunlight_move_axis_in(diff.y, &point_being_lit->vec.y);
      moved_point |= sunlight_move_axis_in(diff.z, &point_being_lit->vec.z);

      if (moved_point)
         ComputeCellForLocation(point_being_lit);

      if (point_being_lit->cell == CELL_INVALID)
         point_being_lit = point_in_poly;
   }

   mx_sub_vec(&light.vec, &point_being_lit->vec, &portal_sunlight);

#ifdef EDITOR
   if (objcast)
   {
      eObjCastResult r = ObjRaycastC(point_being_lit, &light, &dummy, TRUE,
                                     g_pObjCastList, g_ObjCastListSize, 0, FALSE);
      switch (r)
      {
         case kObjCastNone:
            Warning(("portlit: sunlight cast hit nothing!.\n"));
            break;
         case kObjCastTerrain:
            if (TouchedTheSky())
               return compute_sunlight_at_point(&cell->plane_list[cell->poly_list[polygon_index].planeid].normal);
         case kObjCastMD:
         case kObjCastMesh:
            break;
      }
   }
   else
#endif
   {
      if (PortalRaycast(point_being_lit, &light, &dummy, 0)) {
         Warning(("portlit: sunlight cast hit nothing!.\n"));
         return 0;
      }

      if (TouchedTheSky())
         return compute_sunlight_at_point(&cell->plane_list[cell->poly_list[polygon_index].planeid].normal);
   }

   return 0;
}


bool       portal_spotlight;
mxs_vector portal_spotlight_loc;
mxs_vector portal_spotlight_dir;
float      portal_spotlight_inner;
float      portal_spotlight_outer;

float portal_evaluate_spotlight(mxs_vector *loc,
                                mxs_vector *light_loc,
                                mxs_vector *light_dir,
                                float inner, float outer)
{
   float dot;
   mxs_vector delta;
   mx_sub_vec(&delta, loc, light_loc);
   mx_normeq_vec(&delta);
   dot = mx_dot_vec(&delta, light_dir);
   if (dot >= inner) return 1;
   if (dot <= outer) return 0;
   return (dot-outer)/(inner-outer);
}

void portal_raycast_light_poly_lightmap(PortalCell *r, int s, int vc,
                                        Location *lt, LightmapEntry *bits,
                                        bool quadruple_lighting, bool objcast)
{
   // iterate over all of the points in the light map
   int i, j, lux;
   LightmapEntry *light_point;
   float u,v;
   Location dest, source;
   mxs_vector where, src, step;
   mxs_vector quarter_offset_plus;      // offset to upper-right quadrant
   mxs_vector quarter_offset_minus;     // offset to upper-left quadrant
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor

   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].tex_u, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].tex_v, -r->render_list[s].v_base / (16*256.0));
   mx_scale_addeq_vec(&src,
                      &r->plane_list[r->poly_list[s].planeid].normal,
                      DIST_IN_FROM_POLYGON);

   // start with a point inside the polygon
   where = r->render_list[s].center;
   mx_scale_addeq_vec(&where,
                      &r->plane_list[r->poly_list[s].planeid].normal,
                      DIST_IN_FROM_POLYGON);

   // We're using the point inside the poly to check whether each
   // light point really exists in its poly, rather than being outside
   // the world rep.
   MakeLocationFromVector(&source, &where);
   source.cell = source.hint = cur_raycast_cell;

   if (lightmap_callback)
      lightmap_callback(&r->light_list[s]);

   if (quadruple_lighting) {
      mx_scale_vec(&quarter_offset_plus,
                   &r->render_list[s].tex_u, .25);
      quarter_offset_minus = quarter_offset_plus;

      mx_scale_addeq_vec(&quarter_offset_plus,
                         &r->render_list[s].tex_v, .25);

      mx_scale_addeq_vec(&quarter_offset_minus,
                         &r->render_list[s].tex_v, -.25);
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].tex_u, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].tex_v, v);
      mx_scale_vec(&step, &r->render_list[s].tex_u, LIGHT_MAP_SIZE);

      for (i = 0; i < r->light_list[s].w; ++i) {
         MakeLocationFromVector(&dest, &where);
         if (quadruple_lighting) {
            mx_add_vec(&dest.vec, &where, &quarter_offset_plus);
            UpdateChangedLocation(&dest);
            lux = portal_illumination_from_light(&dest, lt, &source, r, s, objcast);

            mx_add_vec(&dest.vec, &where, &quarter_offset_minus);
            UpdateChangedLocation(&dest);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s, objcast);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_plus);
            UpdateChangedLocation(&dest);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s, objcast);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_minus);
            UpdateChangedLocation(&dest);
            lux += portal_illumination_from_light(&dest, lt, &source, r, s, objcast);

            lux /= 4;
         } else
            lux = portal_illumination_from_light(&dest, lt, &source, r, s, objcast);

         if (lux && portal_spotlight) {
            lux *= portal_evaluate_spotlight(&where,
                      &portal_spotlight_loc, &portal_spotlight_dir,
                      portal_spotlight_inner, portal_spotlight_outer);
         }

         light_point = &(bits[j * r->light_list[s].pixel_row + i]);
#ifndef RGB_LIGHTING
         lux += *light_point;
         if (lux > 255)
            lux = 255;
         *light_point = lux;
#else
         if (lux > 255)
            lux = 255;
         *light_point = rgb_add_clamp(*light_point, lux);
#endif
         mx_addeq_vec(&where, &step);
         u += LIGHT_MAP_SIZE;
      }
      v += LIGHT_MAP_SIZE;
   }
}


void portal_raycast_sunlight_poly_lightmap(PortalCell *r, int s, int vc,
                                           LightmapEntry *bits,
                                           bool quad_lighting, bool objcast)
{
   // iterate over all of the points in the light map
   int i, j, lux;
   LightmapEntry *light_point;
   float u,v;
   Location dest, source;
   mxs_vector where, src, step;
   mxs_vector quarter_offset_plus;      // offset to upper-right quadrant
   mxs_vector quarter_offset_minus;     // offset to upper-left quadrant
   mxs_vector *base = &r->vpool[r->vertex_list[vc]];

   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].tex_u, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].tex_v, -r->render_list[s].v_base / (16*256.0));
   mx_scale_addeq_vec(&src, &r->plane_list[r->poly_list[s].planeid].normal,
                      DIST_IN_FROM_POLYGON);

   // start with a point inside the polygon
   where = r->render_list[s].center;
   mx_scale_addeq_vec(&where, &r->plane_list[r->poly_list[s].planeid].normal,
                      DIST_IN_FROM_POLYGON);

   // We're using the point inside the poly to check whether each
   // light point really exists in its poly, rather than being outside
   // the world rep.
   MakeLocationFromVector(&source, &where);
   source.cell = source.hint = cur_raycast_cell;

   if (quad_lighting) {
      mx_scale_vec(&quarter_offset_plus,
                   &r->render_list[s].tex_u, .25);
      quarter_offset_minus = quarter_offset_plus;

      mx_scale_addeq_vec(&quarter_offset_plus,
                         &r->render_list[s].tex_v, .25);

      mx_scale_addeq_vec(&quarter_offset_minus,
                         &r->render_list[s].tex_v, -.25);
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].tex_u, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].tex_v, v);
      mx_scale_vec(&step, &r->render_list[s].tex_u, LIGHT_MAP_SIZE);

      for (i = 0; i < r->light_list[s].w; ++i) {
         MakeLocationFromVector(&dest, &where);
         dest.cell = dest.hint = cur_raycast_cell;

         if (quad_lighting) {
            mx_add_vec(&dest.vec, &where, &quarter_offset_plus);
            lux = portal_illumination_from_sunlight(&dest, &source, r,
                                                    s, objcast);

            mx_add_vec(&dest.vec, &where, &quarter_offset_minus);
            lux += portal_illumination_from_sunlight(&dest, &source, r,
                                                     s, objcast);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_plus);
            lux += portal_illumination_from_sunlight(&dest, &source, r,
                                                     s, objcast);

            mx_sub_vec(&dest.vec, &where, &quarter_offset_minus);
            lux += portal_illumination_from_sunlight(&dest, &source, r,
                                                     s, objcast);

            lux /= 4;
         } else
            lux = portal_illumination_from_sunlight(&dest, &source, r,
                                                    s, objcast);

         light_point = &(bits[j * r->light_list[s].pixel_row + i]);
#ifndef RGB_LIGHTING
         lux += *light_point;
         if (lux > 255)
            lux = 255;
         *light_point = lux;
#else
         if (lux > 255)
            lux = 255;
         *light_point = rgb_add_clamp(*light_point, lux);
#endif
         mx_addeq_vec(&where, &step);
         u += LIGHT_MAP_SIZE;
      }
      v += LIGHT_MAP_SIZE;
   }
}


void portal_light_poly_lightmap(PortalCell *r, int s, int vc, Location *lt,
                                LightmapEntry *bits)
{
   // iterate over all of the points in the light map
   int i,j;
   float u,v, dist;
   mxs_vector where, src, step;
   mxs_vector *base = &r->vpool[r->vertex_list[vc]]; // TODO: texture_anchor

   mxs_vector *norm = &r->plane_list[r->poly_list[s].planeid].normal;
   // currently base is at texture coordinate (base_u, base_v)
   // we want base to be at (0,0)

   mx_scale_add_vec(&src, base,
      &r->render_list[s].tex_u, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].tex_v, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].normal, &lt->vec);

   if (fast_compute_light_at_center(dist) < 2.0)
      return;

   if (lightmap_callback)
      lightmap_callback(&r->light_list[s]);

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      LightmapEntry *output = &bits[j*r->light_list[s].pixel_row];

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].tex_u, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].tex_v, v);
      mx_scale_vec(&step, &r->render_list[s].tex_u, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = compute_light_at_point(&where, norm, &lt->vec);
         if (amt && portal_spotlight) {
            amt *= portal_evaluate_spotlight(&where,
                      &portal_spotlight_loc, &portal_spotlight_dir,
                      portal_spotlight_inner, portal_spotlight_outer);
         }

         if (amt > 1) {
            illumination_reached_polygon = TRUE;
#ifndef RGB_LIGHTING
            amt += *output;
            if (amt > 255) amt = 255;
            *output = amt;
#else
            if (amt > 255) amt = 255;
            *output = rgb_add_clamp(*output, amt);
#endif
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
      &r->render_list[s].tex_u, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].tex_v, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].normal, &lt->vec);

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

   if (!r->light_list[s].dynamic_light)
      get_dynamic_lm(&r->light_list[s]);
   else {
      lit=1;
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      int offset = j*r->light_list[s].pixel_row;
      LightmapEntry *output = r->light_list[s].dynamic_light + offset;

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].tex_u, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].tex_v, v);
      mx_scale_vec(&step, &r->render_list[s].tex_u, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = fast_compute_dynamic_light_at_point(&where, &lt->vec, dist);
#ifndef RGB_LIGHTING
         if (amt > 8) {
            amt += *output;
            if (amt > 255) amt = 255;
            *output = amt;
            lit = TRUE;
         }
#else
         if (amt > 8) {
            if (amt > 255) amt = 255;
            *output = rgb_add_clamp(*output, amt);
            lit = TRUE;
         }
#endif
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
      &r->render_list[s].tex_u, -r->render_list[s].u_base / (16*256.0));
   mx_scale_addeq_vec(&src,
      &r->render_list[s].tex_v, -r->render_list[s].v_base / (16*256.0));

   dist = fast_precompute_light(&src,
        &r->plane_list[r->poly_list[s].planeid].normal, &lt->vec);

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

   if (!r->light_list[s].dynamic_light)
      get_dynamic_lm(&r->light_list[s]);
   else {
      lit=1;
   }

   v = r->light_list[s].base_v * LIGHT_MAP_SIZE;
   for (j=0; j < r->light_list[s].h; ++j) {
      int offset = j*r->light_list[s].pixel_row;
      LightmapEntry *output = r->light_list[s].dynamic_light + offset;

      u = r->light_list[s].base_u * LIGHT_MAP_SIZE;

      mx_scale_add_vec(&where, &src, &r->render_list[s].tex_u, u);
      mx_scale_addeq_vec(&where, &r->render_list[s].tex_v, v);
      mx_scale_vec(&step, &r->render_list[s].tex_u, LIGHT_MAP_SIZE);

      for (i=0; i < r->light_list[s].w; ++i) {
         int amt = fast_compute_dynamic_light_at_point(&where, &lt->vec, dist);
#ifndef RGB_LIGHTING
         if (amt > 52) {
            amt = *output - amt;
            if (amt < 0) amt = 0;
            *output = amt;
            lit = TRUE;
         }
#else
         if (amt > 8) {
            *output = rgb_sub_clamp(*output, amt);
            lit = TRUE;
         }
#endif
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
   bool objcast = FALSE;
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

      if (perm & LIGHT_OBJCAST)
         objcast = TRUE;

      // The bitmask for a polygon's animated lights maps into its
      // cell's light_indices.  So we advance the bitmask even if this
      // polygon is not reached by this light, since the light still
      // appears in the list.
      if (check_surface_visible(r, poly, voff)) {
         LightmapEntry *bits = lm->data;

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
               int area = lm->h * lm->pixel_row;

               lm->data = Realloc(lm->data, area * (num_images + 1) * LMSIZE);
               memmove(lm->data + area * 2, lm->data + area,
                       area * (num_images - 1) * LMSIZE);

               // point to new second image in lightmap bits & clear image
               bits = lm->data + area;
               memset (bits, 0, area*LMSIZE);

               // We don't bother to keep the separate lightmap if the
               // light doesn't reach this surface.
               illumination_reached_polygon = FALSE;

               portal_raycast_light_poly_lightmap(r, i, voff, lt, bits, quad, objcast);

               if (illumination_reached_polygon) {
                  lm->anim_light_bitmask |= 1;
                  illumination_reached_cell = TRUE;
               } else {
                  memmove(lm->data + area, lm->data + area * 2,
                          area * (num_images - 1) * LMSIZE);

                  lm->data = Realloc(lm->data, area * num_images * LMSIZE);
               }
            } else {
               portal_raycast_light_poly_lightmap(r, i, voff, lt, bits,
                                                  quad, objcast);
               if (illumination_reached_polygon)
                  illumination_reached_cell = TRUE;
            }
         }
      }
      voff += poly->num_vertices;
      ++poly;
   }

   if ((perm & LIGHT_ANIMATED) && !illumination_reached_cell)
      for (i = 0; i < n; ++i) {
         PortalLightMap *lm = &r->light_list[i];
         lm->anim_light_bitmask >>= 1;
      }

   return illumination_reached_cell;
}


bool portal_raycast_sunlight(PortalCell *r, uchar perm)
{
   PortalPolygonCore *poly = r->poly_list;
   PortalPlane *plane = r->plane_list;
   int voff=0,i,n = r->num_render_polys;
   bool quad = FALSE;
   bool objcast = FALSE;
   bool illumination_reached_cell = FALSE;

   illumination_cutoff = 0;
   if (perm & LIGHT_QUAD)
      quad = TRUE;

   if (perm & LIGHT_OBJCAST)
      objcast = TRUE;

   for (i = 0; i < n; ++i) {
      // backface check
      if (mx_dot_vec(&plane[poly->planeid].normal, &portal_sunlight) < 0) {
         PortalLightMap *lm = &r->light_list[i];
         LightmapEntry *bits = lm->data;

         if (bits) {
            portal_raycast_sunlight_poly_lightmap(r, i, voff, bits, quad,
                                                  objcast);
            if (illumination_reached_polygon)
               illumination_reached_cell = TRUE;
         }
      }
      voff += poly->num_vertices;
      ++poly;
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
         LightmapEntry *bits = lm->data;

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
               int area = lm->h * lm->pixel_row;

               lm->data = Realloc(lm->data, area * (num_images + 1) * LMSIZE);
               memmove(lm->data + area * 2, lm->data + area,
                       area * (num_images - 1) * LMSIZE);

               bits = lm->data + area;
               memset (bits, 0, area*LMSIZE);

               // We don't bother to keep the separate lightmap if the
               // light doesn't reach this surface.
               illumination_reached_polygon = FALSE;

               portal_light_poly_lightmap(r, i, voff, lt, bits);

               if (illumination_reached_polygon) {
                  lm->anim_light_bitmask |= 1;
                  illumination_reached_cell = TRUE;
               } else {
                  memmove(lm->data + area, lm->data + area * 2,
                          area * (num_images - 1) * LMSIZE);

                  lm->data = Realloc(lm->data, area * num_images * LMSIZE);
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
         if (r->light_list[i].data)
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

      if (plane->normal.z > 0.7071067811865  // cos 45
           && check_surface_visible(r, poly, voff)
           && r->light_list[i].data)
         portal_dynamic_dark_lightmap(r, i, voff, lt, dist);
      voff += poly->num_vertices;
      ++poly;
   }
}


void init_portal_light(void)
{
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

