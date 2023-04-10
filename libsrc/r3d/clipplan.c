// $Header: x:/prj/tech/libsrc/r3d/RCS/clipplan.c 1.7 1996/06/13 11:53:36 JAEMZ Exp $

// User-defined clipping plane management

#include <lg.h>
#include <r3ds.h>
#include <clipu.h>
#include <space.h>
#include <clip_.h>

#define cclip   (r3d_state.cur_con->clip)

void r3_compute_camera_planes(void)
{
   r3s_clip_plane *clip = (r3s_clip_plane *) cclip.clip_data;
   int i,n = cclip.num_user_planes;
   for (i=0; i < n; ++i) {
      r3_w2c_plane(&clip[i].view_plane, &clip[i].world_plane);
   }
}

void r3_clear_clip_planes(void)
{
   cclip.num_user_planes = 0;
}

void r3_pop_clip_plane(void)
{
   if (cclip.num_user_planes <= 0) {
      Warning(("r3_pop_clip_plane: tried to pop too many planes\n"));
      return;
   }

   --cclip.num_user_planes;
}

  // worldspace plane
void r3_push_clip_plane(mxs_plane *p)
{
   int n;
   r3s_clip_plane *clip;

   if (cclip.num_user_planes >= NUM_USER_PLANES) {
      // there's only room for NUM_USER_PLANES clip codes,
      // so planes beyond that don't work correctly.

      Warning(("r3_push_clip_plane: tried to have more than %d planes\n", NUM_USER_PLANES));

      // but we'll still allow it to get allocated so that
      // pushes match pops
   }

   if (cclip.num_user_planes == cclip.max_user_planes) {
      int n;
      void *q;
      n = cclip.max_user_planes * 2;
      if (n == 0) n = 1;
      
      q = Realloc(cclip.clip_data, sizeof(r3s_clip_plane) * n);
#ifdef DBG_ON
      if (!q) {
         Warning(("r3d: r3_push_clip_plane: out of memory at depth %d\n", n));
         return;
      }
#endif

      cclip.clip_data = q;
      cclip.max_user_planes = n;

      Warning(("r3d: r3_push_clip_plane: needs to realloc, increase default clipping planes\n"));
   }

   clip = (r3s_clip_plane *) cclip.clip_data;   

   n = cclip.num_user_planes++;

     // copy the passed in user plane
   clip[n].world_plane = *p;

     // compute the version in camera space
   r3_w2c_plane(&clip[n].view_plane, &clip[n].world_plane);
}
