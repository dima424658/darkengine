/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/pgroup.c,v 1.38 1998/10/31 14:31:34 CCAROLLO Exp $

#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <dev2d.h>
#include <g2.h>
#include <objpos.h>
#include <simtime.h>
#include <wrtype.h>
#include <objshape.h>
#include <objedit.h>
#include <parttype.h>
#include <pgrpprop.h>
#include <rendprop.h>
#include <scrnman.h>
#include <lgd3d.h>
#include <dbmem.h>
#include <rand.h>
#include <objmodel.h>
#include <palmgr.h>
#include <crjoint.h>
#include <mprintf.h>

#undef small

#ifndef SHIP
#define PARTICLE_COUNTS
#endif

#ifndef SHIP

int show_particle_counts;

#ifdef PARTICLE_COUNTS
int sim_count, render_count;
#define SHOW_PARTICLE_OBJS()   (show_particle_counts == 2)

void pgroup_show_counts(void)
{
   if (show_particle_counts)
      mprintf("render %d; sim %d\n", render_count, sim_count);
   sim_count = render_count = 0;
}
#else
void pgroup_show_counts(void)
{

}
#endif
#endif

extern BOOL g_lgd3d;

// general utility macro
// is this really the right condition?
#define USE_SCALE(pg)   ((pg)->spin_group)

////////////////////////////////////////////////////////////////////////
//
//  random numbers
//

static double RandFloat(void)
{
   int x = Rand();

   return (float) x / 0x8000;
}

static int MyRand(void)
{
   return ((Rand() << 22) ^ (Rand() << 11) ^ (Rand() >> 3)) & 0x7fffffff;
}

static fix RandRangeFix(fix x0, fix x1)
{
   if (x0 == x1) return x0;
   if (x0 < x1)
      return x0 + (MyRand() % (x1-x0));   
   else
      return x1 + (MyRand() % (x0-x1));   
}

static float MyRandRange(fix x0, fix x1)
{
   return fix_float(x0) + RandFloat()*fix_float(x1-x0);
}

////////////////////////////////////////////////////////////////////////
//
//  driver interface over callbacks
//

extern void _r3_concat_matrix(mxs_matrix *m, mxs_angvec *a,r3e_order o);

static BOOL should_sim(ParticleGroup *pg)
{
   // sim if visible
   if (pg->seen) return TRUE;

   // sim if forced
   if (pg->always_simulate) return TRUE;

   // if we're turned off... sounds bad, but normally 0 particles,
   // and it catches edge triggering effects
   if (!pg->active) return TRUE;

   // if we're turned on but incompletely launched (i.e. not pausible)
   if (pg->anim_type == PAT_LAUNCH_CONTINUOUS && pg->pl.n < pg->n)
      return TRUE;
   
   return FALSE;
}

static int sim_time(ParticleGroup *pg)
{
   // here are various sim time cases:
   //
   //   pausable group:
   //      use last frame time
   //   always simulate:
   //      doesn't matter, since both mean the same thing
   //   others:
   //      use last_sim_time

#if 0
   // these are forced by always_simulate
   if (pg->anim_type == PAT_LAUNCH_ONE_SHOT || pg->motion_type == PGMT_OBJECT_TRAILING) {
   }
#endif

   // check if it's pausable
   // we don't bother checking always_simulate, as noted above
   if (pg->active && pg->anim_type == PAT_LAUNCH_CONTINUOUS &&
                                                  pg->pl.n == pg->n) {
      pg->last_sim_time = GetSimTime();
      return GetSimFrameTime();
   } else {
      int time = GetSimTime();
      int delta = time - pg->last_sim_time;
      pg->last_sim_time = time;
      return delta;
   }
}

// do the processing for this particle group, based on
// whether we saw it last frame and its flags; return TRUE
// to delete the object
BOOL ParticleGroupSim(ObjID obj)
{
   BOOL result = FALSE;
   ParticleGroup *pg = ObjGetParticleGroup(obj);

   if (should_sim(pg)) {
      int time = sim_time(pg);
      float simtime = (float) time / SIM_TIME_SECOND;
      if (pg->pc.group_sim) {
         if (pg->always_simulate_group)
            pg->pc.group_sim(pg, (float) GetSimFrameTime() / SIM_TIME_SECOND);
         else
            pg->pc.group_sim(pg, simtime);
      }
      if (pg->pc.particle_sim) result = pg->pc.particle_sim(pg, simtime);
      pg->seen = FALSE;
   } else if (pg->always_simulate_group) {
      int time = GetSimFrameTime();
      float simtime = (float) time / SIM_TIME_SECOND;
      if (pg->pc.group_sim) pg->pc.group_sim(pg, simtime);
   }
   return result;
}

static void ComputeObjMat(ParticleGroup *pg)
{
   ObjPos *pos = ObjPosGet(pg->obj);
   if (!USE_SCALE(pg) || pos == NULL) 
      mx_identity_mat(&pg->obj_rot_mat);
   else
      _r3_concat_matrix(&pg->obj_rot_mat, &pos->fac, R3_DEFANG);
}

static void ParticleGroupStartObject(ParticleGroup *pg)
{
   ObjPos *pos = ObjPosGet(pg->obj);  // @TODO: what about when attached?
 
   if (!USE_SCALE(pg)) 
   {
      r3_start_object_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);
      mx_identity_mat(&pg->obj_rot_mat);
   } else {
      mxs_trans t;

      _r3_concat_matrix(&pg->obj_rot_mat, &pos->fac, R3_DEFANG);

      if (pg->motion_type == PGMT_FIREANDFORGET)
         mx_identity_mat(&t.mat);
      else
      {
         mx_copy_mat(&t.mat, &pg->obj_rot_mat);
         mx_scaleeq_mat(&t.mat, pg->cur_scale);
      }
      t.vec = pos->loc.vec;
      r3_start_object_trans(&t);
   }
}

static void ParticleGroupEndObject(ParticleGroup *pg)
{
   r3_end_object();
}

static ParticleGroup *temp_pg;
static int compare_z(const void *p, const void *q)
{
   int a = * (const int *) p;
   int b = * (const int *) q;

   if (temp_pg->points[a].p.z > temp_pg->points[b].p.z)
      return -1;
   return (temp_pg->points[a].p.z < temp_pg->points[b].p.z);
}

// sort each of the lists of points for this group
void ParticleGroupSortList(ParticleGroup *pg, int *list)
{
   Assert_(list != NULL);
   temp_pg = pg;
   qsort(list+1, list[0], sizeof(*list), compare_z);
}

// use the r3d to transform points from objectspace or worldspace
// as provided by one of the other routines; this is the default
// function for   pg->pc.particle_transform
void ParticleGroupTransformPoints(ParticleGroup *pg)
{
   // if there is a "convert to objectspace" or "convert to worldspace"
   // routine, call it.  if neither is defined, then assume 'location'
   // is already defined, and use the 'worldspace' flag to determine
   // which transformation is appropriate.

   if (!pg->pc.particle_objectspace) {
      if (pg->pc.particle_worldspace || pg->worldspace) {
         if (pg->pc.particle_worldspace)
            pg->pc.particle_worldspace(pg);   // update 'location'
         r3_start_block();
         if (pg->pl.n) r3_transform_block(pg->pl.n, pg->points, pg->pl.location);
         r3_end_block();
         return;
      }
      // fall through
   } else {
      pg->pc.particle_objectspace(pg);  // update 'location'
   }

   ParticleGroupStartObject(pg);
   r3_start_block();
   if (pg->pl.n)
      if (pg->shared_list)
         r3_transform_block(pg->pl.n, pg->points, *(mxs_vector **) pg->pl.location);
      else
         r3_transform_block(pg->pl.n, pg->points, pg->pl.location);
   r3_end_block();
   ParticleGroupEndObject(pg);
}

// because we might render as multiple "subclumps", there's
// a fair amount of common setup code.  if not for this possibility,
// we could just call one of the two rendering routines, and let
// them call the appropriate conversion to r3s_point or worldspace.
// however, we want to do that conversion once even if we render
// in several chunks, so we have to go through these intermidate functions,
// rather than letting the core render routine do the setup itself


void ParticleGroupRenderStart(ParticleGroup *pg)
{
   if (pg->pl.n)
      pg->points = Malloc(sizeof(r3s_point) * pg->pl.n);
   pg->pc.particle_transform(pg);
}

void ParticleGroupRenderEnd(ParticleGroup *pg)
{
   if (pg->points) {
      Free(pg->points);
      pg->points = NULL;
   }
   if (pg->free_locs && pg->pl.location) {
      Free(pg->pl.location);
      pg->pl.location = NULL;
   }
}

void ParticleGroupRenderList(ParticleGroup *pg, int *list)
{
   if (pg->zsort)
      ParticleGroupSortList(pg, list);
   pg->pc.particle_render(pg, list);
}

void ParticleGroupRenderAllSorted(ParticleGroup *pg)
{
   int *items = Malloc((pg->pl.n + 1) * sizeof(int));
   int i;
   items[0] = pg->pl.n;
   for (i=0; i < pg->pl.n; ++i)
      items[i+1] = i;
   ParticleGroupRenderList(pg, items);
   Free(items);
}

// a very simple entry point
void ParticleGroupRender(ObjID obj)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);
   if (pg == NULL) return;

   if (pg->launch == NULL)
   {
      if ((pg->launch = ObjGetParticleGroupLaunchInfo(obj)) == NULL)
      {
         Warning(("No launch info for %s?\n", ObjWarnName(obj)));
         return;
      }
   }

#ifdef PARTICLE_COUNTS
   render_count += pg->pl.n;
   if (SHOW_PARTICLE_OBJS())
      mprintf("render(%d,%d) ", obj, pg->pl.n);
#endif

   pg->seen = TRUE;
   ParticleGroupRenderStart(pg);
   if (pg->zsort) {
      // by default we don't build a list, so we have to do something special
      ParticleGroupRenderAllSorted(pg);
   } else {
      ParticleGroupRenderList(pg, NULL);
   }
   ParticleGroupRenderEnd(pg);
}

// render a group sorted around an object
// pass in the z-depth of the center of the object
// hmm, this ain't gonna work if the object splits
void ParticleGroupRenderAroundObjectStart(ObjID obj, float z)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);
   uchar need_flags;
   r3s_point *p;
   int *list;
   int i,j,k,n;

   Assert_(pg != NULL);
   pg->seen = TRUE;

   need_flags = pg->need_flags;

   pg->need_flags |= NEED_Z;      // make sure we get zs for sorting!
   ParticleGroupRenderStart(pg);
   pg->need_flags = need_flags;

   p = pg->points;
   n = pg->pl.n;

   // compute the lists of particles, leave room for each to store a count
   list = Malloc(sizeof(int) * (n + 2));

   j = k = 0;
   for (i=0; i < n; ++i,++p) {
      if (p->p.z < z)
         list[++j] = i;
      else
         list[n+2 - ++k] = i;
   }
   list[0] = j;
   list[j+1] = k;

   pg->sort_lists = Malloc(sizeof(int *) * 2);

   pg->sort_lists[0] = &list[0];
   pg->sort_lists[1] = &list[j+1];
}

void ParticleGroupRenderAroundObject(ObjID obj, BOOL in_front)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);
   ParticleGroupRenderList(pg, pg->sort_lists[in_front]);
}

void ParticleGroupRenderAroundObjectEnd(ObjID obj)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);
   Free(pg->sort_lists[0]);
   Free(pg->sort_lists);
   pg->sort_lists = NULL;
   ParticleGroupRenderEnd(pg);
}

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                     Rendering Implementations                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

static int row_table[1024];
static void set_row_table(void)
{
   int i;
   static int cur_row;
   if (grd_bm.row == cur_row) return;
   for (i=0; i < 1024; ++i)
      row_table[i] = grd_bm.row * i;
}

// test that they're not off the left edge
#define UNCLIPPED(sx,sy)   (((sx) | (sy)) >= 0)

static void RenderPixelFixedColor(ParticleGroup *pg, int *list)
{
   r3_start_block();
   r3_set_color(pg->render_datum);
   if (list) {
      int i,n = *list++;
      for (i=0; i < n; ++i)
         r3_draw_point(&pg->points[list[i]]);
   } else {
      r3s_point *p = pg->points;
      int i,n = pg->pl.n;
      for (i=0; i < n; ++i)
         r3_draw_point(p++);
   }
   r3_end_block();
}

static void RenderSquareFixedColor(ParticleGroup *pg, int *list)
{
   mxs_real hs = r3_get_hsize(1.0,pg->size);
   mxs_real vs = r3_get_vsize(1.0,pg->size);
   r3_start_block();
   r3_set_color(pg->render_datum);
   if (g_lgd3d) {
      r3s_point *p = pg->points;
      g2s_point ptlist[4], *vlist[4];
      int i,n = pg->pl.n;

      for (i=0; i < 4; ++i) {
         vlist[i] = &ptlist[i];
      }
      if (list) n = *list++;

      for (i=0; i < n; ++i, ++p) {
         float h,v;
         fix dx,dy;
         if (list) p = &pg->points[list[i]];
         if (p->ccodes) continue;

         h  = hs * p->grp.w;
         v  = vs * p->grp.w;
         dx = fix_from_float(h);
         dy = fix_from_float(v);

         if (dy < 0x8000) {
            if (dy < 0x4000 && pg->tiny_dropout) continue;
            dx = dy = 0x8000;
         }

         ptlist[0].sx = ptlist[3].sx = p->grp.sx - dx;
         ptlist[0].sy = ptlist[1].sy = p->grp.sy - dy;
         ptlist[1].sx = ptlist[2].sx = p->grp.sx + dx;
         ptlist[2].sy = ptlist[3].sy = p->grp.sy + dy;
         lgd3d_set_z(p->p.z);
         lgd3d_set_texture(NULL);

         lgd3d_g2poly(4, vlist);
      }
   } else {
      r3s_point *p = pg->points;
      int i,n = pg->pl.n;

      if (list) n = *list++;
      for (i=0; i < n; ++i, ++p) {
         float h,v;
         fix dx,dy;
         if (list) p = &pg->points[list[i]];
         if (p->ccodes) continue;

         h  = hs * p->grp.w;
         v  = vs * p->grp.w;
         dx = fix_from_float(h);
         dy = fix_from_float(v);

         if (dy < 0x8000) {
            if (dy < 0x4000 && pg->tiny_dropout) continue;
            dx = dy = 0x8000;
         }

         gd_rect(fix_int(p->grp.sx-dx),fix_int(p->grp.sy-dy),
                 fix_int(p->grp.sx+dx),fix_int(p->grp.sy+dy));
      }
   }

   // tell r3d it needs to refresh
   r3d_do_setup = TRUE;

   r3_end_block();
}

#define HACK_NUM   16
#define HACK_DIM   16
#define HACK_SZ    (HACK_DIM * HACK_DIM)

static grs_bitmap alpha_bm[HACK_NUM];
static void init_bm(void)
{
   int i;
   char *mem;
   static BOOL init=FALSE;
   if (init) return;
   mem = Malloc(HACK_NUM * HACK_SZ);

   for (i=0; i < HACK_NUM; ++i) {
      int x,y;
      gr_init_bitmap(&alpha_bm[i], mem, BMT_FLAT8, BMF_TLUC, HACK_DIM, HACK_DIM);
      for (y=0; y < HACK_DIM; ++y) {
         for (x=0; x < HACK_DIM; ++x) {
            // we want a decaying alpha based on distance from center,
            // using a nice smooth function... how about (1+cos)/2?
            float radius = sqrt((x-7.5)*(x-7.5) + (y-7.5)*(y-7.5)) / 7.5;
            if (radius > 1) radius = 1;
            // ranges smoothly from 0..1
            // we want 0..pi/2
            radius *= 3.141592/2;
            mem[y*HACK_DIM + x] = (int) ((i+0.5) * ((cos(radius))));
         }
      }
      mem += HACK_SZ;
   }
   init = TRUE;
}

static void compute_linear_color_func(float *base, float *delta, int pal0, int pal1, int t0, int t1)
{
   int i;
   uchar *pal = palmgr_get_pal(0);
   for (i=0; i < 3; ++i) {
      // delta = (pal1 - pal0) / (t1-t0)
      delta[i] = (pal[pal1*3+i] - pal[pal0*3+i]) / (float) (t1-t0);
      // base (intecept)
      // pal0 = base + slope*t0
      base[i] = pal[pal0*3+i] - t0*delta[i];
   }
    base[0] *= 0x10000;  // shift left by 16 to save later shifts
   delta[0] *= 0x10000;
    base[1] *= 0x100;    // shift left by 8 to save later shifts
   delta[1] *= 0x100;
}

static void RenderHardwareFadeMulticolor(ParticleGroup *pg, int *list, mxs_real hs, int n)
{
   r3s_point *p = pg->points;
   int time_1, time_2, time_0;
   int i;
   float color_base[2][3];
   float color_delta[2][3];

   lgd3d_disable_palette();

   time_0 = pg->launch->time_range[0];
   if (pg->cg && pg->cb) {
      time_1 = (unsigned) pg->launch->time_range[0] / 2;
      time_2 = 0;
      compute_linear_color_func(color_base[0], color_delta[0], pg->cr, pg->cg, time_0, time_1);
      compute_linear_color_func(color_base[1], color_delta[1], pg->cg, pg->cb, time_1, time_2);
   } else {
      time_1 = 0;
      time_2 = 0;
      compute_linear_color_func(color_base[0], color_delta[0], pg->cr, pg->cg, time_0, time_1);
   }

   for (i=0; i < n; ++i, ++p) {
      float h;
      fix tm;
      int c, which;

      if (list) {
         p = &pg->points[list[i]];
         tm = pg->pl.time_info[list[i]];
      } else
         tm = pg->pl.time_info[i];

      if (p->ccodes) continue;

      h  = max(1.1, hs * p->grp.w);

      if (tm < pg->fade_time)
         c = ((pg->ca) * tm / pg->fade_time) >> 4;
      else
         c = pg->ca >> 4;

      if (tm > time_0) tm = time_0;
      which = (tm < time_1) ? 1 : 0;
      {
         float col[3];
         col[0] = color_base[which][0] + color_delta[which][0]*tm;
         col[1] = color_base[which][1] + color_delta[which][1]*tm;
         col[2] = color_base[which][2] + color_delta[which][2]*tm;
         r3_set_color((((int) col[0])&0xff0000) + (((int) col[1])&0xff00) + (((int) col[2])&0xff));
      }

      lgd3d_hack_light_extra(p, h, &alpha_bm[c]);
   }

   lgd3d_enable_palette();
}

static void RenderHardwareFade(ParticleGroup *pg, int *list, mxs_real hs, int n)
{
   r3s_point *p = pg->points;
   int i;
   for (i=0; i < n; ++i, ++p) {
      float h;
      fix tm;
      int c;

      if (list) {
         p = &pg->points[list[i]];
         tm = pg->pl.time_info[list[i]];
      } else
         tm = pg->pl.time_info[i];

      if (p->ccodes) continue;

      h  = max(1.1, hs * p->grp.w);

      if (tm < pg->fade_time)
         c = ((pg->ca) * tm / pg->fade_time) >> 4;
      else
         c = pg->ca >> 4;
      lgd3d_hack_light_extra(p, h, &alpha_bm[c]);
   }
}

static void RenderHardwareNoFade(ParticleGroup *pg, int *list, mxs_real hs, int n)
{
   int i;
   r3s_point *p = pg->points;
   grs_bitmap *bm = &alpha_bm[pg->ca >> 4];
   for (i=0; i < n; ++i, ++p) {
      float h;
      if (list) p = &pg->points[list[i]];
      if (p->ccodes) continue;
      h  = max(1.1, hs * p->grp.w);
      lgd3d_hack_light_extra(p, h, bm);
   }
}

static void RenderSoftwareFadeMulticolor(ParticleGroup *pg, int *list, mxs_real hs, mxs_real vs, int n)
{
   int i;
   r3s_point *p = pg->points;
   // compute the three ranges
   int time_1, time_2;
   if (pg->cg && pg->cb) {
      time_2 = (unsigned) pg->launch->time_range[0] / 3;
      time_1 = time_2*2;
   } else {
      time_2 = pg->launch->time_range[0] / 2;
      time_1 = 0;
   }

   for (i=0; i < n; ++i, ++p) {
      float h,v;
      fix dx,dy;
      fix tm;
      int c;
      if (list) {
         p = &pg->points[list[i]];
         tm = pg->pl.time_info[list[i]];
      } else
         tm = pg->pl.time_info[i];

      if (p->ccodes) continue;

      h  = hs * p->grp.w;
      v  = vs * p->grp.w;
      dx = fix_from_float(h);
      dy = fix_from_float(v);

      if (dy < 0x10000) {
         if (dy < 0x4000 && pg->tiny_dropout) continue;
         dx = dy = 0x10000;
      }

      // compute which of the three phases we're in
      // 

      if (tm < pg->fade_time)
         c = ((pg->ca) * tm / pg->fade_time) >> 5;
      else
         c = pg->ca >> 5;

      if (tm < time_2) 
         gr_set_fcolor((pg->render_datum >> 16) + c);
      else if (tm < time_1) 
         gr_set_fcolor(((pg->render_datum >> 8) & 255) + c);
      else
         gr_set_fcolor((pg->render_datum & 255) + c);

      if (dy < 0x10000) {
         if (dy < 0x4000 && pg->tiny_dropout) continue;

         dx = dy = 0x8000;

         gd_rect(fix_int(p->grp.sx-dx),fix_int(p->grp.sy-dy),
                 fix_int(p->grp.sx+dx),fix_int(p->grp.sy+dy));
      }
      else
         g2_oval(p->grp.sx, p->grp.sy, dx, dy);
   }
}

static void RenderSoftwareFade(ParticleGroup *pg, int *list, mxs_real hs, mxs_real vs, int n)
{
   int i;
   r3s_point *p = pg->points;
   for (i=0; i < n; ++i, ++p) {
      float h,v;
      fix dx,dy;
      fix tm;
      int c;
      if (list) {
         p = &pg->points[list[i]];
         tm = pg->pl.time_info[list[i]];
      } else
         tm = pg->pl.time_info[i];

      if (p->ccodes) continue;

      h  = hs * p->grp.w;
      v  = vs * p->grp.w;
      dx = fix_from_float(h);
      dy = fix_from_float(v);

      if (dy < 0x10000) {
         if (dy < 0x4000 && pg->tiny_dropout) continue;
         dx = dy = 0x10000;
      }

      if (tm < pg->fade_time)
         c = ((pg->ca) * tm / pg->fade_time) >> 5;
      else
         c = pg->ca >> 5;

      gr_set_fcolor(pg->render_datum + c);

      if (dy < 0x10000) {
         if (dy < 0x4000 && pg->tiny_dropout) continue;

         dx = dy = 0x8000;

         gd_rect(fix_int(p->grp.sx-dx),fix_int(p->grp.sy-dy),
                 fix_int(p->grp.sx+dx),fix_int(p->grp.sy+dy));
      }
      else
         g2_oval(p->grp.sx, p->grp.sy, dx, dy);
   }
}

static void RenderSoftwareNoFade(ParticleGroup *pg, int *list, mxs_real hs, mxs_real vs, int n)
{
   int i;
   r3s_point *p = pg->points;
   gr_set_fcolor(pg->render_datum + (pg->ca >> 5));
   for (i=0; i < n; ++i, ++p) {
      float h,v;
      fix dx,dy;
      if (list) p = &pg->points[list[i]];
      if (p->ccodes) continue;

      h  = hs * p->grp.w;
      v  = vs * p->grp.w;
      dx = fix_from_float(h);
      dy = fix_from_float(v);

      if (dy < 0x10000) {
         if (dy < 0x4000 && pg->tiny_dropout) continue;

         dx = dy = 0x8000;

         gd_rect(fix_int(p->grp.sx-dx),fix_int(p->grp.sy-dy),
                 fix_int(p->grp.sx+dx),fix_int(p->grp.sy+dy));
      } else
         g2_oval(p->grp.sx, p->grp.sy, dx, dy);
   }
}

static void RenderDiskFixedColor(ParticleGroup *pg, int *list)
{
   mxs_real hs = r3_get_hsize(1.0,pg->size);
   mxs_real vs = r3_get_vsize(1.0,pg->size);
   r3_start_block();
   r3_set_color(pg->render_datum);
   if (g_lgd3d) {
      int n = pg->pl.n;

      init_bm();

      if (list) n = *list++;

      if (pg->pl.time_info && pg->fade_time)
         if (pg->cg || pg->cb)
            RenderHardwareFadeMulticolor(pg, list, hs, n);
         else
            RenderHardwareFade(pg, list, hs, n);
      else
         RenderHardwareNoFade(pg, list, hs, n);

   } else {
      int n = pg->pl.n;

      hs *= 0.5;
      vs *= 0.5;

      if (list) n = *list++;
      gr_set_fill_type(FILL_BLEND);

      if (pg->pl.time_info && pg->fade_time)
         if (pg->cg || pg->cb)
            RenderSoftwareFadeMulticolor(pg, list, hs, vs, n);
          else
            RenderSoftwareFade(pg, list, hs, vs, n);
      else
         RenderSoftwareNoFade(pg, list, hs, vs, n);

      gr_set_fill_type(FILL_NORM);
   }

   // tell r3d it needs to refresh
   r3d_do_setup = TRUE;

   r3_end_block();
}

static void RenderBitmap(ParticleGroup *pg, int *list)
{
   bool multiframe;
   int k,f=0;
   mxs_real hs = r3_get_hsize(1.0,pg->size);
   mxs_real vs = r3_get_vsize(1.0,pg->size);
   r3s_texture tex;

   r3_start_block();

   if (pg->modelnum == -1)
   {
      Warning(("No modelnum on particle obj %d?\n", pg->obj));
      return;
   }

   k = objmodelBitmapNumFrames(pg->modelnum);
   tex = objmodelSetupBitmapTexture(pg->modelnum, 0);
   vs = vs * tex->h / tex->w;  // maintain dimensions of bitmap
   objmodelReleaseBitmapTexture(pg->modelnum, 0);

   multiframe = (k > 1);

   if (!multiframe)
   {
      tex = objmodelSetupBitmapTexture(pg->modelnum, 0);
      if (g_lgd3d)
         lgd3d_set_texture(tex);
   } 
         
   if (g_lgd3d) {
      r3s_point *p = pg->points;
      g2s_point ptlist[4], *vlist[4];
      int i,n = pg->pl.n;

      for (i=0; i < 4; ++i) {
         vlist[i] = &ptlist[i];
      }
      if (list) n = *list++;

      ptlist[0].u = ptlist[3].u = 0;
      ptlist[1].u = ptlist[2].u = 1.0;

      ptlist[0].v = ptlist[1].v = 0;
      ptlist[2].v = ptlist[3].v = 1.0;

      ptlist[0].i = ptlist[1].i = ptlist[2].i = ptlist[3].i = 1.0;

      for (i=0; i < n; ++i, ++p, ++f == k ? f=0 : 0) {
         float h,v;
         float c;
         fix dx,dy;
         fix tm;
         
         if (multiframe)
         {
            if (++f == k)
               f = 0;
         }
         else
            f = 0;

         if (list) { p = &pg->points[list[i]]; f = list[i] % k; }
         if (p->ccodes) continue;

         h  = hs * p->grp.w;
         v  = vs * p->grp.w;
         dx = fix_from_float(h);
         dy = fix_from_float(v);

         if (dy < 0x10000) {
            if (dy < 0x4000 && pg->tiny_dropout) continue;
            dx = dy = 0x10000;
         }

         ptlist[0].sx = ptlist[3].sx = p->grp.sx - dx;
         ptlist[0].sy = ptlist[1].sy = p->grp.sy - dy;
         ptlist[1].sx = ptlist[2].sx = p->grp.sx + dx;
         ptlist[2].sy = ptlist[3].sy = p->grp.sy + dy;
         lgd3d_set_z(p->p.z);

         if (multiframe)
         {
            tex = objmodelSetupBitmapTexture(pg->modelnum, f);
            lgd3d_set_texture(tex);
         }

         if (pg->pl.time_info && ((tm = pg->pl.time_info[i]) < pg->fade_time))
         {
            c = (float)((pg->ca) * tm / pg->fade_time) / 256.0;
            if (c == 0)
               c = 0.001;
         }
         else
            c = (float)(pg->ca) / 256.0;

         if (c > 0)
            lgd3d_set_alpha(c);
         
         lgd3d_g2trifan(4, vlist);

         if (multiframe)
            objmodelReleaseBitmapTexture(pg->modelnum, f);
      }

      lgd3d_set_alpha(1.0);
   } else {
      r3s_point *p = pg->points;
      int i,n = pg->pl.n;

      if (list) n = *list++;
      for (i=0; i < n; ++i, ++p, ++f == k ? f=0 : 0) {
         float h,v;
         fix dx,dy;
         if (list) { p = &pg->points[list[i]]; f = list[i] % k; }
         if (p->ccodes) continue;

         h  = hs * p->grp.w;
         v  = vs * p->grp.w;
         dx = fix_from_float(h);
         dy = fix_from_float(v);

         if (dy < 0x10000) {
            if (dy < 0x4000 && pg->tiny_dropout) continue;
            dx = dy = 0x10000;
         }

         if (multiframe)
            tex = objmodelSetupBitmapTexture(pg->modelnum, f);

         gr_scale_bitmap(tex,
                 fix_int(p->grp.sx-dx),fix_int(p->grp.sy-dy),
                 fix_int(2*dx),fix_int(2*dy));

         if (multiframe)
            objmodelReleaseBitmapTexture(pg->modelnum, f);
      }
   }

   if (!multiframe)
      objmodelReleaseBitmapTexture(pg->modelnum, 0);

   // tell r3d it needs to refresh
   r3d_do_setup = TRUE;

   r3_end_block();
}


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                    Simulation Implementations                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

static void pgroup_basic_sim(ParticleGroup *pg, float simtime)
{

   // all of this code is going to have precision problems
   // as time advances... switch it to integers?

   if (pg->spin_group) {
      // This is wrong... we're not supposed to stuff Pos directly!
      ObjPos *pos = ObjPosGet(pg->obj);
      mxs_vector addend;
      mx_scale_vec(&addend, &pg->spin, simtime);
      pos->fac.tx += addend.x * 256;   // float to sfix
      pos->fac.ty += addend.y * 256;
      pos->fac.tz += addend.z * 256;

      if (pg->scale_vel) {
         pg->fixed_scale += pg->scale_vel * simtime;
         if (pg->fixed_scale < 0)
            pg->fixed_scale = 0;
      }

      // compute current scale for pulsing
      if (pg->pulse_percentage != 1.0 && pg->pulse_period != 0) {
         int cur_time  = GetSimTime() - pg->ms_offset + pg->pulse_period;
         // convert into number from 0..1
         float cur_offset = (float) cur_time / pg->pulse_period;
         cur_offset = sin(3.141592*2*cur_offset);
         cur_offset = (cur_offset + 1)/2 * pg->pulse_percentage;
         pg->cur_scale = pg->fixed_scale * (1 - cur_offset);
      } else
         pg->cur_scale = pg->fixed_scale;
   }
}

static void pgroup_velocity_sim(ParticleGroup *pg, float simtime)
{
   Position *pos = ObjPosGet(pg->obj);
   pgroup_basic_sim(pg, simtime);
   if (pos != NULL) {
      mx_scale_addeq_vec(&pos->loc.vec, &pg->velocity, simtime);
      if (pg->motion_type == PGMT_VELOCITY_GRAVITY) {
         mx_scale_addeq_vec(&pg->velocity, &pg->gravity, simtime);
         mx_scale_addeq_vec(&pos->loc.vec, &pg->gravity, simtime*simtime*0.5);
      }
      UpdateChangedLocation(&pos->loc);
      // we really need ObjPosTouch();
      ObjPosSetLocation(pg->obj, &pos->loc);
   }
}

static void pgroup_attached_sim(ParticleGroup *pg, float simtime)
{
   // attachment stuff now happens in objpos listener

   pgroup_basic_sim(pg, simtime);
}

static mxs_vector cur_loc;
static float cur_frame_time;

static void pgroup_trailing_sim(ParticleGroup *pg, float simtime)
{
   ObjPos *pos = ObjPosGet(pg->obj);
   if (pos)
      cur_loc = pos->loc.vec;
   pgroup_attached_sim(pg, simtime);
   if (pos)
      pg->prev_loc = cur_loc;
}

static void determine_launch_loc(ParticleGroup *pg, mxs_vector *launch_pt)
{
   switch (pg->launch->type)
   {
      case PLT_BBOX:
      {
         launch_pt->x = MyRandRange(pg->launch->start_loc_bbox[0][0], pg->launch->start_loc_bbox[1][0]);
         launch_pt->y = MyRandRange(pg->launch->start_loc_bbox[0][1], pg->launch->start_loc_bbox[1][1]);
         launch_pt->z = MyRandRange(pg->launch->start_loc_bbox[0][2], pg->launch->start_loc_bbox[1][2]);

         break;
      }

      case PLT_SPHERE:
      {
         mxs_vector launch_dir;

         launch_dir.x = MyRandRange(-65536, 65536);
         launch_dir.y = MyRandRange(-65536, 65536);
         launch_dir.z = MyRandRange(-65536, 65536);

         if ((launch_dir.x != 0.0) && (launch_dir.y != 0.0) && (launch_dir.z != 0.0))
            mx_normeq_vec(&launch_dir);

         mx_scale_vec(launch_pt, &launch_dir, MyRandRange(pg->launch->radius[0], pg->launch->radius[1]));

         break;
      }

      case PLT_CYLINDER:
      {
         mxs_vector launch_dir;

         launch_dir.x = MyRandRange(-65536, 65536);
         launch_dir.y = MyRandRange(-65536, 65536);
         launch_dir.z = 0.0;

         if ((launch_dir.x != 0.0) && (launch_dir.y != 0.0))
            mx_normeq_vec(&launch_dir);

         mx_scale_vec(launch_pt, &launch_dir, MyRandRange(pg->launch->radius[0], pg->launch->radius[1]));

         launch_pt->z = MyRandRange(pg->launch->start_loc_bbox[0][2], pg->launch->start_loc_bbox[1][2]);

         break;
      }
   }
}

static void launch_particle(ParticleGroup *pg, int index, fix time_past)
{
   Assert_((unsigned) index < pg->list_length);

   determine_launch_loc(pg, &pg->pl.location[index]);

   if (USE_SCALE(pg))
      mx_scaleeq_vec(&pg->pl.location[index], pg->cur_scale);

   if (!pg->launch->loc_unrotated) {
      // rotate by object orientation
      mxs_vector temp_loc;

      mx_mat_mul_vec(&temp_loc, &pg->obj_rot_mat, &pg->pl.location[index]);
      mx_copy_vec(&pg->pl.location[index], &temp_loc);
   }

   if (pg->motion_type == PGMT_OBJECT_TRAILING) {
      // look at the last two positions, and interpolate
      // back according to time passing
      mxs_vector start_pos;
      mx_interpolate_vec(&start_pos, &cur_loc, &pg->prev_loc,
              fix_float(time_past) / cur_frame_time);
      // note that we could get  time_past > cur_frame_time, but that's ok
      mx_addeq_vec(&pg->pl.location[index], &start_pos);
   }

   // compute velocity
   pg->pl.velocity[index].x = MyRandRange(
          pg->launch->start_vel_bbox[0][0], pg->launch->start_vel_bbox[1][0]);
   pg->pl.velocity[index].y = MyRandRange(
          pg->launch->start_vel_bbox[0][1], pg->launch->start_vel_bbox[1][1]);
   pg->pl.velocity[index].z = MyRandRange(
          pg->launch->start_vel_bbox[0][2], pg->launch->start_vel_bbox[1][2]);

   if (!pg->launch->vel_unrotated) {
      // rotate by object orientation
      mxs_vector temp_vel;

      mx_mat_mul_vec(&temp_vel, &pg->obj_rot_mat, &pg->pl.velocity[index]);
      mx_copy_vec(&pg->pl.velocity[index], &temp_vel);
   }

   Assert_(pg->launch->time_range[0] > 0);
   Assert_(pg->launch->time_range[0] > 0);

   // compute time info
   pg->pl.time_info[index] = RandRangeFix(
          pg->launch->time_range[0], pg->launch->time_range[1]);
}

static void delete_particle(ParticleGroup *pg, int index)
{
   int num = pg->pl.n-1;

   Assert_((unsigned) index < pg->pl.n);
   if (pg->pl.location)
      pg->pl.location[index] = pg->pl.location[num];
   if (pg->pl.velocity)
      pg->pl.velocity[index] = pg->pl.velocity[num];
   if (pg->pl.time_info)
      pg->pl.time_info[index] = pg->pl.time_info[num];
   if (pg->pl.render_data)
      pg->pl.render_data[index] = pg->pl.render_data[num];

   pg->pl.n = num;
   ++pg->delete_count;
}

static void update_particles(ParticleGroup *pg, int index, int num, fix time)
{
   BOOL particle_gravity = (pg->motion_type != PGMT_VELOCITY_GRAVITY);
   int i;

#ifdef PARTICLE_COUNTS
   sim_count += num;
#endif

   num = index + num;

   for (i=index; i < num; ) {
      fix time_left = time;
      Assert_((unsigned) i < pg->pl.n);
      if (pg->pl.time_info[i] < time_left) {
         // if the time for this particle has expired...
         if (pg->anim_type == PAT_LAUNCH_CONTINUOUS && pg->launching && pg->active) {
            // compute time left for it to have relaunched
            time_left = time - pg->pl.time_info[i];
            // if a lot of time passed, don't sit in a loop
            // launching particles, just decrement the time left to sim
            if (pg->launch->time_range[0] > 0)
            {
               time_left %= pg->launch->time_range[0];
               launch_particle(pg, i, time_left);
            }
         } else {
            delete_particle(pg, i);
            --num;
            continue;  // don't increment i!
         }
      }
      // ok, now simulate the particle for time_left time

      pg->pl.time_info[i] -= time_left;
      if (pg->pl.time_info[i] < 0)
      {
         time_left -= pg->pl.time_info[i];
         pg->pl.time_info[0] = 0;
      }

      {
         float time = fix_float(time_left);
         mx_scale_addeq_vec(&pg->pl.location[i], &pg->pl.velocity[i], time);
         if (particle_gravity) {
            mx_scale_addeq_vec(&pg->pl.velocity[i], &pg->gravity, time);
            mx_scale_addeq_vec(&pg->pl.location[i], &pg->gravity,
                               time*time*0.5);      // d = 1/2at^2 + vt
         }
      }
      ++i;
   }
}

#define DEFAULT_LIST_LENGTH    16

static void grow_lists(ParticleGroup *pg, int new_len);
static void do_launching(ParticleGroup *pg, fix time)
{
   while (time > pg->next_launch) {
      int k = pg->pl.n++;
      // make sure the list is long enough
      if (pg->list_length <= k) {
         int len = pg->list_length * 2;
         if (len == 0) len = DEFAULT_LIST_LENGTH;
         if (len > pg->n) len = pg->n;
         grow_lists(pg, len);
      }
      // how far back in the past? time - pg->next_launch
      launch_particle(pg, k, time - pg->next_launch);
      
      if (pg->anim_type == PAT_LAUNCH_CONTINUOUS)
      {
         if (pg->delete_count > 0)
            pg->delete_count--;
      }
      // now integrate them up until *now*
      update_particles(pg, k, 1, time - pg->next_launch);

      if (pg->pl.n+pg->delete_count == pg->n) 
      {
         if (pg->anim_type == PAT_LAUNCH_ONE_SHOT)
            pg->launching = FALSE;
         return; // no more launching to do!
      }

      time -= pg->next_launch;
      pg->next_launch = pg->launch_period;
   }
   pg->next_launch -= time;
}

static void compute_current_minmax(ParticleGroup *pg);
static BOOL particle_launch_sim(ParticleGroup *pg, float time)
{
   BOOL expired;
   fix fix_time = fix_from_float(time);

   Assert_(pg->pl.n <= pg->n);

   cur_frame_time = time;

   ComputeObjMat(pg);

#ifdef PARTICLE_COUNTS
   if (SHOW_PARTICLE_OBJS())
      mprintf("sim(%d,%d) ", pg->obj, pg->pl.n);
#endif

   update_particles(pg, 0, pg->pl.n, fix_time);

   // if we haven't got all particles launched, we might need more
   if (pg->active && 
       ((pg->pl.n+pg->delete_count != pg->n) ||
        ((pg->anim_type == PAT_LAUNCH_CONTINUOUS) && (pg->pl.n < pg->n))))
   {
      // we could be in the startup of a continuous not-prelaunch,
      // or we could be in the middle of a one-shot
      expired = FALSE;
      if (pg->launching)
         do_launching(pg, fix_time);
      else
         expired = (pg->pl.n == 0);
   } else {

      // we might just be turned inactive
      if (pg->anim_type == PAT_LAUNCH_CONTINUOUS)
         expired = FALSE;
      else
      {
         // if we have gotten all particles launched, we might be done
         expired = (pg->delete_count == pg->n);
      }
   }

   if (!expired && pg->worldspace)
      compute_current_minmax(pg);

   return expired;
}

/////////////////////////////////////////////////////////////////////////

static void compute_launch_minmax(ParticleGroup *pg)
{
   BOOL particle_gravity = (pg->motion_type != PGMT_VELOCITY_GRAVITY);
   int i;
   // for each axis
   for (i=0; i < 3; ++i) {
      // grab the minimum and maximum locations
      float a,b, da,db, g, a2,b2, t, small,large;

      switch (pg->launch->type)
      {
         case PLT_BBOX:
         {
            a  = fix_float(pg->launch->start_loc_bbox[0][i]);
            b  = fix_float(pg->launch->start_loc_bbox[1][i]);

            break;
         }

         case PLT_CYLINDER:
         {
            if (i == 2) {
               a  = fix_float(pg->launch->start_loc_bbox[0][i]);
               b  = fix_float(pg->launch->start_loc_bbox[1][i]);
            } else {
               a = fix_float(-pg->launch->radius[1]);
               b = fix_float( pg->launch->radius[1]);
            }
            break;
         }

         case PLT_SPHERE:
         {
            a = fix_float(-pg->launch->radius[1]);
            b = fix_float( pg->launch->radius[1]);

            break;
         }
      }

      da = fix_float(pg->launch->start_vel_bbox[0][i]);
      db = fix_float(pg->launch->start_vel_bbox[1][i]);

      g = particle_gravity ? pg->gravity.el[i] : 0;
      // now, seed the min and max locations
#define BBOX_MIN 0.01
      small = a < -BBOX_MIN ? a : -BBOX_MIN;
      large = b >  BBOX_MIN ? b :  BBOX_MIN;

      // now find the endpoint of the trajectories with gravity
      t = fix_float(pg->launch->time_range[1]);

      a2 = a + da*t + 0.5*t*t*g;
      b2 = b + db*t + 0.5*t*t*g;

      if (a2 < small) small = a2;
      if (b2 > large) large = b2;

      // now find the critical points along the trajectory
      if (g < 0) {
         if (db > 0) {
            float tm = -db / g;
            if (tm < t) {
               b2 = b + db*tm + 0.5*tm*tm*g;
               if (b2 > large) large = b2;
            }
         }
      } else if (g > 0) {
         if (da < 0) {
            float tm = -da / g;
            if (tm < t) {
               a2 = b + da*tm + 0.5*tm*tm*g;
               if (a2 < small) small = a2;
            }
         }
      }
      pg->bmin.el[i] = small;
      pg->bmax.el[i] = large;
   }

   // compute the radius as the max of any one axis (not really right)
   {
      float rad;
      pg->radius = fabs(pg->bmin.x);
      rad = fabs(pg->bmin.y);
      if (rad > pg->radius) pg->radius = rad;
      rad = fabs(pg->bmin.z);
      if (rad > pg->radius) pg->radius = rad;
      rad = fabs(pg->bmax.x);
      if (rad > pg->radius) pg->radius = rad;
      rad = fabs(pg->bmax.y);
      if (rad > pg->radius) pg->radius = rad;
      rad = fabs(pg->bmax.z);
      if (rad > pg->radius) pg->radius = rad;
   }
}
   
static void compute_current_minmax(ParticleGroup *pg)
{
   int i;
   mxs_vector *mn, *mx, *cur;

   mx_mk_vec(&pg->bmin,-.01,-.01,-.01);
   mx_mk_vec(&pg->bmax, .01, .01, .01);

   mn = &pg->bmin;
   mx = &pg->bmax;

   for (i=0; i < pg->pl.n; ++i) {
      cur = &pg->pl.location[i];
      if (cur->x < mn->x) mn->x = cur->x;
      if (cur->y < mn->y) mn->y = cur->y;
      if (cur->z < mn->z) mn->z = cur->z;
      if (cur->x > mx->x) mx->x = cur->x;
      if (cur->y > mx->y) mx->y = cur->y;
      if (cur->z > mx->z) mx->z = cur->z;
   }
}

/////////////////////////////////////////////////////////////////////////

// shared point lists

static mxs_vector *shared_list[PAT_NUM_USER_EDITTABLE];
static int shared_list_len[PAT_NUM_USER_EDITTABLE];

void initialize_list(int index, int start, int end)
{
   int i;
   float temp;

   for (i=start; i < end; ++i) {
      mxs_vector *loc = &shared_list[index][i];
      switch((enum ParticleAnimationType) index) {
         case PAT_FIXED_CUBE_SHELL:
         case PAT_FIXED_DISK:
         case PAT_FIXED_CIRCLE:
         case PAT_FIXED_PYRAMID:

         case PAT_FIXED_SPHERE_SHELL:
            // this is weird, but graphics gem says it's right
            loc->x = RandFloat() * 2 - 1;
            temp = RandFloat() * 2 * 3.141592; // compute angle
            loc->y = sqrt(1 - loc->x * loc->x) * cos(temp);
            loc->z = sqrt(1 - loc->x * loc->x) * sin(temp);
            break;
            
         case PAT_FIXED_CUBE:
            loc->x = RandFloat() * 2 - 1;
            loc->y = RandFloat() * 2 - 1;
            loc->z = RandFloat() * 2 - 1;
            break;

         case PAT_FIXED_SPHERE:
            do {
               loc->x = RandFloat() * 2 - 1;
               loc->y = RandFloat() * 2 - 1;
               loc->z = RandFloat() * 2 - 1;
            } while (loc->x*loc->x + loc->y*loc->y + loc->z*loc->z > 1);
            break;
      }
   }
}

mxs_vector *build_shared_list(enum ParticleAnimationType type, int n)
{
   int index = (int) type;
   if (n > shared_list_len[index]) {
      int new_len = shared_list_len[index];
      if (!new_len)
         new_len = 1;
      while (new_len < n)
         new_len += new_len;

      if (!shared_list[index])
         shared_list[index] = Malloc(sizeof(mxs_vector) * new_len);
      else
         shared_list[index] = Realloc(shared_list[index],
                                           sizeof(mxs_vector) * new_len);

      initialize_list(index, shared_list_len[index], new_len);
      shared_list_len[index] = new_len;
   }
   // @HACK: return the pointer to the pointer to the list, not the
   // list, because it gets moved around by Realloc
   return (mxs_vector *) &(shared_list[index]);
}

/////////////////////////////////////////////////////////////////////////

// Initial particle group setup

#define SAFE_FREE(x)   if ((x) != NULL) Free(x); else

static void free_lists(ParticleGroup *pg)
{
   if (!pg->shared_list) {
      SAFE_FREE(pg->pl.location);
      SAFE_FREE(pg->pl.velocity);
      SAFE_FREE(pg->pl.time_info);
      SAFE_FREE(pg->pl.cell);
      pg->pl.n = 0;
      pg->list_length = 0;
   }
   pg->pl.location = NULL;
   pg->pl.velocity = NULL;
   pg->pl.time_info = NULL;
   pg->pl.cell = NULL;
   pg->free_locs = FALSE;
   pg->sort_lists = NULL;
}

#define ALLOCATE(a,b)  (a) = Malloc(sizeof((a)[0]) * (b))

static void allocate_lists(ParticleGroup *pg, int n)
{
   if (!pg->shared_list) {
      pg->list_length = n;
      ALLOCATE(pg->pl.location, pg->list_length);
      ALLOCATE(pg->pl.velocity, pg->list_length);
      ALLOCATE(pg->pl.time_info, pg->list_length);
      pg->pl.cell = NULL;
   }
}

#define REALLOC(a,b)   (a) = Realloc((a), sizeof((a)[0]) * (b))

static void grow_lists(ParticleGroup *pg, int new_len)
{
   Assert_(new_len <= pg->n);
   Assert_(new_len > pg->list_length);

   pg->list_length = new_len;
   REALLOC(pg->pl.location, pg->list_length);
   REALLOC(pg->pl.velocity, pg->list_length);
   REALLOC(pg->pl.time_info, pg->list_length);
}

void setup_initial_particle_lists(ParticleGroup *pg)
{
   ComputeObjMat(pg);

   if (pg->shared_list) {
      pg->pl.location = build_shared_list(pg->anim_type, pg->n); 
      pg->pl.n = pg->n;
   } else if (!pg->pre_launch) {
      pg->pl.n = 0;
   } else {
      int i;
      ObjPos *p = ObjPosGet(pg->obj);
      pg->pl.n = pg->n;
      Assert_(pg->pl.n <= pg->list_length);

      if (p == NULL)
         mx_mk_vec(&cur_loc,0,0,0);
      else
         cur_loc = p->loc.vec;

      pg->prev_loc = cur_loc;

      for (i=0; i < pg->n; ++i) {
         launch_particle(pg, i, 0);
         // now force the just-launched particle to sim for some fraction
         // of its lifetime
         update_particles(pg, i, 1, pg->pl.time_info[i] * RandFloat());
      }
   }
   
   if (pg->worldspace)
      compute_current_minmax(pg);
}

/////////////////////////////////////////////////////////////////////////

// this should only be called at shutdown, because individual particle groups
// have pointers into the array that are being left dangling.
void free_shared_lists()
{
   int index;

   for (index=0; index<PAT_NUM_USER_EDITTABLE; index++)
   {
      SAFE_FREE(shared_list[index]);
      shared_list_len[index] = 0;
   }
}

void ParticleGroupCleanup()
{
   free_shared_lists();
}

/////////////////////////////////////////////////////////////////////////

// respond to messages about the property being updated / deleted

void particle_group_delete(ObjID obj)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);
   free_lists(pg);
}

// note--product of below two numbers must be <= 255
#define NUM_COLORS 31
#define NUM_ALPHA  8

#define MAP_COLOR_INDEX_TO_TLUC8TAB(x)    ((x)*NUM_ALPHA + 1)

uchar  *particle_tluc8;
ushort *particle_tluc16;
static uchar tluc_colors[NUM_COLORS];
static int num_alpha_colors=0;

void free_old_tluc_tables(void)
{
   if (particle_tluc8)  Free(particle_tluc8);
   if (particle_tluc16) Free(particle_tluc16);
   particle_tluc8 = NULL;
   particle_tluc16 = NULL;
}

static BOOL gRenderMode = FALSE; 

void ParticleGroupUpdateModeStart(void)
{
   num_alpha_colors = 0;
   free_old_tluc_tables();
   if (!g_lgd3d) 
   {
      if (grd_bm.type == BMT_FLAT8) {
         particle_tluc8 = Malloc(NUM_COLORS * NUM_ALPHA * 256);
      } else {
         particle_tluc16 = Malloc(NUM_COLORS*NUM_ALPHA*256*sizeof(short) * 2);
         // two tables indexed by 256 values, each with sizeof(short) output
      }
   }
   gRenderMode = TRUE; 
}

void ParticleGroupUpdateModeFinish(void)
{
   gRenderMode = FALSE; 
}


static int get_tluc_color(int color)
{
   int n;
   // ok, this is moronic, but we don't get the very
   // initial mode setting, duh

   if (!particle_tluc8 && !particle_tluc16)
      ParticleGroupUpdateModeStart();

   for (n=0; n < num_alpha_colors; ++n)
      if (tluc_colors[n] == color)
         return MAP_COLOR_INDEX_TO_TLUC8TAB(n);

   if (num_alpha_colors == NUM_COLORS) {
      Warning(("Maximum number of unique alpha'd colors exceeded.\n"));
      return MAP_COLOR_INDEX_TO_TLUC8TAB(0);
   }

   n = num_alpha_colors++;
   tluc_colors[n] = color;

   // ok, now prepare the tables themselves
   if (grd_bm.type == BMT_FLAT8) {
      grs_rgb rgb;
      int i;
      uchar *ipal = palmgr_get_ipal(0), *dest;
      rgb = gr_bind_rgb(grd_pal[color*3], grd_pal[color*3+1], grd_pal[color*3+2]);
      grd_ipal = ipal;
      dest = particle_tluc8 + n*NUM_ALPHA*256;
      for (i=0; i < NUM_ALPHA; ++i) {
         fix opacity; // use 1,3,5,7,9,11,13,15 out of 0..16
         opacity = (i*2+1)*FIX_UNIT/(NUM_ALPHA*2);
         gr_init_simple_translucency_table(dest, opacity, rgb, grd_pal);
         tluc8tab[MAP_COLOR_INDEX_TO_TLUC8TAB(n)+i] = dest;
         dest += 256;
      }
   } else {
      int i;
      ushort *dest = particle_tluc16 + n*NUM_ALPHA*512;
      int bitdepth;
      {
         grs_rgb_bitmask bmask;
         gr_get_screen_rgb_bitmask(&bmask);
         bitdepth = (bmask.green == 0x3e0 ? BMF_RGB_555 : BMF_RGB_565);
      }
      if (grd_pal16_list && grd_pal16_list[0])
         color = grd_pal16_list[0][color];
      else if (grd_pal16)
         color = grd_pal16[color];
      else
         color = (15 << 11) | 31;  // pinkish

      for (i=0; i < NUM_ALPHA; ++i) {
         float opacity = (float) (i*2+1)/(NUM_ALPHA*2);
         tluc8tab[MAP_COLOR_INDEX_TO_TLUC8TAB(n)+i] = (uchar *) dest;
         gr_init_clut16(dest, color, opacity, bitdepth);
         dest += 512;
      }
   }

   return MAP_COLOR_INDEX_TO_TLUC8TAB(n);
}

void particle_group_update_mode_change(ObjID obj)
{
   BOOL alpha;
   ParticleGroup *pg; 

   if (!gRenderMode)
      return ;

   pg = ObjGetParticleGroup(obj);
   alpha = (pg->render_type == PRT_SINGLE_COLOR_DISK);
   if (g_lgd3d)
      pg->need_flags |= NEED_Z;

   if (alpha && !g_lgd3d) {

      // convert color to an index into tluc8tab
      pg->render_datum = get_tluc_color(pg->cr);
      if (pg->cg) pg->render_datum |= get_tluc_color(pg->cg) << 8;
      if (pg->cb) pg->render_datum |= get_tluc_color(pg->cb) << 16;

   } else if (grd_bm.type == BMT_FLAT8 || g_lgd3d) {

      // use 8-bit color directly
      pg->render_datum = pg->cr;

   } else {

      // convert 8-bit color to device 16-bit color
      if (grd_pal16_list && grd_pal16_list[0])
         pg->render_datum = grd_pal16_list[0][pg->cr];
      else if (grd_pal16)
         pg->render_datum = grd_pal16[pg->cr];
      else
         pg->render_datum = (15 << 11) | 31;  // pinkish
   }

   switch(pg->render_type) {
      case PRT_SINGLE_COLOR_PIXEL:
      case PRT_VARIABLE_COLOR_PIXEL:
         pg->pc.particle_render = RenderPixelFixedColor;
         break;

      // unimplemented
      case PRT_SINGLE_COLOR_SQUARE:
      case PRT_VARIABLE_COLOR_SQUARE:
         pg->pc.particle_render = RenderSquareFixedColor;
         pg->need_flags |= NEED_W;
         break;

      case PRT_SINGLE_COLOR_DISK:
         pg->pc.particle_render = RenderDiskFixedColor;
         pg->need_flags |= NEED_W;
         break;

      case PRT_SCALED_BITMAP:
         pg->pc.particle_render = RenderBitmap;
         pg->modelnum = objmodelLoadModel(pg->modelname);
         break;
   }

   pg->zsort = (pg->render_type == PRT_SCALED_BITMAP);
}

BOOL ParticleGroupTranslucent(ObjID obj)
{
   ParticleGroup *pg; 
   pg = ObjGetParticleGroup(obj);
   if (pg->render_type == PRT_SINGLE_COLOR_DISK) return TRUE;
   if (pg->render_type == PRT_SCALED_BITMAP)     return TRUE;
   return FALSE;
}

void ParticleGroupUpdateMode(ObjID obj)
{
   particle_group_update_mode_change(obj);
}

void particle_group_update(ObjID obj)
{
   int start_len;
   ParticleGroup *pg = ObjGetParticleGroup(obj);

   free_lists(pg);

   // instantiate this in the appropriate way

   pg->obj = obj;

   // first build the function pointers
   // this -isn't- data driven because there might be choices
   // made from more than one data value, and because that way
   // you don't get bugs from having mismatched arrays

   pg->need_flags = 0;

   pg->pc.particle_transform = ParticleGroupTransformPoints;
   pg->pc.particle_worldspace = NULL;
   pg->pc.particle_objectspace = NULL;

   // unimplemented
   pg->pc.particle_compute_cell = NULL;
   pg->pc.particle_cell_partition = NULL;

   particle_group_update_mode_change(obj);

   pg->free_locs = FALSE;   // no types currently require loc freeing

   switch(pg->anim_type) {
      case PAT_FIXED_SPHERE_SHELL:
      case PAT_FIXED_SPHERE:
      case PAT_FIXED_CUBE_SHELL:
      case PAT_FIXED_CUBE:
      case PAT_FIXED_DISK:
      case PAT_FIXED_CIRCLE:
      case PAT_FIXED_PYRAMID:
         pg->shared_list = TRUE;
         pg->pc.particle_sim = NULL; // none needed
         pg->pc.particle_sim_precompute = NULL; // unimplemented
         mx_mk_vec(&pg->bmin, -1.0, -1.0, -1.0);
         mx_mk_vec(&pg->bmax,  1.0,  1.0,  1.0);
         pg->radius = 1.0;
         break;

      case PAT_LAUNCH_ONE_SHOT:
         pg->pre_launch = FALSE; // not allowed--will break stuff if it happens
         pg->always_simulate = TRUE; // otherwise it is nonsensical
         /* FALL THROUGH */

      case PAT_LAUNCH_CONTINUOUS:
         pg->shared_list = FALSE;
         pg->pc.particle_sim = particle_launch_sim;
         pg->pc.particle_sim_precompute = NULL; // unimplemented
         compute_launch_minmax(pg);
         pg->radius = 1.0;
         if (pg->pre_launch) {
            start_len = pg->n;
         } else {
            start_len = DEFAULT_LIST_LENGTH;
            // for continuous, the launching rate must be chosen
            // to spread all the particles out over the normal time...
            if (pg->anim_type == PAT_LAUNCH_CONTINUOUS && pg->n)
               pg->launch_period =
                (pg->launch->time_range[0]+pg->launch->time_range[1])/2 / pg->n;
         }
         pg->launching = TRUE;
         break;
   }

   switch(pg->motion_type) {
      case PGMT_VELOCITY:
      case PGMT_VELOCITY_GRAVITY:
         pg->pc.group_sim = pgroup_velocity_sim;
         pg->worldspace = FALSE; // (pg->spin_group == FALSE);
         break;

      case PGMT_IMMOBILE:
         pg->pc.group_sim = pgroup_basic_sim;
         pg->worldspace = FALSE; // (pg->spin_group == FALSE);
         break;

      case PGMT_FIREANDFORGET:
         pg->pc.group_sim = pgroup_basic_sim;
         pg->worldspace = FALSE;
         break;

      case PGMT_OBJECT_TRAILING:
         if (pg->anim_type <= PAT_LAUNCH_CONTINUOUS) {
            pg->pc.group_sim = pgroup_trailing_sim;
            pg->worldspace = TRUE;
            pg->always_simulate = TRUE;
            break;
         }

         pg->motion_type = PGMT_OBJECT_ATTACHED;
         Warning(("Forcing particle type 'trailing' to 'attached'.\n"));

         /*  FALL THROUGH */

      case PGMT_OBJECT_ATTACHED:
         pg->pc.group_sim = pgroup_attached_sim;
         pg->worldspace = FALSE;
         break;
   }

   allocate_lists(pg, start_len);
   setup_initial_particle_lists(pg);

   pg->delete_count = 0;   // this is the wrong thing to do on load
   pg->seen = FALSE;
   pg->cur_scale = pg->fixed_scale;

   pg->last_sim_time = GetSimTime();
}

// an attached object went away, or something like that
BOOL ParticleGroupShutdown(ObjID obj)
{
   ParticleGroup *pg = ObjGetParticleGroup(obj);

   // probably already deleting
   if (!pg) return FALSE;

   if (pg->launching) {
      pg->launching = FALSE;
      return FALSE;
   }
   return TRUE;
}

void ParticleGroupTerm(void)
{
   free_old_tluc_tables();
}

///// OBJSHAPE functions

EXTERN void ObjRel2ObjOffsetBBox(ObjID obj,mxs_vector *bmin,mxs_vector *bmax);
EXTERN void ObjRel2WorldBBox(ObjID obj,mxs_vector *bmin,mxs_vector *bmax);

float GetParticleRadius(ObjID objID, int idx)
{
   ParticleGroup *pg = ObjGetParticleGroup(objID);
   if (USE_SCALE(pg))
      return pg->radius * pg->cur_scale;
   else
      return pg->radius;
}

int GetParticleObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax)
{
   ParticleGroup *pg = ObjGetParticleGroup(objID);
   if (pg == NULL || pg->worldspace)
      return OBJ_BBOX_TYPE_NONE;

   if (USE_SCALE(pg)) {
      mx_scale_vec(bmin, &pg->bmin, pg->cur_scale);
      mx_scale_vec(bmax, &pg->bmax, pg->cur_scale);
   } else {
      *bmin = pg->bmin;
      *bmax = pg->bmax;
   }
   return OBJ_BBOX_TYPE_OBJREL;
}

int GetParticleObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax)
{
   if (GetParticleObjRelBBox(objID, idx, bmin, bmax) == OBJ_BBOX_TYPE_OBJREL) {
      ObjRel2ObjOffsetBBox(objID,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

int GetParticleWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax)
{
   ParticleGroup *pg = ObjGetParticleGroup(objID);
   if (pg == NULL) return OBJ_BBOX_TYPE_NONE;
   if (pg->worldspace) {
      *bmin = pg->bmin;
      *bmax = pg->bmax;
   } else {
      if (GetParticleObjRelBBox(objID, idx, bmin, bmax) == OBJ_BBOX_TYPE_NONE)
         return OBJ_BBOX_TYPE_NONE;
      ObjRel2WorldBBox(objID,bmin,bmax);
   }
   return OBJ_BBOX_TYPE_WORLD;
}

int GetParticleFavoriteBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax)
{
   ParticleGroup *pg = ObjGetParticleGroup(objID);
   if (pg == NULL) return OBJ_BBOX_TYPE_NONE;
   if (pg->worldspace)
      return GetParticleWorldBBox(objID, idx, bmin, bmax);
   else
      return GetParticleObjRelBBox(objID, idx, bmin, bmax);
}
