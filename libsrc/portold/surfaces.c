/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/libsrc/portold/surfaces.c,v 1.7 1997/12/29 14:01:07 MAT Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <mprintf.h>

#include <portal_.h>
#include <portclip.h>
#include <pt.h>
#include <pt_clut.h>

#include <profile.h>
#include <oracle.h>

#ifdef DBG_ON
  #define STATIC
#else
  #define STATIC static
#endif


#define STATS_ON

#ifdef DBG_ON
  #ifndef STATS_ON
  #define STATS_ON
  #endif
#endif

#define LM_STEP   16

float matrix_norm(mxs_vector *a, mxs_vector *b, mxs_vector *c)
{
   return
      a->x * (b->y * c->z - b->z * c->y)
    + a->y * (b->z * c->x - b->x * c->z)
    + a->z * (b->x * c->y - b->y * c->x);
}

// given the u,v info in r, and the vertex in p, compute the (u,v) coordinates of the corner
static void compute_uv(float *u, float *v, PortalPolygonRenderInfo *r, mxs_vector *norm, Vertex *p, Vertex *base)
{
   // r->u->raw, r->v->raw are the vectors
   //   the formulas are
   // base + (u - u_base) * U + (v - v_base) * V = p

   //  u*u->x + v*v->x + d*N->x = p->x - base->x + u_base*U->x + v_base*V->x 
   //  u*u->y + v*v->y + d*N->y = p->y - base->y + u_base*U->y + v_base*V->y 
   //  u*u->z + v*v->z + d*N->z = p->z - base->z + u_base*U->z + v_base*V->z 

   mxs_vector right;
   float denom;

   mx_scale_add_vec(&right, p, &r->u->raw, r->u_base / (16*256.0));
   mx_scale_addeq_vec(&right, &r->v->raw, r->v_base / (16*256.0));
   mx_subeq_vec(&right, base);

   // ok, so, now we just want to use cramers rule
   denom = matrix_norm(&r->u->raw, &r->v->raw, norm);
   *u = matrix_norm(&right, &r->v->raw, norm) / denom;
   *v = matrix_norm(&r->u->raw, &right, norm) / denom;
}

float uv_results[32][2];
static void compute_uv_light(float *u, float *v, PortalCell *p,
         int s, int i, int vc)
{
   compute_uv(u, v, &p->render_list[s],
      &p->plane_list[p->poly_list[s].planeid].norm->raw,
      &p->vpool[p->vertex_list[vc+i]], &p->vpool[p->vertex_list[vc]]);

   *u -= p->light_list[s].base_u / 4.0;
   *v -= p->light_list[s].base_v / 4.0;
}

void compute_uv_light_data(PortalCell *p, int s, int vc)
{
   int i, n = p->poly_list[s].num_vertices;

   for (i=0; i < n; ++i)
       compute_uv_light(&uv_results[i][0], &uv_results[i][1], p, s, i, vc);
}

void wr_alloc_light_map(PortalCell *p, int s, int vc)
{
   // compute (u,v) extents of this surface
   int n = p->poly_list[s].num_vertices,i;
   float um[2],vm[2],u,v;

   for (i=0; i < n; ++i) {
      compute_uv(&u, &v, &p->render_list[s], &p->plane_list[p->poly_list[s].planeid].norm->raw,
            &p->vpool[p->vertex_list[vc+i]], &p->vpool[p->vertex_list[vc]]);
                          // TODO: texture_anchor
      if (i == 0) {
         um[0] = um[1] = u;
         vm[0] = vm[1] = v;
      } else {
         if (u < um[0]) um[0] = u; else if (u > um[1]) um[1] = u;
         if (v < vm[0]) vm[0] = v; else if (v > vm[1]) vm[1] = v;
      }
   }

   // now these are in individual tmap extents.  so scale by the default tmap scale, divided by light map extent

   um[0] *= 64 / LM_STEP;
   um[1] *= 64 / LM_STEP;
   vm[0] *= 64 / LM_STEP;
   vm[1] *= 64 / LM_STEP;

   p->light_list[s].base_u = floor(um[0]+0.0001);
   p->light_list[s].base_v = floor(vm[0]+0.0001);

   p->light_list[s].w = ceil(um[1]-0.0001) - floor(um[0]+0.0001) + 1;
   p->light_list[s].h = ceil(vm[1]-0.0001) - floor(vm[0]+0.0001) + 1;

   p->light_list[s].row = p->light_list[s].w;
   n = p->light_list[s].w * p->light_list[s].h;
   if (p->light_list[s].bits != 0)
      Free(p->light_list[s].bits);
   p->light_list[s].bits = Malloc(n);
   p->light_list[s].dynamic = 0;
   p->light_list[s].decal = 0;

   if (p->light_list[s].bits)
      memset(p->light_list[s].bits, 128, n);
}

