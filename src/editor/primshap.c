/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primshap.c,v 1.12 2000/02/19 13:11:15 toml Exp $
// primshap(e)... code and data for generating the primal shapes

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <matrix.h>
#include <primal.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////
// setup template silliness

// declare the base static setup, ie. the primalInfo inlined struct
#define _static_declare_base(NAME) \
  static primalInfo NAME##_base= \
    {##NAME##_PT_CNT,##NAME##_EDGE_CNT,##NAME##_FACE_CNT,##NAME##_FACE_PTS,NULL,NULL,NULL,NULL}

// various array declarations, here to be consistently named, and have array sizes hardcoded
#define _static_declare_edges(NAME) \
  static int NAME##_edge_list[##NAME##_EDGE_CNT*2]
#define _static_declare_face_pts(NAME) \
  static int NAME##_face_pts_list[##NAME##_FACE_CNT * ##NAME##_FACE_PTS]
#define _static_declare_face_edge(NAME) \
  static int NAME##_face_edge_list[##NAME##_FACE_CNT * ##NAME##_FACE_PTS]
#define _static_declare_pt_list(NAME) \
  static mxs_vector NAME##_pts_list[##NAME##_PT_CNT]

// memory setup ops
#define _build_named_primal(NAME,newprim) \
  do { \
     memcpy(newprim,&##NAME##_base,sizeof(primalInfo)); \
     if (!_primalBr_GetMem(newprim)) return FALSE; \
     memcpy(newprim->edge_list,##NAME##_edge_list,sizeof(##NAME##_edge_list)); \
     memcpy(newprim->face_pts_list,##NAME##_face_pts_list,sizeof(##NAME##_face_pts_list)); \
     memcpy(newprim->face_edge_list,##NAME##_face_edge_list,sizeof(##NAME##_face_edge_list)); \
     memcpy(newprim->pt_array,##NAME##_pts_list,sizeof(##NAME##_pts_list)); \
  } while (0)
  
///////////////////
// basic cube!

#define cube_PT_CNT     (8)
#define cube_EDGE_CNT  (12)
#define cube_FACE_CNT   (6)
#define cube_FACE_PTS (4+1)

// here is our primalInfo intself
_static_declare_base(cube);

_static_declare_edges(cube)=
 { 0,1, 1,2, 2,3, 3,0,
   0,4, 4,7, 7,3, 2,6,
   6,5, 5,1, 4,5, 6,7};
_static_declare_face_pts(cube)=
 { 0,1,2,3,-1,
   3,2,6,7,-1,
   7,6,5,4,-1,
   4,5,1,0,-1,
   1,5,6,2,-1,
   4,0,3,7,-1 };
_static_declare_face_edge(cube)=
 { 0,1,2,3,-1,
   2,7,11,6,-1,
   11,8,10,5,-1,
   10,9,0,4,-1,
   9,8,7,1,-1,
   4,3,6,5,-1 };
_static_declare_pt_list(cube)=
 { {-1,1,-1},{-1,1,1},{-1,-1,1},{-1,-1,-1},
   { 1,1,-1},{ 1,1,1},{ 1,-1,1},{ 1,-1,-1}};

BOOL PrimShape_CreateCube(primalInfo *new_cube)
{
   _build_named_primal(cube,new_cube);  // can return FALSE internally
   return TRUE;
}

/////////////////////
// look, a line - really just here to simplify some other code

#define line_PT_CNT     (2)
#define line_EDGE_CNT   (1)
#define line_FACE_CNT   (1)
#define line_FACE_PTS (2+1)

// here is our primalInfo intself
_static_declare_base(line);

_static_declare_edges(line)=
 { 0,1 };
_static_declare_face_pts(line)=
 { 0,1,-1 };
_static_declare_face_edge(line)=
 { 0,1,-1 };
_static_declare_pt_list(line)=
 { {-1,0,0},{1,0,0} };

BOOL PrimShape_CreateLine(primalInfo *new_line)
{
   _build_named_primal(line,new_line);  // can return FALSE internally
   return TRUE;
}

//////////////////////
// tri pyramid hack for test

#define tri_PT_CNT     (4)
#define tri_EDGE_CNT   (6)
#define tri_FACE_CNT   (4)
#define tri_FACE_PTS (3+1)

#define Root3Over2 (0.866025)
#define EdgeLn     (1.0)

// here is our primalInfo intself
_static_declare_base(tri);

_static_declare_edges(tri)=
 { 0,1, 1,2, 2,0, 3,2, 3,1, 0,3 };
_static_declare_face_pts(tri)=
 { 0,1,2,-1,
   2,1,3,-1,
   3,1,0,-1,
   0,2,3,-1 };
_static_declare_face_edge(tri)=
 { 0,1,2,-1,
   1,4,3,-1,
   4,0,5,-1,
   2,3,5,-1 };
_static_declare_pt_list(tri)=
 { {          0,   EdgeLn, -EdgeLn/2 },
   {-Root3Over2,-EdgeLn/2, -EdgeLn/2 },
   { Root3Over2,-EdgeLn/2, -EdgeLn/2 },
   {          0,        0,    EdgeLn } };

BOOL PrimShape_CreateTri(primalInfo *new_tri)
{
   _build_named_primal(tri,new_tri);
   return TRUE;
}

//////////////////////
// light

#define light_PT_CNT     (6)
#define light_EDGE_CNT   (3)
#define light_FACE_CNT   (1)
#define light_FACE_PTS   (1)

// here is our primalInfo intself
_static_declare_base(light);

_static_declare_edges(light)=
 { 0,1, 2,3, 4,5 };
_static_declare_face_pts(light)=
 { -1 };
_static_declare_face_edge(light)=
 { -1 };
_static_declare_pt_list(light)=
 { { 0, 1, 0}, { 0,-1, 0},
   { 1, 0, 0}, {-1, 0, 0},
   { 0, 0, 1}, { 0, 0,-1} };

BOOL PrimShape_CreateLight(primalInfo *new_light)
{
   _build_named_primal(light,new_light);
   return TRUE;
}

/////////////////////
// generic ngon basis stuff

// fill pts with an n-gon in xy with z coodinate z
static void build_ngon_base(int n, mxs_vector *pts, float z, BOOL face_align)
{
   double face_mod=face_align?1.0:0.0, scale_f=1.0;
   int i;

   for (i=0; i<n; i++)
   {
      double ang=MX_REAL_2PI*(i*2.0+face_mod)/(n*2.0);  // currently vertex aligned, add 1.0 to numerator for face
      if (face_align&&(i==0))
         scale_f=(double)1.0/cos(ang);
      pts[i].y= cos(ang)*scale_f;
      pts[i].x=-sin(ang)*scale_f;
      pts[i].z= z;
   }
}

static void build_ngon_edge_list(int n, int *base_pt, int base_idx)
{
   int i;
   for (i=0; i<n-1; i++)
   {
      base_pt[i*2]=base_idx+i;
      base_pt[i*2+1]=base_idx+i+1;
   }
   base_pt[i*2]=base_idx+i;
   base_pt[i*2+1]=base_idx;
}

////////////////////
// ngon pyramids

BOOL PrimShape_CreateNGonPyr(primalInfo *new_pyr, int n, BOOL face_align, BOOL corner_p)
{
   int i;
   
   new_pyr->faces=n+1;
   new_pyr->points=n+1;
   new_pyr->edges=2*n;
   new_pyr->face_stride=n+1;
   if (!_primalBr_SzCheck(new_pyr)) return FALSE;
   if (!_primalBr_GetMem(new_pyr)) return FALSE;

   //////// PTLIST
   build_ngon_base(n,new_pyr->pt_array,-1,face_align);
   if (corner_p)
   {
      new_pyr->pt_array[n].x=new_pyr->pt_array[0].x;
      new_pyr->pt_array[n].y=new_pyr->pt_array[0].y;
      new_pyr->pt_array[n].z=1.0;
   }
   else
   {
      new_pyr->pt_array[n].x=new_pyr->pt_array[n].y=0;
      new_pyr->pt_array[n].z=1.0;
   }

   //////// EDGELIST
   build_ngon_edge_list(n,new_pyr->edge_list,0);   // ring around the base
   for (i=0; i<n; i++)
   {                                               // and here are edges up to the top
      new_pyr->edge_list[(n+i)*2]=i;
      new_pyr->edge_list[(n+i)*2+1]=n;
   }

   //////// FACEPTS   
   for (i=0; i<n; i++)                             // faces up to the top 
   {
      new_pyr->face_pts_list[new_pyr->face_stride*i+0]=i;
      new_pyr->face_pts_list[new_pyr->face_stride*i+1]=n+1;
      new_pyr->face_pts_list[new_pyr->face_stride*i+2]=(i+1)%n;
      new_pyr->face_pts_list[new_pyr->face_stride*i+3]=-1;
   }
   for (i=0; i<n; i++)                             // right around the base
      new_pyr->face_pts_list[new_pyr->face_stride*n+i]=i;
   new_pyr->face_pts_list[new_pyr->face_stride*n+i]=-1;

   //////// FACEEDGES   
   for (i=0; i<n; i++)                             // faces up to the top 
   {
      new_pyr->face_edge_list[new_pyr->face_stride*i+0]=i;
      new_pyr->face_edge_list[new_pyr->face_stride*i+1]=n+i;
      new_pyr->face_edge_list[new_pyr->face_stride*i+2]=n+((i+1)%n);
      new_pyr->face_edge_list[new_pyr->face_stride*i+3]=-1;
   }
   for (i=0; i<n; i++)                             // right around the base
      new_pyr->face_edge_list[new_pyr->face_stride*n+i]=i;
   new_pyr->face_edge_list[new_pyr->face_stride*n+i]=-1;

   return TRUE;
}

////////////////////
// ngon cylinders

BOOL PrimShape_CreateNGonCyl(primalInfo *new_cyl, int n, BOOL face_align)
{
   int i;   
   new_cyl->faces=n+2;
   new_cyl->points=2*n;
   new_cyl->edges=3*n;
   new_cyl->face_stride=max(n+1,4+1);  // in case you have a 3 sided base on a cyl - rect sides have more edges
   if (!_primalBr_SzCheck(new_cyl)) return FALSE;
   if (!_primalBr_GetMem(new_cyl)) return FALSE;

   //////// PTLIST
   build_ngon_base(n,new_cyl->pt_array  ,-1, face_align); // bottom of cylinder
   build_ngon_base(n,new_cyl->pt_array+n, 1, face_align); // top of cylinder

   //////// EDGELIST
   build_ngon_edge_list(n,new_cyl->edge_list  ,0);   // ring around the base
   build_ngon_edge_list(n,new_cyl->edge_list+2*n,n); // ring around the top
   for (i=0; i<n; i++)
   {                                                 // and here are sides
      new_cyl->edge_list[(2*n+i)*2]  =i;
      new_cyl->edge_list[(2*n+i)*2+1]=n+i;
   }

   //////// FACEPTS
   for (i=0; i<n; i++)                               // faces up to the top 
   {
      new_cyl->face_pts_list[new_cyl->face_stride*i+0]=i;
      new_cyl->face_pts_list[new_cyl->face_stride*i+1]=n+i;
      new_cyl->face_pts_list[new_cyl->face_stride*i+2]=n+((i+1)%n);
      new_cyl->face_pts_list[new_cyl->face_stride*i+3]=(i+1)%n;      
      new_cyl->face_pts_list[new_cyl->face_stride*i+4]=-1;
   }
   new_cyl->face_pts_list[new_cyl->face_stride*n+0]=n;
   for (i=1; i<n; i++)                               // around the top
      new_cyl->face_pts_list[new_cyl->face_stride*n+i]=2*n-i;
   new_cyl->face_pts_list[new_cyl->face_stride*n+i]=-1;   
   for (i=0; i<n; i++)                               // around the top
      new_cyl->face_pts_list[new_cyl->face_stride*(n+1)+i]=i;
   new_cyl->face_pts_list[new_cyl->face_stride*(n+1)+i]=-1;

   //////// FACEEDGES
   for (i=0; i<n; i++)                               // faces up to the top 
   {
      new_cyl->face_edge_list[new_cyl->face_stride*i+0]=i;
      new_cyl->face_edge_list[new_cyl->face_stride*i+1]=2*n+i;
      new_cyl->face_edge_list[new_cyl->face_stride*i+2]=n+i;
      new_cyl->face_edge_list[new_cyl->face_stride*i+3]=2*n+((i+1)%n);
      new_cyl->face_edge_list[new_cyl->face_stride*i+4]=-1;
   }
   for (i=0; i<n; i++)                               // around the top
      new_cyl->face_edge_list[new_cyl->face_stride*n+i]=2*n-i-1;
   new_cyl->face_edge_list[new_cyl->face_stride*n+i]=-1;
   for (i=0; i<n; i++)                               // around the top
      new_cyl->face_edge_list[new_cyl->face_stride*(n+1)+i]=i;
   new_cyl->face_edge_list[new_cyl->face_stride*(n+1)+i]=-1;

   return TRUE;
}

///////////////////
// Wedge Antilles

#define wedge_PT_CNT     (6)
#define wedge_EDGE_CNT   (9)
#define wedge_FACE_CNT   (5)
#define wedge_FACE_PTS (4+1)

// here is our primalInfo intself
_static_declare_base(wedge);

_static_declare_edges(wedge)=
 { 0,1, 1,2, 2,0,
   3,4, 4,5, 5,3,
   0,3, 2,5, 1,4,
};

_static_declare_face_pts(wedge)=
{  
   0,3,4,1,-1,
   2,1,4,5,-1,
   0,2,5,3,-1,
   0,1,2,-1,-1,
   3,5,4,-1,-1,
};
_static_declare_face_edge(wedge)=
{  
   0,6,3,8,-1,
   1,8,4,7,-1,
   6,2,7,5,-1,
   0,1,2,-1,-1,
   3,5,4,-1,-1,
};

_static_declare_pt_list(wedge)=
{  {1,-1,1},{1,1,-1},{1,-1,-1},
   {-1,-1,1},{-1,1,-1},{-1,-1,-1},
};

/*
_static_declare_pt_list(wedge)=
{  {-1,1,1},{1,-1,1},{-1,-1,1},
   {-1,1,-1},{1,-1,-1},{-1,-1,-1},
};
*/
BOOL PrimShape_CreateWedge(primalInfo *new_wedge)
{
   _build_named_primal(wedge,new_wedge);  // can return FALSE internally
   return TRUE;
}

/////////////////////////////////////////////
// Dodecahedron, or a d12 for you philistines

#define dodec_PT_CNT    (20)
#define dodec_EDGE_CNT  (30)
#define dodec_FACE_CNT  (12)
#define dodec_FACE_PTS (5+1)

// here is our primalInfo intself
_static_declare_base(dodec);

_static_declare_edges(dodec)=
 { 0,1, 1,2, 2,3, 3,4, 4,0,
   4,9, 9,15, 15,5, 5,0, 5,19,
   19,6, 6,1, 6,18, 18,7, 7,2,
   7,17, 17,8, 8,3, 8,16, 16,9,
   10,11, 11,12, 12,13, 13,14, 14,10,
   14,19, 10,15, 11,16, 17,12, 13,18,
   /*
   1,2, 2,3, 3,4, 4,5, 5,1,
   5,10, 10,16, 16,6, 6,1, 6,20,
   20,7, 7,2, 7,19, 19,8, 8,3,
   8,18, 18,9, 9,4, 9,17, 17,10,
   11,12, 12,13, 13,14, 14,15, 15,11,
   15,20, 11,16, 12,17, 18,13, 14,19,
   */
};
   

_static_declare_face_pts(dodec)=
{  0,1,2,3,4,-1,
   0,4,9,15,5,-1,
   1,0,5,19,6,-1,
   2,1,6,18,7,-1,
   3,2,7,17,8,-1,

   4,3,8,16,9,-1,
   10,11,12,13,14,-1,
   10,14,19,5,15,-1,
   11,10,15,9,16,-1,
   12,11,16,8,17,-1,

   13,12,17,7,18,-1,
   14,13,18,6,19,-1,
};
   
_static_declare_face_edge(dodec)=
{  0,1,2,3,4,-1,
   4,5,6,7,8,-1,
   0,8,9,10,11,-1,
   1,11,12,13,14,-1,
   2,14,15,16,17,-1,

   3,17,18,19,5,-1,
   20,21,22,23,24,-1,
   24,25,9,7,26,-1,
   20,26,6,19,27,-1,
   21,27,18,16,28,-1,

   22,28,15,13,29,-1,
   23,29,12,10,25,-1,
};

#define DOD_SCALE 1.0F
#define DODA   (0.0F * DOD_SCALE)
#define DODB   (0.5773502692F * DOD_SCALE)
#define DODC   (0.7946544723F * DOD_SCALE)
#define DODD   (0.1875924741F * DOD_SCALE)
#define DODE   (0.9822469464F * DOD_SCALE)
#define DODF   (0.6070619982F * DOD_SCALE)
#define DODG   (0.9341723590F * DOD_SCALE)
#define DODH   (0.3568220898F * DOD_SCALE)
#define DODI   (0.4911234732F * DOD_SCALE)
#define DODJ   (0.3035309991F * DOD_SCALE)

_static_declare_pt_list(dodec)=
{  
   {DODB,DODD,-DODC},{DODA,DODF,-DODC},{-DODB,DODD,-DODC},{-DODH,-DODI,-DODC},{DODH,-DODI,-DODC},
   {DODG,DODJ,-DODD},{DODA,DODE,-DODD},{-DODG,DODJ,-DODD},{-DODB,-DODC,-DODD},{DODB,-DODC,-DODD},
   {DODB,-DODD,DODC},{DODA,-DODF,DODC},{-DODB,-DODD,DODC},{-DODH,DODI,DODC},{DODH,DODI,DODC},
   {DODG,-DODJ,DODD},{DODA,-DODE,DODD},{-DODG,-DODJ,DODD},{-DODB,DODC,DODD},{DODB,DODC,DODD},
};

BOOL PrimShape_CreateDodecahedron(primalInfo *new_dodec)
{
   _build_named_primal(dodec,new_dodec);  // can return FALSE internally
   return TRUE;
}
