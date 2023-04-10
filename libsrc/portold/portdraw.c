/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/libsrc/portold/portdraw.c,v 1.70 1998/04/21 15:36:07 KEVIN Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <stdlib.h>
#include <math.h>

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

#include <profile.h>

#ifdef DBG_ON
  #define STATIC
#else
  #define STATIC static
#endif

extern mxs_vector portal_camera_loc;

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

// Here's how large the texels of a polygon can be and still be drawn
// using a given MIP level.  Entry 0 is full detail.
mxs_real _portal_MIP_threshold[5];

// We can fiddle this up and down to vary the level of detail.
// Higher means more texels.  Right now it only affects MIP mapping.
// The expected range is 0.8-1.5; the default setting is just what looks
// good to me without clobbering the frame rate.
float portal_detail_level = 1.10;

float dot_clamp=0.6;
#define VISOBJ_NULL      (-1)


// If this is on, we blow off the usual rendering and use these flags.
// Each face is a solid, flat polygons, with a white wireframe outline.
#ifndef SHIP
   bool draw_solid_by_MIP_level = FALSE;
   bool draw_solid_by_cell = FALSE;
   bool draw_solid_wireframe = FALSE;
   bool draw_solid_by_poly_flags = FALSE;
   bool draw_solid_by_cell_flags = FALSE;
   bool draw_wireframe_around_tmap = FALSE;
   bool draw_wireframe_around_poly = FALSE;
   uchar polygon_cell_color;
   uchar _polygon_cell_flags_color;

   #define COLOR_WHITE 1
#endif // ~SHIP


  // use watcom inline assembler?
#define USE_ASM

  // are we using 2d rendering primitives instead of PT ones?
//#define USE_2D

r3s_point   *cur_ph;    // list of pointers to r3s_points for current pool
Vector      *cur_pool;  // list of untransformed vectors
PortalCell  *cur_cell;
ushort      *cur_anim_light_index_list;
extern int   cur_cell_num;

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
   float dist = mx_dot_vec(&portal_camera_loc, &p->norm->raw)
              + p->plane_constant;

   STAT(++stat_num_backface_tests;)
   return dist > 0;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We determine the MIP level of a polygon based the scaling of its
   texture divided by the distance from its center to the camera.
   This routine finds the ratios up to which the different MIP levels
   are used into _portal_MIP_threshold[].

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static double portal_detail_2, dot_clamp_2;
static double pixel_scale, pixel_scale_2;
static double premul, premul2; // premul2 != premul*premul, not named premul_2
void PortalSetMIPTable(void)
{
   portal_detail_2 = portal_detail_level * portal_detail_level;
   pixel_scale = grd_bm.w / 128.0;
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

STATIC
mxs_vector *get_cached_vector(mxs_vector *where, CachedVector *v)
{  PROF
   r3_rotate_o2c(where, &v->raw);
   END_PROF;
   return where;   // should copy it into cache
}

  // our 3x3 texture perspective correction matrix

  // tables mapping lighting values from 0..255 (which is how
  // we store them in polygons) into floating point 'i' values
  // appropriate for clipping.
float light_mapping[256];
float light_mapping_dithered[256];
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
void draw_polygon_wireframe(r3s_phandle *points, int num_points, uchar color)
{
   int p1, p2;

   r3_set_color(color);
   r3_set_line_context(R3_LN_FLAT);

   p2 = num_points - 1;
   for (p1 = 0; p1 < num_points; p1++) {
      r3_draw_line(points[p1], points[p2]);
      p2 = p1;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we're showing the polygons in the visualization tools, each
   visible face is a flat, unshaded polygon with a wireframe border.

   The context constants come from x:\prj\tech\libsrc\r3d\prim.h.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void draw_polygon_outline(PortalPolygonCore *poly, r3s_phandle *points,
                          uchar polygon_color, uchar outline_color)
{
   int num_points = poly->num_vertices;

   // Here's our polygon.
   r3_set_clipmode(R3_CLIP);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_UNLIT | R3_PL_SOLID);
   r3_set_color(polygon_color);

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
      int light_level = 256;
      for (i = MIP_level; i < 4; i++)
         light_level += 512;
      draw_polygon_outline(poly, points,
                           grd_light_table[COLOR_WHITE + light_level],
                           grd_light_table[COLOR_WHITE + light_level + 512]);
      return TRUE;
   }

   if (draw_solid_by_cell) {
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


extern BOOL g_lgd3d;

// this is to set up the sky hack under lgd3d
static int sky_id=-1;
static float w_min;
static float z_max;
void portal_setup_star_hack(int tex_id)
{
   sky_id = tex_id;
}

void portal_set_znearfar(double z_near, double z_far)
{
   w_min = 1.0/z_far;
   z_max = z_far;
}


// The following code is the interface to do two pass
// terrain rendering under lgd3d

typedef struct scale_info {
   double u0, v0, scale_u, scale_v;
} scale_info;

extern int portal_clip_num;

STATIC
void ptlgd3d_calc_uv(int n, r3s_phandle *vlist, scale_info *info)
{
   int i;
   for (i=0; i < n; ++i) {
      double sx = vlist[i]->grp.sx / 65536.0;
      double sy = vlist[i]->grp.sy / 65536.0;
      double c = (g2pt_tmap_data[2] + g2pt_tmap_data[5]*sx + g2pt_tmap_data[8]*sy);
      double ic = 1/c;

      double u = g2pt_tmap_data[0]+g2pt_tmap_data[3]*sx + g2pt_tmap_data[6]*sy;
      double v = g2pt_tmap_data[1]+g2pt_tmap_data[4]*sx + g2pt_tmap_data[7]*sy;
      vlist[i]->grp.u = (u*ic/65536) * info->scale_u + info->u0;
      vlist[i]->grp.v = (v*ic/65536) * info->scale_v + info->v0;
      vlist[i]->grp.i = 1.0;
      // if background hack, fixup w values to match texture not skypoly
   }
}

STATIC
void ptlgd3d_recalc_uv(int n, r3s_phandle *vlist, scale_info *info)
{
   int i;
   for (i=0; i < n; ++i) {
      vlist[i]->grp.u += info->u0;
      vlist[i]->grp.v += info->v0;
      vlist[i]->grp.u *= info->scale_u;
      vlist[i]->grp.v *= info->scale_v;
   }
}


ushort *pt_alpha_pal=NULL;
static ushort *water_pal=NULL;
static int water_id = -1;
static short water_argb=0;

void portal_setup_water_hack(int id, float alpha, int *rgb)
{
   water_argb = 0xf * alpha;
   water_argb = (water_argb<<12) + ((rgb[0]>>4)<<8) + ((rgb[1]>>4)<<4) + (rgb[2]>>4);
   water_id = id;
}

static void init_water_pal(void)
{
   int i;
   water_pal = (ushort *)Malloc(512);
   water_pal[0] = water_argb;
   for (i=1; i<256; i++) {
      water_pal[i] = 0xf000 + ((grd_pal[3*i]>>4)<<8) +
                        ((grd_pal[3*i+1]>>4)<<4) +
                        (grd_pal[3*i+2]>>4);
   }
}

static void init_alpha_pal(void)
{
   int i;
   ulong *pal = (ulong *)Malloc(512);

   pt_alpha_pal = (ushort *)pal;
   pal = &pal[127];
   for (i = 0; i<128; i++)
      *(pal--) = 0x10001000 * (i>>3);
}

// this is quite hateful.

#define MAX_WATER_POINTS 100
#define MAX_WATER_POLYS 25
static r3s_point water_points[MAX_WATER_POINTS];
static int water_polys[MAX_WATER_POLYS];
static r3s_texture water_textures[MAX_WATER_POLYS];
static int num_water_points=0;
extern BOOL g_zbuffer;

int num_water_polys=0;

static void queue_water_poly(r3s_texture tex, int n, r3s_phandle *vlist)
{
   int i;
   r3s_point *dest;

   if (num_water_polys >= MAX_WATER_POLYS) {
      Warning(("Too many water polys! increase MAX_WATER_POLYS\n"));
      return;
   }
   if (num_water_points + n > MAX_WATER_POINTS) {
      Warning(("Too many water points! increase MAX_WATER_POINTS\n"));
      return;
   }

   water_textures[num_water_polys] = tex;
   water_polys[num_water_polys] = n;

   dest = &water_points[num_water_points];
   for (i=0; i<n; i++)
      *dest++ = *(vlist[i]);

   num_water_points += n;
   num_water_polys++;
}

static void render_water_poly(r3s_texture tex, int n, r3s_phandle *vlist)
{
   lgd3d_set_alpha_pal(water_pal);
   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_blend(TRUE);
   r3_set_texture(tex);
   r3_draw_poly(n, vlist);
   lgd3d_set_blend(FALSE);
   gr_set_fill_type(FILL_NORM);
}

void portal_render_water_polys(void)
{
   int i;
   r3s_point *next_poly = water_points;

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);

   lgd3d_set_alpha_pal(water_pal);
   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_blend(TRUE);
   lgd3d_set_zwrite(FALSE);

   for (i=0; i < num_water_polys; i++) {
      int j, n = water_polys[i];
      r3s_texture tex = water_textures[i];
      r3s_phandle *vlist = (r3s_phandle *)temp_malloc(n * sizeof(r3s_phandle));

      for (j=0; j<n; j++)
         vlist[j] = &next_poly[j];
      next_poly += n;

      tex->flags &= ~BMF_TRANS;
      r3_set_texture(tex);
      r3_draw_poly(n, vlist);
      temp_free(vlist);
      tex->flags |= BMF_TRANS;
   }

   lgd3d_set_zwrite(TRUE);
   lgd3d_set_blend(FALSE);
   gr_set_fill_type(FILL_NORM);

   r3_end_block();
   num_water_polys = 0;
   num_water_points = 0;
}


static int sky_spans = 0;

STATIC
void ptlgd3d_draw_perspective(hw_render_info *hw, int n, r3s_phandle *vlist, PortalLightMap *lt)
{
   scale_info info;
   double pix_per_lm;

   if (hw->flags & HWRIF_SKY)
   {
      if (ptsky_type == PTSKY_SPAN)
         sky_spans += ptsky_calc_spans(n, vlist);
      else if (ptsky_type == PTSKY_NONE)
         return;
   }
   else if (sky_spans > 0)
   {
      ptsky_render_stars();
      sky_spans = 0;
   }

   // first render texture...

   info.scale_u = 1.0/hw->tex->w;
   info.scale_v = 1.0/hw->tex->h;

   if (hw->flags & HWRIF_NOT_LIGHT) {
      info.u0 = info.v0 = 0.0;
   } else {
      double scale = 1.0 / (1<<hw->mip_level);
      info.u0 = scale * info.scale_u * (lt->base_u << 4);
      info.v0 = scale * info.scale_v * (lt->base_v << 4);
   }

   ptlgd3d_calc_uv(n, vlist, &info);

   if (hw->flags & HWRIF_WATER) {
      if (water_pal == NULL)
         init_water_pal();
      if (g_zbuffer)
         queue_water_poly(hw->tex, n, vlist);
      else
         render_water_poly(hw->tex, n, vlist);
      return;
   }

   r3_set_texture(hw->tex);

   if ((hw->flags & HWRIF_SKY) && (ptsky_type == PTSKY_ZBUFFER)) {
      int i;
      // render it real far away, so stars will render in front

      float *zw_save;
      zw_save = (float *)temp_malloc(2*n*sizeof(float));
      for (i=0; i<n; i++) {
         zw_save[i] = vlist[i]->p.z;
         zw_save[i+n] = vlist[i]->grp.w;
         vlist[i]->p.z = z_max;
         vlist[i]->grp.w = w_min;
      }
      r3_draw_poly(n, vlist);
      for (i=0; i<n; i++) {
         vlist[i]->p.z =   zw_save[i];
         vlist[i]->grp.w = zw_save[i+n];
      }
      temp_free(zw_save);
   }
   else 
      r3_draw_poly(n, vlist);

   // is there a lightmap?

   if (hw->lm == NULL)
      return;

   // render it...

   pix_per_lm = 64 >> (hw->mip_level +2);

   info.scale_u = 1.0 / (info.scale_u * pix_per_lm * hw->lm->w);
   info.scale_v = 1.0 / (info.scale_v * pix_per_lm * hw->lm->h);
   info.u0 = (hw->lm_u0 / (hw->lm->w * info.scale_u)) - info.u0;
   info.v0 = (hw->lm_v0 / (hw->lm->h * info.scale_v)) - info.v0;

   ptlgd3d_recalc_uv(n, vlist, &info);

   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_blend(TRUE);

   if (pt_alpha_pal == NULL)
      init_alpha_pal();
   lgd3d_set_alpha_pal(pt_alpha_pal);

   r3_set_texture(hw->lm);
   r3_draw_poly(n, vlist);
   lgd3d_set_blend(FALSE);
   gr_set_fill_type(FILL_NORM);
}

STATIC
void do_poly_linear(r3s_texture tex, int n, r3s_phandle *vpl, fix u_offset, fix v_offset)
{
   scale_info info;

   info.scale_u = 1.0 / tex->w;
   info.scale_v = 1.0 / tex->h;

   info.u0 = u_offset * info.scale_u / 65536.0;
   info.v0 = v_offset * info.scale_v / 65536.0;

   ptlgd3d_calc_uv(n, vpl, &info);
   g2_lin_umap_setup(tex);
   {
      static g2s_point *ppl[20];
      int i;

      for (i=0; i<n; i++)
         ppl[i] = (g2s_point *)&(vpl[i]->grp);

      g2_draw_poly_func(n, ppl);
   }
}

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

   get_cached_vector(&u_vec, render->u);
   get_cached_vector(&v_vec, render->v);
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
   b = mx_dot_vec(&eye, &p->norm->raw);

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

STATIC
int compute_mip(PortalPolygonRenderInfo *render, PortalPlane *p)
{
   double sz,dist,k;
   mxs_vector eye;

   mx_sub_vec(&eye, &portal_camera_loc, &render->center);
   dist = mx_mag_vec(&eye);

   k = mx_dot_vec(&eye, &p->norm->raw);  // k/dist == foreshortening amount

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

static void draw_background_hack(int n, r3s_phandle *vlist)
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
   if (!texture) {
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

   not_light = poly->flags & RENDER_DOESNT_LIGHT;

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

   if (portal_clip_num) {
      n3 = portbg_clip_sky(n3, final, &final);
   }

   desired_mip = compute_mip(render, &cur_cell->plane_list[poly->planeid]);
   mip_level = 0;
   while ((desired_mip > 0) || (texture->w > 128)) {
      if ((!g_lgd3d) && (desired_mip==0))
         break;
      if (texture[1].w == 0) break; // not enough mip levels
      ++mip_level;
      --desired_mip;
      ++texture;
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
   sc = 64 >> mip_level; // texture->w;

   for(i=0; i < 3; ++i) {
      g2pt_tmap_data[3*i  ] *= sc;
      g2pt_tmap_data[3*i+1] *= sc;
   }

   if (g_lgd3d)
   {
      hw_render_info hw;
      hw.tex = texture;
      hw.mip_level = mip_level;
      hw.flags = (not_light ? HWRIF_NOT_LIGHT : 0);
      if (render->texture_id == sky_id)
         hw.flags |= HWRIF_SKY;

// Sadly, this just doesn't work
//      else if (render->texture_id == water_id)
// Instead we brutally hack:
      if (texture->flags & BMF_TRANS)
         hw.flags |= HWRIF_WATER;

      if ((portal_render_from_texture && !position_from_motion) || not_light)
         hw.lm = NULL;
      else
         porthw_get_cached_lightmap(&hw, render, lt);

      ptlgd3d_draw_perspective(&hw, n3, final, lt);
   } else {
      // bug: if not lighting, don't deref 'lt'!!!
      if (not_light) {
         corner_u_offset = corner_v_offset = 0;
         tex = texture;
      } else if (portal_render_from_texture && !position_from_motion) {
         // We shift by 20 places because we're both converting from int
         // to fix and scaling the u and v of the lightmap base to the
         // 16x16 block size of our largest MIP level.
         corner_u_offset = (lt->base_u << 20) >> mip_level;
         corner_v_offset = (lt->base_v << 20) >> mip_level;
         tex = texture;
#ifdef AGGREGATE_LIGHTMAPS_IN_SOFTWARE
      // this is just so we can run the lightmap aggregation code in the debugger.
      // if this is enabled, lighting will actually be turned off altogether
      } else if (pt_aggregate_lightmaps) {
         hw_render_info hw;
         porthw_get_cached_lightmap(&hw, render, lt);
         corner_u_offset = corner_v_offset = 0;
         tex = texture;
#endif
      } else {
         corner_u_offset = corner_v_offset = fix_make(2, 0);
         // go get it from surface cache or light it or whatever
         tex = get_cached_surface(render, lt, texture, mip_level);
      }

      if (linear_map)
         do_poly_linear(tex, n3, final, corner_u_offset, corner_v_offset);
      else
         g2pt_poly_perspective_uv(tex, n3, final, 
                                 corner_u_offset, corner_v_offset, FALSE);
   }

#ifndef SHIP
#if 0
   if (draw_wireframe_around_tmap)
      draw_polygon_wireframe(vlist, poly->num_vertices, COLOR_WHITE);
#else
   if (draw_wireframe_around_tmap)
      draw_polygon_wireframe(final, n3, COLOR_WHITE);
#endif
#endif // ~SHIP

   END_PROF;
   return TRUE;
}


void draw_wireframe(PortalPolygonCore *p, int voff, uchar color)
{
   int i,j,n;
   n = p->num_vertices;

   r3_set_color(color);
   j = n-1;
   for (i=0; i < n; ++i) {
      r3_draw_line(&cur_ph[r_vertex_list[voff+i]],
                   &cur_ph[r_vertex_list[voff+j]]);
      j = i;
   }
}


void draw_cell_wireframe(PortalCell *cell, uchar color)
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


////  A hacked system for mapping distance in water to a clut id

static water_clut[32] =
{
   0,0,1,1, 2,2,3,3, 4,4,5,5, 5,6,6,6,
   7,7,7,8, 8,8,9,9, 9,10,10,10, 10,11,11,11
};

int compute_water_clut(mxs_real water_start, mxs_real water_end)
{
   int len;

   // compute the clut to use after passing through water
   // at distance water_start to water_end

   len = (water_end - water_start)*10.0;

   if (len > 255) len = 255;
   if (len < 0) len = 0;

   return water_clut[len >> 3];
}

static void draw_many_objects(void);

extern bool background_needs_clut;
extern uchar background_clut[];
extern bool g2pt_span_clip;

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

   // if we've computed dynamic lighting, use that, otherwise
   // go ahead and use the static lighting
#if 0
   if (r->vertex_list_dynamic)
      r_vertex_lighting = r->vertex_list_dynamic;
   else
      r_vertex_lighting = r->vertex_list_lighting;
#endif

   r_clip = CLIP_DATA(r);

   // if we end in water, then we have to do a clut for
   // this water which we haven't already added to our clut list
   // note that you probably need to read the clut tree document
   // to understand what's going on here.

   if (r->medium == 255)
      g2pt_clut = pt_clut_list[255]; // background?
   else {
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

   if (!n) goto skip_poly_draw;

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
      polygon_cell_color = ((uchar) (r->sphere_center.x * 5.12737321727
                                   - r->sphere_center.y * 3.33228937433)
                          ^ (uchar) (r->sphere_center.z * 11.22029342383)
                          ^ r->num_portal_polys
                          ^ (r->num_render_polys << 2)
                          ^ (r->num_polys << 3)
                          ^ r->num_vertices);
      // A white polygon doesn't look too good with white outlines.
      // On top of that, the upper part of the palette has reserved chunks
      // of solid blue, and it's all the same solid blue.
      if (polygon_cell_color == COLOR_WHITE
       || polygon_cell_color > 224)
         polygon_cell_color += (uchar) (r->sphere_radius * 7.3772347112);
   }

   // We don't draw the flags by their actual color because they're in
   // a dim part of the palette in Dark.
   if (draw_solid_by_cell_flags)
      _polygon_cell_flags_color = (r->flags) ^ 57;

#endif  // ~SHIP

   // now draw all the polygons

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);

   if (g_lgd3d)
      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);

   if (light) {
      uint light_bitmask = r->changed_anim_light_bitmask;

      while (n--) {
         if (check_surface_visible(r, poly, voff)) {

            if (light->anim_light_bitmask & light_bitmask)
               uncache_surface(render);

            if (!draw_surface(poly, render, light, voff, CLIP_DATA(r)))
#if 0
               if (poly < r->portal_poly_list)
                  // we drew an opaque wall transparent.  must be background
                  if (g2pt_clut)
                     if (!background_needs_clut) {
                        memcpy(background_clut, g2pt_clut, 256);
                        background_needs_clut = TRUE;
                     }
#else
               ;
#endif
         }
         voff += poly->num_vertices;
         ++poly;
         ++render;
         ++light;
      }
      r->changed_anim_light_bitmask = 0;
   } else {
      while (n--) {
         if (check_surface_visible(r, poly, voff))
            if (!draw_surface(poly, render, 0, voff, CLIP_DATA(r)))
               if (poly < r->portal_poly_list)
                  if (g2pt_clut)
                     if (!background_needs_clut) {
                        memcpy(background_clut, g2pt_clut, 256);
                        background_needs_clut = TRUE;
                     }
         voff += poly->num_vertices;
         ++poly;
         ++render;
      }
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

// can't be bigger than 32 due to bitfields!
#define MAX_SORTED_OBJS 32

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


void topological_sort(ObjVisibleID *obj_list, int n)
{
   int x,y, i,j, b;
   ulong blocked[MAX_SORTED_OBJS];
   ObjVisibleID my_list[MAX_SORTED_OBJS];

   // we should special case 2 (and maybe 3) objects!

   if (n > MAX_SORTED_OBJS) n = MAX_SORTED_OBJS;

   memcpy(my_list, obj_list, sizeof(my_list[0])*n);

   // collect all n^2 comparison data
   for (x=0; x < n; ++x) {
      b = 0;
      for (y=0; y < n; ++y) {
         if (x != y && portal_object_blocks(vis_objs[obj_list[y]].obj,
                                  vis_objs[obj_list[x]].obj)) {
            // check if they form a cycle
            if (y < x && (blocked[y] & (1 << x))) {
               // they do, so they're too close to each other...
               // compare their centers:   dist-x > dist-y  ???
               if (obj_compare(obj_list[x],obj_list[y]) > 0) {
                  // y is closer, so no x blocks y
                  blocked[y] ^= 1 << x;
                  b |= 1 << y;  // yes y blocks x
               }
               // else say x blocks y (already coded), and no y blocks x
            } else
               // no cycle, so y blocks x
               b |= 1 << y;
         }
      }
      blocked[x] = b;
   }

   // ok, now we know everything.  search for somebody who is
   // unblocked
   for (i=0; i < n; ++i) {
      // find guy #n
      for (j=0; j < n; ++j)
         if (my_list[j] != VISOBJ_NULL && !blocked[j])
            goto use_j;
      // nobody is unblocked... oops... must break cycle
      // we should use farthest guy, but let's just hack it
#ifndef SHIP
      mprintf("Breaking object-sorting cycle.\n");
#endif
      for (j=0; j < n; ++j)
         if (my_list[j] != VISOBJ_NULL)
            goto use_j;
      Error(1, "Ran out of objects inside object sorter.");
     use_j:
      obj_list[i] = my_list[j];
      my_list[j] = VISOBJ_NULL;
      blocked[j] = 0;
      // unblock anybody this guy blocked
      b = 1 << j;
      for (j=0; j < n; ++j)
         if (blocked[j] & b)
            blocked[j] ^= b;
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

   if (sky_spans > 0)
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
      obj_list[num++] = id;
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
      if (!disable_topsort)
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
   float dist = mx_dot_vec(center, &p->norm->raw) + p->plane_constant;

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

   if (FLOAT_PTR_NEG(&p->norm->raw.x))
      temp.x = bbox_max->x;
   else
      temp.x = bbox_min->x;

   if (FLOAT_PTR_NEG(&p->norm->raw.y))
      temp.y = bbox_max->y;
   else
      temp.y = bbox_min->y;

   if (FLOAT_PTR_NEG(&p->norm->raw.z))
      temp.z = bbox_max->z;
   else
      temp.z = bbox_min->z;

   dist = mx_dot_vec(&temp, &p->norm->raw) + p->plane_constant;
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
      p.x = pl->norm->raw.x;
      p.y = pl->norm->raw.y;
      p.z = pl->norm->raw.z;
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
