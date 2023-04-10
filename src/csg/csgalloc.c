/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgalloc.c,v 1.6 2000/02/19 12:26:38 toml Exp $

#include <hep.h>
#include <csg.h>
#include <bspdata.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//   storage management routines
//
//  Modularized so we can later change these
//  to allocate out of arrays or something

Hep PPH, PP, PPE, PE;

void *PortalPolyhedronAlloc(void)   { return HepAlloc(&PPH); }
void  PortalPolyhedronFree(void *p) { HepFree(&PPH, p); }

void *PortalPolygonAlloc(void)    { return HepAlloc(&PP); }
void  PortalPolygonFree(void *p)  { HepFree(&PP, p); }

void *PortalPolyEdgeAlloc(void)   { return HepAlloc(&PPE); }
void  PortalPolyEdgeFree(void *p) { HepFree(&PPE, p); }

void *PortalEdgeAlloc(void)       { return HepAlloc(&PE); }
void  PortalEdgeFree(void *p)     { HepFree(&PE, p); }

void portalize_mem_init(void)
{
   MakeHep(&PPH, sizeof(PortalPolyhedron));
   MakeHep(&PP , sizeof(PortalPolygon));
   MakeHep(&PPE, sizeof(PortalPolyEdge));
   MakeHep(&PE , sizeof(PortalEdge));
}

void quiet_portalize_mem_reset(void)
{
   // clear things so we get no warning on reset
   PPH.alloc = PP.alloc = PPE.alloc = PE.alloc = 0;

   ResetHep(&PPH);
   ResetHep(&PP);
   ResetHep(&PPE);
   ResetHep(&PE);
}

void portalize_mem_reset(void)
{
   int size = HepSize(&PPH) + HepSize(&PP) + HepSize(&PPE) + HepSize(&PE);
   if (size) {
      mprintf("Bytes used for internal portal structures: %d bytes.\n", size);
      mprintf(" Polyhedra %d", HepSize(&PPH));
      mprintf(" Polys %d", HepSize(&PP));
      mprintf(" Portal PolyEdges %d", HepSize(&PPE));
      mprintf(" PolyEdges %d\n", HepSize(&PE));
   }

   quiet_portalize_mem_reset();
}
