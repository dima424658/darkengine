// $Header: x:/prj/tech/libsrc/vec2/RCS/vec2.h 1.4 1997/12/16 15:28:41 TOML Exp $
// 2d vector math based on mxs_reals
// This stuff implicitly references some stuff that assumes mxs_reals
// are floats - eg uses the * operator, float trig functions, etc.

#ifndef __VEC2_H
#define __VEC2_H

#include <matrixs.h>
#include <fix.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Vec2
{
   union
   {
      struct {mxs_real x,y;};
      mxs_real xy[2];
   };
} Vec2;

#define MAX_POLY2_VERTICES 10
typedef struct Poly2
{
   ushort v_num;
   Vec2 v[MAX_POLY2_VERTICES];
} Poly2;

#define Vec2Zero(dest) \
{ \
   (dest)->x = 0; \
   (dest)->y = 0; \
}

#define Vec2Unit(dest) \
{ \
   (dest)->x = MXS_REAL_UNIT; \
   (dest)->y = MXS_REAL_UNIT; \
}

#define Vec2From3(v2, v3) {(v2)->x = (v3)->x; (v2)->y = (v3)->y;}

#define Vec2Equal(v1, v2) (((v1)->x == (v2)->x) && ((v1)->y == (v2)->y))
#define Vec2Reverse(v1, v2) ((-1*(v1)->x == (v2)->x) && (-1*(v1)->y == (v2)->y))

#define Vec2Copy(dest, src) {(dest)->x=(src)->x;(dest)->y=(src)->y;}

#define Vec2Sub(dest, src1, src2) \
{ \
   (dest)->x = (src1)->x-(src2)->x; \
   (dest)->y = (src1)->y-(src2)->y; \
}

#define Vec2Add(dest, src1, src2) \
{ \
   (dest)->x = (src1)->x+(src2)->x; \
   (dest)->y = (src1)->y+(src2)->y; \
}

#define Vec2Mag(v) (sqrt((v)->x*(v)->x+(v)->y*(v)->y))
// in case you want fast code
#define Vec2MagSquared(v) ((v)->x*(v)->x+(v)->y*(v)->y)

extern mxs_real Vec2Dist(Vec2 *v1, Vec2 *v2);

#define Vec2Scale(dest, src, s) \
{ \
   (dest)->x = (src)->x*(s); \
   (dest)->y = (src)->y*(s); \
}

#define Vec2Div(dest, src, s) \
{ \
   (dest)->x = (src)->x/(s); \
   (dest)->y = (src)->y/(s); \
}

#define Vec2FastNormalize(v) \
{ \
   mxs_real mag = Vec2FastMag(v); \
   Vec2Div(v, v, mag); \
}

#define Vec2Normalize(v) \
{ \
   mxs_real mag = Vec2Mag(v); \
   Vec2Div(v, v, mag); \
}

#define Vec2Dotprod(v0, v1) ((v0)->x*(v1)->x+(v0)->y*(v1)->y)
#define Vec2Cos(v0, v1) (Vec2Dotprod((v0), (v1))/(Vec2Mag(v0)*Vec2Mag(v1)))
#define Vec2Angle(v0, v1) (atan2((v1)->y-(v0)->y, (v1)->x-(v0)->x))

extern mxs_real Vec2PointLineDist(Vec2 *p, Vec2 *l1, Vec2 *l2);
extern mxs_real Vec2PointSegmentDist(Vec2 *p, Vec2 *l1, Vec2 *l2);
extern mxs_real Vec2PointSegmentClosest(Vec2 *p, Vec2 *l1, Vec2 *l2, mxs_real *s, Vec2 *c);

// returns squared distance to point
// NOTE: this is much faster when you are off end of segment
mxs_real Vec2PointSegmentSquared(Vec2 *p, Vec2 *l1, Vec2 *l2);

extern void Vec2LHSNorm(Vec2 *n, Vec2 *v0, Vec2 *v1);
extern void Vec2RHSNorm(Vec2 *n, Vec2 *v0, Vec2 *v1);

extern bool Vec2LHSLine(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4);
extern bool Vec2RHSLine(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4);

extern bool InPoly2Inclusive(mxs_real x, mxs_real y, Poly2 *poly);
extern bool InPoly2Exclusive(mxs_real x, mxs_real y, Poly2 *poly);
#define Vec2InPoly2Inclusive(v, poly) InPoly2Inclusive((v)->x, (v)->y, poly)
#define Vec2InPoly2Exclusive(v, poly) InPoly2Exclusive((v)->x, (v)->y, poly)

extern bool Poly2VertexAdd(Poly2 *poly, Vec2 *pt, ushort va);

bool Vec2IntersectLines(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, mxs_real *s);
bool Vec2IntersectLinesSloppy(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop);
bool Vec2IntersectSegments(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop);
bool Vec2IntersectLineAndSegment(Vec2 *pt1, Vec2 *pt2, Vec2 *pt3, Vec2 *pt4, Vec2 *intersect_pt, mxs_real *s, mxs_real *t, mxs_real slop);

#define POLY2_NEXT_V(poly, vi) (((vi)==(poly)->v_num-1)?0:(vi)+1)
#define POLY2_PREV_V(poly, vi) (((vi)==0)?(poly)->v_num-1:(vi)-1)
#define POLY2_NEXT(poly, vi) (&((poly)->v[POLY2_NEXT_V(poly, vi)]))
#define POLY2_PREV(poly, vi) (&((poly)->v[POLY2_PREV_V(poly, vi)]))

// determine if a point is on the right hand side of a line pt1->pt2
// not strict (on line is OK)
#define VEC2_SIDE_PTS(pt, pt1, dx, dy) ((dx)*((pt)->y-(pt1)->y)-(dy)*((pt)->x-(pt1)->x))

#define VEC2_RHS_PTS(pt, pt1, pt2) (VEC2_RHS_PT(pt, pt1, (pt2)->x-(pt1)->x, (pt2)->y-(pt1)->y))
#define VEC2_LHS_PTS(pt, pt1, pt2) (VEC2_LHS_PT(pt, pt1, (pt2)->x-(pt1)->x, (pt2)->y-(pt1)->y))
#define VEC2_NHS_PTS(pt, pt1, pt2) (VEC2_NHS_PT(pt, pt1, (pt2)->x-(pt1)->x, (pt2)->y-(pt1)->y))
#define VEC2_RHS_PT(pt, pt1, dx, dy) (VEC2_SIDE_PTS(pt, pt1, dx, dy)<0)
#define VEC2_LHS_PT(pt, pt1, dx, dy) (VEC2_SIDE_PTS(pt, pt1, dx, dy)>0)
#define VEC2_NHS_PT(pt, pt1, dx, dy) (VEC2_SIDE_PTS(pt, pt1, dx, dy)==0)

#define VEC2_LEFT 0
#define VEC2_RIGHT 1

void Vec2TangentThruPoint(Vec2 *c, mxs_real r, Vec2 *p, Vec2 *tangent_pt, fixang *tangent_angle, uchar dir);
#define Vec2CirclesIntersect(c0, r0, c1, r1) (float_pyth_dist((c0)->x-(c1)->x, (c0)->y-(c1)->y)<=(r0)+(r1))
bool Vec2IntersectLineAndCircle(Vec2 *pt1, Vec2 *pt2, Vec2 *c, mxs_real r, mxs_real *t1, mxs_real *t2);
bool Vec2IntersectLineAndCirclePt(Vec2 *pt1, Vec2 *pt2, Vec2 *c, mxs_real r, Vec2 *ip1, Vec2 *ip2, mxs_real *t1, mxs_real *t2);
#define Vec2IntersectSegmentAndCircle(pt1, pt2, c, r, t1, t2) \
   (Vec2IntersectLineAndCircle(pt1, pt2, c, r, t1, t2) && (((*t1>=0) && (*t1<=MXS_REAL_UNIT)) || ((*t2>=0) && (*t2<=MXS_REAL_UNIT))))
#define Vec2IntersectSegmentAndCirclePt(pt1, pt2, c, r, ip1, ip2, t1, t2) \
   (Vec2IntersectLineAndCirclePt(pt1, pt2, c, r, ip1, ip2, t1, t2) && (((*t1>=0) && (*t1<=MXS_REAL_UNIT)) || ((*t2>=0) && (*t2<=MXS_REAL_UNIT))))

void Vec2PointFromParamVector(Vec2 *pt, Vec2 *p, Vec2 *v, mxs_real t);
void Vec2PointFromParamSegment(Vec2 *pt, Vec2 *p, Vec2 *q, mxs_real t);

#ifdef __cplusplus
};
#endif

#endif
