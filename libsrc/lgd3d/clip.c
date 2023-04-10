// $Header: x:/prj/tech/libsrc/lgd3d/RCS/clip.c 1.3 1997/12/04 13:02:45 KEVIN Exp $

#include <dev2d.h>
#include <lgassert.h>
#include <memall.h>
#include <tmpalloc.h>

#include <d3dpoint.h>

static lgd3ds_point *tmp;
static float (*intersect)(lgd3ds_point *v0, lgd3ds_point *v1);
static float clip_val;

extern float lgd3d_clip[4];

#define left lgd3d_clip[0]
#define right lgd3d_clip[1]
#define top lgd3d_clip[2]
#define bot lgd3d_clip[3]

static void code_xy(lgd3ds_point *v)
{
   v->flags = 0;
   if (v->sx < left)
      v->flags|=CLIP_LEFT;
   else if (v->sx > right)
      v->flags|=CLIP_RIGHT;
   if (v->sy < top) 
      v->flags|=CLIP_TOP;
   else if (v->sy > bot)
      v->flags|=CLIP_BOT;
}

static void code_y(lgd3ds_point *v)
{
   v->flags = 0;
   if (v->sy < top) 
      v->flags|=CLIP_TOP;
   else if (v->sy > bot)
      v->flags|=CLIP_BOT;
}

static void clip_func(lgd3ds_point *v0, lgd3ds_point *v1, float r)
{
   float da = v1->a - v0->a;
   float dr = v1->r - v0->r;
   float dg = v1->g - v0->g;
   float db = v1->b - v0->b;
   float du = v1->u - v0->u;
   float dv = v1->v - v0->v;

   tmp->a = v0->a + da*r;
   tmp->r = v0->r + dr*r;
   tmp->g = v0->g + dg*r;
   tmp->b = v0->b + db*r;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
   tmp->rhw = v0->rhw;
   tmp->sz = v0->sz;
}

static float x_intersect(lgd3ds_point *v0, lgd3ds_point *v1)
{
   float dy,r;
   tmp->sx = clip_val;
   r = clip_val - v0->sx;
   r /= v1->sx - v0->sx;
   dy = v1->sy - v0->sy;
   tmp->sy = v0->sy + r*dy;
   tmp->flags = 0;
   code_y(tmp);
   return r;
}

static float y_intersect(lgd3ds_point *v0, lgd3ds_point *v1)
{
   float dx,r;
   tmp->sy = clip_val;
   r = clip_val - v0->sy;
   r /= v1->sy - v0->sy;
   dx = v1->sx - v0->sx;
   tmp->sx = v0->sx + r*dx;
   tmp->flags = 0;
   return r;
}

static uchar get_codes(lgd3ds_point **a, int n)
{
   uchar ccr, cc;
   lgd3ds_point **last;

   last = a + n;
   ccr = CLIP_LEFT|CLIP_RIGHT|CLIP_TOP|CLIP_BOT;
   cc = 0;
   for (; a<last; a++) {
      cc |= (*a)->flags;
      ccr &= (*a)->flags;
   }
   if (ccr!=0)
      return CLIP_ALL;  // trivial reject
   return (cc);
}

static int do_plane(lgd3ds_point **src, lgd3ds_point **dest, int n, uchar code)
{
   uchar c0;
   int m=0;
   lgd3ds_point **start, **end, **last;

   start = last = src+n-1;
   end = src;
   c0=((*start)->flags&code);
   do {
      uchar c1=((*end)->flags&code);

      // output all the points that are in!
      if (c0==0) {
         *dest = *start;
         dest++, m++;
      }
      // if one's in and one's out, compute the intersection!
      if (c1 ^ c0) {
         lgd3ds_point *v0, *v1;
         float r;

         v0 = *start;
         v1 = *end;
         // sort vertices to avoid cracking.
         if ((v0->sx < v1->sx)||
               (v0->sx==v1->sx)&&(v0->sy<v1->sy)) {
            lgd3ds_point *swap = v0;
            v0 = v1;
            v1 = swap;
         }
         r = intersect(v0, v1);
         if (clip_func != NULL)
            clip_func(v0, v1, r);
         *dest = tmp;
         dest++, m++, tmp++;
      }
      start = end, c0=c1;
      end++;
   } while (start != last);
   return m;
}

#define MAX_TEMP_POINTS 8 // (2 per clipping plane)

int lgd3d_clip_poly(int n, lgd3ds_point **vpl, lgd3ds_point ***pcvpl)
{
   lgd3ds_point **src, **dest, **next; /* lists of vertex pointers */
   int max_verts, i;                /* max number of output vertexes */
   uchar cc;                        /* aggregate clip codes for entire poly */

   for (i=0; i<n; i++)
      code_xy(vpl[i]);

   cc = get_codes(vpl, n);
   if (cc==CLIP_ALL)
      return 0;
   if (cc==CLIP_NONE) {
      *pcvpl = vpl;
      return n;
   }

   max_verts = n+4;  // Maximum # of vertices in clipped poly
   if (*pcvpl==NULL) {
      *pcvpl = temp_malloc(2*max_verts*sizeof(*src) + MAX_TEMP_POINTS*sizeof(*tmp));
      AssertMsg (*pcvpl!=NULL,"g2_clip_poly(): out of temporary memory!\n"); // i.e., we have temp memory.
   }

   src = vpl;
   dest = *pcvpl;
   next = dest + max_verts;
   tmp = (lgd3ds_point *)(next + max_verts);

   intersect = x_intersect;
   if (cc&CLIP_LEFT) {
   // clip to left plane.
      clip_val = left;
      n = do_plane(src, dest, n, CLIP_LEFT);
      src = dest; dest = next; next = src;
      // if we're clipping right, might as well wait to recode.
      if ((cc&CLIP_RIGHT)==0) {
         cc = get_codes(src, n);
         if (cc==CLIP_ALL) return 0;
      }
   }

   if (cc&CLIP_RIGHT) {
   // clip to right plane.
      clip_val = right;
      n = do_plane(src, dest, n, CLIP_RIGHT);
      src = dest; dest = next; next = src;
      cc = get_codes(src, n);
      if (cc==CLIP_ALL) return 0;
   }

   intersect = y_intersect;
   if (cc&CLIP_TOP) {
   // clip to top plane.
      clip_val = top;
      n = do_plane(src, dest, n, CLIP_TOP);
      src = dest; dest = next; next = src;
   }

   if (cc&CLIP_BOT) {
   // clip to bottom plane.
      clip_val = bot;
      n = do_plane(src, dest, n, CLIP_BOT);
      src = dest; dest = next; next = src;
   }

   if (src != *pcvpl) // need to copy list, sadly
      for (i=0; i<n; i++)
         dest[i] = src[i];


   AssertMsg (n<=max_verts, "g2_clip_poly(): too many vertexes generated! (was that a concave polygon?!)\n");
   return n;
}
