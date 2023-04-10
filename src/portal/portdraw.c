/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portdraw.c,v 1.119 2000/02/19 13:18:57 toml Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <lg.h>
#include <r3d.h>
#include <g2pt.h>
#include <mprintf.h>
#include <lgd3d.h>
#include <tmpalloc.h>

#include <portwatr.h>
#include <portal_.h>
#include <portclip.h>
#include <porthw.h>
#include <pt_clut.h>
#include <portsky.h>
#include <wrdbrend.h>

#include <profile.h>

#ifdef DBG_ON
  #define STATIC
#else
  #define STATIC static
#endif

extern mxs_vector portal_camera_loc;
extern bool show_lightmap;

#define STATS_ON

#ifdef DBG_ON
  #ifndef STATS_ON
  #define STATS_ON
  #endif
#endif

#ifdef STATS_ON
  #define STAT(x)     x
#else
  #define STAT(x)
#endif

// Here's the current placement of textures applied to medium
// boundaries:
PortalCellMotion portal_cell_motion[MAX_CELL_MOTION];

// We can fiddle this up and down to vary the level of detail.
// Higher means more texels.  Right now it only affects MIP mapping.
// The expected range is 0.8-1.5; the default setting is just what looks
// good to me without clobbering the frame rate.
float portal_detail_level = 1.90;

float dot_clamp=0.6;
#define VISOBJ_NULL      (-1)


// If this is on, we blow off the usual rendering and use these flags.
// Each face is a solid, flat polygons, with a white wireframe outline.
#ifndef SHIP
#include <uigame.h>
#include <guistyle.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
   bool draw_solid_by_MIP_level = FALSE;
   bool draw_solid_by_cell = FALSE;
   bool draw_solid_wireframe = FALSE;
   bool draw_solid_by_poly_flags = FALSE;
   bool draw_solid_by_cell_flags = FALSE;
   bool draw_wireframe_around_tmap = FALSE;
   bool draw_wireframe_around_poly = FALSE;
   uint polygon_cell_color;
   uint _polygon_cell_flags_color;

   #define COLOR_WHITE 0x1ffffff
#endif // ~SHIP

r3s_point   *cur_ph;    // list of pointers to r3s_points for current pool
Vector      *cur_pool;  // list of untransformed vectors
PortalCell  *cur_cell;
ushort      *cur_anim_light_index_list;
extern int   cur_cell_num;

extern ulong fog_r3_color;      // from portal.c

bool portal_clip_poly = TRUE;
bool portal_render_from_texture = FALSE;

#ifdef STATS_ON
extern int stat_num_poly_drawn;
extern int stat_num_poly_raw;
extern int stat_num_poly_considered;
extern int stat_num_backface_tests;
#endif

///// determine if a surface is visible /////
// This rejects if it is backfacing
// right now we use the r3d, which is slow; we should compute
//   the polygon normal once and use that, and eventually use
//   the normal cache and make it superfast

#define MAX_VERT 32

  // is there a reason this isn't a bool?
int check_surface_visible(PortalCell *cell, PortalPolygonCore *poly, int voff)
{
   // evaluate the plane equation for this surface

   extern mxs_vector portal_camera_loc;
   int plane = poly->planeid;
   PortalPlane *p = &cell->plane_list[plane];
   float dist = mx_dot_vec(&portal_camera_loc, &p->normal)
              + p->plane_constant;

   STAT(++stat_num_backface_tests;)
   return dist > 0;
}


static double portal_detail_2, dot_clamp_2;
static double pixel_scale, pixel_scale_2;
static double premul, premul2; // premul2 != premul*premul, not named premul_2
void portal_mip_setup(float zoom)
{
   portal_detail_2 = portal_detail_level * portal_detail_level;
   pixel_scale = zoom * grd_bm.w * (1.0 / 128.0);
   pixel_scale_2 = pixel_scale * pixel_scale;

   dot_clamp_2 = dot_clamp * dot_clamp;

   premul = pixel_scale * portal_detail_level;
   premul2 = pixel_scale_2 * portal_detail_2 * dot_clamp_2;
}


//////////////////////////////////////////////////////////////////

///// render a single region /////

bool show_region, show_portal, linear_map=FALSE;
extern void draw_objects_in_node(int n);

uchar *r_vertex_list;//, *r_vertex_lighting;
void *r_clip;

// get a transformed vector.  since we haven't implemented the cache
// yet, we always do it.  since the r3d doesn't implement o2c, we just
// have to transform two points and subtract them.  This will change.

mxs_vector *get_cached_vector(mxs_vector *where, mxs_vector *v)
{  PROF
   r3_rotate_o2c(where, v);
   END_PROF;
   return where;   // should copy it into cache
}

  // our 3x3 texture perspective correction matrix

  // tables mapping lighting values from 0..255 (which is how
  // we store them in polygons) into floating point 'i' values
  // appropriate for clipping.
#if 0
float light_mapping[256];
float light_mapping_dithered[256];
#endif
extern int dither;
int max_draw_polys = 1024;

extern grs_bitmap *get_cached_surface(PortalPolygonRenderInfo *render,
                                      PortalLightMap *lt, grs_bitmap *texture,
                                      int MIP_level);

#ifndef SHIP

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This draws a wireframe around one polygon (it doesn't have to be
   one that's normally rendered).

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void draw_polygon_wireframe(r3s_phandle *points, int num_points, uint color)
{
   int p1, p2;

   r3_set_color(guiScreenColor(color));
   r3_set_line_context(R3_LN_FLAT);

   p2 = num_points - 1;
   for (p1 = 0; p1 < num_points; p1++) {
      r3_draw_line(points[p1], points[p2]);
      p2 = p1;
   }
}

#define VERT_DIST   7

void draw_polygon_vertices(r3s_phandle *points, int num_points, uint color)
{
   int p1;
   double cx,cy;
   r3_set_color(guiScreenColor(color));

   num_points = r3_clip_polygon(num_points, points, &points);

   if (num_points) {

      // find center point of polygon

      cx = cy = 0;
      for (p1 = 0; p1 < num_points; p1++) {
          cx += points[p1]->grp.sx;
          cy += points[p1]->grp.sy;
      }

      cx /= num_points;
      cy /= num_points;

      // now draw all the vertices, displaced towards the center

      for (p1 = 0; p1 < num_points; p1++) {
          r3s_point temp = *points[p1];

          double dx = (cx - temp.grp.sx);
          double dy = (cy - temp.grp.sy);
          double len = sqrt(dx*dx + dy*dy);
          // displace by at most 3, and at least len/2

          if (len/2 < fix_make(VERT_DIST,0)) {
             temp.grp.sx = temp.grp.sx + dx/2;
             temp.grp.sy = temp.grp.sy + dy/2;
          } else {
             temp.grp.sx = temp.grp.sx + (dx/len) * fix_make(VERT_DIST,0);
             temp.grp.sy = temp.grp.sy + (dy/len) * fix_make(VERT_DIST,0);
          }

          r3_draw_point(&temp);
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we're showing the polygons in the visualization tools, each
   visible face is a flat, unshaded polygon with a wireframe border.

   The context constants come from x:\prj\tech\libsrc\r3d\prim.h.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void draw_polygon_outline(PortalPolygonCore *poly, r3s_phandle *points,
                          uint polygon_color, uint outline_color)
{
   int num_points = poly->num_vertices;

   // Here's our polygon.
   r3_set_clipmode(R3_CLIP);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_UNLIT | R3_PL_SOLID);
   r3_set_color(guiScreenColor(polygon_color));

   r3_draw_poly(poly->num_vertices, points);

   // And here go the lines.
   draw_polygon_wireframe(points, num_points, outline_color);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The only difference between one visualization tool and the next is
   the color of the polygons.  We return TRUE if we draw a flat poly.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool poly_outline_by_flags(PortalPolygonCore *poly, r3s_phandle *points,
                           int MIP_level)
{
   if (draw_solid_by_MIP_level) {
      int i;
      int light_level = 32;
      for (i = MIP_level; i < 4; i++)
         light_level += 40;
      draw_polygon_outline(poly, points,
                           uiRGB(light_level, light_level, light_level),
                           uiRGB(light_level + 48, light_level + 48,
                                 light_level + 48));
      return TRUE;
   }

   if (draw_solid_by_cell) {
      //draw_polygon_outline(poly, points, cur_cell->refs ? COLOR_WHITE : polygon_cell_color, COLOR_WHITE);
      draw_polygon_outline(poly, points, polygon_cell_color, COLOR_WHITE);
      return TRUE;
   }

   if (draw_solid_wireframe) {
      draw_polygon_outline(poly, points, 0, COLOR_WHITE);
      return TRUE;
   }

   if (draw_solid_by_poly_flags) {
      draw_polygon_outline(poly, points, poly->flags, COLOR_WHITE);
      return TRUE;
   }

   if (draw_solid_by_cell_flags) {
      draw_polygon_outline(poly, points, _polygon_cell_flags_color,
                           COLOR_WHITE);
      return TRUE;
   }

   return FALSE;
}

#endif // ~SHIP


extern int portal_clip_num;

extern BOOL g_lgd3d;
extern BOOL g_zbuffer;

extern int portal_hack_blend;

#ifndef SHIP

#define TEST_TEXTURE 1

extern ushort portal_color_convert(ushort);

static r3s_point bt_verts[4];
static r3s_phandle bt_vlist[4] = { bt_verts+0, bt_verts+1, bt_verts+2, bt_verts+3 };

static grs_bitmap blend_texture;
void portal_do_blendtest(void)
{
#ifdef RGB_LIGHTING
#ifdef RGB_888
   ushort blend_tex_out[16];

   r3s_texture texture = portal_get_texture(TEST_TEXTURE);

   if (!g_lgd3d) return;

   r3_start_block();

   r3_set_texture(texture);

   bt_verts[0].p.z = bt_verts[1].p.z =
   bt_verts[2].p.z = bt_verts[3].p.z = 1.0;
   bt_verts[0].grp.w = bt_verts[1].grp.w =
   bt_verts[2].grp.w = bt_verts[3].grp.w = 1.0;

   bt_verts[0].grp.sx = bt_verts[3].grp.sx = fix_make(10,0);
   bt_verts[1].grp.sx = bt_verts[2].grp.sx = fix_make(60,0);
   bt_verts[0].grp.sy = bt_verts[1].grp.sy = fix_make(10,0);
   bt_verts[2].grp.sy = bt_verts[3].grp.sy = fix_make(60,0);

   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_GOURAUD);
   bt_verts[0].grp.i = bt_verts[1].grp.i = 0;
   bt_verts[2].grp.i = bt_verts[3].grp.i = 1.0;

   bt_verts[0].grp.u = bt_verts[3].grp.u = 0;
   bt_verts[1].grp.u = bt_verts[2].grp.u = 1;
   bt_verts[0].grp.v = bt_verts[1].grp.v = 0;
   bt_verts[2].grp.v = bt_verts[3].grp.v = 1;

   r3_draw_poly(4, bt_vlist);

   bt_verts[0].grp.sx = bt_verts[3].grp.sx = fix_make(10+80,0);
   bt_verts[1].grp.sx = bt_verts[2].grp.sx = fix_make(60+80,0);

   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
   r3_draw_poly(4, bt_vlist);

   bt_verts[0].grp.u = bt_verts[3].grp.u = 0.25 * 0.5;
   bt_verts[1].grp.u = bt_verts[2].grp.u = 0.25 * 0.5;
   bt_verts[0].grp.v = bt_verts[1].grp.v = 0.25 * 0.5;
   bt_verts[2].grp.v = bt_verts[3].grp.v = 1 - 0.25*1.5;

   // build lightmap texture in device space
   gr_init_bitmap(&blend_texture, (char *) blend_tex_out, BMT_FLAT32, 0, 4, 4);

   /* @TODO: figger out what todo.
   blend_tex_out[0] = portal_color_convert(0);
   blend_tex_out[4] = portal_color_convert(15+15*32+15*32*32);
   blend_tex_out[8] = portal_color_convert(31+31*32+31*32*32);
   */
   blend_tex_out[2] = blend_tex_out[1] = blend_tex_out[0];
   blend_tex_out[6] = blend_tex_out[5] = blend_tex_out[4];
   blend_tex_out[10] = blend_tex_out[9] = blend_tex_out[8];
   blend_tex_out[12] = blend_tex_out[8];

   lgd3d_blend_multiply(portal_hack_blend);
   lgd3d_set_blend(TRUE);
   lgd3d_set_alpha(0.5);
   r3_set_texture(&blend_texture);
   r3_draw_poly(4, bt_vlist);
   lgd3d_blend_normal();
   lgd3d_set_blend(FALSE);
   lgd3d_set_alpha(1.0);

   r3_end_block();

   lgd3d_unload_texture(&blend_texture);
#else // RGB_888
   ushort blend_tex_out[16];

   r3s_texture texture = portal_get_texture(TEST_TEXTURE);

   if (!g_lgd3d) return;

   r3_start_block();

   r3_set_texture(texture);

   bt_verts[0].p.z = bt_verts[1].p.z =
   bt_verts[2].p.z = bt_verts[3].p.z = 1.0;
   bt_verts[0].grp.w = bt_verts[1].grp.w =
   bt_verts[2].grp.w = bt_verts[3].grp.w = 1.0;

   bt_verts[0].grp.sx = bt_verts[3].grp.sx = fix_make(10,0);
   bt_verts[1].grp.sx = bt_verts[2].grp.sx = fix_make(60,0);
   bt_verts[0].grp.sy = bt_verts[1].grp.sy = fix_make(10,0);
   bt_verts[2].grp.sy = bt_verts[3].grp.sy = fix_make(60,0);

   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_GOURAUD);
   bt_verts[0].grp.i = bt_verts[1].grp.i = 0;
   bt_verts[2].grp.i = bt_verts[3].grp.i = 1.0;

   bt_verts[0].grp.u = bt_verts[3].grp.u = 0;
   bt_verts[1].grp.u = bt_verts[2].grp.u = 1;
   bt_verts[0].grp.v = bt_verts[1].grp.v = 0;
   bt_verts[2].grp.v = bt_verts[3].grp.v = 1;

   r3_draw_poly(4, bt_vlist);

   bt_verts[0].grp.sx = bt_verts[3].grp.sx = fix_make(10+80,0);
   bt_verts[1].grp.sx = bt_verts[2].grp.sx = fix_make(60+80,0);

   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
   r3_draw_poly(4, bt_vlist);

   bt_verts[0].grp.u = bt_verts[3].grp.u = 0.25 * 0.5;
   bt_verts[1].grp.u = bt_verts[2].grp.u = 0.25 * 0.5;
   bt_verts[0].grp.v = bt_verts[1].grp.v = 0.25 * 0.5;
   bt_verts[2].grp.v = bt_verts[3].grp.v = 1 - 0.25*1.5;

   // build lightmap texture in device space
   gr_init_bitmap(&blend_texture, (char *) blend_tex_out, BMT_FLAT16, 0, 4, 4);

   blend_tex_out[0] = portal_color_convert(0);
   blend_tex_out[4] = portal_color_convert(15+15*32+15*32*32);
   blend_tex_out[8] = portal_color_convert(31+31*32+31*32*32);
   blend_tex_out[2] = blend_tex_out[1] = blend_tex_out[0];
   blend_tex_out[6] = blend_tex_out[5] = blend_tex_out[4];
   blend_tex_out[10] = blend_tex_out[9] = blend_tex_out[8];
   blend_tex_out[12] = blend_tex_out[8];

   lgd3d_blend_multiply(portal_hack_blend);
   lgd3d_set_blend(TRUE);
   lgd3d_set_alpha(0.5);
   r3_set_texture(&blend_texture);
   r3_draw_poly(4, bt_vlist);
   lgd3d_blend_normal();
   lgd3d_set_blend(FALSE);
   lgd3d_set_alpha(1.0);

   r3_end_block();

   lgd3d_unload_texture(&blend_texture);
#endif // RGB_888
#endif
}
#endif


extern void do_poly_linear(r3s_texture tex, int n, r3s_phandle *vpl, fix u_offset, fix v_offset);

// compute the P,M,N vectors and hand them to portal-tmappers
STATIC
void compute_tmapping(PortalPolygonRenderInfo *render, uchar not_light,
         PortalLightMap *lt, r3s_point *anchor_point)
{
   // compute texture mapping data by getting our u,v vectors,
   // the anchor point, and the translation values, and translating
   // the anchor point by the translation lengths
   mxs_vector u_vec, v_vec, pt;
   mxs_real usc, vsc;

   usc = ((float) render->u_base) * 1.0 / (16.0*256.0); // u translation
   vsc = ((float) render->v_base) * 1.0 / (16.0*256.0); // v translation

   if (!not_light) {
      usc -= ((float) lt->base_u) * 0.25;
      vsc -= ((float) lt->base_v) * 0.25;
   }

   get_cached_vector(&u_vec, &render->tex_u);
   get_cached_vector(&v_vec, &render->tex_v);
   mx_scale_add_vec(&pt, &anchor_point->p, &u_vec, -usc);
   mx_scale_add_vec(&pt, &pt, &v_vec, -vsc);

   // This gives us our 3x3 texture perspective correction matrix.
   g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
}


STATIC
int compute_mip3(PortalPolygonRenderInfo *render, PortalPlane *p)
{
   // this is the fast way, which doesn't need to take a sqrt
   // the slow way, which makes some sense, appears at end of file
   double a,b,d;
   mxs_vector eye;
   mx_sub_vec(&eye, &portal_camera_loc, &render->center);

   a = mx_mag2_vec(&eye);
   b = mx_dot_vec(&eye, &p->normal);

   if (b * b >= a*dot_clamp_2) {
      d = premul * b * render->texture_mag;
      // do first test because it doesn't require multiplies
      if (d >= a) return 0;
      // now binary search the remaining ones
      if (d < a*0.25)
         return d >= a*0.125 ? 3 : 4;
      else
         return d >= a*0.5 ? 1 : 2;
   } else {
      d = premul2 * render->texture_mag * render->texture_mag;
      if (d >= a) return 0;
      if (d < a*0.25*0.25)
         return d >= a*0.125*0.125 ? 3 : 4;
      else
         return d >= a*0.5*0.5 ? 1 : 2;
   }
}

int compute_mip(PortalPolygonRenderInfo *render, PortalPlane *p)
{
   double sz,dist,k;
   mxs_vector eye;

   mx_sub_vec(&eye, &portal_camera_loc, &render->center);
   dist = mx_mag_vec(&eye);

   k = mx_dot_vec(&eye, &p->normal);  // k/dist == foreshortening amount

   // estimate distance to nearest point
   dist = dist - render->texture_mag*2*(1-k/dist);
   if (dist <= 0) return 0;

   // compute foreshortening amount, note this uses post-modified
   // dist, which is geometrically correct if the post-modified dist
   // weren't an approximation
   k /= dist;
   if (k > 1.0) k = 1.0;

   sz = premul * render->texture_mag / dist;

   if (k < dot_clamp) k = dot_clamp;
   sz *= k;

   if (sz >= 1.0) return 0;
   if (sz >= 0.5) return 1;
   if (sz >= 0.25) return 2;
   if (sz >= 0.125) return 3;
   return 4;
}

extern void render_background_hack_clipped(int n, r3s_phandle *vlist);
extern int portbg_clip_sky(int, r3s_phandle *, r3s_phandle **);

// from portal.c. True if we are drawing new-style sky
extern BOOL bRenderNewSky;

void draw_background_hack(int n, r3s_phandle *vlist)
{
   r3s_point *ph = cur_ph;
   Vector *pool = cur_pool;
   PortalCell *cell = cur_cell;
   ushort *anim_light_index_list = cur_anim_light_index_list;
   void *clip = r_clip;
   uchar *vertex_list = r_vertex_list;

   render_background_hack_clipped(n,vlist);

   cur_ph = ph;
   cur_pool = pool;
   cur_cell = cell;
   cur_anim_light_index_list = anim_light_index_list;
   r_clip = clip;
   r_vertex_list = vertex_list;
}

extern int portal_sky_id;
extern int portal_sky_spans;

// returns FALSE if it was totally transparent; return TRUE if it was
// non-transparent, even if not visible (e.g. clipped away)
STATIC
bool draw_surface(PortalPolygonCore *poly, PortalPolygonRenderInfo *render,
                  PortalLightMap *lt, int voff, void *clip)
{
   int i, n,n2,n3, sc;
   int desired_mip, mip_level;
   r3s_texture texture;
   grs_bitmap *tex=0;
   fix corner_u_offset, corner_v_offset;
   uchar not_light;
   bool position_from_motion = FALSE;
   r3s_phandle vlist[MAX_VERT], *valid3d, *final;

   // get the raw, unlit texture
   texture = portal_get_texture(render->texture_id);
   // It is our impression (wsf) that this only happens for the sky hack:
   if (!texture && !bRenderNewSky) {
      n = poly->num_vertices;
      for (i=0; i < n; ++i)
         vlist[i] = &cur_ph[r_vertex_list[voff + i]];

      // clip against the view cone
      n2 = r3_clip_polygon(n, vlist, &valid3d);
      if (n2 <= 2) { END_PROF; return TRUE; }
      STAT(++stat_num_poly_considered;)

      if (portal_clip_poly) {
           // clip against the portal
         n3 = portclip_clip_polygon(n2, valid3d, &final, clip);
         if (n3 <= 2) { END_PROF; return TRUE; }
      } else {
         n3 = n2;
         final = valid3d;
      }

      if (n3)
         draw_background_hack(n3, final);
      return FALSE;  // an invisible portal, or background hack
   }

   not_light = (lt == NULL) || (poly->flags & RENDER_DOESNT_LIGHT);

   // prepare the vertex list
   n = poly->num_vertices;
   if (n > MAX_VERT) Error(1, "draw_surface: too many vertices.\n");

   for (i=0; i < n; ++i)
      vlist[i] = &cur_ph[r_vertex_list[voff + i]];

     // clip against the view cone
   n2 = r3_clip_polygon(n, vlist, &valid3d);
   if (n2 <= 2) { END_PROF; return TRUE; }
   STAT(++stat_num_poly_considered;)

   if (portal_clip_poly) {
        // clip against the portal
      n3 = portclip_clip_polygon(n2, valid3d, &final, clip);
      if (n3 <= 2) { END_PROF; return TRUE; }
   } else {
      n3 = n2;
      final = valid3d;
   }

   if (portal_clip_num) {  // only true if it's the sky
      n3 = portbg_clip_sky(n3, final, &final);
      sc = texture->w;
   } else {
      desired_mip = compute_mip(render, &cur_cell->plane_list[poly->planeid]);
      mip_level = 0;
      while (desired_mip > 0) {
         if (texture[1].w == 0) break; // not enough mip levels
         ++mip_level;
         --desired_mip;
         ++texture;
      }
      sc = 64 >> mip_level; // texture->w;
   }

#ifdef STATS_ON
   ++stat_num_poly_drawn;
   if (stat_num_poly_drawn > max_draw_polys) return TRUE;
#endif

#ifndef SHIP
   if (poly_outline_by_flags(poly, vlist, mip_level)) {
      END_PROF;
      return TRUE;
   }
#endif // ~SHIP

#ifdef DBG_ON
   if (texture->w & (texture->w - 1))
      Error(1, "Texture non-power-of-two in w!\n");
   if (texture->h & (texture->h - 1))
      Error(1, "Texture non-power-of-two in h!\n");
#endif

   if ((poly->motion_index)
    && (portal_cell_motion[poly->motion_index].in_motion)) {
      mxs_vector u_vec, v_vec, pt;

      portal_position_portal_texture(&u_vec, &v_vec, &pt,
             &(cur_pool[r_vertex_list[voff + render->texture_anchor]]),
             render, &cur_cell->plane_list[poly->planeid],
             &portal_cell_motion[poly->motion_index]);

      g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      position_from_motion = TRUE;
   } else
      compute_tmapping(render, not_light, lt, vlist[render->texture_anchor]);

     // rescale based on mipmap scaling
   for(i=0; i < 3; ++i) {
      g2pt_tmap_data[3*i  ] *= sc;
      g2pt_tmap_data[3*i+1] *= sc;
   }

   // bug: if not lighting, don't deref 'lt'!!!
   if (not_light) {
      corner_u_offset = corner_v_offset = 0;
      tex = texture;
   } else {
      corner_u_offset = corner_v_offset = fix_make(2, 0);
      // go get it from surface cache or light it or whatever
      tex = get_cached_surface(render, lt, texture, mip_level);

#ifdef EDITOR
      // This should only happen if the lightmap is too big, which
      // should only happen when someone's fiddling with the texture
      // scale in the editor.
      if (!tex) {
         corner_u_offset = corner_v_offset = 0;
         tex = texture;
      }
#endif // EDITOR
   }


   if ((render->texture_id == portal_sky_id) && !bRenderNewSky)
   {
      switch (ptsky_type) {
      case PTSKY_SPAN:
         portal_sky_spans += ptsky_calc_spans(n3, final);
         break;
      case PTSKY_NONE:
         goto poly_done;
      }
   }
   else if (portal_sky_spans > 0)
   {
      ptsky_render_stars();
      portal_sky_spans = 0;
   }

   if (linear_map)
      do_poly_linear(tex, n3, final, corner_u_offset, corner_v_offset);
   else
      g2pt_poly_perspective_uv(tex, n3, final,
                              corner_u_offset, corner_v_offset, FALSE);
poly_done:
#ifndef SHIP
#if 0
   if (draw_wireframe_around_tmap)
      draw_polygon_wireframe(vlist, poly->num_vertices, COLOR_WHITE);
#else
   if (draw_wireframe_around_tmap) {
      draw_polygon_wireframe(final, n3, COLOR_WHITE+2);
      draw_polygon_vertices(final, n3, COLOR_WHITE);
   }
#endif
#endif // ~SHIP

   END_PROF;
   return TRUE;
}

#ifndef SHIP
void draw_wireframe(PortalPolygonCore *p, int voff, uint color)
{
   int i,j,n;
   n = p->num_vertices;

   r3_set_color(guiScreenColor(color));
   j = n-1;
   for (i=0; i < n; ++i) {
      r3_draw_line(&cur_ph[r_vertex_list[voff+i]],
                   &cur_ph[r_vertex_list[voff+j]]);
      j = i;
   }
}


void draw_cell_wireframe(PortalCell *cell, uint color)
{
   int voff, i;

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(cell) ? R3_CLIP : R3_NO_CLIP);
   voff = 0;

   for (i=0; i < cell->num_polys; ++i) {
      draw_wireframe(&cell->poly_list[i], voff, color);
      voff += cell->poly_list[i].num_vertices;
   }
   r3_end_block();
}
#endif

////  A hacked system for mapping distance in water to a clut id

static water_clut[32] =
{
   0,0,1,1, 2,2,3,3, 4,4,5,5, 5,6,6,6,
   7,7,7,8, 8,8,9,9, 9,10,10,10, 10,11,11,11
};

int compute_water_clut(mxs_real water_start, mxs_real water_end)
{
#if 0
   int len;

   // compute the clut to use after passing through water
   // at distance water_start to water_end

   len = (water_end - water_start)*10.0;

   if (len > 255) len = 255;
   if (len < 0) len = 0;

   return water_clut[len >> 3];
#else
   // explicitly force the clut to an amount at a middling
   // distance under the old system
   return water_clut[64 >> 3];
#endif
}

static void draw_many_objects(void);

extern bool background_needs_clut;
extern uchar background_clut[];
extern bool g2pt_span_clip;

// preload lightmaps for a single region
void portal_preload_lightmaps(int cell)
{  PROF

   PortalCell *r;
   int n, voff;
   uint light_bitmask;
   PortalPolygonCore *poly;
   PortalPolygonRenderInfo *render;
   PortalLightMap *light;

   r = WR_CELL(cell);
   n = r->num_render_polys;
   light = r->light_list;

   if (r->num_full_bright || portal_render_from_texture || (n==0) || (light==NULL))
      return;

   voff=0;
   poly = r->poly_list;
   render = r->render_list;

   cur_ph = POINTS(r);
   cur_pool = r->vpool;
   cur_cell = r;
   cur_anim_light_index_list = r->anim_light_index_list;
   r_vertex_list = r->vertex_list;
   light_bitmask = r->changed_anim_light_bitmask;

   while (n--) {
      if (light->anim_light_bitmask & light_bitmask)
         porthw_uncache_lightmap(render);

      if (!(poly->flags & RENDER_DOESNT_LIGHT))
         if (check_surface_visible(r, poly, voff))
            porthw_preload_lightmap(render, light);

      voff += poly->num_vertices;
      ++poly;
      ++render;
      ++light;
   }
   r->changed_anim_light_bitmask = 0;
   END_PROF;
}

extern void draw_surface_lgd3d(
      PortalPolygonCore *poly,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt,
      int voff,
      void *clip);

BOOL portal_draw_lgd3d = TRUE;
BOOL portal_punt_draw_surface = FALSE;

static void check_and_draw_surface(
      PortalPolygonCore *poly,
      PortalPolygonRenderInfo *render,
      PortalLightMap *light,
      int voff,
      PortalCell *r)
{
   if ((!portal_punt_draw_surface) && check_surface_visible(r, poly, voff))
      if (g_lgd3d)
         draw_surface_lgd3d(poly, render, light, voff, CLIP_DATA(r));
      else
         draw_surface(poly, render, light, voff, CLIP_DATA(r));
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   draw a single cell

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
extern void uncache_surface(PortalPolygonRenderInfo *);
void draw_region(int cell)
{  PROF

   PortalCell *r = WR_CELL(cell);
   int n = r->num_render_polys;
   int voff=0;
   PortalPolygonCore *poly = r->poly_list;
   PortalPolygonRenderInfo *render = r->render_list;
   PortalLightMap *light = r->light_list;
   ClutChain temp;
   uchar clut;

   if (r->num_full_bright || portal_render_from_texture)
      light = NULL;  // disable lighting if any light_brights shining on

   // maybe i fixed this now, but it's a waste of time probably
   // if (!n && OBJECTS(r) == 0) { END_PROF; return; }

   // copy common data into globals for efficient communicating
   // someday we should inline the function "draw_surface" and
   // then get rid of these globals

   cur_ph = POINTS(r);
   cur_pool = r->vpool;
   cur_cell = r;
   cur_anim_light_index_list = r->anim_light_index_list;
   r_vertex_list = r->vertex_list;

   r_clip = CLIP_DATA(r);

   // if we end in water, then we have to do a clut for
   // this water which we haven't already added to our clut list
   // note that you probably need to read the clut tree document
   // to understand what's going on here.

   if (r->medium == 255)
      g2pt_clut = pt_clut_list[255]; // background?
   else {

      int mot = r->motion_index;
      clut = 0;

      if (mot > 0)
         clut = pt_motion_haze_clut[mot];

      if (!clut)
         clut = pt_medium_haze_clut[r->medium];

      if (clut) {
         temp.clut_id = clut + compute_water_clut(ZWATER(r), DIST(r));
         temp.clut_id2 = 0;
         // this is a bit bogus, we shoud really do per-poly not per-cell
         temp.next = CLUT(r).clut_id ? &CLUT(r) : 0;
         g2pt_clut = pt_get_clut(&temp);
      } else if (CLUT(r).clut_id) {
         g2pt_clut = pt_get_clut(&CLUT(r));
      } else {
         g2pt_clut = 0;
      }
   }

   // if span clipping, draw the objects first
   if (g2pt_span_clip && OBJECTS(r) >= 0)
      // at least one object
      draw_many_objects();

   if (!n && !(r->flags & CELL_OBSCURED)) goto skip_poly_draw;

#ifdef STATS_ON
   stat_num_poly_raw += n;
#endif

   // setup our default clip parameters (since we don't use
   // primitives, just the clipper, this isn't set automagically).
   // we could set it once elsewhere, and if we have self-lit polys
   // we might want to set it every poly.  setting it here makes
   // us interact safely with object rendering.
   r3_set_clip_flags(0);

#ifndef SHIP

   // The other polygon outline tools are handled in draw_surface().
   if (draw_solid_by_cell || draw_wireframe_around_poly) {
      polygon_cell_color = (((uint) (r->sphere_center.x * 85.12737321727
                                   - r->sphere_center.y * 123.33228937433)
                             ^ (uint)(r->sphere_center.z * 311.22029342383)
                             ^ r->num_portal_polys
                             ^ (r->num_render_polys << 8)
                             ^ (r->num_polys << 12)
                             ^ r->num_vertices) & 0xffffff) | 0x1000000;
   }

   if (draw_solid_by_cell_flags)
      _polygon_cell_flags_color = (r->flags) << 8;

#endif  // ~SHIP

   // now draw all the polygons

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);

   if (g_lgd3d) {
      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
      if (portal_fog_on)
         lgd3d_set_fog_enable(!!(CELL_FLAGS(r) & CELL_FOG));
   }

   if (light) {
      uint light_bitmask = r->changed_anim_light_bitmask;

      while (n--) {
         if (light->anim_light_bitmask & light_bitmask)
            uncache_surface(render);

         check_and_draw_surface(poly, render, light, voff, r);
         voff += poly->num_vertices;
         ++poly;
         ++render;
         ++light;
      }
      r->changed_anim_light_bitmask = 0;
   } else {
      while (n--) {
         check_and_draw_surface(poly, render, NULL, voff, r);
         voff += poly->num_vertices;
         ++poly;
         ++render;
      }
   }

   // In hardware, if we're not going to draw terrain here then we
   // need to set the z for this cell the old-fashioned way.
   if (g_lgd3d && (r->flags & CELL_OBSCURED)) {
      r3s_phandle vlist[MAX_VERT];
      int i, num_vertices;

      ulong color = 0;
      /*//zb
      if (r->flags & CELL_FOGGED_OUT) {
         color = fog_r3_color;
         r->flags &= ~CELL_FOGGED_OUT;
      }
      */
      r3_set_color(guiScreenColor(color));

      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_UNLIT | R3_PL_SOLID);

      voff = r->portal_vertex_list;
      poly = r->portal_poly_list;
      n = r->num_portal_polys;

      lgd3d_disable_palette();

      while (n--) {
         num_vertices = poly->num_vertices;

         if (check_surface_visible(r, poly, voff)) {
            for (i = 0; i < num_vertices; ++i)
               vlist[i] = &cur_ph[r_vertex_list[voff + i]];

            r3_draw_poly(num_vertices, vlist);
         }

         voff += num_vertices;
         ++poly;
      }

      lgd3d_enable_palette();
   }

   r3_end_block();

skip_poly_draw:

#ifndef SHIP
   if (r->flags & (CELL_RENDER_WIREFRAME | CELL_RENDER_WIREFRAME_ONCE)
    || draw_wireframe_around_poly) {
      draw_cell_wireframe(r, COLOR_WHITE);
      r->flags &= ~CELL_RENDER_WIREFRAME_ONCE;
   }
#endif // ~SHIP

   if (!g2pt_span_clip && OBJECTS(r) >= 0)
      draw_many_objects();

   if (r->flags & 128)
      portal_sfx_callback(cell);

   END_PROF;
}

extern void draw_surface_multitexture(
      PortalPolygonCore *poly,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt,
      int voff,
      void *clip);

extern void draw_surface_lightmap_only(
      PortalPolygonCore *poly,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt,
      int voff,
      void *clip);

extern void draw_surface_texture_only(
      PortalPolygonCore *poly,
      PortalPolygonRenderInfo *render,
      int voff,
      void *clip);

BOOL portal_multitexture = FALSE;
extern bool punt_hardware_lighting;

void draw_region_lgd3d(int cell)
{  PROF

   PortalCell *r = WR_CELL(cell);
   int n = r->num_render_polys;
   int voff=0;
   PortalPolygonCore *poly = r->poly_list;
   PortalPolygonRenderInfo *render = r->render_list;
   PortalLightMap *light = r->light_list;

   // copy common data into globals for efficient communicating
   // someday we should inline the function "draw_surface" and
   // then get rid of these globals

   cur_ph = POINTS(r);
   cur_pool = r->vpool;
   cur_cell = r;
   r_vertex_list = r->vertex_list;

   r_clip = CLIP_DATA(r);

   if (!n && !(r->flags & CELL_OBSCURED)) goto skip_poly_draw1;

#ifdef STATS_ON
   stat_num_poly_raw += n;
#endif

   // setup our default clip parameters (since we don't use
   // primitives, just the clipper, this isn't set automagically).
   // we could set it once elsewhere, and if we have self-lit polys
   // we might want to set it every poly.  setting it here makes
   // us interact safely with object rendering.
   r3_set_clip_flags(0);

#ifndef SHIP

   // The other polygon outline tools are handled in draw_surface().
   if (draw_solid_by_cell || draw_wireframe_around_poly) {
      polygon_cell_color = (((uint) (r->sphere_center.x * 85.12737321727
                                   - r->sphere_center.y * 123.33228937433)
                             ^ (uint)(r->sphere_center.z * 311.22029342383)
                             ^ r->num_portal_polys
                             ^ (r->num_render_polys << 8)
                             ^ (r->num_polys << 12)
                             ^ r->num_vertices) & 0xffffff) | 0x1000000;
   }

   if (draw_solid_by_cell_flags)
      _polygon_cell_flags_color = (r->flags) << 8;

#endif  // ~SHIP

   // now draw all the polygons

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);

   if (n)
   {
      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
      if (portal_fog_on)
         lgd3d_set_fog_enable(!!(CELL_FLAGS(r) & CELL_FOG));
      if (portal_multitexture)
      { // we're doing single pass, multi-texturing!
         while (n--)
         {
            if (check_surface_visible(r, poly, voff))
               draw_surface_multitexture(poly, render, light, voff, CLIP_DATA(r));

            voff += poly->num_vertices;
            ++poly;
            ++render;
            ++light;
         }
      } else
      { // two pass, texture first
         while (n--) {
            if ((!portal_punt_draw_surface) && check_surface_visible(r, poly, voff))
               draw_surface_texture_only(poly, render, voff, CLIP_DATA(r));
            voff += poly->num_vertices;
            ++poly;
            ++render;
         }

         // if we're not zbuffering, we need to do lightmaps immediately;
         // otherwise they get done in a completely seperate pass
         // (see draw_region_lightmap_only())
         if ((!g_zbuffer)&&(!punt_hardware_lighting))
         {
            n = r->num_render_polys;
            poly = r->poly_list;
            render = r->render_list;
            voff = 0;

            lgd3d_blend_multiply(portal_hack_blend);
            lgd3d_set_blend(TRUE);
#ifdef RGB_LIGHTING
            lgd3d_set_alpha(0.5);
#endif
            while (n--) {
               if (!(poly->flags & RENDER_DOESNT_LIGHT))
                  if (check_surface_visible(r, poly, voff))
                     draw_surface_lightmap_only(poly, render, light, voff, CLIP_DATA(r));

               voff += poly->num_vertices;
               ++poly;
               ++render;
               ++light;
            }
            lgd3d_blend_normal();
            lgd3d_set_blend(FALSE);
#ifdef RGB_LIGHTING
            lgd3d_set_alpha(1.0);
#endif
         }
      }
   }

   // In hardware, if we're not going to draw terrain here then we
   // need to set the z for this portal the old-fashioned way.
   if (g_lgd3d && (r->flags & CELL_OBSCURED)) {
      r3s_phandle vlist[MAX_VERT];
      int i, num_vertices;

      ulong color = 0;
      /*//zb
      if (r->flags & CELL_FOGGED_OUT) {
         color = fog_r3_color;
         r->flags &= ~CELL_FOGGED_OUT;
      }
      */
     // r3_set_color(guiScreenColor(color));
      r3_set_color( color );

      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_UNLIT | R3_PL_SOLID);

      voff = r->portal_vertex_list;
      poly = r->portal_poly_list;
      n = r->num_portal_polys;

      lgd3d_disable_palette();

      while (n--) {
         num_vertices = poly->num_vertices;

         if (check_surface_visible(r, poly, voff)) {
            for (i = 0; i < num_vertices; ++i)
               vlist[i] = &cur_ph[r_vertex_list[voff + i]];

            r3_draw_poly(num_vertices, vlist);
         }

         voff += num_vertices;
         ++poly;
      }

      lgd3d_enable_palette();
   }

   r3_end_block();

skip_poly_draw1:

#ifndef SHIP
   if (r->flags & (CELL_RENDER_WIREFRAME | CELL_RENDER_WIREFRAME_ONCE)
    || draw_wireframe_around_poly) {
      draw_cell_wireframe(r, COLOR_WHITE);
      r->flags &= ~CELL_RENDER_WIREFRAME_ONCE;
   }
#endif // ~SHIP

   if (OBJECTS(r) >= 0)
      draw_many_objects();

   if (r->flags & 128)
      portal_sfx_callback(cell);

   END_PROF;
}

void draw_region_lightmap_only(int cell)
{  PROF

   PortalCell *r = WR_CELL(cell);
   int n = r->num_render_polys;
   int voff=0;
   PortalPolygonCore *poly = r->poly_list;
   PortalPolygonRenderInfo *render = r->render_list;
   PortalLightMap *light = r->light_list;

   if (r->num_full_bright || portal_render_from_texture || (light==NULL))
      {END_PROF; return;}

   // copy common data into globals for efficient communicating
   // someday we should inline the function "draw_surface" and
   // then get rid of these globals

   cur_ph = POINTS(r);
   cur_pool = r->vpool;
   cur_cell = r;
   cur_anim_light_index_list = r->anim_light_index_list;
   r_vertex_list = r->vertex_list;

   r_clip = CLIP_DATA(r);

#ifdef STATS_ON
   stat_num_poly_raw += n;
#endif

   r3_set_clip_flags(0);

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);

   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
   if (portal_fog_on)
      lgd3d_set_fog_enable(!!(CELL_FLAGS(r) & CELL_FOG));

   while (n--) {
      if (!(poly->flags & RENDER_DOESNT_LIGHT))
         if (check_surface_visible(r, poly, voff))
            draw_surface_lightmap_only(poly, render, light, voff, CLIP_DATA(r));

      voff += poly->num_vertices;
      ++poly;
      ++render;
      ++light;
   }

   r3_end_block();


   END_PROF;
}


// can't be bigger than 64 due to sorting limitation!
// see sBlockedBits below!
#define MAX_SORTED_OBJS 64

Position* portal_object_pos_default(ObjID obj)
{
   static Position pos;
   return &pos;
}

Position* (*portal_object_pos)(ObjID obj) = portal_object_pos_default;

static int obj_compare(ObjVisibleID p, ObjVisibleID q)
{
   extern mxs_vector portal_camera_loc;
   ObjID x = vis_objs[p].obj;
   ObjID y = vis_objs[q].obj;

   // compute distance from camera

   float dist1, dist2;

   dist1 = mx_dist2_vec(&portal_camera_loc, &portal_object_pos(x)->loc.vec);
   dist2 = mx_dist2_vec(&portal_camera_loc, &portal_object_pos(y)->loc.vec);

   if (dist1 < dist2)
      return -1;
   else
      return dist1 > dist2;
}

typedef struct sBlockedBits
{
   ulong Bits[2];
} sBlockedBits;

// nByte !! yields 0 or 1:
// nSetBit is nBit shifted by 32 if nByte is 1, else not shifted
#define SetBlockedBit(bits, nBit) \
      nByte = !!(nBit>>32); \
      nSetBit = nBit>>(nByte<<5); \
      bits.Bits[nByte] |= nSetBit

// Assumes bit is set:
#define ResetBlockedBit(bits, nBit) \
      nByte = !!(nBit>>32); \
      nSetBit = nBit>>(nByte<<5); \
      bits.Bits[nByte] ^= nSetBit

// Just stuff a 0 there.
#define ZeroBlockedBit(bits, nBit) \
      nByte = !!(nBit>>32); \
      nSetBit = nBit>>(nByte<<5); \
      bits.Bits[nByte] &= ~nSetBit

// Returns last rvalue in sequence, and performs operations first to last:
#define IsBitSet(bits, nBit) \
      (nByte = !!(nBit>>32), \
       nSetBit = nBit>>(nByte<<5), \
       bits.Bits[nByte] & nSetBit)

void topological_sort(ObjVisibleID *obj_list, int n)
{
//   int x,y, i,j, b;
   int x,y, i,j;

   ulong nByte; // Helper for macros
   ulong nSetBit; // Helper for macros
   ulong nVal;
   sBlockedBits blocked[MAX_SORTED_OBJS];
   sBlockedBits b;

   // used to be this, which gave us at most 32 objects we could sort:
   // ulong blocked[MAX_SORTED_OBJS];

   ObjVisibleID my_list[MAX_SORTED_OBJS];

   // we should special case 2 (and maybe 3) objects!

   if (n > MAX_SORTED_OBJS) n = MAX_SORTED_OBJS;

   memcpy(my_list, obj_list, sizeof(my_list[0])*n);

   memset(blocked, 0, sizeof(sBlockedBits)*MAX_SORTED_OBJS);

   // Well, it turns out player is always in way of camera. Because we're hardware, we only
   // really need to sort objects that are alpha, yet this sorter sorts ALL objects, which is
   // unnecessary. We should move sorting into rendobj level, and only sort alpha objects.
   //

   // collect all n^2 comparison data
   for (x=0; x < n; ++x) {
      b.Bits[0] = b.Bits[1] = 0;
      for (y=0; y < n; ++y) {
         if (x != y && portal_object_blocks(vis_objs[obj_list[y]].obj,
                                  vis_objs[obj_list[x]].obj)) {
            // check if they form a cycle
//            if (y < x && (blocked[y] & (1 << x))) {
            if (y < x && IsBitSet(blocked[y], (1<<x))) {
               // they do, so they're too close to each other...
               // compare their centers:   dist-x > dist-y  ???
               if (obj_compare(obj_list[x],obj_list[y]) > 0) {
                  // y is closer, so no x blocks y
                  ResetBlockedBit(blocked[y],(1<<x));
                  SetBlockedBit(b, (1<<y));
//                  blocked[y] ^= 1 << x;
//                  b |= 1 << y;  // yes y blocks x
               }
               // else say x blocks y (already coded), and no y blocks x
            } else
               // no cycle, so y blocks x
               SetBlockedBit(b, (1<<y));
               // b |= 1 << y;
         }
      }
      blocked[x] = b;
   }

   // ok, now we know everything.  search for somebody who is
   // unblocked
   for (i=0; i < n; ++i) {
      // find guy #n
      for (j=0; j < n; ++j)
         if (my_list[j] != VISOBJ_NULL && !blocked[j].Bits[0] && !blocked[j].Bits[1])
//         if (my_list[j] != VISOBJ_NULL && !blocked[j])
            goto use_j;
      // nobody is unblocked... oops... must break cycle
      // we should use farthest guy, but let's just hack it
#ifndef SHIP
//      mprintf("Breaking object-sorting cycle.\n");
#endif
#ifndef SHIP
      for (j=0; j < n; ++j)
         if (my_list[j] != VISOBJ_NULL)
            goto use_j;
      Error(1, "Ran out of objects inside object sorter.");
#endif
     use_j:
      obj_list[i] = my_list[j];
      my_list[j] = VISOBJ_NULL;
      blocked[j].Bits[0] = blocked[j].Bits[1] = 0;
//      blocked[j] = 0;
      // unblock anybody this guy blocked
      nVal = 1 << j;
//      b = 1 << j;
      for (j=0; j < n; ++j)
         ZeroBlockedBit(blocked[j], nVal);
//         if (blocked[j] & b)
//            blocked[j] ^= b;
   }
}

extern bool obj_dealt[];  // HACK: need real object dealt flags
extern bool obj_hide[];   // HACK
void core_render_object(ObjVisibleID id, uchar *clut)
{
   if (!obj_hide[vis_objs[id].obj])
      portal_render_object(vis_objs[id].obj, clut, vis_objs[id].fragment);
   obj_dealt[vis_objs[id].obj] = 0;
}

extern long (*portal_get_time)(void);
extern int stat_num_object_ms;
bool disable_topsort;
static void draw_many_objects(void)
{
   PortalCell *r = cur_cell;
   ObjVisibleID id = OBJECTS(r);
   uchar *clut = g2pt_clut;
   ObjVisibleID obj_list[MAX_SORTED_OBJS];
   int num=0, i;

   if (portal_sky_spans > 0)
      ptsky_render_stars();

#ifdef STATS_ON
   stat_num_object_ms -= portal_get_time();
#endif

   // reset the polygon context so the fact we
   // stuffed the r3_clip_flags won't screw us up...
   // there must be a better way to do this...
   if (!g_lgd3d) {
      r3_set_polygon_context(0);

      // because portal goes behind r3d's back to access g2 directly,
      // we need to set this flag as well
      r3d_do_setup = TRUE;
   }

   // put the first MAX_SORTED_OBJS in an array

   while (id >= 0 && num < MAX_SORTED_OBJS) {
      if (!obj_hide[vis_objs[id].obj])
         obj_list[num++] = id;
      else
         obj_dealt[vis_objs[id].obj] = 0;
      id = vis_objs[id].next_visobj;
   }

   // if there are still more objects, just render 'em

   while (id >= 0) {
      Warning(("draw_many_objects: Too many objects to sort.\n"));
      core_render_object(id, clut);
      id = vis_objs[id].next_visobj;
   }

   // now sort and draw the remaining objects

   if (num == 1)
      core_render_object(obj_list[0], clut);
   else if (num) {

// wsf: if we're in hardware, don't need to sort all objects. We're moving sort
// into rendobj level, and only sorting alpha objects, that don't write into z-buffer.
      if (!disable_topsort && !g_lgd3d)
         topological_sort(obj_list, num);

      // the order of drawing should depend on render_back_to_front-ness
      for (i=num-1; i >= 0; --i)
         core_render_object(obj_list[i], clut);
   }

#ifdef STATS_ON
   stat_num_object_ms += portal_get_time();
#endif

   // restore g2pt_clut in case object rendering trashed it
   g2pt_clut = clut;
}

BOOL sphere_intersects_plane(mxs_vector *center, float radius, PortalPlane *p)
{
   // compute distance from sphere center to plane
   float dist = mx_dot_vec(center, &p->normal) + p->plane_constant;

   // if sphere is at least radius away, don't bother
   if (dist >= radius)
      return FALSE;

   // if sphere is at least radius _behind_ the plane, we don't need
   // to draw anything, but (a) that should never happen, and (b) we
   // don't have a distinct return value to indicate it, anyway

   return TRUE;
}

#define FLOAT_PTR_NEG(x)   (* (int *) (x) < -0.005)

BOOL bbox_intersects_plane(mxs_vector *bbox_min, mxs_vector *bbox_max,
          PortalPlane *p)
{
   mxs_vector temp;
   float dist;

   // find the point as far _behind_ the plane as possible

   if (FLOAT_PTR_NEG(&p->normal.x))
      temp.x = bbox_max->x;
   else
      temp.x = bbox_min->x;

   if (FLOAT_PTR_NEG(&p->normal.y))
      temp.y = bbox_max->y;
   else
      temp.y = bbox_min->y;

   if (FLOAT_PTR_NEG(&p->normal.z))
      temp.z = bbox_max->z;
   else
      temp.z = bbox_min->z;

   dist = mx_dot_vec(&temp, &p->normal) + p->plane_constant;
   if (dist >= 0)
      return FALSE;

   return TRUE;
}

static int num_pushed;

void portal_push_clip_planes(
     mxs_vector *bbox_min, mxs_vector *bbox_max,
     mxs_vector *sphere_center, float radius)
{
   int i,plane_count = cur_cell->num_planes;
   PortalPlane *pl = cur_cell->plane_list;

   num_pushed = 0;
   for (i=0; i < plane_count; ++i, ++pl) {
      mxs_plane p;
      if (sphere_center && !sphere_intersects_plane(sphere_center, radius, pl))
         continue;
      if (bbox_min && !bbox_intersects_plane(bbox_min, bbox_max, pl))
         continue;
      ++num_pushed;
      p.x = pl->normal.x;
      p.y = pl->normal.y;
      p.z = pl->normal.z;
      p.d = pl->plane_constant+0.002;
      r3_push_clip_plane(&p);
   }
}

void portal_pop_clip_planes(void)
{
   int i;
   for (i=0; i < num_pushed; ++i)
      r3_pop_clip_plane();
}


//////////       code that could probably be deleted      /////////

// (old palette-based lighting stuff)

static float rescale(float val, float *map)
{
   // find where 'val' is from 0..32
   int i;
   float where;

   if (val == 1.0) return 1.0;

   for (i=0; val >= map[i]; ++i);
   // ok, now val < map[i]

   --i;
   // map[i] <= val < map[i+1]

   // now determine where val occurs if it were linear interpolated
   //    map[i] + where * (map[j] - map[i]) == val

   where = (val - map[i]) / (map[i+1] - map[i]);
   return (where + i) / 32.0;
}


// is any of this used in the new regime?
uchar length_mapping[1024];
void init_portal_shading(int dark, int light)
{
#if 0
   int i, j;
   float last, step, cur, val;
   float map[33];

   // map 0 ->  dark + 0.5
   // map 128 ->  light + 0.5

   //    i/128 * (light - dark) + dark + 0.5

   // except we want to remap them to account for
   // nonlinearity; i/128 -> 0..1, but now we want
   // to deal with the fact that the output device
   // really takes x (0..1) and computes
   // a decaying series, 0.1 ^ (1-x), which outputs
   // from 0.1...1

   // Closed form this seems messy, so I'll use a lookup
   // table!

   // first compute the table which decays the way the
   // real thing decays

   cur = 1.0;
   // cur * step^32 == 0.1
   // step^32 == 0.1
   // step = (0.1)^1/32

   step = pow(0.1, 1.0/32);

   for (i=32; i >= 0; --i) {
         // rescale it from 0.1--1.0 into 0.0--1.0
      val = (cur-0.1)/0.9;
      map[i] = val;
      cur *= step;
   }
   map[32] = 1.0;
   map[0] = 0;

   for (i=0; i < 128; ++i)
      light_mapping[i] =
         (rescale((float) i / 128.0, map) * (light - dark)
            + dark + 0.5) * 65536;

   last = light_mapping[i-1];
   for (   ; i < 256; ++i)
      light_mapping[i] = last;

   for (i=0; i < 256; ++i)
      light_mapping_dithered[i] = light_mapping[i]*2;

   for (i=1; i <= 16; ++i)
      length_mapping[i] = i-1;

   j = 3;
   for (; i < 1024; ++i) {
      if (i >= (4 << j)) ++j;
      if (i >= (3 << j)) length_mapping[i] = (j+5)*2+1;
      else length_mapping[i] = (j+5)*2;
   }
#endif
}

///// find the clipping data for a portal /////
// returns TRUE if poly non empty
// we save away info about the final polygon shape
//   in case we end up needing more info; e.g. the
//   water comes back and checks out the average z depth

static r3s_phandle port_vr[MAX_VERT], *port_p;
static int port_n;

ClipData *PortalGetClipInfo(PortalCell *cell, PortalPolygonCore *poly,
                            int voff, void *clip)
{
   int i, n = poly->num_vertices;
   uchar *vlist = cell->vertex_list + voff;

   if (n > MAX_VERT)
      Error(1, "PortalGetClipInfo: Portal has too many vertices.\n");

   for (i=0; i < n; ++i)
      port_vr[i] = &cur_ph[*vlist++];

   n = r3_clip_polygon(n, port_vr, &port_p);
   if (n <= 2)
      return NULL;

   port_n = n;

   return PortalClipFromPolygon(n, port_p, clip);
}

mxs_real compute_portal_z(void)
{
   mxs_real z=0;
   int i;

   for (i=0; i < port_n; ++i)
      z += port_p[i]->p.z;

     // compute the average
   z /= port_n;

   return (z > 0.1 ? z : 0.1);
}
