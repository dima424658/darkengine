/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portclip.c,v 1.15 2000/02/19 13:18:38 toml Exp $
//
//  PORTAL
//
//  Portal clipping
//
//    The portal clipper uses wacky octagonal clipping;
//    we probably want to add support for multiple disjoint clip regions
//    for better portal testing.  The idea here is that on a union operation,
//    we check if the total area of the union is too much bigger than the
//    the sum of the areas.  If so, we keep a tree of the main one and each
//    of the sub ones.  When we need to clip a polygon against a tree, we
//    first check against a given node.  If it is trivially rejected, done.
//    Otherwise, check each of the children.  If one child trivial rejects,
//    recurse on the other child.  As soon as we get two children which don't
//    trivially reject, form a clipping region that is the union of all of the
//    non-trivial rejecters, and clip against that.
//

#include <lg.h>
#include <mprintf.h>
#include <portclip.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

bool clip_lighting=TRUE;

int clip_uv=0;            // set to C_CLIP_UV_OFFSET if needed

extern BOOL g_lgd3d;

bool portal_clip_fast=TRUE;
bool poly_clip_fast;

// if defined, we maintain clipping regions as integers;
// if not, we maintain them as fixed point
//#define PORTCLIP_INT

struct st_ClipData
{
#ifdef PORTCLIP_INT
   int l,r,t,b;
   int tl,tr,bl,br;
#else
   fix l,r,t,b;
   fix tl,tr,bl,br;
#endif
};

#ifdef PORTCLIP_INT
   #define ROUND_UP(x)  fix_cint(x)
   #define ROUND_DOWN(x) fix_int(x)
#else
#if 1
   #define ROUND_UP(x)  (x)
   #define ROUND_DOWN(x)  (x)
#else
   #define ROUND_UP(x)  fix_ceil(x)
   #define ROUND_DOWN(x)  fix_floor(x)
#endif
#endif

ClipData *ClipAlloc(void);

#define Min(x,y)   ((x) < (y) ? (x) : (y))
#define Max(x,y)   ((x) > (y) ? (x) : (y))

// Compute ClipData from polygon, clipped to existing ClipData
//   returns NULL if entirely clipped away

ClipData *PortalClipFromPolygonNaive(int n, r3s_phandle *p, ClipData *clipsrc)
{
   ClipData *result;
   int l,r,t,b, i;
   int tl,tr,br,bl;

     // find the straight 2d bounding box

   l = r = p[0]->grp.sx;
   t = b = p[0]->grp.sy;

   for (i=1; i < n; ++i) {
      if (p[i]->grp.sx < l) l = p[i]->grp.sx;
      else if (p[i]->grp.sx > r) r = p[i]->grp.sx;
      if (p[i]->grp.sy < t) t = p[i]->grp.sy;
      else if (p[i]->grp.sy > b) b = p[i]->grp.sy;
   }

   l = ROUND_DOWN(l);
   r = ROUND_UP(r);

   l = Max(l, clipsrc->l);
   r = Min(r, clipsrc->r);
   if (l >= r) return FALSE;

   t = ROUND_DOWN(t);
   b = ROUND_UP(b);

   t = Max(t, clipsrc->t);
   b = Min(b, clipsrc->b);
   if (t >= b) return FALSE;

     // find the 45-degree rotated bounding box

   tl = br = p[0]->grp.sy + p[0]->grp.sx;
   tr = bl = p[0]->grp.sx - p[0]->grp.sy;

   for (i=1; i < n; ++i) {
      fix xy = p[i]->grp.sx + p[i]->grp.sy;
      fix yx = p[i]->grp.sx - p[i]->grp.sy;

      if (xy < tl) tl = xy;
      else if (xy > br) br = xy;
      if (yx < bl) bl = yx;
      else if (yx > tr) tr = yx;
   }

   tl = ROUND_DOWN(tl);
   br = ROUND_UP(br);

   tl = Max(tl, clipsrc->tl);
   br = Min(br, clipsrc->br);

   if (tl >= br) return FALSE;

   bl = ROUND_DOWN(bl);
   tr = ROUND_UP(tr);

   bl = Max(bl, clipsrc->bl);
   tr = Min(tr, clipsrc->tr);

   if (bl >= tr) return FALSE;

   result = ClipAlloc();

   result->l = l;
   result->r = r;
   result->t = t;
   result->b = b;

   result->tl = tl;
   result->tr = tr;
   result->bl = bl;
   result->br = br;

   return result;
}

// instead of turning the polygon into an octagon and clipping those,
// we directly clip the polygon by the octagon, and then turn the result
// into an octagon.  this is a tighter bounds.  We could do better if
// we actually stored the portal clipping region as a <= 6 sided polygon,
// and punted to the octagon when it got too complicated.  Union would
// mean taking the convex hull, and intersection would require clipping
// one against the other.  [6 is an invented number]

ClipData *PortalClipFromPolygon(int n, r3s_phandle *p, ClipData *clipsrc)
{
   fix l,r,t,b;
   fix tl,tr,br,bl;
   int i;
   r3s_phandle *q;
   ClipData *result;
   bool old_lighting = clip_lighting, old_poly_clip = poly_clip_fast;

   if (portal_clip_fast)
      return PortalClipFromPolygonNaive(n, p, clipsrc);

   clip_lighting = FALSE;
   poly_clip_fast = FALSE;
   n = portclip_clip_polygon(n, p, &q, clipsrc);
   clip_lighting = old_lighting;
   poly_clip_fast = old_poly_clip;

   if (n < 3) return NULL;

   result = ClipAlloc();

   l = r = q[0]->grp.sx;

   for (i=1; i < n; ++i) {
      if (q[i]->grp.sx < l) l = q[i]->grp.sx;
      else if (q[i]->grp.sx > r) r = q[i]->grp.sx;
   }

   result->l = l;
   result->r = r;

   t = b = q[0]->grp.sy;

   for (i=1; i < n; ++i) {
      if (q[i]->grp.sy < t) t = q[i]->grp.sy;
      else if (q[i]->grp.sy > b) b = q[i]->grp.sy;
   }

   result->t = t;
   result->b = b;

     // find the 45-degree rotated bounding box

   tl = br = q[0]->grp.sy + q[0]->grp.sx;

   for (i=1; i < n; ++i) {
      fix xy = q[i]->grp.sx + q[i]->grp.sy;
      if (xy < tl) tl = xy;
      else if (xy > br) br = xy;
   }

   result->tl = tl;
   result->br = br;

   tr = bl = q[0]->grp.sx - q[0]->grp.sy;

   for (i=1; i < n; ++i) {
      fix yx = q[i]->grp.sx - q[i]->grp.sy;
      if (yx < bl) bl = yx;
      else if (yx > tr) tr = yx;
   }

   result->tr = tr;
   result->bl = bl;

   return result;
}

// Take the ClipData src, add in the clip data s2, and
// return whether it got any larger

bool PortalClipUnion(ClipData *src, ClipData *more)
{
   bool changed = FALSE;

   if (src->l > more->l) { changed = TRUE; src->l = more->l; }
   if (src->r < more->r) { changed = TRUE; src->r = more->r; }
   if (src->t > more->t) { changed = TRUE; src->t = more->t; }
   if (src->b < more->b) { changed = TRUE; src->b = more->b; }

   if (src->tl > more->tl) { changed = TRUE; src->tl = more->tl; }
   if (src->tr < more->tr) { changed = TRUE; src->tr = more->tr; }
   if (src->bl > more->bl) { changed = TRUE; src->bl = more->bl; }
   if (src->br < more->br) { changed = TRUE; src->br = more->br; }

   return changed;
}

  // check if a clipping region overlaps a bounding octagon
extern bool PortalClipOverlap(ClipData *c, fix *min2d, fix *max2d)
{
   if (c->l > max2d[0]) return FALSE;
   if (c->r < min2d[0]) return FALSE;
   if (c->t > max2d[1]) return FALSE;
   if (c->b < min2d[1]) return FALSE;
   if (c->tl > max2d[2]) return FALSE;
   if (c->br < min2d[2]) return FALSE;
   if (c->bl > max2d[3]) return FALSE;
   if (c->tr < min2d[3]) return FALSE;
   return TRUE;
}

bool PortClipTestPoint(ClipData *c, fix x, fix y)
{
#ifdef PORTCLIP_INT
   x = fix_int(x);
   y = fix_int(y);
#endif
   if (x < c->l) return FALSE;
   if (x > c->r) return FALSE;
   if (y < c->t) return FALSE;
   if (y > c->b) return FALSE;

   if (x+y < c->tl) return FALSE;
   if (x+y > c->br) return FALSE;
   if (x-y < c->bl) return FALSE;
   if (x-y > c->tr) return FALSE;

   return TRUE;
}

ClipData *PortalClipRectangle(int l, int t, int r, int b)
{
   ClipData *d = ClipAlloc();

#ifdef PORTCLIP_INT
   d->l = l;
   d->t = t;
   d->r = r;
   d->b = b;
#else
   d->l = fix_make(l,0);
   d->t = fix_make(t,0);
   d->r = fix_make(r,0);
   d->b = fix_make(b,0);
#endif

   d->tl = d->l + d->t;
   d->tr = d->r - d->t;
   d->bl = d->l - d->b;
   d->br = d->r + d->b;

   return d;
}

#define MAX_CLIP_REGIONS     2048

ClipData clip_raw[MAX_CLIP_REGIONS]; // 32K
int clip_first_free=-1, clip_init=0;

void PortalClipInit(void)
{
   int i;
   clip_init = 1;

   for (i=0; i < MAX_CLIP_REGIONS; ++i)
      clip_raw[i].l = i+1;

   clip_raw[MAX_CLIP_REGIONS-1].l = -2;

   clip_first_free = 0;
}

ClipData *ClipAlloc(void)
{
   int n = clip_first_free;
   if (n < 0) {
      if (n == -2)
         Error(1, "ClipAlloc: Scene complexity too high.\n");
      if (!clip_init) {
         PortalClipInit();      
         n = clip_first_free;
      } else
         return NULL;
   }
   clip_first_free = clip_raw[n].l;
   return &clip_raw[n];
}

void PortalClipFree(ClipData *c)
{
   c->l = clip_first_free;
   clip_first_free = c - clip_raw;
}

//////////////////////////////////////////////////////////////

//            clip 2d polygon

// The big problem is that we don't have anywhere to store
// our 2d point codes.  So we have to make our own array of
// "point handles", which store pointers to the r3s_points,
// plus store the clipcodes.

// We might want to cache the x&y values directly into these
// things for speed, but probably not, we probably just need
// the codes rapidly accessible.

#define MAX_INPUT_PTS       32
#define NUMBER_CLIP_PLANES  8
#define MAX_DYNAMIC_PTS     (2 * NUMBER_CLIP_PLANES)
#define MAX_PTS             (MAX_INPUT_PTS + MAX_DYNAMIC_PTS)
#define MAX_OUTPUT_PTS      (MAX_INPUT_PTS + NUMBER_CLIP_PLANES)
   // every time we add two dynamic pts, must lose at least one input pt

typedef struct
{
   r3s_point *p;
   ulong code;
} ClipPoint;

static ClipPoint pts[MAX_PTS];
static int num_pts=0;

  // temporary points we can allocate as we clip
r3s_point clip_results[MAX_DYNAMIC_PTS];
int clip_num_pts=0;

static ClipData curclip;

#define CP_CODE(a)   (pts[a].code)
#define CP_SX(a)     (pts[a].p->grp.sx)
#define CP_SY(a)     (pts[a].p->grp.sy)
#define CP_I(a)      (pts[a].p->grp.i)
#define CP_U(a)      (pts[a].p->grp.u)
#define CP_V(a)      (pts[a].p->grp.v)

#define CP_SXY(a)    (CP_SX(a) + CP_SY(a))
#define CP_SYX(a)    (CP_SX(a) - CP_SY(a))

#define C_L        0
#define C_R        1
#define C_T        2
#define C_B        3
#define C_TL       4
#define C_BR       5
#define C_TR       6
#define C_BL       7

#define C_OFF_L   (1 << C_L)
#define C_OFF_R   (1 << C_R)
#define C_OFF_T   (1 << C_T)
#define C_OFF_B   (1 << C_B)
#define C_OFF_TL  (1 << C_TL)
#define C_OFF_BR  (1 << C_BR)
#define C_OFF_TR  (1 << C_TR)
#define C_OFF_BL  (1 << C_BL)

int code_point(int a)
{
   int x = CP_SX(a), y = CP_SY(a);
   int xy = x + y, yx = x - y;
   int code;

   if (x < curclip.l) code = C_OFF_L;
   else if (x > curclip.r) code = C_OFF_R;
   else code = 0;

   if (y < curclip.t) code |= C_OFF_T;
   else if (y > curclip.b) code |= C_OFF_B;

   if (xy < curclip.tl) code |= C_OFF_TL;
   else if (xy > curclip.br) code |= C_OFF_BR;

   if (yx > curclip.tr) code |= C_OFF_TR;
   else if (yx < curclip.bl) code |= C_OFF_BL;
      
   CP_CODE(a) = code;
   return code;
}

void clip2d_intersect(fix x, fix *dx, fix *dy, fix x1, fix y1, fix x2, fix y2, float *i, float i1, float i2)
{
   if (x2 < x1) {
      fix temp;
      float ti;
      temp = x1;
      x1 = x2;
      x2 = temp;
      temp = y1;
      y1 = y2;
      y2 = temp;
      ti = i1;
      i1 = i2;
      i2 = ti;
   }

   *dx = x;
#ifdef WIN32
   *dy = y1 + (double) (y2-y1) * (x-x1) / (x2-x1);
#else
   *dy = y1 + fix_mul_div(y2-y1, x-x1, x2-x1);
#endif
   if (clip_lighting)
      *i  = i1 + (i2-i1)*(x-x1)/(x2-x1);
}

void clip2d_intersect_uv(fix x, fix *dx, fix *dy, fix x1, fix y1, fix x2, fix y2, ClipPoint *dest, ClipPoint *p1, ClipPoint *p2)
{
   if (x1 < x2) {
      double interp = (double) (x - x1) / (x2 - x1);
      *dx = x;
      *dy = y1 + (y2-y1) * interp;
      if (clip_lighting)
         dest->p->grp.i  = p1->p->grp.i + (p2->p->grp.i - p1->p->grp.i)*interp;

      dest->p->grp.w = p1->p->grp.w + (p2->p->grp.w - p1->p->grp.w) * interp;
      dest->p->p.z = 1 / dest->p->grp.w;
   } else {
      double interp = (double) (x - x2) / (x1 - x2);
      *dx = x;
      *dy = y2 + (y1-y2) * interp;
      if (clip_lighting)
         dest->p->grp.i  = p2->p->grp.i + (p1->p->grp.i - p2->p->grp.i)*interp;

      dest->p->grp.w = p2->p->grp.w + (p1->p->grp.w - p2->p->grp.w) * interp;
      dest->p->p.z = 1 / dest->p->grp.w;
   }
}

void clip2d_compute_intersection(int dest, int p1, int p2, int pass)
{
   fix xy,yx;
#ifdef DBG_ON
   CP_SX(dest) = -1;
   CP_SY(dest) = -1;
#endif

   switch(pass+(g_lgd3d<<3)) {  // determine which plane & what to clip

  // CLIPPING JUST (x,y,i)

      case C_L:
         clip2d_intersect(curclip.l, &CP_SX(dest), &CP_SY(dest), CP_SX(p1),
             CP_SY(p1), CP_SX(p2), CP_SY(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         break;
      case C_R:
         clip2d_intersect(curclip.r, &CP_SX(dest), &CP_SY(dest), CP_SX(p1),
             CP_SY(p1), CP_SX(p2), CP_SY(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         break;
      case C_T:
         clip2d_intersect(curclip.t, &CP_SY(dest), &CP_SX(dest), CP_SY(p1),
             CP_SX(p1), CP_SY(p2), CP_SX(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         break;
      case C_B:
         clip2d_intersect(curclip.b, &CP_SY(dest), &CP_SX(dest), CP_SY(p1),
             CP_SX(p1), CP_SY(p2), CP_SX(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         break;

      case C_TL:
         clip2d_intersect(curclip.tl, &xy, &yx, CP_SXY(p1), CP_SYX(p1),
             CP_SXY(p2), CP_SYX(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         goto recode_xy;
      case C_BR:
         clip2d_intersect(curclip.br, &xy, &yx, CP_SXY(p1), CP_SYX(p1),
             CP_SXY(p2), CP_SYX(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         goto recode_xy;
      case C_TR:
         clip2d_intersect(curclip.tr, &yx, &xy, CP_SYX(p1), CP_SXY(p1),
             CP_SYX(p2), CP_SXY(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         goto recode_xy;
      case C_BL:
         clip2d_intersect(curclip.bl, &yx, &xy, CP_SYX(p1), CP_SXY(p1),
             CP_SYX(p2), CP_SXY(p2), &CP_I(dest), CP_I(p1), CP_I(p2));
         goto recode_xy;


  // CLIPPING (x,y,u,v,i)

      case C_L + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.l, &CP_SX(dest), &CP_SY(dest), CP_SX(p1),
             CP_SY(p1), CP_SX(p2), CP_SY(p2), &pts[dest], &pts[p1], &pts[p2]);
         break;
      case C_R + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.r, &CP_SX(dest), &CP_SY(dest), CP_SX(p1),
             CP_SY(p1), CP_SX(p2), CP_SY(p2), &pts[dest], &pts[p1], &pts[p2]);
         break;
      case C_T + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.t, &CP_SY(dest), &CP_SX(dest), CP_SY(p1),
             CP_SX(p1), CP_SY(p2), CP_SX(p2), &pts[dest], &pts[p1], &pts[p2]);
         break;
      case C_B + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.b, &CP_SY(dest), &CP_SX(dest), CP_SY(p1),
             CP_SX(p1), CP_SY(p2), CP_SX(p2), &pts[dest], &pts[p1], &pts[p2]);
         break;

      case C_TL + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.tl, &xy, &yx, CP_SXY(p1), CP_SYX(p1),
             CP_SXY(p2), CP_SYX(p2), &pts[dest], &pts[p1], &pts[p2]);
         goto recode_xy;
      case C_BR + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.br, &xy, &yx, CP_SXY(p1), CP_SYX(p1),
             CP_SXY(p2), CP_SYX(p2), &pts[dest], &pts[p1], &pts[p2]);
         goto recode_xy;
      case C_TR + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.tr, &yx, &xy, CP_SYX(p1), CP_SXY(p1),
             CP_SYX(p2), CP_SXY(p2), &pts[dest], &pts[p1], &pts[p2]);
         goto recode_xy;
      case C_BL + C_CLIP_UV_OFFSET:
         clip2d_intersect_uv(curclip.bl, &yx, &xy, CP_SYX(p1), CP_SXY(p1),
             CP_SYX(p2), CP_SXY(p2), &pts[dest], &pts[p1], &pts[p2]);
         goto recode_xy;

      recode_xy:
         CP_SX(dest) = (xy + yx)/2;
         CP_SY(dest) = xy - CP_SX(dest);
         break;

      default:
         Error(1, "clip2d_compute_intersection: Invalid clip code type\n");
   }
   code_point(dest);
#ifdef DBG_ON
   if (CP_SX(dest) == -1 && CP_SY(dest) == -1)
      Error(1, "clip2d_compute_intersection: failed mysteriously\n");
#endif
}

static int poly[MAX_OUTPUT_PTS], poly2[MAX_OUTPUT_PTS];
static r3s_phandle poly_out[MAX_OUTPUT_PTS];

int portclip_clip_polygon(int n, r3s_phandle *p, r3s_phandle **q, ClipData *c)
{
   // compute clip codes
   int i, code, c_and, c_or, clip_pass;
   int *src  = poly;
   int *dest = poly2;      
   int *temp;

#if 0
   curclip.l = fix_floor(c->l);
   curclip.r = fix_ceil(c->r);
   curclip.t = fix_floor(c->t);
   curclip.b = fix_ceil(c->b);
   curclip.tl = fix_floor(c->tl);
   curclip.br = fix_ceil(c->br);
   curclip.bl = fix_floor(c->bl);
   curclip.tr = fix_ceil(c->tr);
#else
   curclip = *c;
#endif

   c_and = 0xff;
   c_or  = 0;

#ifdef DBG_ON
   if (n > MAX_INPUT_PTS)
      Error(1, "portclip_clip_poly: MAX_INPUT_POINTS exceeded.\n");
#endif

   for (i=0; i < n; ++i) {
      pts[i].p = p[i];
      src[i] = i;
#ifdef DBG_ON
      if (CP_SX(src[i]) < fix_make(-1,0))
         Error(1, "portclip_clip_poly: Bad pt in list 0!\n");
#endif
      code = code_point(i);
      c_and &= code;
      c_or  |= code;
   }

   if (c_and) return 0;  // trivial reject

   if (q==NULL) // only wanted to check for trivial reject...
      return n;

   if (poly_clip_fast || !c_or) {
      *q = p;
      return n;
   } // trivial accept

   clip_num_pts = 0;
   num_pts = n;

   // now try clipping against all of the appropriate codes
   i = 1;
   clip_pass = 0;
     
   while (c_or >= i) {
      if (c_or & i) {
         int m=0,j,k = n-1;

         // iterate over all the vertices
         for (j=0; j < n; ++j) {
#ifdef DBG_ON
            if (CP_SX(src[k]) < fix_make(-1,0))
               Error(1, "portclip_clip_poly: Bad pt in list 1!\n");
#endif
            if (!(CP_CODE(src[k]) & i))   // k is in, so output it
               dest[m++] = src[k];

            if ((CP_CODE(src[j]) ^ CP_CODE(src[k])) & i) {
#ifdef DBG_ON
               if (num_pts >= MAX_PTS)
                  Error(1,"portclip_clip_poly: exceeded MAX_PTS\n");
               if (clip_num_pts >= MAX_DYNAMIC_PTS)
                  Error(1,"portclip_clip_poly: exceeded MAX_DYNAMIC_PTS\n");
               if (m >= MAX_OUTPUT_PTS)
                  Error(1,"portclip_clip_poly: exceeded MAX_OUTPUT_PTS\n");
#endif
               // we cross from in to out or vice versa here
               pts[num_pts].p = &clip_results[clip_num_pts++];
               clip2d_compute_intersection(num_pts, src[k], src[j], clip_pass);
               dest[m] = num_pts++;
#ifdef DBG_ON
               if (CP_SX(dest[m]) < fix_make(-1,0))
                  Error(1, "portclip_clip_poly: Bad pt in list 4!\n");
#endif
               ++m;
            }
            k = j;
         }
         if (m == 0) return 0;
#ifdef DBG_ON
         if (m > MAX_OUTPUT_PTS)
            Error(1, "portclip_clip_poly: exceeded MAX_OUTPUT_PTS #2\n");
#endif
         n = m;
         temp = src; src = dest; dest = temp;

            // resum the points if there seems to be more clipping to do
         if (c_or > i) {
            c_or = 0;
            c_and = 0xff;
            for (j=0; j < n; ++j) {
               code = CP_CODE(src[j]);
               c_and &= code;
               c_or  |= code;
            }
            if (c_and) return 0;
#ifdef DBG_ON
#ifndef WIN32 // allow for floating pt slop
            if (c_or & i)        // did we clip it ok?
               Error(1, "portclip_clip_poly: clip %x failed\n", i);
            if (c_or & (i - 1))  // did we screw up old clipping?
               Error(1, "portclip_clip_poly: invalid clip %x:%x\n", i, c_or);
#endif
#endif
         }
      }
      i <<= 1;
      ++clip_pass;
   }

   // copy points from src into temp output array
   for (i=0; i < n; ++i) {
#ifdef DBG_ON
      if (src[i] >= num_pts)
         Error(1, "portclip_clip_poly: Bad pt in list 2!\n");
      if (CP_SX(src[i]) < fix_make(-1,0))
         Error(1, "portclip_clip_poly: Bad pt in list 3!\n");
#endif
      poly_out[i] = pts[src[i]].p;      
   }

   *q = poly_out;

   return n;
}
