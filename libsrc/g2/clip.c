// $Header: x:/prj/tech/libsrc/g2/RCS/clip.c 1.3 1997/05/16 17:27:26 KEVIN Exp $

#include <dev2d.h>  // (sigh) only for clip codes...
#include <g2clip.h>
#include <buffer.h>
#include <lgassert.h>

#define CLIP_MASK 0xfffffff0
#define TIMES_W   0x00000010

static g2s_point *tmp;
static float (*intersect)(g2s_point *v0, g2s_point *v1);
static void (*clip_func)(g2s_point *v0, g2s_point *v1, float r);
static fix clip_val, left, right, top, bot;

static void code_xy(g2s_point *v)
{
   v->flags &= CLIP_MASK;
   if (v->sx < left)
      v->flags|=CLIP_LEFT;
   else if (v->sx > right)
      v->flags|=CLIP_RIGHT;
   if (v->sy < top) 
      v->flags|=CLIP_TOP;
   else if (v->sy > bot)
      v->flags|=CLIP_BOT;
}

static void code_y(g2s_point *v)
{
   v->flags &= CLIP_MASK;
   if (v->sy < top) 
      v->flags|=CLIP_TOP;
   else if (v->sy > bot)
      v->flags|=CLIP_BOT;
}

static void clip_uvwih(g2s_point *v0, g2s_point *v1, float r)
{
   float dw, du, dv, di, dh;
   if (!(v0->flags&TIMES_W)) {
      v0->u *= v0->w;
      v0->v *= v0->w;
      v0->flags |= TIMES_W;
   }
   if (!(v1->flags&TIMES_W)) {
      v1->u *= v1->w;
      v1->v *= v1->w;
      v1->flags |= TIMES_W;
   }
   du = v1->u - v0->u;
   dv = v1->v - v0->v;
   dw = v1->w - v0->w;
   di = v1->i - v0->i;
   dh = v1->h - v0->h;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
   tmp->w = v0->w + dw*r;
   tmp->i = v0->i + di*r;
   tmp->h = v0->h + dh*r;
   tmp->flags |= TIMES_W;
}

static void clip_uvwi(g2s_point *v0, g2s_point *v1, float r)
{
   float dw, du, dv, di;
   if (!(v0->flags&TIMES_W)) {
      v0->u *= v0->w;
      v0->v *= v0->w;
      v0->flags |= TIMES_W;
   }
   if (!(v1->flags&TIMES_W)) {
      v1->u *= v1->w;
      v1->v *= v1->w;
      v1->flags |= TIMES_W;
   }
   du = v1->u - v0->u;
   dv = v1->v - v0->v;
   dw = v1->w - v0->w;
   di = v1->i - v0->i;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
   tmp->w = v0->w + dw*r;
   tmp->i = v0->i + di*r;
   tmp->flags |= TIMES_W;
}

static void clip_uvw(g2s_point *v0, g2s_point *v1, float r)
{
   float dw, du, dv;
   if (!(v0->flags&TIMES_W)) {
      v0->u *= v0->w;
      v0->v *= v0->w;
      v0->flags |= TIMES_W;
   }
   if (!(v1->flags&TIMES_W)) {
      v1->u *= v1->w;
      v1->v *= v1->w;
      v1->flags |= TIMES_W;
   }
   du = v1->u - v0->u;
   dv = v1->v - v0->v;
   dw = v1->w - v0->w;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
   tmp->w = v0->w + dw*r;
   tmp->flags |= TIMES_W;
}

static void clip_uvi(g2s_point *v0, g2s_point *v1, float r)
{
   float du = v1->u - v0->u;
   float dv = v1->v - v0->v;
   float di = v1->i - v0->i;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
   tmp->i = v0->i + di*r;
}

static void clip_uv(g2s_point *v0, g2s_point *v1, float r)
{
   float du = v1->u - v0->u;
   float dv = v1->v - v0->v;
   tmp->u = v0->u + du*r;
   tmp->v = v0->v + dv*r;
}

static void clip_i(g2s_point *v0, g2s_point *v1, float r)
{
   float di = v1->i - v0->i;
   tmp->i = v0->i + di*r;
}

static float x_intersect(g2s_point *v0, g2s_point *v1)
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

static float y_intersect(g2s_point *v0, g2s_point *v1)
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

static ulong get_codes(g2s_point **a, int n)
{
   ulong ccr, cc;
   g2s_point **last;

   last = a + n;
   ccr = CLIP_LEFT|CLIP_RIGHT|CLIP_TOP|CLIP_BOT;
   cc = 0;
   for (; a<last; a++) {
      cc |= (*a)->flags;
      ccr &= (*a)->flags;
   }
   if (ccr!=0)
      return CLIP_ALL;  // trivial reject
   return (cc&(~CLIP_MASK));
}

static int do_plane(g2s_point **src, g2s_point **dest, int n, ulong code)
{
   ulong c0, m=0;
   g2s_point **start, **end, **last;

   start = last = src+n-1;
   end = src;
   c0=((*start)->flags&code);
   do {
      int c1=((*end)->flags&code);

      // output all the points that are in!
      if (c0==0) {
         *dest = *start;
         dest++, m++;
      }
      // if one's in and one's out, compute the intersection!
      if (c1 ^ c0) {
         g2s_point *v0, *v1;
         float r;

         v0 = *start;
         v1 = *end;
         // sort vertices to avoid cracking.
         if ((v0->sx < v1->sx)||
               (v0->sx==v1->sx)&&(v0->sy<v1->sy)) {
            g2s_point *swap = v0;
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

#define TOO_BIG (fix_make(0x2000,0)) // avoids overflows
#define MAX_TEMP_POINTS 8 // (2 per clipping plane)

int g2_clip_poly(int n, int flags, g2s_point **vpl, g2s_point ***pcvpl)
{
   g2s_point **src, **dest, **next; /* lists of vertex pointers */
   int max_verts, i;                /* max number of output vertexes */
   ulong cc;                        /* aggregate clip codes for entire poly */
   bool perspective = FALSE;

   left = grd_gc.clip.f.left;
   right = grd_gc.clip.f.right;
   top = grd_gc.clip.f.top;
   bot = grd_gc.clip.f.bot;
   for (i=0; i<n; i++) {
      g2s_point *v = vpl[i];
      code_xy(v);
      if ((v->sx>=TOO_BIG)||(v->sx<=-TOO_BIG)||
          (v->sy>=TOO_BIG)||(v->sy<=-TOO_BIG))
         return 0;
   }

   cc = get_codes(vpl, n);
   if (cc==CLIP_ALL)
      return 0;
   if (cc==CLIP_NONE) {
      *pcvpl = vpl;
      return n;
   }

   switch (flags) {
   case G2C_CLIP_UV:   clip_func = clip_uv; break;
   case G2C_CLIP_UVW:  clip_func = clip_uvw; perspective = TRUE; break;
   case G2C_CLIP_UVWI: clip_func = clip_uvwi; perspective = TRUE; break;
   case G2C_CLIP_UVWIH:clip_func = clip_uvwih; perspective = TRUE; break;
   case G2C_CLIP_UVI:  clip_func = clip_uvi; break;
   case G2C_CLIP_I:    clip_func = clip_i; break;
   default:            clip_func = NULL; break;
   }

   max_verts = n+4;  // Maximum # of vertices in clipped poly
   if (*pcvpl==NULL) {
      *pcvpl = gr_alloc_temp(2*max_verts*sizeof(*src) + MAX_TEMP_POINTS*sizeof(*tmp));
      AssertMsg (*pcvpl!=NULL,"g2_clip_poly(): out of temporary memory!\n"); // i.e., we have temp memory.
   }

   src = vpl;
   dest = *pcvpl;
   next = dest + max_verts;
   tmp = (g2s_point *)(next + max_verts);

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

   if (perspective)
      g2_clip_fixup(n, src);

   if (src != *pcvpl) // need to copy list, sadly
      for (i=0; i<n; i++)
         dest[i] = src[i];


   AssertMsg (n<=max_verts, "g2_clip_poly(): too many vertexes generated! (was that a concave polygon?!)\n");
   return n;
}

void g2_clip_fixup(int n, g2s_point **vpl)
{
   int i;
   for (i=0; i<n; i++) {
      g2s_point *v = vpl[i];
      if (v->flags&TIMES_W) {
         float r = 1.0 / v->w;
         v->u *= r;
         v->v *= r;
         v->flags &= ~TIMES_W;
      }
   }
}
