/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/eosapp.c,v 1.34 2000/02/24 23:43:11 mahk Exp $

// Interface to object system

#include <lgsprntf.h>

#include <objshape.h>
#include <refsys.h>
#include <wrobj.h>
#include <wrtype.h>
#include <osysbase.h>

#include <portal.h>
#include <hilight.h>
#include <config.h>
#include <objpos.h>

#include <string.h>

////////////////////////////////////////////////////////////
//
// OBJECT SYSTEM FUN

// Here are the things we need to define ourselves

#define MAX_REFS 1408

////////////////////////////////////////////////////////////
//
//  Ref System... we use those pointers dangling
//  off of cells...

void CellRefInit (void)
{
   int i;
   for (i=0; i < wr_num_cells; ++i)
      WR_CELL(i)->refs = 0;
}

#pragma off(unreferenced)
ObjRefID *CellRefListFunc (void *bin, BOOL create)
{
   int n = * (int *) bin;
   return (ObjRefID *) &WR_CELL(n)->refs;
}
#pragma on(unreferenced)

void CellRefListClearFunc (void *bin)
{
   int n = * (int *) bin;
   WR_CELL(n)->refs = 0;
}

#define BINS_EQUAL(b1,b2) (*((int *)(b1)) == *((int *)(b2)))
int CellBinCompareFunc (void *bin1, void *bin2)
{
   return BINS_EQUAL(bin1,bin2);
}

#define BIN_UPDATE_FUNC CellBinUpdateFunc
#define BIN int
#include <objupd.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
#undef BIN_UPDATE_FUNC
#undef BIN
#undef BINS_EQUAL

void CellBinPrintFunc (void *bin, char *str, int maxstrlen)
{
   int n = *(int *)bin;

   if (maxstrlen < 8) {*str = 0; return;}
   lg_sprintf (str, "[%d]", n);
}

bool show_cells;

#define OUR_MAX_REFS 64

// @HACK: test info gathering hack
#ifndef SHIP
BOOL always_split_all_models=FALSE;
#endif 

#ifdef  EDITOR
BOOL hilight_split_objs;      // used by editor to show split models
#endif


extern uchar portal_area_flags;
extern bool obj_split[];

int CellBinComputeFunc (ObjID obj, BoundingPrism *bound, void *bins)
{
   int *gbins = (int *) bins;
   int x, count = 0;
   ObjPos* pos = ObjPosGet(obj); 

   x = CellFromPos(pos);
   if (x != CELL_INVALID) {
      mxs_vector bmin, bmax;
      bmin.x = bound->xmin;
      bmin.y = bound->ymin;
      bmin.z = bound->zmin;
      bmax.x = bound->xmax;
      bmax.y = bound->ymax;
      bmax.z = bound->zmax;

      count = PortalCellsInArea(gbins, OUR_MAX_REFS, &bmin, &bmax, &pos->loc);

#ifndef SHIP
      if (always_split_all_models)
         obj_split[obj] = (count >= 1) ? 2 : 0;
      else
#endif
         obj_split[obj] = (portal_area_flags & PORTAL_SPLITS_OBJECT ? 2 : 0);

#ifdef EDITOR
      if (hilight_split_objs && count > 1)
         hilightAddByObjId(obj);
#endif

      if (show_cells)
      {
         int i;
         for (i=0; i < count; ++i)
            WR_CELL(gbins[i])->flags |= CELL_RENDER_WIREFRAME_ONCE;
      }
   }

   return count;
}

int CellRefRegister (void)
{
   return ObjRefSystemRegister (sizeof (int),
                                CellRefListFunc,
                                CellRefListClearFunc,
                                CellBinCompareFunc,
                                CellBinUpdateFunc,
                                CellBinPrintFunc,
                                CellBinComputeFunc);
}

////////////////////////////////////////////////////////////
// Bounding prism function
void EOSBoundingPrism (ObjID obj, void *pos_, BoundingPrism *bound)
{
   //   Obj *objp        = OBJID_TO_PTR (obj);
   //   Position *pos    = (Position *) pos_;
   mxs_vector       minvec, maxvec;

   // is this the right thing?
   ObjGetWorldBBox(obj, &minvec, &maxvec);
   bound->xmin = minvec.el[0];
   bound->xmax = maxvec.el[0];
   bound->ymin = minvec.el[1];
   bound->ymax = maxvec.el[1];
   bound->zmin = minvec.el[2];
   bound->zmax = maxvec.el[2];

}

//////////////////////////////
//
// Tying it all together

int mCellRefSys;
#define MAX_REF_OBJECTS 4096

void EOSInit (void)
{
   extern void init_object_rendering();
   extern bool span_clip;
   int max_refs = MAX_REFS; 

   // Initialize RefSystems
   config_get_int("max_refs",&max_refs);
   
   //@TODO: make ref system resizable
   ObjRefSystemInit (HACK_MAX_OBJ, max_refs, 2, EOSBoundingPrism, ObjPosGetUnsafe);
   mCellRefSys = CellRefRegister();

   ObjsInitRefs(); 

   init_object_rendering();
}

////////////////////////////////////////

void EOSTerm (void)
{
   ObjRefSystemTerm();
}


