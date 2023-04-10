// $Header: x:/prj/tech/libsrc/mm/RCS/sortpoly.c 1.1 1997/08/22 19:38:48 kate Exp $
// this module contains the various mesh polygon sorting routines.

#include <sortpoly.h>
#include <mm.h>
#include <stdlib.h>
#include <string.h>
#include <r3d.h>

#define Z_DEPTH(index) (mmd_buff_point(index)->p.z)

// XXX could perhaps make this faster if created a pgon list where vertex
// values are right there, instead of all this indirection
static int pgon_compare(const void *a, const void *b)
{
   mms_pgon *pA=(mms_pgon *)a;
   mms_pgon *pB=(mms_pgon *)b;

   return (Z_DEPTH(pB->v[0])+Z_DEPTH(pB->v[1])+Z_DEPTH(pB->v[2])) - \
      (Z_DEPTH(pA->v[0])+Z_DEPTH(pA->v[1])+Z_DEPTH(pA->v[2])) ;

#if 0
   return (Z_DEPTH(pA->v[0])+Z_DEPTH(pA->v[1])+Z_DEPTH(pA->v[2])) - \
      (Z_DEPTH(pB->v[0])+Z_DEPTH(pB->v[1])+Z_DEPTH(pB->v[2])) ;
#endif
}   

void mm_polysort_global(mms_model *m)
{
   memcpy(mmd_buff_sorted_pgons,mmd_pgons,sizeof(*mmd_pgons)*m->pgons);
   qsort(mmd_buff_sorted_pgons,m->pgons,sizeof(*mmd_buff_sorted_pgons),pgon_compare);
}

