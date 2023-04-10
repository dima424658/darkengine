/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/merge.c,v 1.17 2000/02/19 13:18:33 toml Exp $

// Portal database optimizer

// This is a standalone program because I wanted it
// to link faster, so it works under DOS, which means
// it's standalone.

// It uses the world-rep database format, so it includes
// that, but it doesn't use the normal world-rep global
// variables and such, so it doesn't link with anything

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lg.h>

#include <matrix.h>
#include <math.h>
#include <wr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define EXACT_MATCH

int allocations;

void MyFree(void *p)
{
   --allocations;
   Free(p);
}

#ifdef Free
#undef Free
#endif

#define Free  MyFree

bool debug;

#define VALIDATE_SIDEDNESS

//////////////////    STATIC LIMITS    //////////////////////

#define MAX_GLOBAL_POINTS     (65536*2)  // 64*2*16K == 2^(6+1+4+10) = 2^21
#define MAX_GLOBAL_PLANES     16384
#define MAX_GLOBAL_RENDER     8000
#define POINT_HASH            16

//////////////////    BEHINDEDNESS     //////////////////////

enum {
  COPLANAR=0,
  INSIDE=1,
  OUTSIDE=2,
  CROSSES=3
};

typedef int Side;

// note CROSSES == INSIDE|OUTSIDE!!!


int max_alloc;

///////////////////   MEMORY MANAGEMENT   //////////////////////

void *MallocSafe(size_t sz)
{
   void *p = Malloc(sz);
   if (sz == 0) {
      printf("Allocated 0.\n");
      return 0;
   }
   if (!p) Error(1, "Out of memory allocating %d.\n", sz);
   if (++allocations > max_alloc) max_alloc = allocations;
   memset(p, 0xAA, sz);
   return p;
}

void *MallocCounted(size_t sz)
{
   void *p = MallocSafe(sz + sizeof(int));
   * (int *) p = 0;
   return (int *) p + 1;
}

#define COUNT(p)  (((int *) p) - 1)

int FreeCounted(void *p)
{
   Free(COUNT(p));
   return 0;
}

int AddCount(void *p)
{
   * COUNT(p) += 1;
   return 0;
}

int SubCount(void *p, char *func)
{
   int *q = COUNT(p);
   if ((*q -= 1) <= 0) {
      if (*q < 0)
         Error(1, "RefCount decremented below 0: %s!\n", func); 
      return 1;
   }
   return 0;
}

int SubCountFree(void *p, char *name)
{
   if (SubCount(p, name))
      FreeCounted(p);
   return 0;
}

int RefCount(void *p)
{
   return *COUNT(p);
}

  // macros to update ref counts while setting fields

#define SET_PTR(x,y)    (AddCount(y), (x)=(y))
#define CHANGE_PTR(x,y,name) (AddCount(y), SubCountFree(x,name), (x)=(y))
#define CLEAR_PTR(x,y,name)  (SubCountFree(x,name), (x)=y)

  // macros to update ref counts & call a cleanup func

#define CHANGE_PTR_CLEANUP(x,y,func,name) \
    (AddCount(y), (SubCount(x,name) ? func(x) : 0), (x) = (y))

#define CLEAR_PTR_CLEANUP(x,y,func,name) \
    (SubCount(x,name) && func(x), (x) = y)

///////   Build something I don't know how big it is yet  //////

typedef struct {
   void *memory;
   uint allocated_size;
   uint current_size;
} DynamicArray;

DynamicArray *MallocArray(int size)
{
   DynamicArray *a = MallocSafe(sizeof(DynamicArray));
   a->memory = MallocSafe(size);
   a->allocated_size = 0;
   a->current_size = 0;
   return a;
}

void FreeArray(DynamicArray *a)
{
   Free(a->memory);
   Free(a);
}

DynamicArray *GetArray(void)
{
   return MallocArray(32);
}

void SetArray(DynamicArray *a, int min)
{
   if (min > a->allocated_size) {
      int new_size = 2*a->current_size;
      if (new_size < min)
         new_size = min;

      a->memory = Realloc(a->memory, new_size);
      if (!a->memory) Error(1, "Out of memory.\n");
   }
}

void AddArray(DynamicArray *a, int n)
{
   SetArray(a, a->current_size + n);
}

void AppendArray(DynamicArray *a, int n, void *p)
{
   AddArray(a, n);
   memcpy((char *) a->memory + a->current_size, p, n);
   a->current_size += n;
}

void *FreezeArray(DynamicArray *a)
{
   void *p = MallocSafe(a->current_size);
   memcpy(p, a->memory, a->current_size);
   FreeArray(a);   
   return p;
}

void *FreezeArrayCounted(DynamicArray *a)
{
   void *p = MallocCounted(a->current_size);
   memcpy(p, a->memory, a->current_size);
   FreeArray(a);   
   return p;
}

///////////////////  FLOAT MATH STUFF  ////////////////////////

#define PLANE_COMP_EPSILON   0.00001
#define PLANE_CONST_EPSILON  0.0005


double PLANE_EPSILON =       0.001;

#define REAL_EPSILON         0.0001
#define ZERO_EPSILON         0.0001
#define CONTINUOUS_EPSILON   0.001
#define POINT_EPSILON        0.001

#define FIXUP_EPSILON        0.0005


#define GE_ONE(x)     ((x) >= (1 - REAL_EPSILON))
#define IS_ZERO(x)    (fabs(x) < ZERO_EPSILON)
#define FLOAT_EQ(x,y) IS_ZERO((x)-(y))
#define FLOAT_EQ_EPS(x,y,e) (fabs((x)-(y)) < (e))

#define GE_ONE_DOT(x) ((x) >= (1 - PLANE_COMP_EPSILON))

bool eq_vec(mxs_vector *a, mxs_vector *b)
{
   return FLOAT_EQ(a->x,b->x) && FLOAT_EQ(a->y,b->y) && FLOAT_EQ(a->z,b->z);
}

void cleanup(mxs_real *val)
{
   if (floor(*val-FIXUP_EPSILON) != floor(*val)) {
      *val = floor(*val);
   } else if (floor(*val+FIXUP_EPSILON) != floor(*val)) {
      *val = floor(*val+FIXUP_EPSILON);
   }
}

///////////////////   PLANE OPERATIONS   //////////////////////

enum {
   NORM_X=0,
   NORM_Y=1,
   NORM_Z=2,
   NON_AXIAL=3
};

#define IS_AXIAL(x)   ((x) <= NORM_Z)

typedef struct st_RenderPoly RenderPoly;

typedef struct
{
   int plane_type;
   mxs_vector  norm;
   mxs_real    d;
   RenderPoly *data;        // collect per-plane data temporarily
} mPlane;

mPlane plane[MAX_GLOBAL_PLANES];
int num_planes;

typedef int Plane;

bool reversed;

mPlane *createPlane(int id, mxs_vector *normal, mxs_real d)
{
   mPlane *p = &plane[id];

   p->norm = *normal;
   p->d = d;

#ifdef EXACT_MATCH
   if (normal->x == 1 || normal->x == -1) {
#else
   if (GE_ONE(fabs(normal->x))) {
#endif
      p->plane_type = NORM_X;
      p->norm.x = 1;
      p->norm.y = 0;
      p->norm.z = 0;
      reversed = (normal->x < 0);
      if (reversed) p->d = -p->d;
#ifdef EXACT_MATCH
   } else if (normal->y == 1 || normal->y == -1) {
#else
   } else if (GE_ONE(fabs(normal->y))) {
#endif
      p->plane_type = NORM_Y;
      p->norm.x = 0;
      p->norm.y = 1;
      p->norm.z = 0;
      reversed = (normal->y < 0);
      if (reversed) p->d = -p->d;
#ifdef EXACT_MATCH
   } else if (normal->z == 1 || normal->z == -1) {
#else
   } else if (GE_ONE(fabs(normal->z))) {
#endif
      p->plane_type = NORM_Z;
      p->norm.x = 0;
      p->norm.y = 0;
      p->norm.z = 1;
      reversed = (normal->z < 0);
      if (reversed) p->d = -p->d;
   } else {
      p->plane_type = NON_AXIAL;
      reversed = FALSE;
   }

   return p;
}

  // find a matching plane
Plane findPlane(mxs_vector *normal, mxs_real d)
{
   int i;
   mPlane *p = plane;
   mxs_real res;

#ifndef EXACT_MATCH
   cleanup(&normal->x);
   cleanup(&normal->y);
   cleanup(&normal->z);
   cleanup(&d);
#endif

   for(i=0; i < num_planes; ++i,++p) {
#ifdef EXACT_MATCH
      if (d == p->d && normal->x == p->norm.x
             && normal->y == p->norm.y && normal->z == p->norm.z)
         res = 1;  // dot product
      else if (d == -p->d && normal->x == -p->norm.x
             && normal->y == -p->norm.y && normal->z == -p->norm.z)
         res = -1;  // dot product
      else
         continue;
#else
      if (! FLOAT_EQ_EPS(fabs(d), fabs(p->d), PLANE_CONST_EPSILON))
         continue;
      res = mx_dot_vec(normal, &p->norm);
      if (!GE_ONE_DOT(fabs(res)))
         continue;

      // now, check that the sign swap is right
      if (res > 0) {
         if (!FLOAT_EQ_EPS(d, p->d, PLANE_CONST_EPSILON))
            continue;
      } else {
         if (!FLOAT_EQ_EPS(d, -p->d, PLANE_CONST_EPSILON))
            continue;
      }
#endif

      reversed = (res < 0);

      // this is a match (but it may be facing backwards)
      return i;
   }
   if (num_planes == MAX_GLOBAL_PLANES)
      Error(1, "Increase MAX_GLOBAL_PLANES\n");

   // make a new one!
   createPlane(num_planes++, normal, d);
   return i;
}

void dump_plane(Plane p)
{
   if (p < 0 || p > num_planes)
      printf("Invalid plane %d\n", p);
   else
      printf("Plane %d: %g %g %g %g\n", p,
         plane[p].norm.x, plane[p].norm.y, plane[p].norm.z,
         plane[p].d);
}

typedef struct {
   int num_planes;
   Plane planes[1];
} PlaneArray;

PlaneArray *makePlaneArray(int max_elems)
{
   PlaneArray *pa = MallocSafe(sizeof(PlaneArray) + max_elems*sizeof(Plane));
   pa->num_planes = 0;
   return pa;
}

void freePlaneArray(PlaneArray *pa)
{
   Free(pa);
}

///////////////////   VERTEX OPERATIONS   //////////////////////

FILE *out_file;

typedef int Point;
Point hash[POINT_HASH][POINT_HASH][POINT_HASH];

typedef struct {
   mxs_vector pt;
   Point next_hash;
} sPoint;

sPoint point[MAX_GLOBAL_POINTS];
int num_point = 0;

#define PT(x)   (&point[x].pt)
int hash_base[3], hash_size[3];

mxs_real FLOOR(mxs_real x)
{
   mxs_real y = floor(x + POINT_EPSILON);
   if (y > x) return y;
   return floor(x);
}

void initPointHash(mxs_vector *minv, mxs_vector *maxv)
{
   int i,j,k;

   mxs_vector mn,mx;

   mn.x = FLOOR(minv->x);
   mn.y = FLOOR(minv->y);
   mn.z = FLOOR(minv->z);

   mx.x = FLOOR(maxv->x);
   mx.y = FLOOR(maxv->y);
   mx.z = FLOOR(maxv->z);

   hash_base[0] = mn.x;
   hash_base[1] = mn.y;
   hash_base[2] = mn.z;

   hash_size[0] = (mx.x - mn.x) / (POINT_HASH) + 1;
   hash_size[1] = (mx.y - mn.y) / (POINT_HASH) + 1;
   hash_size[2] = (mx.z - mn.z) / (POINT_HASH) + 1;

   for (i=0; i < POINT_HASH; ++i)
      for (j=0; j < POINT_HASH; ++j)
         for (k=0; k < POINT_HASH; ++k)
            hash[k][j][i] = -1;             

   num_point = 0;
}

#define MP(x) ((int) (65536.0 * (x)))

Point findPoint(mxs_vector *pt)
{
   int i,j,k;
   Point *p, q;

#if 0
   if (FLOOR(pt->x) > pt->x) pt->x = FLOOR(pt->x);
   if (FLOOR(pt->y) > pt->y) pt->y = FLOOR(pt->y);
   if (FLOOR(pt->z) > pt->z) pt->z = FLOOR(pt->z);
#endif

   i = (FLOOR(pt->x) - hash_base[0]) / hash_size[0];
   j = (FLOOR(pt->y) - hash_base[1]) / hash_size[1];
   k = (FLOOR(pt->z) - hash_base[2]) / hash_size[2];

   if (i < 0 || j < 0 || k < 0 ||
        i >= POINT_HASH || j >= POINT_HASH || k >= POINT_HASH) {
      printf("Hash base: %d %d %d\n", hash_base[0], hash_base[1], hash_base[2]);
      printf("Hash size: %d %d %d\n", hash_size[0], hash_size[1], hash_size[2]);
      Error(1, "Invalid hash %d %d %d for %f %f %f\n", i,j,k,
              MP(pt->x), MP(pt->y), MP(pt->z));
      
   }

   p = &hash[k][j][i];

   while ( (q = *p) != -1) {
      // see if we're q
      if (FLOAT_EQ_EPS(pt->x, PT(q)->x, POINT_EPSILON)
          && FLOAT_EQ_EPS(pt->y, PT(q)->y, POINT_EPSILON)
          && FLOAT_EQ_EPS(pt->z, PT(q)->z, POINT_EPSILON))
         return q;
      p = &point[q].next_hash;
   }

   if (num_point == MAX_GLOBAL_POINTS)
      Error(1, "Increase MAX_GLOBAL_POINTS");

   point[num_point].next_hash = *p;
   point[num_point].pt = *pt;

   *p = num_point;

   return num_point++;
}

void dump_point(Point p)
{
   if (p < 0 || p >= num_point)
      printf("Invalid point %d\n", p);
   else
      printf("%d: [%g %g %g]", (int) p, PT(p)->x, PT(p)->y, PT(p)->z);
}

void emit_point(Point p)
{
   fprintf(out_file, "%g %g %g ", PT(p)->x, PT(p)->y, PT(p)->z);
}

////////////////   VERTEX-PLANE COMPARISON   ////////////////

#if 0
4  We will try caching vertex-plane comparisons.
  In this case, when ref counts go to 0 we won't free
  them.  (Or rather, we'll increment the ref count by
  one while it's in the shared system.)
#endif

Side pointPlaneCompare(Point a, Plane p)
{
   mPlane *q = &plane[p];
   mxs_vector *v = PT(a);

   if (IS_AXIAL(q->plane_type)) {
      mxs_real res = v->el[q->plane_type] + q->d;
      if (res > PLANE_EPSILON) return INSIDE;
      if (res < -PLANE_EPSILON) return OUTSIDE;
      return COPLANAR;
   } else {
      mxs_real res = mx_dot_vec(v, &q->norm) + q->d;
      if (res > PLANE_EPSILON) return INSIDE;
      if (res < -PLANE_EPSILON) return OUTSIDE;
      return COPLANAR;
   }
}

////////////////        POLYGONS           ////////////////

typedef struct
{
   Plane p;
   int inside;
   int num_verts;
   Point vert[1];
} Polygon;

Polygon *allocatePolygon(int num_v)
{
   Polygon *p = MallocCounted(sizeof(Polygon) + num_v * sizeof(Point));
   p->num_verts = num_v;
   return p;
}

typedef void TempPoly;

TempPoly *dynamicPolygon(void)
{
   DynamicArray *a = GetArray();
   Plane p = 0;
   int n=0, inside=0;

   AppendArray(a, sizeof(p), &p);
   AppendArray(a, sizeof(inside), &inside);
   AppendArray(a, sizeof(n), &n);

   return a;
}

void AddVertexToPolygon(TempPoly *p, Point v)
{
   AppendArray((DynamicArray *) p, sizeof(v), &v);
     // have an unparseable line of code
   ((Polygon *) ((DynamicArray *) p)->memory)->num_verts += 1;
}   

Polygon *freezePolygon(TempPoly *t)
{
   return (Polygon *) FreezeArrayCounted(t);
}

void dump_polygon(Polygon *a)
{
   printf("Polygon: %d vertices  plane %d\n", a->num_verts, a->p);
   if (a->num_verts < 20) {
      int i;
      for (i=0; i < a->num_verts; ++i)
          dump_point(a->vert[i]);
      printf("\n");
   }
}

void emit_polygon(Polygon *a)
{
   int i;
   fprintf(out_file, "%d ", a->num_verts);
   for (i=0; i < a->num_verts; ++i)
       emit_point(a->vert[i]);
   fprintf(out_file, "\n");
}

////////////////    ARRAYS OF POINTERS     ////////////////

//  A generic shared-thingy abstraction like the above.
//  We'll use this for collections of polygons, collections
//  of cells, etc.

typedef struct
{
   int num_elems;
   void *data[1];
} PtrArray;

PtrArray *allocatePtrArray(int num_elems)
{
   PtrArray *p = MallocCounted(sizeof(PtrArray) +
                      (num_elems-1) * sizeof(void *));
   p->num_elems = num_elems;
   return p;
}

typedef void TempPtrArray;

TempPtrArray *dynamicPtrArray(void)
{
   DynamicArray *a = GetArray();
   int n=0;

   AppendArray(a, sizeof(n), &n);
   return a;
}

void AddPtrToArray(TempPtrArray *p, void *q)
{
   AppendArray((DynamicArray *) p, sizeof(q), &q);
   ((PtrArray *) ((DynamicArray *) p)->memory)->num_elems += 1;
}   

void AddPtrCountedToArray(TempPtrArray *p, void *q)
{
   AppendArray((DynamicArray *) p, sizeof(q), &q);
   ((PtrArray *) ((DynamicArray *) p)->memory)->num_elems += 1;
   AddCount(q);
}   

PtrArray *freezePtrArray(TempPtrArray *t)
{
   return (PtrArray *) FreezeArrayCounted(t);
}

////////////////     RENDERING INFO        ////////////////

typedef struct {
   mxs_vector u,v;           // 24
   mxs_real u_base, v_base;  // u&v values at projection of origin on plane
   uchar texture_id;
} RenderData;

RenderData render[MAX_GLOBAL_RENDER];
int num_render;

void emit_render_aspects(void)
{
   int i;
   fprintf(out_file, "%d\n", num_render);
   for (i=0; i < num_render; ++i) {
      fprintf(out_file, "%g %g %g  %g %g %g  %g %g  %d\n",
           render[i].u.x, render[i].u.y, render[i].u.z,
           render[i].v.x, render[i].v.y, render[i].v.z,
           render[i].u_base, render[i].v_base, render[i].texture_id);
   }
   fprintf(out_file, "\n\n");
}

#define UV_EPSILON 0.05
#define UV_ROUND   (UV_EPSILON/2)

int FindRenderCore(uchar tex, mxs_vector *u, mxs_vector *v,
                 mxs_real u_base, mxs_real v_base)
{
   int i;
   for (i=0; i < num_render; ++i) {
       if (render[i].texture_id == tex &&
#if 1
           eq_vec(u, &render[i].u) &&
           eq_vec(v, &render[i].v) &&
           FLOAT_EQ_EPS(u_base, render[i].u_base, UV_EPSILON) &&
           FLOAT_EQ_EPS(v_base, render[i].v_base, UV_EPSILON))
#else
           1)
#endif
          return i;
   }

   if (i == MAX_GLOBAL_RENDER)
      Error(1, "Increase MAX_GLOBAL_RENDER\n");

   ++num_render;

   render[i].u = *u;
   render[i].v = *v;
   render[i].u_base = u_base;
   render[i].v_base = v_base;   
   render[i].texture_id = tex;

   return i;
}

//  FindRender which works from worldrep--
// Convert u_base & v_base from the meaning in the
// worldrep to the meaning above

int FindRender(uchar tex, mxs_vector *u, mxs_vector *v,
                 Plane p, ushort u_base, ushort v_base, mxs_vector *anchor)
{
   // project origin onto plane
   mxs_vector pt;
   mxs_real nu,nv, det, f;

   mx_scale_vec(&pt, &plane[p].norm, -plane[p].d);

   // now, the value of u&v at the anchor is u_base,v_base,
   // so origin of (u,v) is
   //      origin = anchor - u_base*u - v_base*v
   // now, we want the (u,v) value at our new point pt
   //   origin + nu * u + nv * v = pt
   // or  anchor + (nu-u_base)*u + (nv-v_base)*v = pt
   // so let tu = nu-u_base, tv = nv-v_base, and just
   // solve for that:
   //     anchor + tu * u + tv * v = pt

   // This is three equations and only two variables, but
   // I don't know of any easier way to solve it than to
   // use Cramer's rule as if there were 3 variables; i.e.
   // we introduce anchor * w, where we know w will turn
   // out to be 1.

   det = anchor->x * (u->y * v->z - u->z * v->y) +
         anchor->y * (u->z * v->x - u->x * v->z) +
         anchor->z * (u->x * v->y - u->y * v->x);

   det = 1 / det;

   nu = anchor->x * (pt.y * v->z - pt.z * v->y) +
        anchor->y * (pt.z * v->x - pt.x * v->z) +
        anchor->z * (pt.x * v->y - pt.y * v->x);

   nv = anchor->x * (u->y * pt.z - u->z * pt.y) +
        anchor->y * (u->z * pt.x - u->x * pt.z) +
        anchor->z * (u->x * pt.y - u->y * pt.x);

   nu = nu * det + u_base/(16*256.0);
   nv = nv * det + v_base/(16*256.0);

   // now we want to extract a number 0..1 from these
   f = floor(nu + UV_ROUND);
   if (f > nu)  // did we round up?
      nu = 0;
   else
      nu = nu - floor(nu);

   f = floor(nv + UV_ROUND);
   if (f > nv)
      nv = 0;
   else
      nv = nv - floor(nv);

   cleanup(&nu);
   cleanup(&nv);

   return FindRenderCore(tex, u, v, nu, nv);
}

struct st_RenderPoly
{
   int rend_info;
   int on_boundary;   // have we found a BSP cutting plane coplanar to this
   Polygon *poly;
   RenderPoly *next;
};

static void bad(char *s)
{
   printf("ERROR: Polygon list (%s) had cycle\n");
   exit(1);
}

void validatePolyList(RenderPoly *db, char *caller)
{
   RenderPoly *cur;

   cur = db->next;
   while (cur != NULL) {
      if (cur == db)
         bad(caller);
      cur = cur->next;
      if (cur == NULL) return;
      if (cur == db)
         bad(caller);
      cur = cur->next;
      if (cur == db)
         bad(caller);
      db = db->next;
   }
   return;
}

#define FreePoly   FreeCounted

int FreeRenderPoly(RenderPoly *rp)
{
   FreePoly(rp->poly);
   Free(rp);
   return 0;
}

RenderPoly *makeRenderPoly(Polygon *p, RenderPoly *src)
{
   RenderPoly *rp = MallocSafe(sizeof(RenderPoly));
   rp->poly = p;
   rp->rend_info = src->rend_info;
   rp->on_boundary = src->on_boundary;
   return rp;
}

void emit_renderpoly(RenderPoly *p)
{
   fprintf(out_file, "%d ", p->rend_info);
   emit_polygon(p->poly);
}

#if 0
   We needs lists of things.  We could either make
  linked list, or big arrays.  Since the linked lists
  need to be built outside of the data structures themselves,
  they'll require 8 bytes/entry, compared to 2-4 for arrays.
  So we'll assume we're using arrays.  If we null terminate
  the list regardless and provide an iterator function, we
  might be able to hide what decision this is.  We call
  the "head" of the list abstraction a "node".  These things
  should be ref counted.

   A cell consists of a pointer to a node of portals.

   A portal is a pointer to a polygon, a pointer to the
  matching portal, and a pointer to the matching portal's
  cell.  (Watch for problems with sharing the maching
  portal or cell.)

   An output portal is a pointer to a polygon and a pointer
  to a matching output portal.

   An output portal collection consists of a node of output portals.

   A cell database consists of a node of cells.
#endif

///////////////////////////////////////////////////////////////

//   Well, after seven hundred lines of data structures,
//   maybe it's time to actually do something interesting.

// The next two functions are the only interesting functions
// in the entire system

//   ONE:  Polygon-plane comparison

// Actually, we have three versions of the first function. 
// They move from easy...hard to understand, and slow...fast

Side polygonPlaneCompareSlow(Polygon *p, Plane q)
{
   Side side = COPLANAR;
   int i, n = p->num_verts;
   if (p->p == q)
      return p->inside ? INSIDE : OUTSIDE;

   for (i=0; i < n; ++i)
      side |= pointPlaneCompare(p->vert[i], q);
   return side;
}

Side polygonPlaneCompareMiddlin(Polygon *p, Plane q)
{
   if (IS_AXIAL(plane[q].plane_type)) {
      int index = plane[q].plane_type;
      mxs_real res;
      double inside_val = -plane[q].d + PLANE_EPSILON;
      double outside_val = -plane[q].d - PLANE_EPSILON;
      int n = p->num_verts;
      Point *a = p->vert;
      Side side=COPLANAR;
      for (; n; --n,++a) {
         res = PT(*a)->el[index];
         if (res > inside_val) side |= INSIDE;
         if (res < outside_val) side |= OUTSIDE;
      }
      return side;
   } else
      return polygonPlaneCompareSlow(p, q);
}

Side polygonPlaneCompareFast(Polygon *p, Plane q)
{
   if (IS_AXIAL(plane[q].plane_type)) {
      int index = plane[q].plane_type;
      double res, inside_val = -plane[q].d + PLANE_EPSILON;
      double outside_val = -plane[q].d - PLANE_EPSILON;
      int n = p->num_verts;
      Point *a = p->vert;

        // here's one that checks for the early out of CROSSES,
        // and cuts out half of the comparisons once it's found
        // a point not on the plane.  Compare this to Quake's solidbsp.c
        // attempt at this, which only early outs on CROSSES

        // Note that in general they handle this code faster because
        // they don't indirect to their vertices; this means the
        // vertices are right there in the poly, which allows for
        // a cool pointer arithmetic hack, so they don't have to
        // constantly offset by [index]...

        // this loop runs while all points are coplanar
      for (; n; --n,++a) {
         res = PT(*a)->el[index];
         if (res > inside_val) goto inside_loop;
         if (res < outside_val) goto outside_loop;
      }
      return COPLANAR;

     inside_loop: --n,++a;
      for (; n; --n,++a) {
         res = PT(*a)->el[index];
         if (res < outside_val) return CROSSES;
      }
      return INSIDE;

     outside_loop: --n,++a;
      for (; n; --n,++a) {
         res = PT(*a)->el[index];
         if (res > inside_val) return CROSSES;
      }
      return OUTSIDE;

   } else
      return polygonPlaneCompareSlow(p, q);
}

//  Decide which one we'll actually use!

#define  polygonPlaneCompare    polygonPlaneCompareFast


//   TWO:  Polygon-plane splitting

//  Take a polygon and a plane.  Split the polygon
// by the plane, returning two new polygons (one of
// which can be empty).

//  Go check out the first polygon clipper in csg.c,
// which uses 9 explicit cases to deal with everything.
// It's a "one-pass" clipper--it just iterates over the
// points and does everything right the first time.

//  The alternative, which I discovered in Quake's csg4.c,
// is to use two passes.  The first pass computes all the
// information about each vertex once.  (This would all
// get computed during the one pass of the other algorithm.)
// It can then immediately deal with certain cases, such
// as the polygon being on one side of the plane, but having
// a colinear edge, which cause a mess in my stuff.

//  Also, my old stuff is extra messy since it wants to
// return the new intermediate portal by inferring it from
// the coplanar edges and edges generated during splitting.
// This is a mess because then the edges aren't ordered,
// which is why I used unordered edges in my portalizer,
// and in the first bsp it does a little build-the-sorted-list
// step.  In Quake's portals.c, he uses a different approach:
// the newly generated portal is computed independent of
// clipping the other planes--just take the split plane, extend
// it to "infinity", and then clip it by the planes of the
// polyhedron.

#define MAX_POLY_POINT 1024

mxs_real pt_dist[MAX_POLY_POINT];
uchar pt_side[MAX_POLY_POINT];

  // if plane is non-zero, then if the polygon is coplanar,
  // it goes into <plane>.  otherwise, it goes into whichever
  // side is appropriate
bool PartitionPolygonByPlane(Polygon *a, Plane p,
       Polygon **in, Polygon **out, Polygon **planar)
{
   int i,n = a->num_verts, j;
   int c_in, c_out, c_planar;
   mxs_vector *norm = &plane[p].norm;
   mxs_real d = plane[p].d;
#ifdef VALIDATE_SIDEDNESS
   Side side = polygonPlaneCompare(a, p);
#endif

   if (n > MAX_POLY_POINT)
      Error(1, "PartitionPolygonByPlane: %d > MAX_POLY_POINT.\n", n);

   *in = *out = 0;

     // special case if coplanar
   if (a->p == p) {
      if (planar) {
         *planar = a;
         return TRUE;
      }

      if (a->inside)
         *in = a;
      else
         *out = a;
      return TRUE;
   }

   c_in = c_out = c_planar = 0;

   for (i=0; i < n; ++i) {
      mxs_vector *q = PT(a->vert[i]);
      mxs_real dist = mx_dot_vec(q, norm) + d;

      pt_dist[i] = dist;

      if (dist > PLANE_EPSILON) { pt_side[i] = INSIDE; ++c_in; }
      else if (dist < -PLANE_EPSILON) { pt_side[i] = OUTSIDE; ++c_out; }
      else { pt_side[i] = COPLANAR; ++c_planar; }
   }

#ifdef VALIDATE_SIDEDNESS
   if (side == COPLANAR && (c_in || c_out))
      Error(1, "Expected coplanar and got inside or outside.\n");
   if (side == INSIDE && c_out)
      Error(1, "Expected inside and got outside.\n");
   if (side == OUTSIDE && c_in)
      Error(1, "Expected outside and got inside.\n");
//printf("%s ", "IN\0\0OUT\0CROSS"+4*(side-1));
#endif

   if (!c_in && !c_out) {
      printf("WARNING: Didn't grab coplanar case!\n");
      if (planar) {
         *planar = a;
         return TRUE;
      }

      if (a->inside)
         *in = a;
      else
         *out = a;
      return TRUE;
   }

   if (!c_out) {
      *in = a;
      return FALSE;
   }
   if (!c_in) {
      *out = a;
      return FALSE;
   }

   // ok, now it's well and truely split

   {
      TempPoly *in_p = dynamicPolygon();
      TempPoly *out_p = dynamicPolygon();

      j = n-1;
      for (i=0; i < n; ++i) {
         if (pt_side[i] != pt_side[j]
                && pt_side[i] != COPLANAR
                && pt_side[j] != COPLANAR) {
              // generate a new intermediate point
            Point z;
            mxs_vector loc, *p1 = PT(a->vert[i]), *p2 = PT(a->vert[j]);
            mxs_real r;
            int k;

              //   a + (b-a)*t = 0
              //   (b-a)*t = -a
              //   t = -a / (b-a)
            r = -pt_dist[i] / (pt_dist[j] - pt_dist[i]);
            for (k=0; k < 3; ++k) {
               if (plane[p].norm.el[k] == 1)
                  loc.el[k] = -plane[p].d;
               else
                  loc.el[k] = p1->el[k] + r * (p2->el[k] - p1->el[k]);
            }
            z = findPoint(&loc);
            AddVertexToPolygon(in_p, z);
            AddVertexToPolygon(out_p, z);
         }
         if (pt_side[i] == COPLANAR) {
            AddVertexToPolygon(in_p, a->vert[i]);
            AddVertexToPolygon(out_p, a->vert[i]);
         } else if (pt_side[i] == INSIDE)
            AddVertexToPolygon(in_p, a->vert[i]);
         else
            AddVertexToPolygon(out_p, a->vert[i]);
        
         j = i;
      }

      *in = freezePolygon(in_p);
      *out = freezePolygon(out_p);

      (*in)->p = (*out)->p = a->p;
      (*in)->inside = (*out)->inside = a->inside;

      if (!(*in)->num_verts || !(*out)->num_verts)
         Error(1, "Bad split.\n");
   }
   return FALSE;
}

void PartitionRenderPolyByPlane(RenderPoly *rp, Plane p,
       RenderPoly **in, RenderPoly **out, RenderPoly **plane)
{
   // partition the polygon itself
   Polygon *in_p, *out_p, *plane_p=0;

   if (plane) *plane = 0;

   if (PartitionPolygonByPlane(rp->poly, p, &in_p, &out_p, plane?&plane_p:0)) {
         // if coplanar, mark it as being on a boundary already
      rp->on_boundary = TRUE;
   }

   if (plane && plane_p) {
      if (plane_p != rp->poly)
         Error(1, "A coplanar plane wasn't the original!\n");
      *in = 0;
      *out = 0;
      return;
   }

   if (in_p == rp->poly) {
//printf("in ");
      *in = rp;
      *out = 0;
      return;
   }

   if (out_p == rp->poly) {
//printf("out ");
      *out = rp;
      *in = 0;
      return;
   }

   if (!in_p || !out_p)
      Error(1, "Split polygon into one fragment different from original\n");

//printf("split ");
   *in  = makeRenderPoly(in_p, rp);
   *out = makeRenderPoly(out_p, rp);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// a partition is a linked list of plane ptrs each with
// a linked list of RenderPolys

typedef struct st_PlanePartition PlanePartition;
struct st_PlanePartition
{
   int plane;
   RenderPoly *head;
   PlanePartition *next;
};

PlanePartition *allocPlanePartition(void)
{
   return MallocSafe(sizeof(PlanePartition));
}

void FreePlanePartition(PlanePartition *db)
{
   RenderPoly *cur;
   while (db->head) {
      cur = db->head;
      db->head = cur->next;
      FreeRenderPoly(cur);
   }
   Free(db);
}

void FreePlanePartitionDatabase(PlanePartition *db)
{
   PlanePartition *next;
   while (db) {
      next = db->next;
      FreePlanePartition(db);
      db = next;
   }
}

// split a single plane
void SplitPlanePartition(PlanePartition *db, Plane p,
        PlanePartition **inside_db, PlanePartition **outside_db)
{
   RenderPoly *inside, *outside;
   RenderPoly *cur, *next;

   if (eq_vec(&plane[p].norm, &plane[db->plane].norm)) {
      // they're parallel, so we can do this quickly
      // are they the same plane?
      if (p == db->plane) {
         // they're all coplanar
         FreePlanePartition(db);
         *inside_db = *outside_db = NULL;
      } else if (plane[db->plane].d < plane[p].d) {
         // ax + by + cz + d = 0
         *inside_db = db;
         *outside_db = NULL;
      } else {
         *inside_db = NULL;
         *outside_db = db;
      }
      return;
   }

   // really split them

   cur = db->head;
   db->head = NULL;
   inside = outside = NULL;

   while (cur) {
      RenderPoly *in, *out;

      next = cur->next;
      cur->next = NULL;
      PartitionRenderPolyByPlane(cur, p, &in, &out, NULL);

      if (in) { in->next = inside; inside = in; }
      if (out) { out->next = outside; outside = out; }

      cur = next;
   }

   if (!inside && !outside) {
      // huh
      printf("Totally coplanar... ick.\n");
      *inside_db = NULL;
      *outside_db = NULL;
      FreePlanePartition(db);
   }

   if (!inside) {
      // all of them went outside, so reuse db as outside
      *outside_db = db;
      db->head = outside;
      *inside_db = NULL;
      return;
   }

   if (!outside) {
      // all of them went outside, so reuse db as outside
      *inside_db = db;
      db->head = inside;
      *outside_db = NULL;
      return;
   }

   // reuse db as inside, and allocate a new one for outside
   *inside_db = db;
   *outside_db = allocPlanePartition();
   **outside_db = *db;

   (*outside_db)->head = outside;
   (*inside_db)->head = inside;
}

void SplitPartition(PlanePartition *db, Plane p,
        PlanePartition **inside_db, PlanePartition **outside_db)
{
   int i,j,k;
   PlanePartition *in, *out, *next;

   // iterate over each of the plane partitions
   *inside_db = NULL;
   *outside_db = NULL;

   i = j = k = 0;

   while (db != NULL) {
      next = db->next;
      db->next = NULL;
      ++i;
      SplitPlanePartition(db, p, &in, &out);
      if (in) {
         //validatePolyList(in->head, "inside partition");
         in->next = *inside_db;
         *inside_db = in;
         ++j;
      }
      if (out) {
         //validatePolyList(out->head, "outside partition");
         out->next = *outside_db;
         *outside_db = out;
         ++k;
      }
      db = next;
   }
   //printf("Split %d planesets into %d and %d\n", i, j, k);
}

/////////////////////////////////////
//
//   DATABASE PARTITIONING
//
//  We use a recursive n-way paritioning.
//  First we try to portal-partition as
//  much as we can.  If we can, we recurse
//  on each of those partitions.  Otherwise
//  we BSP partition.
//

//  Currently the data structure passed in is
//  a PtrArray of polygons to partition, and the
//  array of output "final" partitions.  Leaves
//  just append themselves onto the array.  This
//  means we don't bother storing the tree
//  structure.

//  If the output array is null, then we should
//  go through the motions but not have any side-effects
//  (don't bother adding on to the array), and we should
//  return the value of the metric (i.e. this computes
//  what the outcome of a partition is without carrying
//  it out.)

int plane_split_metric_infinity = (1 << 24);

bool always_take_0_split;
int split_weight=1;
int percent_split_weight;
int no_split_cost;

int evaluateSplit(PlanePartition *db, Plane p, bool show_res)
{
   int weight, count;

   int split, inside, outside;

   split = 0; // count up number of splits
   inside = 0;
   outside = 0;

   while (db) {
      RenderPoly *cur = db->head;
      while (cur) {
        Side s = polygonPlaneCompare(cur->poly, p);
        if (s == CROSSES)
           ++split;
        else if (s == INSIDE)
           ++inside;
        else if (s == OUTSIDE)
           ++outside;
        cur = cur->next;
      }
      db = db->next;
   }

   count = inside + outside + split;

   if (show_res)
      printf("(Split results) plane %d: %d inside, %d outside, %d split\n",
                    p, inside, outside, split);

   if (split == 0 && always_take_0_split)
      return 0;

   weight = 0;
   if (split_weight)
      weight += split_weight * split;
   if (!inside || !outside)
      weight += no_split_cost;
   else if (percent_split_weight) {
      int frac;
      if (inside > outside)
         frac = 100 * inside / (outside + inside);
      else
         frac = 100 * outside / (inside + outside);
      // frac is 50..100
      frac = (frac * 2) - 100;
      // frac is 0..100
      weight += percent_split_weight * frac / 100;
   }

   if (plane[p].plane_type == NON_AXIAL)
      weight += count / 10;

   if (count > 150 && (inside < count/3 || outside < count/3)) {
      // encourage even splits to speed up optimizer
      if (inside < count / 30 || outside < count / 30)
         weight += count / 10;
      else
         weight += (count - 100) / 30;
   }

   return weight;
}

Plane SelectSplitPlane(PlanePartition *db)
{
   int best_value, value;
   Plane best_plane;
   Plane p;
   PlanePartition *pl;

   best_value = plane_split_metric_infinity;

   // we can either iterate over all planes,
   // or iterate over all planes from this partition

   for(pl=db; pl != NULL; pl=pl->next) {
      p = pl->plane;
#if 0
      if (plane[p].data)
         continue;
#endif
      value = evaluateSplit(db, p, FALSE);
      if (value <= best_value) {
         // favor axial planes
         if (value == best_value && !IS_AXIAL(plane[p].plane_type))
            continue;

         if (value == best_value && IS_AXIAL(plane[best_plane].plane_type)) {
            // compare how well they split up the volume
         }

         best_value = value;
         best_plane = p;
      }
   }
   if (best_value == plane_split_metric_infinity) {
      Error(1, "No split plane was any good.\n");
   }

   return best_plane;
}

#define WRITE_LEAF()  fprintf(out_file, "0\n")

void RecursivePartition(PlanePartition *db);
void BSPPartitionByPlane(PlanePartition *db, Plane p)
{
   PlanePartition *inside_part, *outside_part;
   // now partition into two new databases

   SplitPartition(db, p, &inside_part, &outside_part);

   RecursivePartition(inside_part);
   RecursivePartition(outside_part);
}

void BSPPartition(PlanePartition *db)
{
   Plane p;

   // select the best-guess split plane

   p = SelectSplitPlane(db);
   if (debug) evaluateSplit(db, p, TRUE);

   fprintf(out_file, "-1 %15.15g %15.15g %15.15g %15.15g\n",
      plane[p].norm.x, plane[p].norm.y, plane[p].norm.z, plane[p].d);

   BSPPartitionByPlane(db, p);
}

//  The PlaneArray data structure is a list of all of the planes
//  which are potentially valid.  Generally planes become invalid
//  without being deleted from the list when they're used as a
//  partitioning.  The absolute truth of which ones are available
//  is always in plane[p].data.

//  The PlaneArray data structure is there to give us an O(1) data
//  structure when we're iterating over available planes, which is
//  crucially necessary when we're trying to do perfectly optimal
//  splitting, because otherwise we iterate n^n times noting that
//  a plane is unavailable.  Or something like that.

void RecursivePartition(PlanePartition *db)
{
   ////  check for a leaf  ////
   //
   //

   if (!db) {
      WRITE_LEAF();
      return;
   }

   ////  BSP partition  ////
   //
   //  select the optimal partitioning plane

   BSPPartition(db);
}

//////////////////    low level merge attempt       //////////////////

int colinear_merge, num_merged, colinear_del;

Polygon *try_merge(Polygon *a, Polygon *b)
{
   int i,j, n1,n2, e,f, k, pts;
   Point p1,p2;
   mxs_vector norm, edge, rel_norm;
   mxs_real res;
   bool delete_i, delete_j;
   Polygon *out;

   // we know they're from the same plane already
   if (a->inside != b->inside) return 0;

   // now go look for a shared edge

   n1 = a->num_verts;
   n2 = b->num_verts;

   for (i=0; i < n1; ++i) {
      j = (i+1)%n1;

      p1 = a->vert[i];
      p2 = a->vert[j];

      for (e=0; e < n2; ++e) {
         f = (e+1)%n2;
         if (p1 == b->vert[f] && p2 == b->vert[e])
            goto found_match;
      }
   }
   return 0;

  found_match:

   //++num_merged;

   // now see if we can extend the match (which means
   // we have some colinear points to delete)
   //   currently we've matched
   //        i == f
   //        j == e
   // So, we could have points before i, if i is 0.
   // And we could have points after j.

   if (i == 0) {
      pts = 0;
      while (a->vert[(i+n1-1)%n1] == b->vert[(f+1)%n2]) {
         i = (i+n1-1)%n1, f = (f+1)%n2;
         if (++pts == n1) return 0;
      }
   }

   pts = 0;
   while (a->vert[(j+1)%n1] == b->vert[(e+n2-1)%n2]) {
      j = (j+1)%n1, e = (e+n2-1)%n2;
      if (++pts == n1) return 0;
   }

   // ok, now we've extended it...

   // now we need to check if this is really valid.
   // basically,  I.e. whether the result of the
   // merge is actually convex

   // what we do is test the adjacent edges between
   // polys to see if their cross product is the same
   // as that for the poly itself

     // compute the polygon normal
   norm = plane[a->p].norm;
   if (!a->inside) { norm.x = -norm.x; norm.y = -norm.y; norm.z = -norm.z; }

     // compute the outward normal to the back edge
   mx_sub_vec(&edge, PT(a->vert[i]), PT(a->vert[(i+n1-1)%n1]));
   mx_cross_vec(&rel_norm, &norm, &edge);
   mx_normeq_vec(&rel_norm);

     // compute the extending edge on the other poly
   mx_sub_vec(&edge, PT(b->vert[(f+1)%n2]), PT(b->vert[f]));   
   mx_normeq_vec(&edge);
   res = mx_dot_vec(&edge, &rel_norm);

   if (res > CONTINUOUS_EPSILON) {
      return 0;
   }
   delete_i = (res > -CONTINUOUS_EPSILON);

     // compute the outward norml to the forward edge on other poly
   mx_sub_vec(&edge, PT(b->vert[e]), PT(b->vert[(e+n2-1)%n2]));
   mx_cross_vec(&rel_norm, &norm, &edge);
   mx_normeq_vec(&rel_norm);

     // compute the extending edge on this poly
   mx_sub_vec(&edge, PT(a->vert[(j+1)%n1]), PT(a->vert[j]));
   mx_normeq_vec(&edge);
   res = mx_dot_vec(&edge, &rel_norm);

   if (res > CONTINUOUS_EPSILON) {
      return 0;
   }
   delete_j = (res > -CONTINUOUS_EPSILON);

     // alright!
delete_i = delete_j = 0;

   ++num_merged;

   out = allocatePolygon(n1 + n2 - 2 - delete_i - delete_j);
   out->p = a->p;
   out->inside = a->inside;

   colinear_del += delete_i + delete_j;

   // start at j and add points until we reach i

   k = 0;
   for(;;) {
      j = (j+1) % n1;
      if (j == i) break;
      out->vert[k++] = a->vert[j];
   }

   if (!delete_i)
      out->vert[k++] = a->vert[i];

   for(;;) {
      f = (f+1) % n2;
      if (f == e) break;
      out->vert[k++] = b->vert[f];
   }

   if (!delete_j)
      out->vert[k++] = b->vert[e];

   out->num_verts = k;

   return out;
}

int merge_render(RenderPoly *a, RenderPoly *b)
{
   Polygon *p;

   if (a->rend_info != b->rend_info)
      return 0;

   p = try_merge(a->poly, b->poly);
   if (p) {
      // replace a with merged poly
      Free(a->poly);
      a->poly = p;
      return 1;
   }

   return 0;
}

////////       process and merge together rendered polygons    ///////

int add_render_plane(PortalPolygonRenderInfo *r, mxs_vector *anchor, Plane p)
{
   return FindRender(r->texture_id, &r->u->raw, &r->v->raw,
                  p, r->u_base, r->v_base, anchor);
}

void add_poly_to_plane_list(Polygon *a, int rend)
{
   Plane p = a->p;
   RenderPoly *rp = MallocSafe(sizeof(RenderPoly));
   rp->rend_info = rend;
   rp->on_boundary = 0;
   rp->poly = a;

   rp->next = plane[p].data;
   plane[p].data = rp;
}

void merge_poly_list(int p)
{
   RenderPoly *i, *j;
   i = plane[p].data;
   while (i != NULL) {
      j = plane[p].data;
      while (j->next != NULL) {
         if (i != j->next && merge_render(i, j->next)) {
            // they were merged, so delete j->next
            RenderPoly *old = j->next;
            j->next = j->next->next;
            FreeRenderPoly(old);
            // restart j from beginning
            j = plane[p].data;
         } else
            j = j->next;
      }
      i = i->next;
   }
}

int num_input_polys;

  // turn the n'th polygon in the cell p into a
  //   Polygon.  v is the appropriate offset into vertex_list
Polygon *build_poly(PortalCell *p, int n, int v)
{
   int i, k = p->poly_list[n].num_vertices;
   int pl = p->poly_list[n].planeid;
   Polygon *q = allocatePolygon(k);
   q->p = findPlane(&p->plane_list[pl].norm->raw,
                     p->plane_list[pl].plane_constant);

   q->inside = !reversed;
   for (i=0; i < k; ++i)
      q->vert[i] = findPoint(&p->vpool[p->vertex_list[v+i]]);

   return q;
}

void extract_polys_from_cell(PortalCell *p)
{
   int i,v,n,k;

   n = p->num_render_polys;
   v = 0;
   num_input_polys += n;
   for (i=0; i < n; ++i) {
      int pl = p->poly_list[i].planeid;
      k = add_render_plane(&p->render_list[i],
            &p->vpool[p->vertex_list[v+p->render_list[i].texture_anchor]],
            findPlane(&p->plane_list[pl].norm->raw, 
                     p->plane_list[pl].plane_constant));
      add_poly_to_plane_list(build_poly(p, i, v), k);
      v += p->poly_list[i].num_vertices;
   }
}

///////////// interface to world-rep data format /////////////

char *read_into(FILE *f, void **data, char *mem, int sz, int count)
{
   *data = mem;
   fread(mem, count, sz, f);
   return mem + count*sz;   
}

void bad_cell(PortalCell *p)
{
   printf("Bad cell:\n");
   printf("Num verts: %d\n", p->num_vertices);
   printf("Num polys; %d\n", p->num_polys);
   printf("Num render polys: %d\n", p->num_render_polys);
   printf("Num portal polys: %d\n", p->num_portal_polys);
   printf("Num planes: %d\n", p->num_planes);
   Error(1, "Bad cell\n");
}

mxs_vector vec_min, vec_max;

void init_min_max(void)
{
   vec_min.x = 100000;
   vec_min.y = 100000;
   vec_min.z = 100000;

   vec_max.x = -100000;
   vec_max.y = -100000;
   vec_max.z = -100000;
};

void update_min_max(mxs_vector *z, int n)
{
   while (n--) {
      if (z->x < vec_min.x) vec_min.x = z->x;
      if (z->y < vec_min.y) vec_min.y = z->y;
      if (z->z < vec_min.z) vec_min.z = z->z;

      if (z->x > vec_max.x) vec_max.x = z->x;
      if (z->y > vec_max.y) vec_max.y = z->y;
      if (z->z > vec_max.z) vec_max.z = z->z;

      ++z;
   }
}

PortalCell *read_cell(FILE *f)
{
   PortalCell p, *q;
   CachedVector *cv;
   char *mem;
   int vl, sz, i;

   fread(&vl, 1, sizeof(vl), f);
   fread(&p, 1, sizeof(p), f);

   if (p.num_vertices > 230 || p.num_polys > 128 ||
       p.num_render_polys > p.num_polys || p.num_planes > p.num_polys)
       bad_cell(&p);

   // now figure out how much memory we need
   sz = sizeof(p) +
        p.num_vertices * sizeof(Vertex) +
        p.num_polys * sizeof(PortalPolygonCore) +
        p.num_render_polys * sizeof(PortalPolygonRenderInfo) +
        p.num_planes * sizeof(PortalPlane) +
        p.num_planes * sizeof(CachedVector) +
        p.num_render_polys * 2 * sizeof(CachedVector) +
        p.num_render_polys * sizeof(PortalLightMap) + vl;
   //        p.num_vlist; // this was for vertex lighting

   mem = MallocSafe(sz);

   q = (PortalCell *) mem;
   *q = p;

   mem += sizeof(p);
   mem = read_into(f, &q->vpool, mem, sizeof(Vertex), p.num_vertices);
   mem = read_into(f, &q->poly_list, mem,
           sizeof(PortalPolygonCore), p.num_polys);
   mem = read_into(f, &q->render_list, mem,
           sizeof(PortalPolygonRenderInfo), p.num_render_polys);
   mem = read_into(f, &q->plane_list, mem, sizeof(PortalPlane), p.num_planes);
   mem = read_into(f, &q->vertex_list, mem, 1, vl);
//   mem = read_into(f, &q->vertex_list_lighting, mem, 1, p.num_vlist);

   cv = (CachedVector *) mem;
   mem += sizeof(CachedVector) * 2 * p.num_render_polys;

   for (i=0; i < p.num_render_polys; ++i) {
      q->render_list[i].u = &cv[i*2+0];
      q->render_list[i].v = &cv[i*2+1];
      fread(&q->render_list[i].u->raw, sizeof(Vector), 1, f);
      fread(&q->render_list[i].v->raw, sizeof(Vector), 1, f);
   }

   cv = (CachedVector *) mem;
   mem += sizeof(CachedVector) * p.num_planes;

   for (i=0; i < p.num_planes; ++i) {
      q->plane_list[i].norm = &cv[i];
      fread(&q->plane_list[i].norm->raw, sizeof(Vector), 1, f);
   }

   mem = read_into(f, &q->light_list, mem,
           sizeof(PortalLightMap), p.num_render_polys);

   for (i=0; i < p.num_render_polys; ++i) {
      fseek(f, q->light_list[i].w*q->light_list[i].h, SEEK_CUR);
   }

   if ((char *) q + sz != mem)
      Error(1, "read_cell: buffer size different from read amount.");

   update_min_max(q->vpool, p.num_vertices);

   return q;
}

void move_ptr_list(TempPtrArray *dest, PtrArray *src)
{
   int i,n = src->num_elems;
   for (i=0; i < n; ++i)
      AddPtrToArray(dest, src->data[i]);      
}

PortalCell **level;
PtrArray *db, *part_level;
int num_cells;

void process_level(char *fname)
{
   FILE *f = fopen(fname, "rb");
   int n, i;
   char dummy;
   PlanePartition *pp;

   if (!f) {
      fprintf(stderr, "merge: file %s not found.\n", fname);
      exit(1);
   }

   init_min_max();

   fread(&n, sizeof(n), 1, f);
   num_cells = n;
   printf("File has %d cells.\n", n);

   level = MallocSafe(sizeof(PortalCell *)* n);

   for (i=0; i < n; ++i) {
      if (feof(f)) Error(1, "Premature end of input file after %d cells.\n",i);
      level[i] = read_cell(f);
   }
   if (fread(&dummy, 1, 1, f)) Error(1, "Data appears after file end.\n");

   printf("Allocations: %d\n", allocations);

   printf("Min: %g %g %g\n", vec_min.x, vec_min.y, vec_min.z);
   printf("Max: %g %g %g\n", vec_max.x, vec_max.y, vec_max.z);
   initPointHash(&vec_min, &vec_max);

   for (i=0; i < n; ++i)
      extract_polys_from_cell(level[i]);

   printf("Input polys: %d\n", num_input_polys);
   printf("Unique planes: %d\n", num_planes);
   printf("Unique render aspects: %d\n", num_render);
   printf("Unique vertices: %d\n", num_point);

   printf("Allocations: %d\n", allocations);

   pp = NULL;
   for (i=0; i < num_planes; ++i) {
      PlanePartition *pl;
      merge_poly_list(i);
      pl = allocPlanePartition();
      pl->head = plane[i].data;
      pl->plane = i;
      pl->next = pp;
      pp = pl;
   }

   printf("Merges made: %d\n", num_merged);
   printf("Merges with internal colinear points: %d\n", colinear_merge);
   printf("%d colinear points deleted\n", colinear_del);

   printf("Allocations: %d\n", allocations);

   RecursivePartition(pp);

   {
      int i;
      for (i=0; i < num_cells; ++i)
         Free(level[i]);
      Free(level);
   }
}

int main(int argc, char **argv)
{
   if (argc > 1 && !strcmp(argv[1], "-d")) {
      debug = TRUE;
      --argc;
      ++argv;
   }

   if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'e') {
      PLANE_EPSILON = atof(argv[1]+2);
      --argc;
      ++argv;
      printf("Epsilon is %lg\n", PLANE_EPSILON);
   }

   if (argc > 1) {
      out_file = fopen("bsp.out", "w");
      process_level(argv[1]);
      printf("Current allocations: %d\nMax allocations: %d\n",
            allocations, max_alloc);
      fclose(out_file);
   }
   return 0;
}
