/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/ptdrawhw.c,v 1.17 2000/02/19 13:18:43 toml Exp $

#include <math.h>

#include <g2pt.h>
#include <r3d.h>
#include <lgd3d.h>
#include <matrix.h>
#include <memall.h>
#include <tmpalloc.h>

#include <palmgr.h>
#include <portwatr.h>
#include <portal_.h>
#include <portclip.h>
#include <porthw.h>
#include <portsky.h>
#include <wrdbrend.h>

#include <profile.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define STATS_ON

#ifdef STATS_ON
  #define STAT(x)     x
#else
  #define STAT(x)
#endif

#define COLOR_WHITE 0x1ffffff

#define MAX_VERT 32

#ifdef STATS_ON
extern int stat_num_poly_drawn;
extern int stat_num_poly_raw;
extern int stat_num_poly_considered;
extern int stat_num_backface_tests;
#endif

int portal_sky_spans = 0;

int portal_hack_blend = BLEND_SRC_DEST;

BOOL pt_lightmap_first = FALSE;

mxs_real two_to_n_minus_fifteen[31] =
{
   1.0/32768.0,
   1.0/16384.0,
   1.0/8192.0,
   1.0/4096.0,
   1.0/2048.0,
   1.0/1024.0,
   1.0/512.0,
   1.0/256.0,
   1.0/128.0,
   0.015625,
   0.03125,
   0.0625,
   0.125,
   0.25,
   0.5,
   1.0,
   2.0,
   4.0,
   8.0,
   16.0,
   32.0,
   64.0,
   128.0,
   256.0,
   512.0,
   1024.0,
   2048.0,
   4096.0,
   8192.0,
   16384.0,
   32768.0,
};

#ifdef DBG
static mxs_real two_to_n(int n)
{
   AssertMsg1((n>=-12) && (n<=12), "two_to_n(): %i is out of range [-12 to 12]", n);
   return two_to_n_minus_fifteen[n+15];
}
#else
#define two_to_n(n) (two_to_n_minus_fifteen[(n)+15])
#endif

extern BOOL g_zbuffer;
extern int max_draw_polys;
extern bool show_lightmap;
extern bool portal_clip_poly;
extern int portal_clip_num;
extern PortalCell *cur_cell;

extern int portbg_clip_sky(int, r3s_phandle *, r3s_phandle **);
extern void draw_polygon_wireframe(r3s_phandle *points, int num_points, uint color);
extern void draw_polygon_vertices(r3s_phandle *points, int num_points, uint color);
extern void draw_background_hack();
extern int compute_mip(PortalPolygonRenderInfo *render, PortalPlane *p);
extern bool poly_outline_by_flags(PortalPolygonCore *poly, r3s_phandle *points,
                           int MIP_level);
extern mxs_vector *get_cached_vector(mxs_vector *where, mxs_vector *v);

typedef struct scale_info {
   double u0, v0, scale_u, scale_v;
} scale_info;



// this is to set up the sky hack under lgd3d
int portal_sky_id=-1;
static float portal_w_min;
static float portal_z_max;
void portal_setup_star_hack(int tex_id)
{
   portal_sky_id = tex_id;
}

void portal_set_znearfar(double z_near, double z_far)
{
   portal_w_min = 1.0/z_far;
   portal_z_max = z_far;
}



static void ptlgd3d_calc_uv(int n, r3s_phandle *vlist, scale_info *info)
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

static void ptlgd3d_recalc_uv_vlist(int n, r3s_phandle *vlist, scale_info *info)
{
   int i;
   for (i=0; i < n; ++i) {
      vlist[i]->grp.u += info->u0;
      vlist[i]->grp.v += info->v0;
      vlist[i]->grp.u *= info->scale_u;
      vlist[i]->grp.v *= info->scale_v;
   }
}

static void ptlgd3d_recalc_uv(int n, LGD3D_tex_coord *uvs, scale_info *info)
{
   int i;
   for (i=0; i < n; ++i) {
      uvs[i].u += info->u0;
      uvs[i].v += info->v0;
      uvs[i].u *= info->scale_u;
      uvs[i].v *= info->scale_v;
   }
}

typedef struct water_info {
   ushort *pal;
   r3s_texture tex;
   uchar pal_index;
   uchar pad;
   short w;
} water_info;


static water_info *water_info_list=NULL;
static int num_water_textures = 0;

static void init_water_pal(water_info *info, ushort alpha, ushort argb, uchar *pal24)
{
   int i;
   ushort *pal = info->pal;

   if (pal == NULL) {
      pal = (ushort *)Malloc(512);
      info->pal = pal;
   }

   pal[0] = argb;
   for (i=1; i<256; i++)
      pal[i] = alpha + ((pal24[3*i]>>4)<<8) +
                        ((pal24[3*i+1]>>4)<<4) +
                        (pal24[3*i+2]>>4);
}

ushort pt_default_water_texture_flags = BMF_TRANS;

/////////////////////////////////////////////////////////////////////////////
// Setup water hack. Must be called before water can be rendered in hardware.
// num_textures = number of water textures;
// tex_list = pointer to array of pointers to water textures;
// alpha_list = pointer to array of alpha values for each texture
// rgb_list = pointer to array of rgb values for each texture
//
// Before any water texture can be released, portal_cleanup_water_hack() must be called.
// Any call to portal_setup_water_hack() effectively calls portal_cleanup_water_hack() first.

EXTERN BOOL g_lgd3d;

void portal_setup_water_hack(int num_textures, r3s_texture *tex_list, float *alpha_list, int *rgb_list)
{
   int i;

   if (water_info_list != NULL) {
      for (i=0; i<num_water_textures; i++) {
         water_info *info = &water_info_list[i];
         r3s_texture tex = info->tex;
         if (info->pal != NULL)
         {
            Free(info->pal);
            info->pal = NULL;
         }
         if (tex != NULL)
         {
            AssertMsg(tex->w == info->w,
               "Water info out of synch!\n Probable cause: freed water texture in water info list.");
            while (tex->w > 0) {
               tex->align = info->pal_index;
               tex++;
            }
         }
      }
      Free(water_info_list);
      water_info_list = NULL;
   }

   num_water_textures = num_textures;

   if ((num_water_textures <= 0)||(!g_lgd3d))
      return;

   water_info_list = Calloc(num_water_textures * sizeof(*water_info_list));

   for (i = 0; i<num_water_textures; i++) {
      r3s_texture tex;
      short alpha;
      short argb;
      ushort flags;
      uchar *pal24;
      water_info *info=&water_info_list[i];

      if (tex_list != NULL) {
         tex = tex_list[i];
         info->tex = tex;
         info->pal_index = tex->align;
         info->w = tex->w;
         pal24 = palmgr_get_pal(tex->align);
         flags = tex->flags;
         while (tex->w > 0) {
            tex->align = i;
            tex++;
         }
      } else {
         info->tex = NULL;
         pal24 = grd_pal;
         flags = pt_default_water_texture_flags;
      }

      alpha = 0xf * alpha_list[i];
      alpha <<= 12;
      if (flags & BMF_TRANS) {
         argb = alpha + ((rgb_list[3*i]>>4)<<8) + ((rgb_list[3*i+1]>>4)<<4) + (rgb_list[3*i+2]>>4);
         init_water_pal(info, (ushort )0xf000, argb, pal24);
      } else {
         argb = alpha + ((pal24[0]>>4)<<8) +
                        ((pal24[1]>>4)<<4) +
                        (pal24[2]>>4);
         init_water_pal(info, alpha, argb, pal24);
      }
   }
}

void portal_cleanup_water_hack(void)
{
   portal_setup_water_hack(0, NULL, NULL, NULL);
}

// this is quite hateful.

#define MAX_WATER_POINTS 1024
#define MAX_WATER_POLYS 256
static r3s_point water_points[MAX_WATER_POINTS];
static int water_polys[MAX_WATER_POLYS];
static r3s_texture water_textures[MAX_WATER_POLYS];
static int num_water_points=0;
float hw_water_tex_scale=1.0;

int num_water_polys=0;

void (*portal_queue_water_poly)() = NULL;

static void queue_water_poly(r3s_texture tex, int n, r3s_phandle *vlist)
{
   int i;
   r3s_point *dest;
   float u_offset, v_offset;

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

   *dest = *(vlist[0]);
   dest->grp.u *= hw_water_tex_scale;
   dest->grp.v *= hw_water_tex_scale;

   u_offset = floor(dest->grp.u);
   v_offset = floor(dest->grp.v);

   dest->grp.u -= u_offset;
   dest->grp.v -= v_offset;

   ++dest;
   for (i=1; i<n; i++) {
      *dest = *(vlist[i]);
      dest->grp.u *= hw_water_tex_scale;
      dest->grp.v *= hw_water_tex_scale;
      dest->grp.u -= u_offset;
      dest->grp.v -= v_offset;
      ++dest;
   }

   num_water_points += n;
   num_water_polys++;
   if (portal_queue_water_poly != NULL)
      portal_queue_water_poly();
}

static void render_water_poly(r3s_texture tex, int n, r3s_phandle *vlist)
{
   int i;
   float u_offset, v_offset;

   vlist[0]->grp.u *= hw_water_tex_scale;
   vlist[0]->grp.v *= hw_water_tex_scale;

   u_offset = floor(vlist[0]->grp.u);
   v_offset = floor(vlist[0]->grp.v);

   vlist[0]->grp.u -= u_offset;
   vlist[0]->grp.v -= v_offset;

   for (i=1; i<n; i++) {
      r3s_phandle p = vlist[i];
      p->grp.u *= hw_water_tex_scale;
      p->grp.v *= hw_water_tex_scale;
      p->grp.u -= u_offset;
      p->grp.v -= v_offset;
   }

   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_blend(TRUE);

   AssertMsg(water_info_list != NULL, "Must call portal_setup_water_hack() before rendering water in hardware!");

   if (tex->align >= num_water_textures)
      lgd3d_set_alpha_pal(water_info_list[0].pal);
   else
      lgd3d_set_alpha_pal(water_info_list[tex->align].pal);
   r3_set_texture(tex);
   r3_draw_poly(n, vlist);
   lgd3d_set_blend(FALSE);
   gr_set_fill_type(FILL_NORM);
}

void portal_render_water_polys(int n)
{
   static int start=0;
   static r3s_point *next_poly = water_points;
   int i, end;

   end = start + n;

   AssertMsg(water_info_list != NULL, "Must call portal_setup_water_hack() before rendering water in hardware!");
   AssertMsg(end <= num_water_polys, "portal_render_water_polys(): too many polys requested!");

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_UNLIT);
   r3d_do_setup = TRUE;

   gr_set_fill_type(FILL_BLEND);

   for (i=start; i < end; i++) {
      int j, n = water_polys[i];
      r3s_texture tex = water_textures[i];
      r3s_phandle *vlist = (r3s_phandle *)temp_malloc(n * sizeof(r3s_phandle));

      for (j=0; j<n; j++)
         vlist[j] = &next_poly[j];
      next_poly += n;

      if (tex->align >= num_water_textures)
         lgd3d_set_alpha_pal(water_info_list[0].pal);
      else
         lgd3d_set_alpha_pal(water_info_list[tex->align].pal);

      r3_set_texture(tex);
      r3_draw_poly(n, vlist);
      temp_free(vlist);
   }

   gr_set_fill_type(FILL_NORM);

   r3_end_block();

   if (end < num_water_polys)
      start = end;
   else {
      start = 0;
      next_poly = water_points;
      num_water_polys = 0;
      num_water_points = 0;
   }
}


extern BOOL bRenderNewSky;


void draw_surface_lgd3d(PortalPolygonCore *poly, PortalPolygonRenderInfo *render,
                        PortalLightMap *lt, int voff, void *clip)
{
   int i,n;
   int desired_mip;
   hw_render_info hw;
   r3s_phandle vlist[MAX_VERT], *final;
   scale_info info;
   bool motion, post_clip_uv, zsky = FALSE;

   n = poly->num_vertices;
   AssertMsg (n <= MAX_VERT, "draw_surface: too many vertices.");

   hw.tex = portal_get_texture(render->texture_id);
   hw.lm = NULL;

   if (!hw.tex && bRenderNewSky)
   {
      END_PROF;
      return;
   }

   motion = poly->motion_index && portal_cell_motion[poly->motion_index].in_motion;
   post_clip_uv = (hw.tex == NULL)||portal_clip_num||motion;
   if (!post_clip_uv)
   {
      mxs_real uv;
      mxs_vector *p_uvec, *p_vvec, *anchor;
      mxs_real u_scale, v_scale;
      mxs_real u_base, v_base;
      mxs_real u2, v2;

      u_scale = two_to_n(6 - hw.tex->wlog);
      v_scale = two_to_n(6 - hw.tex->hlog);

      p_uvec = &render->tex_u;
      p_vvec = &render->tex_v;
      anchor = &(cur_pool[r_vertex_list[voff + render->texture_anchor]]);
      uv = mx_dot_vec(p_uvec, p_vvec);

      u_base = render->u_base * u_scale / (16.0*256.0); // u translation
      v_base = render->v_base * v_scale / (16.0*256.0); // v translation

      u2 = mx_mag2_vec(p_uvec);
      v2 = mx_mag2_vec(p_vvec);

      if (uv == 0.0) {
         mxs_vector uvec, vvec;

         mx_scale_vec(&uvec, p_uvec, u_scale/u2);
         mx_scale_vec(&vvec, p_vvec, v_scale/v2);
         for (i=0; i < n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            vlist[i] = p;
            p->grp.u = mx_dot_vec(&delta, &uvec) + u_base;
            p->grp.v = mx_dot_vec(&delta, &vvec) + v_base;
         }
      } else {
         mxs_real uvu, uvv, denom;

         denom = 1.0/(u2*v2 - (uv*uv));

         u2 *= v_scale * denom;
         v2 *= u_scale * denom;
         uvu = u_scale * denom * uv;
         uvv = v_scale * denom * uv;
         for (i=0; i<n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            mxs_real du, dv;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            du = mx_dot_vec(&delta, p_uvec);
            dv = mx_dot_vec(&delta, p_vvec);
            vlist[i] = p;
            p->grp.u = u_base + v2 * du - uvu * dv;
            p->grp.v = v_base + u2 * dv - uvv * du;
         }
      }


      // clip against the view cone
      r3_set_clip_flags(R3_CLIP_UV);
   } else {
      for (i=0; i < n; ++i) {
         vlist[i] = &cur_ph[r_vertex_list[voff + i]];
      }
      r3_set_clip_flags(0); // why isn't there an R3_CLIP_NONE?
   }

   n = r3_clip_polygon(n, vlist, &final);

   if (n <= 2) { END_PROF; return; }
   STAT(++stat_num_poly_considered;)

   // check for trivial reject against the portal
   if (portal_clip_poly &&
       (portclip_clip_polygon(n, final, NULL, clip) == 0))
      { END_PROF; return; }

   if (!hw.tex) {
      draw_background_hack(n, final);
      END_PROF;
      return;  // an invisible portal, or background hack
   }

   if (portal_clip_num) { // only true if it's the sky
      n = portbg_clip_sky(n, final, &final);
   } else {
      desired_mip = compute_mip(render, &cur_cell->plane_list[poly->planeid]);
      hw.mip_level = 0;
      while (desired_mip > 0) {
         if (hw.tex[1].w == 0)
             break; // not enough mip levels
         ++hw.mip_level;
         --desired_mip;
         ++hw.tex;
      }
   }

#ifdef STATS_ON
   ++stat_num_poly_drawn;
   if (stat_num_poly_drawn > max_draw_polys) return;
#endif

#ifndef SHIP
   if (poly_outline_by_flags(poly, vlist, hw.mip_level)) {
      END_PROF;
      return;
   }
#endif // ~SHIP

#ifdef DBG_ON
   if (hw.tex->w & (hw.tex->w - 1))
      Error(1, "Texture non-power-of-two in w!\n");
   if (hw.tex->h & (hw.tex->h - 1))
      Error(1, "Texture non-power-of-two in h!\n");
#endif

   if ((lt == NULL)||(poly->flags & RENDER_DOESNT_LIGHT)) {
      hw.lm = NULL;
   } else {
      porthw_get_cached_lightmap(&hw, render, lt);
   }

   if ((render->texture_id == portal_sky_id) && !bRenderNewSky)
   {
      // don't need to lightmap!
      hw.lm = NULL;
      switch (ptsky_type) {
      case PTSKY_SPAN:
         portal_sky_spans += ptsky_calc_spans(n, final);
         break;
      case PTSKY_ZBUFFER:
         zsky = TRUE;
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

   if (post_clip_uv) {
      mxs_vector u_vec, v_vec, pt;
      if (motion) {
         portal_position_portal_texture(&u_vec, &v_vec, &pt,
               &(cur_pool[r_vertex_list[voff + render->texture_anchor]]),
               render, &cur_cell->plane_list[poly->planeid],
               &portal_cell_motion[poly->motion_index]);

         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      } else {
         mxs_real usc, vsc;
         usc = ((float) render->u_base) * 1.0 / (16.0*256.0); // u translation
         vsc = ((float) render->v_base) * 1.0 / (16.0*256.0); // v translation

         get_cached_vector(&u_vec, &render->tex_u);
         get_cached_vector(&v_vec, &render->tex_v);
         mx_scale_add_vec(&pt, &(vlist[render->texture_anchor]->p), &u_vec, -usc);
         mx_scale_add_vec(&pt, &pt, &v_vec, -vsc);

         // This gives us our 3x3 texture perspective correction matrix.
         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      }

      if (portal_clip_num) {
         // for sky, remap coordinates from 0..tex->w to 0.5..tex->w-0.5

         info.scale_u = (hw.tex->w-1) * two_to_n(-hw.tex->wlog);
         info.scale_v = (hw.tex->h-1) * two_to_n(-hw.tex->hlog);
         info.u0 = two_to_n(-hw.tex->wlog - 1);
         info.v0 = two_to_n(-hw.tex->hlog - 1);
      } else {
         int mip_log = 6 - hw.mip_level;
         info.scale_u = two_to_n(mip_log - hw.tex->wlog);
         info.scale_v = two_to_n(mip_log - hw.tex->hlog);
         info.u0 = info.v0 = 0.0;
      }
      ptlgd3d_calc_uv(n, final, &info);
   }

   if (hw.lm != NULL) {
      int pix_per_lm = 4 - hw.mip_level;
      int log_scale_u = hw.tex->wlog - pix_per_lm;
      int log_scale_v = hw.tex->hlog - pix_per_lm;
// This is necessary because wlog is actually log2(row), _not_ log2(w)
// Shoot me now; it's my own fault. -kevin
#ifdef RGB_LIGHTING
#ifdef RGB_888
      info.scale_u = two_to_n(log_scale_u - (hw.lm->wlog - 2));
#else // RGB_888
      info.scale_u = two_to_n(log_scale_u - (hw.lm->wlog - 1));
#endif // RGB_888
#else
      info.scale_u = two_to_n(log_scale_u - hw.lm->wlog);
#endif
      info.scale_v = two_to_n(log_scale_v - hw.lm->hlog);
      info.u0 = (hw.lm_u0 - lt->base_u) * two_to_n(-log_scale_u);
      info.v0 = (hw.lm_v0 - lt->base_v) * two_to_n(-log_scale_v);
#ifndef SHIP
      if (pt_lightmap_first) {
         float *uv_save = temp_malloc(2*n*sizeof(float));
         int i;

         for (i=0; i<n; i++) {
            uv_save[2*i] = final[i]->grp.u;
            uv_save[2*i+1] = final[i]->grp.v;
         }
         ptlgd3d_recalc_uv_vlist(n, final, &info);
         r3_set_texture(hw.lm);
         r3_draw_poly(n, final);
         for (i=0; i<n; i++) {
            final[i]->grp.u = uv_save[2*i];
            final[i]->grp.v = uv_save[2*i+1];
         }
         lgd3d_set_blend(TRUE);
         lgd3d_blend_multiply(portal_hack_blend);
         temp_free(uv_save);
      }
#endif
   }

   // Are we a translucent medium border (e.g., water)?
   if (poly->flags & PORTAL_SPLITS_OBJECT)
   {
      if (g_zbuffer)
         queue_water_poly(hw.tex, n, final);
      else
         render_water_poly(hw.tex, n, final);
      goto poly_done;
   }

   r3_set_texture(hw.tex);

   if (zsky) {
      int i;
      // render it real far away, so stars will render in front

      float *zw_save;
      zw_save = (float *)temp_malloc(2*n*sizeof(float));
      for (i=0; i<n; i++) {
         zw_save[i] = final[i]->p.z;
         zw_save[i+n] = final[i]->grp.w;
         final[i]->p.z = portal_z_max;
         final[i]->grp.w = portal_w_min;
      }
      r3_draw_poly(n, final);
      for (i=0; i<n; i++) {
         final[i]->p.z =   zw_save[i];
         final[i]->grp.w = zw_save[i+n];
      }
      temp_free(zw_save);
   }
   else
      r3_draw_poly(n, final);

   // is there a lightmap?

   if (hw.lm != NULL)
   {
#ifndef SHIP
      // rendered already?
      if (pt_lightmap_first) {
         lgd3d_blend_normal();
         lgd3d_set_blend(FALSE);
      } else
#endif
      {

         // render it...

         ptlgd3d_recalc_uv_vlist(n, final, &info);


#ifndef SHIP
         if (show_lightmap) {
            r3_set_texture(hw.lm);
            r3_draw_poly(n, final);
         } else
#endif
         {
            lgd3d_blend_multiply(portal_hack_blend);
            lgd3d_set_blend(TRUE);
#ifdef RGB_LIGHTING
            lgd3d_set_alpha(0.5);
#endif
            r3_set_texture(hw.lm);
            r3_draw_poly(n, final);
            lgd3d_blend_normal();
            lgd3d_set_blend(FALSE);
#ifdef RGB_LIGHTING
            lgd3d_set_alpha(1.0);
#endif
         }
      }
   }

poly_done:
#ifndef SHIP
   if (draw_wireframe_around_tmap) {
      draw_polygon_wireframe(final, n, COLOR_WHITE+2);
      draw_polygon_vertices(final, n, COLOR_WHITE);
   }
#endif // ~SHIP

   END_PROF;
   return;
}



void draw_surface_multitexture(PortalPolygonCore *poly, PortalPolygonRenderInfo *render,
                        PortalLightMap *lt, int voff, void *clip)
{
   int i,n;
   int desired_mip;
   hw_render_info hw;
   r3s_phandle vlist[MAX_VERT], *final;
   scale_info info;
   bool motion, post_clip_uv, zsky = FALSE;

   n = poly->num_vertices;
   AssertMsg (n <= MAX_VERT, "draw_surface: too many vertices.");

   hw.tex = portal_get_texture(render->texture_id);
   hw.lm = NULL;

   if (!hw.tex && bRenderNewSky)
   {
      END_PROF;
      return;
   }

   motion = poly->motion_index && portal_cell_motion[poly->motion_index].in_motion;
   post_clip_uv = (hw.tex == NULL)||portal_clip_num||motion;
   if (!post_clip_uv)
   {
      mxs_real uv;
      mxs_vector *p_uvec, *p_vvec, *anchor;
      mxs_real u_scale, v_scale;
      mxs_real u_base, v_base;
      mxs_real u2, v2;

      u_scale = two_to_n(6 - hw.tex->wlog);
      v_scale = two_to_n(6 - hw.tex->hlog);

      p_uvec = &render->tex_u;
      p_vvec = &render->tex_v;
      anchor = &(cur_pool[r_vertex_list[voff + render->texture_anchor]]);
      uv = mx_dot_vec(p_uvec, p_vvec);

      u_base = render->u_base * u_scale / (16.0*256.0); // u translation
      v_base = render->v_base * v_scale / (16.0*256.0); // v translation

      u2 = mx_mag2_vec(p_uvec);
      v2 = mx_mag2_vec(p_vvec);

      if (uv == 0.0) {
         mxs_vector uvec, vvec;

         mx_scale_vec(&uvec, p_uvec, u_scale/u2);
         mx_scale_vec(&vvec, p_vvec, v_scale/v2);
         for (i=0; i < n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            vlist[i] = p;
            p->grp.u = mx_dot_vec(&delta, &uvec) + u_base;
            p->grp.v = mx_dot_vec(&delta, &vvec) + v_base;
         }
      } else {
         mxs_real uvu, uvv, denom;

         denom = 1.0/(u2*v2 - (uv*uv));

         u2 *= v_scale * denom;
         v2 *= u_scale * denom;
         uvu = u_scale * denom * uv;
         uvv = v_scale * denom * uv;
         for (i=0; i<n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            mxs_real du, dv;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            du = mx_dot_vec(&delta, p_uvec);
            dv = mx_dot_vec(&delta, p_vvec);
            vlist[i] = p;
            p->grp.u = u_base + v2 * du - uvu * dv;
            p->grp.v = v_base + u2 * dv - uvv * du;
         }
      }


      // clip against the view cone
      r3_set_clip_flags(R3_CLIP_UV);
   } else {
      for (i=0; i < n; ++i) {
         vlist[i] = &cur_ph[r_vertex_list[voff + i]];
      }
      r3_set_clip_flags(0); // why isn't there an R3_CLIP_NONE?
   }

   n = r3_clip_polygon(n, vlist, &final);

   if (n <= 2) { END_PROF; return; }
   STAT(++stat_num_poly_considered;)

   // check for trivial reject against the portal
   if (portal_clip_poly &&
       (portclip_clip_polygon(n, final, NULL, clip) == 0))
      { END_PROF; return; }

   if (!hw.tex) {
      draw_background_hack(n, final);
      END_PROF;
      return;  // an invisible portal, or background hack
   }

   if (portal_clip_num) { // only true if it's the sky
      n = portbg_clip_sky(n, final, &final);
   } else {
      desired_mip = compute_mip(render, &cur_cell->plane_list[poly->planeid]);
      hw.mip_level = 0;
      while (desired_mip > 0) {
         if (hw.tex[1].w == 0)
             break; // not enough mip levels
         ++hw.mip_level;
         --desired_mip;
         ++hw.tex;
      }
   }

#ifdef STATS_ON
   ++stat_num_poly_drawn;
   if (stat_num_poly_drawn > max_draw_polys) return;
#endif

#ifndef SHIP
   if (poly_outline_by_flags(poly, vlist, hw.mip_level)) {
      END_PROF;
      return;
   }
#endif // ~SHIP

#ifdef DBG_ON
   if (hw.tex->w & (hw.tex->w - 1))
      Error(1, "Texture non-power-of-two in w!\n");
   if (hw.tex->h & (hw.tex->h - 1))
      Error(1, "Texture non-power-of-two in h!\n");
#endif

   if ((lt == NULL)||(poly->flags & RENDER_DOESNT_LIGHT)) {
      hw.lm = NULL;
   } else {
      porthw_get_cached_lightmap(&hw, render, lt);
   }

   if ((render->texture_id == portal_sky_id) && !bRenderNewSky)
   {
      hw.lm = NULL;
      switch (ptsky_type) {
      case PTSKY_SPAN:
         portal_sky_spans += ptsky_calc_spans(n, final);
         break;
      case PTSKY_ZBUFFER:
         zsky = TRUE;
         break;
      case PTSKY_NONE:
         goto poly_done_multi;
      }
   }
   else if (portal_sky_spans > 0)
   {
      ptsky_render_stars();
      portal_sky_spans = 0;
   }

   if (post_clip_uv) {
      mxs_vector u_vec, v_vec, pt;
      if (motion) {
         portal_position_portal_texture(&u_vec, &v_vec, &pt,
               &(cur_pool[r_vertex_list[voff + render->texture_anchor]]),
               render, &cur_cell->plane_list[poly->planeid],
               &portal_cell_motion[poly->motion_index]);

         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      } else {
         mxs_real usc, vsc;
         usc = ((float) render->u_base) * 1.0 / (16.0*256.0); // u translation
         vsc = ((float) render->v_base) * 1.0 / (16.0*256.0); // v translation

         get_cached_vector(&u_vec, &render->tex_u);
         get_cached_vector(&v_vec, &render->tex_v);
         mx_scale_add_vec(&pt, &(vlist[render->texture_anchor]->p), &u_vec, -usc);
         mx_scale_add_vec(&pt, &pt, &v_vec, -vsc);

         // This gives us our 3x3 texture perspective correction matrix.
         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      }

      if (portal_clip_num) {
         // for sky, remap coordinates from 0..tex->w to 0.5..tex->w-0.5

         info.scale_u = (hw.tex->w-1) * two_to_n(-hw.tex->wlog);
         info.scale_v = (hw.tex->h-1) * two_to_n(-hw.tex->hlog);
         info.u0 = two_to_n(-hw.tex->wlog - 1);
         info.v0 = two_to_n(-hw.tex->hlog - 1);
      } else {
         int mip_log = 6 - hw.mip_level;
         info.scale_u = two_to_n(mip_log - hw.tex->wlog);
         info.scale_v = two_to_n(mip_log - hw.tex->hlog);
         info.u0 = info.v0 = 0.0;
      }
      ptlgd3d_calc_uv(n, final, &info);
   }

   if (hw.lm != NULL) {
      int pix_per_lm = 4 - hw.mip_level;
      int log_scale_u = hw.tex->wlog - pix_per_lm;
      int log_scale_v = hw.tex->hlog - pix_per_lm;
// This is necessary because wlog is actually log2(row), _not_ log2(w)
// Shoot me now; it's my own fault. -kevin
#ifdef RGB_LIGHTING
#ifdef RGB_888
      info.scale_u = two_to_n(log_scale_u - (hw.lm->wlog - 2));
#else // RGB_888
      info.scale_u = two_to_n(log_scale_u - (hw.lm->wlog - 1));
#endif // RGB_888
#else
      info.scale_u = two_to_n(log_scale_u - hw.lm->wlog);
#endif
      info.scale_v = two_to_n(log_scale_v - hw.lm->hlog);
      info.u0 = (hw.lm_u0 - lt->base_u) * two_to_n(-log_scale_u);
      info.v0 = (hw.lm_v0 - lt->base_v) * two_to_n(-log_scale_v);
      {
         LGD3D_tex_coord *uv2 = temp_malloc(n*sizeof(*uv2));
         LGD3D_tex_coord **puv2 = temp_malloc(n*sizeof(*puv2));
         int i;

         for (i=0; i<n; i++) {
            uv2[i].u = final[i]->grp.u;
            uv2[i].v = final[i]->grp.v;
            puv2[i] = &uv2[i];
         }
         // lgd3d_set_light_map_method(LGD3DTB_MODULATE);
         lgd3d_set_light_map_method( LGD3D_MULTITEXTURE_COLOR );
         ptlgd3d_recalc_uv(n, uv2, &info);

         lgd3d_set_texture_level(1);
         lgd3d_set_texture(hw.lm);
         lgd3d_set_texture_level(0);
         lgd3d_set_texture(hw.tex);
         lgd3d_TrifanMTD(n, final, puv2);
         temp_free(puv2);
         temp_free(uv2);
         r3d_do_setup = TRUE;
      }
   } else {
      if (poly->flags & PORTAL_SPLITS_OBJECT)
      {
         if (g_zbuffer)
            queue_water_poly(hw.tex, n, final);
         else
            render_water_poly(hw.tex, n, final);
      } else {
         r3_set_texture(hw.tex);
         if (zsky) {
            int i;
            // render it real far away, so stars will render in front

            float *zw_save;
            zw_save = (float *)temp_malloc(2*n*sizeof(float));
            for (i=0; i<n; i++) {
               zw_save[i] = final[i]->p.z;
               zw_save[i+n] = final[i]->grp.w;
               final[i]->p.z = portal_z_max;
               final[i]->grp.w = portal_w_min;
            }
            r3_draw_poly(n, final);
            for (i=0; i<n; i++) {
               final[i]->p.z =   zw_save[i];
               final[i]->grp.w = zw_save[i+n];
            }
            temp_free(zw_save);
         }
         else
            r3_draw_poly(n, final);
      }
   }
poly_done_multi:
#ifndef SHIP
   if (draw_wireframe_around_tmap) {
      draw_polygon_wireframe(final, n, COLOR_WHITE+2);
      draw_polygon_vertices(final, n, COLOR_WHITE);
   }
#endif // ~SHIP

   END_PROF;
   return;
}


void draw_surface_texture_only(PortalPolygonCore *poly, PortalPolygonRenderInfo *render, int voff, void *clip)
{
   int i,n;
   int desired_mip;
   hw_render_info hw;
   r3s_phandle vlist[MAX_VERT], *final;
   scale_info info;
   bool motion, post_clip_uv, zsky = FALSE;

   n = poly->num_vertices;
   AssertMsg (n <= MAX_VERT, "draw_surface: too many vertices.");

   hw.tex = portal_get_texture(render->texture_id);
   hw.lm = NULL;

   if (!hw.tex && bRenderNewSky)
   {
      END_PROF;
      return;
   }

   motion = poly->motion_index && portal_cell_motion[poly->motion_index].in_motion;
   post_clip_uv = (hw.tex == NULL)||portal_clip_num||motion;
   if (!post_clip_uv)
   {
      mxs_real uv;
      mxs_vector *p_uvec, *p_vvec, *anchor;
      mxs_real u_scale, v_scale;
      mxs_real u_base, v_base;
      mxs_real u2, v2;

      u_scale = two_to_n(6 - hw.tex->wlog);
      v_scale = two_to_n(6 - hw.tex->hlog);

      p_uvec = &render->tex_u;
      p_vvec = &render->tex_v;
      anchor = &(cur_pool[r_vertex_list[voff + render->texture_anchor]]);
      uv = mx_dot_vec(p_uvec, p_vvec);

      u_base = render->u_base * u_scale / (16.0*256.0); // u translation
      v_base = render->v_base * v_scale / (16.0*256.0); // v translation

      u2 = mx_mag2_vec(p_uvec);
      v2 = mx_mag2_vec(p_vvec);

      if (uv == 0.0) {
         mxs_vector uvec, vvec;

         mx_scale_vec(&uvec, p_uvec, u_scale/u2);
         mx_scale_vec(&vvec, p_vvec, v_scale/v2);
         for (i=0; i < n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            vlist[i] = p;
            p->grp.u = mx_dot_vec(&delta, &uvec) + u_base;
            p->grp.v = mx_dot_vec(&delta, &vvec) + v_base;
         }
      } else {
         mxs_real uvu, uvv, denom;

         denom = 1.0/(u2*v2 - (uv*uv));

         u2 *= v_scale * denom;
         v2 *= u_scale * denom;
         uvu = u_scale * denom * uv;
         uvv = v_scale * denom * uv;
         for (i=0; i<n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            mxs_real du, dv;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            du = mx_dot_vec(&delta, p_uvec);
            dv = mx_dot_vec(&delta, p_vvec);
            vlist[i] = p;
            p->grp.u = u_base + v2 * du - uvu * dv;
            p->grp.v = v_base + u2 * dv - uvv * du;
         }
      }
      r3_set_clip_flags(R3_CLIP_UV);
   } else {
      for (i=0; i < n; ++i) {
         vlist[i] = &cur_ph[r_vertex_list[voff + i]];
      }
      r3_set_clip_flags(0); // why isn't there an R3_CLIP_NONE?
   }

   // clip against the view cone
   n = r3_clip_polygon(n, vlist, &final);

   if (n <= 2) { END_PROF; return; }
   STAT(++stat_num_poly_considered;)

   // check for trivial reject against the portal
   if (portal_clip_poly &&
       (portclip_clip_polygon(n, final, NULL, clip) == 0))
      { END_PROF; return; }

   if (!hw.tex) {
      draw_background_hack(n, final);
      END_PROF;
      return;  // an invisible portal, or background hack
   }

   if (portal_clip_num) { // only true if it's the sky
      n = portbg_clip_sky(n, final, &final);
   } else {
      desired_mip = compute_mip(render, &cur_cell->plane_list[poly->planeid]);
      hw.mip_level = 0;
      while (desired_mip > 0) {
         if (hw.tex[1].w == 0)
             break; // not enough mip levels
         ++hw.mip_level;
         --desired_mip;
         ++hw.tex;
      }
   }

#ifdef STATS_ON
   ++stat_num_poly_drawn;
   if (stat_num_poly_drawn > max_draw_polys) return;
#endif

#ifndef SHIP
   if (poly_outline_by_flags(poly, vlist, hw.mip_level)) {
      END_PROF;
      return;
   }
#endif // ~SHIP

#ifdef DBG_ON
   if (hw.tex->w & (hw.tex->w - 1))
      Error(1, "Texture non-power-of-two in w!\n");
   if (hw.tex->h & (hw.tex->h - 1))
      Error(1, "Texture non-power-of-two in h!\n");
#endif

   if ((render->texture_id == portal_sky_id) && !bRenderNewSky)
   {
      switch (ptsky_type) {
      case PTSKY_SPAN:
         portal_sky_spans += ptsky_calc_spans(n, final);
         break;
      case PTSKY_ZBUFFER:
         zsky = TRUE;
         break;
      case PTSKY_NONE:
         goto poly_done_texture;
      }
   }
   else if (portal_sky_spans > 0)
   {
      ptsky_render_stars();
      portal_sky_spans = 0;
   }

   if (post_clip_uv) {
      mxs_vector u_vec, v_vec, pt;
      if (motion) {
         portal_position_portal_texture(&u_vec, &v_vec, &pt,
               &(cur_pool[r_vertex_list[voff + render->texture_anchor]]),
               render, &cur_cell->plane_list[poly->planeid],
               &portal_cell_motion[poly->motion_index]);

         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      } else {
         mxs_real usc, vsc;
         usc = ((float) render->u_base) * 1.0 / (16.0*256.0); // u translation
         vsc = ((float) render->v_base) * 1.0 / (16.0*256.0); // v translation

         get_cached_vector(&u_vec, &render->tex_u);
         get_cached_vector(&v_vec, &render->tex_v);
         mx_scale_add_vec(&pt, &(vlist[render->texture_anchor]->p), &u_vec, -usc);
         mx_scale_add_vec(&pt, &pt, &v_vec, -vsc);

         // This gives us our 3x3 texture perspective correction matrix.
         g2pt_calc_uvw_deltas(&pt, &u_vec, &v_vec);
      }

      if (portal_clip_num) {
         // for sky, remap coordinates from 0..tex->w to 0.5..tex->w-0.5

         info.scale_u = (hw.tex->w-1) * two_to_n(-hw.tex->wlog);
         info.scale_v = (hw.tex->h-1) * two_to_n(-hw.tex->hlog);
         info.u0 = two_to_n(-hw.tex->wlog - 1);
         info.v0 = two_to_n(-hw.tex->hlog - 1);
      } else {
         int mip_log = 6 - hw.mip_level;
         info.scale_u = two_to_n(mip_log - hw.tex->wlog);
         info.scale_v = two_to_n(mip_log - hw.tex->hlog);
         info.u0 = info.v0 = 0.0;
      }
      ptlgd3d_calc_uv(n, final, &info);
   }

   // Are we a translucent medium border (e.g., water)?
   if (poly->flags & PORTAL_SPLITS_OBJECT)
   {
      if (g_zbuffer)
         queue_water_poly(hw.tex, n, final);
      else
         render_water_poly(hw.tex, n, final);
      goto poly_done_texture;
   }

   r3_set_texture(hw.tex);

   if (zsky) {
      int i;
      // render it real far away, so stars will render in front

      float *zw_save;
      zw_save = (float *)temp_malloc(2*n*sizeof(float));
      for (i=0; i<n; i++) {
         zw_save[i] = final[i]->p.z;
         zw_save[i+n] = final[i]->grp.w;
         final[i]->p.z = portal_z_max;
         final[i]->grp.w = portal_w_min;
      }
      r3_draw_poly(n, final);
      for (i=0; i<n; i++) {
         final[i]->p.z =   zw_save[i];
         final[i]->grp.w = zw_save[i+n];
      }
      temp_free(zw_save);
   }
   else
      r3_draw_poly(n, final);

poly_done_texture:
#ifndef SHIP
   if (draw_wireframe_around_tmap) {
      draw_polygon_wireframe(final, n, COLOR_WHITE+2);
      draw_polygon_vertices(final, n, COLOR_WHITE);
   }
#endif // ~SHIP

   END_PROF;
   return;
}



void draw_surface_lightmap_only(PortalPolygonCore *poly, PortalPolygonRenderInfo *render,
                        PortalLightMap *lt, int voff, void *clip)
{
   int i,n;
   hw_render_info hw;
   r3s_phandle vlist[MAX_VERT], *final;

   // water or sky poly?
   if ((poly->flags & PORTAL_SPLITS_OBJECT)||
       (render->texture_id == portal_sky_id))
      {END_PROF; return;}

   // sky hack?
   if (portal_get_texture(render->texture_id) == NULL)
      {END_PROF; return;}

   porthw_get_cached_lightmap(&hw, render, lt);
   n = poly->num_vertices;
   AssertMsg (n <= MAX_VERT, "draw_surface: too many vertices.");

   {
      mxs_real uv;
      mxs_vector *p_uvec, *p_vvec, *anchor;
      mxs_real u_scale, v_scale;
      mxs_real u_base, v_base;
      mxs_real u2, v2;

      p_uvec = &render->tex_u;
      p_vvec = &render->tex_v;
      anchor = &(cur_pool[r_vertex_list[voff + render->texture_anchor]]);
      uv = mx_dot_vec(p_uvec, p_vvec);

// This is necessary because wlog is actually log2(row), _not_ log2(w)
// Shoot me now; it's my own fault. -kevin
#ifdef RGB_LIGHTING
#ifdef RGB_888
      u_scale = two_to_n(2-(hw.lm->wlog-2));
#else // RGB_888
      u_scale = two_to_n(2-(hw.lm->wlog-1));
#endif // RGB_888
#else
      u_scale = two_to_n(2-hw.lm->wlog);
#endif

      v_scale = two_to_n(2-hw.lm->hlog);

      u_base = u_scale * (render->u_base/(16.0*256.0) + (hw.lm_u0 - lt->base_u)/4.0); // u translation
      v_base = v_scale * (render->v_base/(16.0*256.0) + (hw.lm_v0 - lt->base_v)/4.0); // v translation

      u2 = mx_mag2_vec(p_uvec);
      v2 = mx_mag2_vec(p_vvec);

      if (uv == 0.0) {
         mxs_vector uvec, vvec;

         mx_scale_vec(&uvec, p_uvec, u_scale/u2);
         mx_scale_vec(&vvec, p_vvec, v_scale/v2);
         for (i=0; i < n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            vlist[i] = p;
            p->grp.u = mx_dot_vec(&delta, &uvec) + u_base;
            p->grp.v = mx_dot_vec(&delta, &vvec) + v_base;
         }
      } else {
         mxs_real uvu, uvv, denom;

         denom = 1.0/(u2*v2 - (uv*uv));

         u2 *= v_scale * denom;
         v2 *= u_scale * denom;
         uvu = u_scale * denom * uv;
         uvv = v_scale * denom * uv;
         for (i=0; i<n; ++i) {
            mxs_vector *wvec = &cur_pool[r_vertex_list[voff + i]];
            mxs_vector delta;
            mxs_real du, dv;
            r3s_phandle p = &cur_ph[r_vertex_list[voff + i]];

            mx_sub_vec(&delta, wvec, anchor);
            du = mx_dot_vec(&delta, p_uvec);
            dv = mx_dot_vec(&delta, p_vvec);
            vlist[i] = p;
            p->grp.u = u_base + v2 * du - uvu * dv;
            p->grp.v = v_base + u2 * dv - uvv * du;
         }
      }
      r3_set_clip_flags(R3_CLIP_UV);
   }

   // clip against the view cone
   n = r3_clip_polygon(n, vlist, &final);

   if (n <= 2)
      { END_PROF; return; }

   // check for trivial reject against the portal
   if (portal_clip_poly &&
       (portclip_clip_polygon(n, final, NULL, clip) == 0))
      { END_PROF; return; }

#ifndef SHIP
   if (show_lightmap) {
      r3_set_texture(hw.lm);
      r3_draw_poly(n, final);
   } else
#endif
   {
      r3_set_texture(hw.lm);
      r3_draw_poly(n, final);
   }

   END_PROF;
   return;
}


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

