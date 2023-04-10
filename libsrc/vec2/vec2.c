// $Header: x:/prj/tech/libsrc/vec2/RCS/vec2.c 1.2 1997/08/22 18:43:11 dc Exp $

#include <stdlib.h>

#include <vec2.h>

mxs_real Vec2Dist(Vec2 *v1, Vec2 *v2)
{ 
    Vec2 v;

    v.x = v1->x-v2->x;
    v.y = v1->y-v2->y;
    return Vec2Mag(&v);
}

mxs_real Vec2PointLineDist(Vec2 *p, Vec2 *l1, Vec2 *l2)
{
   Vec2 dist;
   Vec2 norm;
   
   Vec2Sub(&dist, p, l1);
   Vec2LHSNorm(&norm, l1, l2);
   return Vec2Dotprod(&dist, &norm);
}

// returns squared distance to point
// NOTE: is much faster when you are off segment
mxs_real Vec2PointSegmentSquared(Vec2 *p, Vec2 *l1, Vec2 *l2)
{
   Vec2 p_l1, p_l2, along_l;
   float num, den;

   Vec2Sub(&along_l,l2,l1);
   Vec2Sub(&p_l1,p,l1);
   if (Vec2Dotprod(&p_l1,&along_l)<0.0)  // we are behind line
      return Vec2MagSquared(&p_l1);
   Vec2Sub(&p_l2,p,l2);
   if (Vec2Dotprod(&p_l2,&along_l)>0.0)  // we are past line
      return Vec2MagSquared(&p_l2);

   // this ugly code is because, if we do the math, we see....
   // we want dist squared, where dist = (unit norm) . (vec to point from l1)
   //  if we express the line as lx,ly, a non-unit norm nx,ny is <ly,-lx>
   //  the unit norm is then <nx/|n|, ny/|n|>.  if we dot with px,py we get
   //  pxnx/|n| + pyny/|n| as dist, but we are allowed to square it
   // so... we get (pxnx+pyny)^2/|n|^2, but |n| is sqrt(nxnx+nyny), so, we get
   //  (pxnx+pyny)^2/(nxnx+nyny), and the num is really (-pxly+pylx)^2 

   num=p_l1.x*(-along_l.y)+p_l1.y*along_l.x;
   den=(along_l.x*along_l.x)+(along_l.y*along_l.y);
   return num*num/den;
}

// this is pretty slow...
mxs_real Vec2PointSegmentClosest(Vec2 *p, Vec2 *l1, Vec2 *l2, mxs_real *s, Vec2 *c)
{
   Vec2 norm;
   Vec2 dl, dl1, dl2, dv, travel;
   mxs_real dp;
   mxs_real m1, m2;
   
   Vec2Sub(&dl1, p, l1);           // i think these 3 lines are just the 
   Vec2LHSNorm(&norm, l1, l2);     //   function vec2pointlinedist above?
   dp = Vec2Dotprod(&dl1, &norm);
   Vec2Scale(&dl, &norm, dp);
   Vec2Sub(&travel, &dl1, &dl);
   Vec2Sub(&dv, l2, l1);
   *s = Vec2Dotprod(&travel, &dv)/Vec2Dotprod(&dv, &dv);
   if ((*s>=0) && (*s<=1))
   {
      Vec2Add(c, l1, &travel);
      return dp;
   }
   else 
   {
      Vec2Sub(&dl2, p, l2);
      m1 = Vec2Mag(&dl1);
      m2 = Vec2Mag(&dl2);
      if (m1<m2)
      {
         *s = 0;
         *c = *l1;
         return m1;
      }
      else
      {
         *s = 1;
         *c = *l2;
         return m2;
      }
   }
}

// Warning: returns negative if point is on one side of segment, positive if on other
mxs_real Vec2PointSegmentDist(Vec2 *p, Vec2 *l1, Vec2 *l2)
{
   mxs_real s;
   Vec2 c;
   
   return Vec2PointSegmentClosest(p, l1, l2, &s, &c);
}

// Warning: n must be different from v0 & v1
void Vec2RHSNorm(Vec2 *n, Vec2 *v0, Vec2 *v1)
{
   mxs_real mag;
   
   n->x = v1->y-v0->y;
   n->y = v0->x-v1->x;
   mag = Vec2Mag(n);
   Vec2Div(n, n, mag);
} 

void Vec2LHSNorm(Vec2 *n, Vec2 *v0, Vec2 *v1)
{
   mxs_real mag;
   
   n->x = v0->y-v1->y;
   n->y = v1->x-v0->x;
   mag = Vec2Mag(n);
   Vec2Div(n, n, mag);
} 

/*
   True if the line pt1->pt2 crosses the line pt3->pt4 leftwards (has a positive dot product
   with the LHS norm of pt3->pt4).
*/
bool Vec2LHSLine(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4)
{
   Vec2 n;
   Vec2 v;
   
   Vec2LHSNorm(&n, pt3, pt4);
   Vec2Sub(&v, pt2, pt1);
   return (Vec2Dotprod(&v, &n)>0);
}

bool Vec2RHSLine(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4)
{
   Vec2 n;
   Vec2 v;
   
   Vec2RHSNorm(&n, pt3, pt4);
   Vec2Sub(&v, pt2, pt1);
   return (Vec2Dotprod(&v, &n)>0);
}

bool InPoly2Inclusive(mxs_real x, mxs_real y, Poly2 *poly)
{
   int i;
   bool outside = FALSE;
   mxs_real dx, dy, dx1, dy1;
   
   for (i=0; (i<poly->v_num) && !outside; i++)
   {
      if (i==poly->v_num-1)
      {
         dx = poly->v[0].x-poly->v[i].x;
         dy = poly->v[0].y-poly->v[i].y;
      }
      else
      {
         dx = poly->v[i+1].x-poly->v[i].x;
         dy = poly->v[i+1].y-poly->v[i].y;
      }
      dx1 = x-poly->v[i].x;
      dy1 = y-poly->v[i].y;
      outside = ((dx*dy1-dy*dx1)<0);
   }
   return !outside;
}

bool InPoly2Exclusive(mxs_real x, mxs_real y, Poly2 *poly)
{
   int i;
   bool outside = FALSE;
   mxs_real dx, dy, dx1, dy1;
   
   for (i=0; (i<poly->v_num) && !outside; i++)
   {
      if (i==poly->v_num-1)
      {
         dx = poly->v[0].x-poly->v[i].x;
         dy = poly->v[0].y-poly->v[i].y;
      }
      else
      {
         dx = poly->v[i+1].x-poly->v[i].x;
         dy = poly->v[i+1].y-poly->v[i].y;
      }
      dx1 = x-poly->v[i].x;
      dy1 = y-poly->v[i].y;
      outside = ((dx*dy1-dy*dx1)<=0);
   }
   return !outside;
}

/*
   Add a vertex to a polygon at index va, shifting up all successive vertices.
   Return success (fails if already at max vertices).
*/
bool Poly2VertexAdd(Poly2 *poly, Vec2 *pt, ushort va)
{
   ushort v;
   
   if (poly->v_num == MAX_POLY2_VERTICES-1)
      return FALSE;
   for (v=poly->v_num; v>va; v--)
   {
      poly->v[v] = poly->v[v-1];
   }
   poly->v[va] = *pt;
   ++poly->v_num;
   return TRUE;
}

// Return TRUE if two points are sufficiently close to merge
bool Vec2Same(Vec2 *pt1, Vec2 *pt2, mxs_real dist)
{
   Vec2 diff;
   
   Vec2Sub(&diff, pt1, pt2);
   return Vec2Mag(&diff)<dist;
}

void Vec2PointFromParam(Vec2 *pt1, Vec2 *pt2, mxs_real s, Vec2 *pt3)
{
   Vec2 d1;
   
   Vec2Sub(&d1, pt2, pt1);
   Vec2Scale(&d1, &d1, s);
   Vec2Add(pt3, pt1, &d1);
}

mxs_real Vec2ParamFromPoint(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3)
{
   Vec2 d1, d2;
   mxs_real num, denom;
   
   Vec2Sub(&d1, pt2, pt1);
   Vec2Sub(&d2, pt3, pt1);
   num = Vec2Dotprod(&d1, &d2);
   denom = Vec2Dotprod(&d1, &d1);
   return Vec2Dotprod(&d1, &d2)/Vec2Dotprod(&d1, &d1);
}

bool Vec2IntersectLines(Vec2 *p1, Vec2 *p2, Vec2 *p3, Vec2 *p4, mxs_real *s)
{
   Vec2 d21, d13, d43;
   mxs_real num, denom;
   
   Vec2Sub(&d21, p2, p1);
   Vec2Sub(&d13, p1, p3);
   Vec2Sub(&d43, p4, p3);
   denom = d43.y*d21.x-d21.y*d43.x;
   num = d13.y*d43.x-d43.y*d13.x;
/* 
   be nice to have an overflow check for floats
   if (real_div_overflow(num, denom))
      return FALSE;
*/
   *s = num/denom;
   return TRUE;
}

/*
   Check to see if the two segments, pt1->pt2 & pt3->pt4 intersect.
   Includes checking to see if any point lies exactly on an edge with a margin for error described by slop.
   Set the intersect pt and s to be the parameter of the line pt1->pt2 at which the intersection occurs.
   Return FALSE if segments meet at either end point.
*/
bool Vec2IntersectLinesSloppy(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop)
{
   if (Vec2Same(pt1, pt3, slop))
   {
      *intersect_pt = *pt1;
      *s = 0;
      *t = 0;
      return TRUE;
   }
   if (Vec2Same(pt1, pt4, slop))
   {
      *intersect_pt = *pt1;
      *s = 0;
      *t = 1;
      return TRUE;
   }
   if (Vec2Same(pt2, pt3, slop))
   {
      *intersect_pt = *pt2;
      *s = 1;
      *t = 0;
      return TRUE;
   }
   if (Vec2Same(pt2, pt4, slop))
   {
      *intersect_pt = *pt2;
      *s = 1;
      *t = 1;
      return TRUE;
   }
   if (fabs(Vec2PointSegmentDist(pt1, pt3, pt4))<slop)
   {
      *intersect_pt = *pt1;
      *s = 0;
      *t = Vec2ParamFromPoint(pt3, pt4, intersect_pt);
      return TRUE;
   }
   if (fabs(Vec2PointSegmentDist(pt2, pt3, pt4))<slop)
   {
      *intersect_pt = *pt2;
      *s = 1;
      *t = Vec2ParamFromPoint(pt3, pt4, intersect_pt);
      return TRUE;
   }
   if (fabs(Vec2PointSegmentDist(pt3, pt1, pt2))<slop)
   {
      *intersect_pt = *pt3;
      *s = Vec2ParamFromPoint(pt1, pt2, intersect_pt);
      *t = 0;
      return TRUE;
   }
   if (fabs(Vec2PointSegmentDist(pt4, pt1, pt2))<slop)
   {
      *intersect_pt = *pt4;
      *s = Vec2ParamFromPoint(pt1, pt2, intersect_pt);
      *t = 1;
      return TRUE;
   }
   if (Vec2IntersectLines(pt1, pt2, pt3, pt4, s))
   {
      Vec2PointFromParam(pt1, pt2, *s, intersect_pt);
      *t = Vec2ParamFromPoint(pt3, pt4, intersect_pt);
      return TRUE;
   }
   return FALSE;
}

bool Vec2IntersectSegments(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop)
{
   return (Vec2IntersectLinesSloppy(pt1, pt2, pt3, pt4, intersect_pt, s, t, slop) && (*s>=0) && (*s<1) && (*t>=0) && (*t<=1));
}

bool Vec2IntersectLineAndSegment(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop)
{
   return (Vec2IntersectLinesSloppy(pt1, pt2, pt3, pt4, intersect_pt, s, t, slop) && (*t>=0) && (*t<=1));
}

#define FIXANG_PI_2 0x4000

/*
   Calculate a tangent to circle with centre c and radius r that also passes through point p.
   Set the intersection point of this tangent with the circle and the angle that it makes with p.
*/
void Vec2TangentThruPoint(Vec2 *c, mxs_real r, Vec2 *p, Vec2 *tangent_pt, fixang *tangent_angle, uchar dir)
{
   mxs_real d;
   fixang theta, phi, sigma;
   
   d = Vec2Dist(c, p);
   theta = fix_acos(fix_from_float(r/d));
   phi = fix_atan2(fix_from_float(p->y-c->y), fix_from_float(p->x-c->x));
   sigma = (dir==VEC2_RIGHT)?phi+theta:phi-theta;
   tangent_pt->x = r*fix_float(cos(sigma));
   tangent_pt->y = r*fix_float(sin(sigma));
   Vec2Add(tangent_pt, tangent_pt, c);
   *tangent_angle = FIXANG_PI_2-((dir==VEC2_RIGHT)?phi+theta:phi-theta);
}

bool Vec2IntersectLineAndCircle(Vec2 *pt1, Vec2 *pt2, Vec2 *centre, mxs_real r, mxs_real *t1, mxs_real *t2)
{
   mxs_real a, b, c, d;
   Vec2 diff;
   Vec2 v;
   mxs_real s1, s2;
   
   Vec2Sub(&diff, pt1, centre);
   Vec2Sub(&v, pt2, pt1);
   a = Vec2Dotprod(&v, &v);
   b = 2*Vec2Dotprod(&v, &diff);
   c = Vec2Dotprod(&diff, &diff)-r*r;
   d = b*b-4*a*c;
   if (d<0)
      return FALSE;
   s1 = -b+sqrt(d)/(2*a);
   s2 = -b-sqrt(d)/(2*a);
   *t1 = min(s1, s2);
   *t2 = max(s1, s2);
   return TRUE;
}

bool Vec2IntersectLineAndCirclePt(Vec2 *pt1, Vec2 *pt2, Vec2 *centre, mxs_real r, Vec2 *ip1, Vec2 *ip2, mxs_real *t1, mxs_real *t2)
{
   if (Vec2IntersectLineAndCircle(pt1, pt2, centre, r, t1, t2))
   {
      Vec2PointFromParamSegment(ip1, pt1, pt2, *t1);
      Vec2PointFromParamSegment(ip2, pt1, pt2, *t2);
      return TRUE;   
   }
   return FALSE;   
}

void Vec2PointFromParamVector(Vec2 *pt, Vec2 *p, Vec2 *v, mxs_real t)
{
   Vec2 w;
   
   Vec2Scale(&w, v, t);
   Vec2Add(pt, p, &w);
}

void Vec2PointFromParamSegment(Vec2 *pt, Vec2 *p, Vec2 *q, mxs_real t)
{
   Vec2 v;
   
   Vec2Sub(&v, q, p);
   Vec2PointFromParamVector(pt, p, &v, t);
}

