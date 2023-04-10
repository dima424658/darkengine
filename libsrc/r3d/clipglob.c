// $Header: x:/prj/tech/libsrc/r3d/RCS/clipglob.c 1.24 1997/11/04 15:54:02 KEVIN Exp $
#include <math.h>

#include <mprintf.h>
#include <lg.h>
#include <ctxts.h>
#include <r3ds.h>
#include <clipcon_.h>
#include <clip_.h>
#include <clipoff.h>
#include <xform.h>
#include <clipu.h>
#include <_test.h>
#include <matrix.h>
#include <clipprim.h>

// Number of total parameters in a big r3s_point
#define NUM_PARAMETERS   8

typedef struct _r3s_point_extended {
   mxs_vector p;   // 12
   ulong ccodes;   // 4
   grs_point grp;  // 28
   float extended_parm[NUM_PARAMETERS-3];    // 20
} r3s_point_extended;   // 64

// global values
ulong r3d_ccodes_or, r3d_ccodes_and;

#define r3_pt_dist(a,b)        \
   ((a)->p.x*(b)->v.x + (a)->p.y*(b)->v.y + (a)->p.z*(b)->v.z + (b)->d)

#define r3_pt_side_test(a,b)   (r3_pt_dist((a),(b)) < 0)

// recode all user clip planes in any intermediate space
// clear the existing user-ccodes as well
void r3_std_code_points(int n, r3s_point *p_list)
{
   uchar *cur, *last;
   int k = r3d_num_planes;
   r3s_clip_plane *clip_plane = (r3s_clip_plane *) r3d_clip_data;

   TEST_IN_BLOCK("r3_std_code_points");
   
   cur = (uchar *)p_list;
   last = cur + n * r3d_glob.cur_stride;
   if (k == 1) {
      while (cur < last) {
      // if only one clip plane, don't waste time on innermost loop
         r3s_point *src = (r3s_point *)cur;

         cur += r3d_glob.cur_stride;
         src->ccodes &= (1 << NUM_STD_CCODES)-1;

         if (r3_pt_side_test(src, &clip_plane[0].view_plane))
            src->ccodes |= 1 << NUM_STD_CCODES;
      }
   } else {
      while (cur < last) {
         r3s_point *src = (r3s_point *)cur;
         int code,j, this_code;

         cur += r3d_glob.cur_stride;

         code = src->ccodes & ((1 << NUM_STD_CCODES)-1);

         this_code = 1 << NUM_STD_CCODES;
         for (j=0; j < k; ++j, this_code += this_code)
            if (r3_pt_side_test(src, &clip_plane[j].view_plane))
               code |= (1 << NUM_STD_CCODES) << j;

         src->ccodes = code;
      }
   }
}

int r3_code_polygon(int n, r3s_phandle *src)
{
   int i, c_or, c_and,k;
   c_or = c_and = src[0]->ccodes;
   for (i=1; i < n; ++i) {
      k = src[i]->ccodes;
      c_or |= k;
      c_and &= k;
   }
   r3d_ccodes_or |= c_or;
   r3d_ccodes_and |= c_and;
   return c_or;
}

int r3_ccodes_or;
int r3_ccodes_and;

int r3_code_polygon_fast(int n, r3s_phandle *src)
{
   int i, c_or, c_and,k;
   c_or = c_and = src[0]->ccodes;
   for (i=1; i < n; ++i) {
      k = src[i]->ccodes;
      c_or |= k;
      c_and &= k;
   }
   r3_ccodes_or = c_or;
   r3_ccodes_and = c_and;
   return c_or;
}

  // max clip points can be generated:
  //   2 per clip plane
#define NUM_CLIP_PTS 32

static r3s_phandle clip1[128];
static r3s_phandle clip2[128];
static r3s_point_extended clippt[NUM_CLIP_PTS];
r3s_point_extended *r3_next_point;

r3s_point *r3_clip_left  (r3s_point *p, r3s_point *q);
r3s_point *r3_clip_top   (r3s_point *p, r3s_point *q);
r3s_point *r3_clip_bottom(r3s_point *p, r3s_point *q);
r3s_point *r3_clip_right (r3s_point *p, r3s_point *q);
r3s_point *r3_clip_behind(r3s_point *p, r3s_point *q);
r3s_point *r3_clip_user_plane(r3s_point *p, r3s_point *q, int n);

#define FP(x)   ((int) ((x) * 65536))

   // externally callable interface
int r3_clip_polygon(int n, r3s_phandle *src, r3s_phandle **dest)
{
   TEST_IN_BLOCK("r3_clip_polygon");
   if (r3d_clip_mode == R3_NO_CLIP) { *dest = src; return n; }
   return r3_raw_clip_polygon(n, src, dest);
}

   // internally direct-called interface
int r3_raw_clip_polygon(int n, r3s_phandle *src, r3s_phandle **dest)
{
   int i, code, c_or, j, k, last_code, p;
   r3s_phandle *out;

   TEST_IN_BLOCK("r3_raw_clip_polygon");

   if (!r3_code_polygon_fast(n, src)) {
      *dest = src;
      return n;    // trivial accept
   }

   if (r3_ccodes_and)
      return 0;    // trivial reject

   c_or = r3_ccodes_or;
   r3_next_point = clippt;

      // check if we do main cone clipping
   if (r3d_clip_mode == R3_CLIP) {
      // clip by main clip planes

      // clipping logic is as follows:
      //   iterate through all points.  add to the
      //   new polygon all points that are inside and
      //   all points that are intersections of edge
      //   with a given plane.  recompute new c_or code
      //   as we go.
      //
      // If a polygon gets clipped away entirely but isn't
      // trivially rejected, then eventually its vertex count
      // goes to 0.  We don't explicitly check for this;
      // instead its c_or is 0 and it early outs that way.

      if (c_or & R3C_OFF_LEFT) {
         c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
         out = clip1;
         for (i=0; i < n; ++i) {
            code = src[i]->ccodes;

            // if code changes value at this point, generate clipped pt
            if ((code ^ last_code) & R3C_OFF_LEFT) {
               out[k] = r3_clip_left(src[i], src[j]);
               c_or |= out[k++]->ccodes;
            }

            // if leading pt is inside, include it
            if (!(code & R3C_OFF_LEFT))
               out[k++] = src[i], c_or |= code;

            j = i;
            last_code = code;
         }
         src = out;
         n = k;
      }

      if (c_or & R3C_OFF_RIGHT) {
         c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
         out = ((src == clip1) ? clip2 : clip1);
         for (i=0; i < n; ++i) {
            code = src[i]->ccodes;

            // if code changes value at this point, generate clipped pt
            if ((code ^ last_code) & R3C_OFF_RIGHT) {
               out[k] = r3_clip_right(src[i], src[j]);
               c_or |= out[k++]->ccodes;
            }

            // if leading pt is inside, include it
            if (!(code & R3C_OFF_RIGHT))
               out[k++] = src[i], c_or |= code;

            j = i;
            last_code = code;
         }
         src = out;
         n = k;
      }

      if (c_or & R3C_OFF_TOP) {
         c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
         out = ((src == clip1) ? clip2 : clip1);
         for (i=0; i < n; ++i) {
            code = src[i]->ccodes;

            // if code changes value at this point, generate clipped pt
            if ((code ^ last_code) & R3C_OFF_TOP) {
               out[k] = r3_clip_top(src[i], src[j]);
               c_or |= out[k++]->ccodes;
            }

            // if pt is inside, include it
            if (!(code & R3C_OFF_TOP))
               out[k++] = src[i], c_or |= code;

            j = i;
            last_code = code;
         }
         src = out;
         n = k;
      }

      if (c_or & R3C_OFF_BOTTOM) {
         c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
         out = ((src == clip1) ? clip2 : clip1);
         for (i=0; i < n; ++i) {
            code = src[i]->ccodes;

            // if code changes value at this point, generate clipped pt
            if ((code ^ last_code) & R3C_OFF_BOTTOM) {
               out[k] = r3_clip_bottom(src[i], src[j]);
               c_or |= out[k++]->ccodes;
            }

            // if pt is inside, include it
            if (!(code & R3C_OFF_BOTTOM))
               out[k++] = src[i], c_or |= code;

            j = i;
            last_code = code;
         }
         src = out;
         n = k;
      }

      if (c_or & R3C_BEHIND) {
         c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
         out = ((src == clip1) ? clip2 : clip1);
         for (i=0; i < n; ++i) {
            code = src[i]->ccodes;

            // if code changes value at this point, generate clipped pt
            if ((code ^ last_code) & R3C_BEHIND) {
               out[k] = r3_clip_behind(src[i], src[j]);
               c_or |= out[k++]->ccodes;
            }

            // if pt is inside, include it
            if (!(code & R3C_BEHIND))
               out[k++] = src[i], c_or |= code;

            j = i;
            last_code = code;
         }
         src = out;
         n = k;
      }

      // now code all the points by the user planes at once
      // this avoids testing r3d_num_planes at every point
      // but will code all of the points generated by the clipper,
      // not just the ones that currently remain
      if (r3d_num_planes && r3_next_point != clippt) {
         int stride_save = r3d_glob.cur_stride;

         r3d_glob.cur_stride = sizeof(clippt[0]);
         r3_std_code_points(r3_next_point - clippt, (r3s_point *)clippt);
         r3d_glob.cur_stride = stride_save;

         c_or = 0;
         for (i=0; i < n; ++i)
            c_or |= src[i]->ccodes;
      }
   }

   if (c_or) {
      int this_code=1 << NUM_STD_CCODES;
      for (p=0; p < r3d_num_planes; ++p, this_code <<= 1) {
         if (c_or & this_code) {
            c_or = 0; j = n-1; k=0; last_code = src[j]->ccodes;
            out = ((src == clip1) ? clip2 : clip1);
            for (i=0; i < n; ++i) {
               code = src[i]->ccodes;

               // if code changes value at this point, generate clipped pt
               if ((code ^ last_code) & this_code) {
                  out[k] = r3_clip_user_plane(src[i], src[j], p);
                  c_or |= out[k++]->ccodes;
               }

               // if pt is inside, include it
               if (!(code & this_code))
                  out[k++] = src[i], c_or |= code;

               j = i;
               last_code = code;
            }
            src = out;
            n = k;
         }
      }
   }

#ifdef DBG_ON
   if (c_or) {
      Warning(("r3d: r3_raw_clip_polygon: invalid clipping\n"));
      return 0;
   }
#endif

   *dest = src;
   return n;
}

bool r3_clip_line(r3s_phandle *src,r3s_phandle **dest)
{
   r3s_phandle *out;
   int c_or, c_and;

   *dest = src;

   c_or = (src[0]->ccodes | src[1]->ccodes);
   if (!c_or) return TRUE;  // trivial accept
   c_and = (src[0]->ccodes & src[1]->ccodes);
   if (c_and) return FALSE; // trivial reject

   r3_next_point = clippt;

     // check each of the planes one by one
     //   (only the first is commented; they're all cut&pasted)

   if (r3d_clip_mode == R3_CLIP) {
      if (c_or & R3C_OFF_LEFT) {
           // select which buffer to output to
         out = clip1;
           // put the intersection of the line with the plane into output
         out[0] = r3_clip_left(src[0], src[1]);
           // copy whichever point is inside into the output
         out[1] = src[0]->ccodes & R3C_OFF_LEFT ? src[1] : src[0];
           // move the output to be the new input
         src = out;
           // check for it being totally clipped away
           // (required since rest of code assumes it's not)
         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
           // compute the new clip code
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
      if (c_or & R3C_OFF_RIGHT) {
         out = ((src == clip1) ? clip2 : clip1);
         out[0] = r3_clip_right(src[0], src[1]);
         out[1] = src[0]->ccodes & R3C_OFF_RIGHT ? src[1] : src[0];
         src = out;
         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
      if (c_or & R3C_OFF_TOP) {
         out = ((src == clip1) ? clip2 : clip1);
         out[0] = r3_clip_top(src[0], src[1]);
         out[1] = src[0]->ccodes & R3C_OFF_TOP ? src[1] : src[0];
         src = out;
         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
      if (c_or & R3C_OFF_BOTTOM) {
         out = ((src == clip1) ? clip2 : clip1);
         out[0] = r3_clip_bottom(src[0], src[1]);
         out[1] = src[0]->ccodes & R3C_OFF_BOTTOM ? src[1] : src[0];
         src = out;
         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
      if (c_or & R3C_BEHIND) {
         out = ((src == clip1) ? clip2 : clip1);
         out[0] = r3_clip_behind(src[0], src[1]);
         out[1] = src[0]->ccodes & R3C_BEHIND ? src[1] : src[0];
         src = out;
         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
      // recode points by user clip planes
      if (r3d_num_planes && r3_next_point != clippt) {
         int stride_save = r3d_glob.cur_stride;

         r3d_glob.cur_stride = sizeof(clippt[0]);
         r3_std_code_points(r3_next_point - clippt, (r3s_point *)clippt);
         r3d_glob.cur_stride = stride_save;

         if (src[0]->ccodes & src[1]->ccodes) return FALSE;
         c_or = src[0]->ccodes | src[1]->ccodes;
      }
   }

     // clip against user planes
   if (c_or) {
      int p,this_code=1 << NUM_STD_CCODES;
      for (p=0; p < r3d_num_planes; ++p, this_code <<= 1) {
         if (c_or & this_code) {
            out = ((src == clip1) ? clip2 : clip1);
            out[0] = r3_clip_user_plane(src[0], src[1], p);
            out[1] = (src[0]->ccodes & this_code) ? src[1] : src[0];
            src = out;
            if (src[0]->ccodes & src[1]->ccodes) return FALSE;
            c_or = src[0]->ccodes | src[1]->ccodes;
         }
      }
   }

#ifdef DBG_ON
   if (c_or) {
      Warning(("r3d: r3_clip_line: invalid clipping\n"));
      return FALSE;
   }
#endif

   *dest = src;
   return TRUE;
}

#define swap(p,q)    do { r3s_point *temp = p; p = q; q = temp; } while(0)

// General comments about all clip functions:
   // we swap points so result is the same regardless of point order
   // note that unlike old 3d, we don't compare something like z,
   // which can (trivially!) be constant.

#define weight(p,q,parm,frac)    ((p)->parm + frac * ((q)->parm - (p)->parm))

mxs_real r3_clip_frac;

void r3_intersect(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point-1);
   int flags = r3d_clip_flags;

   if (r3_next_point == clippt + NUM_CLIP_PTS) {
      Warning(("r3d fatal error: Overflowed static NUM_CLIP_PTS limit.\n"));
      return;
   }

   if (flags & R3_CLIP_I)
      pt->grp.i = weight(p,q,grp.i,r3_clip_frac);
   if (flags & R3_CLIP_U)
      pt->grp.u = weight(p,q,grp.u,r3_clip_frac);
   if (flags & R3_CLIP_V)
      pt->grp.v = weight(p,q,grp.v,r3_clip_frac);
   if (flags > (R3_CLIP_I | R3_CLIP_U | R3_CLIP_V)) {
      int k = 1 << 3;
      int i;
      
      for (i=0; k <= flags; ++i, k <<= 1)
         if (flags & k)
            ((r3s_point_extended *) pt)->extended_parm[i] =
                     weight((r3s_point_extended *) p,
                            (r3s_point_extended *) q,
                             extended_parm[i],
                             r3_clip_frac);
   }

   r3_project_point(pt);
}

void r3_intersect_user(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point-1);
   int flags = r3d_clip_flags;

   if (r3_next_point == clippt + NUM_CLIP_PTS) {
      Warning(("r3d fatal error: Overflowed static NUM_CLIP_PTS limit.\n"));
      return;
   }

   if (flags & R3_CLIP_I)
      pt->grp.i = weight(p,q,grp.i,r3_clip_frac);
   if (flags & R3_CLIP_U)
      pt->grp.u = weight(p,q,grp.u,r3_clip_frac);
   if (flags & R3_CLIP_V)
      pt->grp.v = weight(p,q,grp.v,r3_clip_frac);
   if (flags > (R3_CLIP_I | R3_CLIP_U | R3_CLIP_V)) {
      int k = 1 << 3;
      int i;
      
      for (i=0; k <= flags; ++i, k <<= 1)
         if (flags & k)
            ((r3s_point_extended *) pt)->extended_parm[i] =
                     weight((r3s_point_extended *) p,
                            (r3s_point_extended *) q,
                             extended_parm[i],
                             r3_clip_frac);
   }

   r3_project_point(pt);
#ifdef DBG_ON
   if (pt->grp.sx == 0x80000000 || pt->grp.sy == 0x80000000)
      Warning(("r3_intersect: bad clip!\n"));
#endif
}

#define CODE_RIGHT(pt)  (pt->p.x > jz ? R3C_OFF_RIGHT : 0)
#define CODE_TOP(pt)    (pt->p.y < -jz ? R3C_OFF_TOP : 0)
#define CODE_BOTTOM(pt) (pt->p.y > jz ? R3C_OFF_BOTTOM : 0)
#define CODE_BEHIND(pt) (pt->p.z < r3d_near_plane ? R3C_BEHIND : 0)

#define PX  (p->p.x)
#define PY  (p->p.y)
#define PZ  (p->p.z)
#define QX  (q->p.x)
#define QY  (q->p.y)
#define QZ  (q->p.z)

//    boundary is x + z = 0
//  dist from boundary is   PX + PZ

static mxs_real iz, jz;

r3s_point *r3_clip_left(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   if (p->ccodes & R3C_OFF_LEFT) swap(p,q);

   r3_clip_frac = (PX + PZ*r3d_glob.x_clip) / (PX - QX + (PZ - QZ) * r3d_glob.x_clip);

   pt->p.y = PY + r3_clip_frac * (QY - PY);
   pt->p.z = PZ + r3_clip_frac * (QZ - PZ);
   pt->p.x = -pt->p.z * r3d_glob.x_clip;

   r3_intersect(p, q);
   iz = pt->p.z * r3d_glob.x_clip;
   jz = pt->p.z * r3d_glob.y_clip;
   pt->ccodes = CODE_RIGHT(pt) | CODE_TOP(pt) | CODE_BOTTOM(pt) | CODE_BEHIND(pt);
   return pt;
}

r3s_point *r3_clip_right(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   if (p->ccodes & R3C_OFF_RIGHT) swap(p,q);

   r3_clip_frac = (r3d_glob.x_clip * PZ - PX) / (r3d_glob.x_clip * (PZ - QZ) + QX - PX);
   pt->p.y = PY + r3_clip_frac * (QY - PY);
   pt->p.z = PZ + r3_clip_frac * (QZ - PZ);
   pt->p.x = pt->p.z * r3d_glob.x_clip;

   r3_intersect(p, q);
   jz = pt->p.z * r3d_glob.y_clip;
   pt->ccodes = CODE_TOP(pt) | CODE_BOTTOM(pt) | CODE_BEHIND(pt);
   return pt;
}

r3s_point *r3_clip_top(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   if (p->ccodes & R3C_OFF_TOP) swap(p,q);

   r3_clip_frac = (PY + PZ*r3d_glob.y_clip) / (r3d_glob.y_clip * (PZ - QZ) + PY - QY);
   pt->p.x = PX + r3_clip_frac * (QX - PX);
   pt->p.z = PZ + r3_clip_frac * (QZ - PZ);
   pt->p.y = -pt->p.z * r3d_glob.y_clip;

   r3_intersect(p, q);
   jz = pt->p.z * r3d_glob.y_clip;
   pt->ccodes = CODE_BOTTOM(pt) | CODE_BEHIND(pt);
   return pt;
}

r3s_point *r3_clip_bottom(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   if (p->ccodes & R3C_OFF_BOTTOM) swap(p,q);

   r3_clip_frac = (PZ*r3d_glob.y_clip - PY) / (r3d_glob.y_clip*(PZ - QZ) + QY - PY);
   pt->p.x = PX + r3_clip_frac * (QX - PX);
   pt->p.z = PZ + r3_clip_frac * (QZ - PZ);
   pt->p.y = pt->p.z * r3d_glob.y_clip;

   r3_intersect(p, q);
   pt->ccodes = CODE_BEHIND(pt);
   return pt;
}

//  z = r3d_near_plane
// dist is   PZ - r3d_near_plane
r3s_point *r3_clip_behind(r3s_point *p, r3s_point *q)
{
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   if (p->ccodes & R3C_BEHIND) swap(p,q);

   r3_clip_frac = (PZ - r3d_near_plane) / (PZ - QZ);
   pt->p.x = PX + r3_clip_frac * (QX - PX);
   pt->p.y = PY + r3_clip_frac * (QY - PY);
   pt->p.z = r3d_near_plane;

   r3_intersect(p, q);
   pt->ccodes = 0;
   return pt;
}

r3s_point *r3_clip_user_plane(r3s_point *p, r3s_point *q, int n)
{
   r3s_clip_plane *clip = r3d_clip_data;
   mxs_plane *m = &clip[n].view_plane;
   r3s_point *pt = (r3s_point *) (r3_next_point++);
   mxs_real dp,dq;
   int i, code = 1 << (n + NUM_STD_CCODES), this_code;

   if (p->ccodes & code) swap(p,q);

   dp = r3_pt_dist(p, m);
   dq = r3_pt_dist(q, m);

   r3_clip_frac = dp / (dp - dq);
   pt->p.x = PX + r3_clip_frac * (QX - PX);
   pt->p.y = PY + r3_clip_frac * (QY - PY);
   pt->p.z = PZ + r3_clip_frac * (QZ - PZ);

   r3_intersect_user(p, q);

   // now clip code it to all of the planes after this one
   code = 0;
   this_code = 1 << (NUM_STD_CCODES+n+1);
   for (i=n+1; i < r3d_num_planes; ++i, this_code += this_code) {
      if (r3_pt_side_test(pt, &clip[i].view_plane))
         code |= (1 << NUM_STD_CCODES) << i;
   }
   pt->ccodes = code;

   return pt;
}
