/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portbg.c,v 1.22 2000/02/19 13:18:49 toml Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <math.h>

#include <lg.h>
#include <dev2d.h>
#include <r3d.h>
#include <mprintf.h>
#include <lgd3d.h>

#include <portal_.h>
#include <portclip.h>
#include <pt_clut.h>
#include <wrdbrend.h>
#include <wrlimit.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

PortalCell background_cube;

#define BG_DIST    1200

Vertex background_vpool[8] =
{
   {  BG_DIST, BG_DIST, BG_DIST }, { -BG_DIST, BG_DIST, BG_DIST },
   { -BG_DIST,-BG_DIST, BG_DIST }, {  BG_DIST,-BG_DIST, BG_DIST },
   {  BG_DIST, BG_DIST,-BG_DIST }, { -BG_DIST, BG_DIST,-BG_DIST },
   { -BG_DIST,-BG_DIST,-BG_DIST }, {  BG_DIST,-BG_DIST,-BG_DIST }
};

PortalPolygonCore background_poly_list[6] =
{
   { RENDER_DOESNT_LIGHT, 4 },
   { RENDER_DOESNT_LIGHT, 4 },
   { RENDER_DOESNT_LIGHT, 4 },
   { RENDER_DOESNT_LIGHT, 4 },
   { RENDER_DOESNT_LIGHT, 4 },
   { RENDER_DOESNT_LIGHT, 4 }
};

uchar background_vlist_lighting[24] =
{
   64,64,64,64, 64,64,64,64, 64,64,64,64,
   64,64,64,64, 64,64,64,64, 64,64,64,64
};

uchar background_vertex_list[24] =
{
   0,1,2,3,
   7,6,5,4,
   1,0,4,5,
   2,1,5,6,
   3,2,6,7,
   0,3,7,4
};

PortalPlane bg_planes[6] =
{
   { { 0,0,-1 }, 1 },
   { { 0,0,1 }, 1 },
   { { 0,-1,0 }, 1 },
   { { 0,1,0 }, 1 },
   { { 1, 0,0 }, 1 },
   { {-1,0,0 }, 1 }
};

#define BG_UV   (BG_DIST * 2 + 0.02)

PortalPolygonRenderInfo background_render_list[6] =
{
   { { BG_UV,0,0 }, { 0,BG_UV,0 }, 0x1000,0x1000, 250, 0 },
   { { BG_UV,0,0 }, { 0,-BG_UV,0 }, 0,0, 251, 0 },
   { { BG_UV,0,0 }, { 0,0,-BG_UV }, 0,0, 252, 0 },
   { { 0,BG_UV,0 }, { 0,0,-BG_UV }, 0,0, 253, 0 },
   { { -BG_UV,0,0 }, { 0,0,-BG_UV }, 0,0, 254, 0 },
   { { 0,-BG_UV,0 }, { 0,0,-BG_UV }, 0,0, 255, 0 },
};

  // Initialize the PortalCell data structure by stuffing fields explicitly
  // to avoid problems with fields not lining up in an initializer
  // since we have too many damn fields
#define BC background_cube
void init_background_hack(void)
{
   BC.num_vertices = 8;
   BC.num_polys = 6;   
   BC.num_render_polys = 6;   
   BC.num_portal_polys = 0;

   BC.num_planes = 0;
   BC.medium = 255;
   BC.flags = 0;

   BC.vpool = background_vpool;
   BC.poly_list = background_poly_list;
   BC.portal_poly_list = 0;
   BC.render_list = background_render_list;

   BC.vertex_list = background_vertex_list;
   BC.portal_vertex_list = 0;

   BC.plane_list = bg_planes;
   BC.render_data = 0;
   BC.refs = 0;
   BC.num_vlist = 24;
}

extern BOOL g_lgd3d;

extern bool setup_cell(PortalCell *);
extern void free_cell(PortalCell *);
extern void draw_region(int region);
extern mxs_vector portal_camera_loc;
extern float fog_dist_modifier;


bool background_needs_clut;
bool background_setup;
uchar background_clut[256];

void setup_background_hack(void)
{
   static Vertex bg_vpool[8] =
   {
      {  BG_DIST, BG_DIST, BG_DIST }, { -BG_DIST, BG_DIST, BG_DIST },
      { -BG_DIST,-BG_DIST, BG_DIST }, {  BG_DIST,-BG_DIST, BG_DIST },
      {  BG_DIST, BG_DIST,-BG_DIST }, { -BG_DIST, BG_DIST,-BG_DIST },
      { -BG_DIST,-BG_DIST,-BG_DIST }, {  BG_DIST,-BG_DIST,-BG_DIST }
   };

   int i=0;
   for (i=0; i < 8; ++i)
      mx_add_vec(&background_cube.vpool[i], &bg_vpool[i], &portal_camera_loc);

   // need to update the plane constants as well, argh
   bg_planes[0].plane_constant =  portal_camera_loc.z + BG_DIST;
   bg_planes[1].plane_constant = -portal_camera_loc.z + BG_DIST;
   bg_planes[2].plane_constant =  portal_camera_loc.y + BG_DIST;
   bg_planes[3].plane_constant = -portal_camera_loc.y + BG_DIST;
   bg_planes[4].plane_constant = -portal_camera_loc.x - BG_DIST;
   bg_planes[5].plane_constant =  portal_camera_loc.x - BG_DIST;

   r3_start_block();
   if (setup_cell(&background_cube)) {
      r3_end_block();
      return;
   }
   r3_end_block();
   CLIP_DATA(&background_cube) = PortalClipRectangle(0,0, grd_bm.w, grd_bm.h);
   pt_clut_list[255] = background_needs_clut ? background_clut : 0;

   background_needs_clut = 0;
   background_setup = TRUE;
}

void render_background_hack(void)
{
   // we always get called from within a block, so we need to end and start...
   r3_end_block();
   if (!background_setup)
      setup_background_hack();
   wr_cell[MAX_REGIONS - 1] = &background_cube;

   // For the background hack we fudge the amount of fog so it we
   // can see the sky when the fog is minimal.
   if (g_lgd3d && portal_fog_dist)
      //lgd3d_set_fog_density(fog_dist_modifier * .1 / portal_fog_dist);
      lgd3d_set_linear_fog_distance( portal_fog_dist );

   draw_region(MAX_REGIONS - 1);

   if (g_lgd3d && portal_fog_dist)
      //lgd3d_set_fog_density(fog_dist_modifier / portal_fog_dist);
      lgd3d_set_linear_fog_distance( portal_fog_dist );


   wr_cell[MAX_REGIONS - 1] = NULL;
   r3_start_block();
}

void background_hack_cleanup(void)
{
   if (background_setup)
      free_cell(&background_cube);
}

static r3s_point portal_clip_point[256];
int portal_clip_num;

void render_background_hack_clipped(int n, r3s_phandle *vlist)
{
   int i;
   portal_clip_num = n;
   for (i=0; i < n; ++i)
      portal_clip_point[i] = *vlist[i];
   render_background_hack();
   portal_clip_num = 0;
}

static double compute_edge_len(r3s_point *e1, r3s_point *e2)
{
   float dx = e2->grp.sx - e1->grp.sx;
   float dy = e2->grp.sy - e1->grp.sy;
   return dx*dx + dy*dy;
}

static double compute_dist(r3s_point *pt, r3s_point *e1, r3s_point *e2)
{
   // compute distance from pt to the edge e1,e2
   //   technically these are fixes, but it doesn't really
   //   matter, we can clip them as integers

   //  (e1x,e1y)  (e2x,e2y) (px, py)
   //  (e1x,e1y)  (dx,dy)   (px,py)
   //  px = e1x + pd * dx + nd * -dy
   //  py = e1y + pd * dy + nd * dx

   //  denominator:    dx    -dy
   //                  dy     dx

   //  numerator:      dx   (px-e1x)
   //                  dy   (py-e1y)

   if (e1->grp.sy < e2->grp.sy) {
      float dx = e2->grp.sx - e1->grp.sx;
      float dy = e2->grp.sy - e1->grp.sy;
      float e1x = e1->grp.sx;
      float e1y = e1->grp.sy;
      float px = pt->grp.sx;
      float py = pt->grp.sy;

      return  (dx*(py-e1y)-dy*(px-e1x));
   } else {
      float dx = e1->grp.sx - e2->grp.sx;
      float dy = e1->grp.sy - e2->grp.sy;
      float e1x = e2->grp.sx;
      float e1y = e2->grp.sy;
      float px = pt->grp.sx;
      float py = pt->grp.sy;

      return -(dx*(py-e1y)-dy*(px-e1x));
   }
}

extern BOOL g_lgd3d;
static void compute_intersect(r3s_point *out, r3s_point *in1, r3s_point *in2,
                                              float d0, float d1)
{
   float interp;

   if (d0 < 0) {
      float tf;
      r3s_point *tp;

      // canonical form to avoid cracking

      tf = d0;
      d0 = d1;
      d1 = tf;

      tp = in1;
      in1 = in2;
      in2 = tp;
   }

   // interpolate from d0 to d1 so that 0 falls at interp along
   //    d0 + (d1-d0) * interp = 0

   interp = -d0 / (d1-d0);

   out->grp.sx = in1->grp.sx + interp*(in2->grp.sx - in1->grp.sx);
   out->grp.sy = in1->grp.sy + interp*(in2->grp.sy - in1->grp.sy);
   if (g_lgd3d) {
      out->grp.w = in1->grp.w + (in2->grp.w - in1->grp.w) * interp;
      out->p.z = 1 / out->grp.w;
   }
}

static r3s_point portal_clip_temp[256];
static r3s_phandle portal_clip_vlist[512];
static double point_dist[256];
static bool point_inside[256];

#if 0
#define EDGE_LEN_EPSILON   0.001   // minimum length edge to clip against
#define POINT_LEN_EPSILON   0.01   // minimum dist from edge ito clip against
#else
#define POINT_LEN_EPSILON 0
#define EDGE_LEN_EPSILON  0
#endif

int portbg_clip_sky(int n, r3s_phandle *vlist, r3s_phandle **result)
{
   r3s_phandle *vl1, *vl2;
   double len;
   int i,j,k,m, o, t=0;
   bool one_out;

#if 0
   vl1 = vlist;
   vl2 = portal_clip_vlist;

   m = portal_clip_num-1;
   for (k=0; k < portal_clip_num; m = k++) {
      // clip polygon in n,vl1 against edge m..k
      
      len = compute_edge_len(&portal_clip_point[m], &portal_clip_point[k]);

      // len is in fixed point, and it's squared

      if (len <= 65536.0 * 65536.0 * EDGE_LEN_EPSILON)
         continue;

      len = 1 / len;

      one_out = FALSE;  // was at least one point out

      for (i=0; i < n; ++i) {
         point_dist[i] = compute_dist(vl1[i],
                 &portal_clip_point[m], &portal_clip_point[k]) * len;
         if (point_dist[i] >= -POINT_LEN_EPSILON)
            point_inside[i] = TRUE;
         else {
            point_inside[i] = FALSE;
            one_out = TRUE;
         }
      }

      if (!one_out)
         continue;

      o = 0; // output points

      j = n-1;
      for (i=0; i < n; j = i, j=i++) {
          // if j was inside, output it
          if (point_inside[j])
             vl2[o++] = vl1[j];
          if (point_inside[i] != point_inside[j]) {
             // point crosses, so generate boundary point
             vl2[o] = &portal_clip_temp[t++];
             compute_intersect(vl2[o++], vl1[j], vl1[i],
                                       point_dist[j], point_dist[i]);
          }
      }

      if (!o) return 0;

      n = o;
      vl1 = vl2;
      vl2 = (vl1 == portal_clip_vlist)
                       ? portal_clip_vlist+256 : portal_clip_vlist;
   }
#else
   int en;

   vl1 = portal_clip_vlist;
   vl2 = portal_clip_vlist + 256;

   if (g_lgd3d) {
      double xs,ys,c;
      // compute w gradient to initialize vertices
      // of polygon

//dr/dx = ((r1-r2)(y0-y2)-(r0-r2)(y1-y2))/((x1-x2)(y0-y2)-(x0-x2)(y1-y2))
#define V0   vlist[0]->grp
#define V1   vlist[1]->grp
#define V2   vlist[2]->grp

#define W0   V0.w
#define W1   V1.w
#define W2   V2.w
#define X0   V0.sx
#define X1   V1.sx
#define X2   V2.sx
#define Y0   V0.sy
#define Y1   V1.sy
#define Y2   V2.sy

      // stuff c temporarily with denominator
      c = (float) (X1-X2)*(Y0-Y2) - (float) (X0-X2)*(Y1-Y2);
      xs =  ((W1-W2)*(Y0-Y2)-(W0-W2)*(Y1-Y2)) / c;
      ys = -((W1-W2)*(X0-X2)-(W0-W2)*(X1-X2)) / c;
      // now load c with w value at (0,0)
      c = W0 - xs*X0 - ys*Y0;

      for (k=0; k < portal_clip_num; ++k) {
         vl1[k] = &portal_clip_point[k];
         vl1[k]->grp.w = c + vl1[k]->grp.sx * xs + vl1[k]->grp.sy * ys;
         vl1[k]->p.z = 1 / vl1[k]->grp.w;
      }
   } else {
      for (k=0; k < portal_clip_num; ++k)
         vl1[k] = &portal_clip_point[k];
   }

   en = n;
   n = portal_clip_num;

   m = en-1;
   for (k=0; k < en; m = k++) {
      // clip polygon in n,vl1 against edge m..k
      
      len = compute_edge_len(vlist[m], vlist[k]);

      // len is in fixed point, and it's squared

      if (len <= 65536.0 * 65536.0 * EDGE_LEN_EPSILON)
         continue;

      len = 1 / len;

      one_out = FALSE;  // was at least one point out

      for (i=0; i < n; ++i) {
         point_dist[i] = compute_dist(vl1[i],
                 vlist[m], vlist[k]) * len;
         if (point_dist[i] >= -POINT_LEN_EPSILON)
            point_inside[i] = TRUE;
         else {
            point_inside[i] = FALSE;
            one_out = TRUE;
         }
      }

      if (!one_out)
         continue;

      o = 0; // output points

      j = n-1;
      for (i=0; i < n; j = i, j=i++) {
          // if j was inside, output it
          if (point_inside[j])
             vl2[o++] = vl1[j];
          if (point_inside[i] != point_inside[j]) {
             // point crosses, so generate boundary point
             vl2[o] = &portal_clip_temp[t++];
             compute_intersect(vl2[o++], vl1[j], vl1[i],
                                       point_dist[j], point_dist[i]);
          }
      }

      if (!o) return 0;

      n = o;
      vl1 = vl2;
      vl2 = (vl1 == portal_clip_vlist)
                       ? portal_clip_vlist+256 : portal_clip_vlist;
   }
#endif
   *result = vl1;
   return n;
}
