/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csg.h,v 1.16 2000/01/29 12:58:24 adurant Exp $
#pragma once
#ifndef __CSG_H
#define __CSG_H

#include <stdlib.h>   // size_t

  // private version of real so we can try both float and double
#if 0
  typedef float Real;
  #define CODE "0"
#else
  typedef double Real;
  #define CODE "0"
#endif

typedef struct
{
   Real x,y,z, dummy;
} BspVertex;

#ifdef EDITOR

EXTERN float REAL_EPSILON;

typedef void (*CSGReadWriteFunc) (void *buf, size_t elsize, size_t nelem);
EXTERN void save_csg_internal_database(CSGReadWriteFunc func);
EXTERN void load_csg_internal_database(CSGReadWriteFunc func);
EXTERN void free_csg_internal_database();
EXTERN void remap_csg_database(int* map);

EXTERN void init_csg_internal_database(void);
EXTERN void default_csg_medium(int medium);

EXTERN void cid_insert_brush(int brushid, void *geom);

EXTERN void portalize_csg_internal_database(void);
EXTERN void free_portal_database(void);
EXTERN void reset_lighting(void);

EXTERN int global_ambient;

// to reset face data
EXTERN void ReassignTexture(int br_id, BOOL texture_only);

EXTERN void *CsgMalloc(size_t size);

#endif

#endif
