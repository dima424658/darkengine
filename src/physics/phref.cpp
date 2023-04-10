/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phref.cpp,v 1.12 1999/06/28 18:56:12 ccarollo Exp $
//
// Physics ref system
//


#include <lg.h>
#include <dynarray.h>
#include <allocapi.h>

#include <portal.h>
#include <wr.h>
#include <objpos.h>
#include <objshape.h>

#include <osysbase.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phref.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

cDynArray<void *>PhysRefs;

////////////////////////////////////////

#pragma off(unreferenced)
ObjRefID *PhysRefListFunc(void *bin, BOOL create)
{
   int n = *(int *)bin;
   return (ObjRefID *) &PhysRefs[n];
}
#pragma on(unreferenced)

////////////////////////////////////////

void PhysRefListClearFunc(void *bin)
{
   int n = *(int *)bin;
   PhysRefs[n] = NULL;
}

////////////////////////////////////////

#define BINS_EQUAL(b1,b2) ((*(int *)b1) == (*(int *)b2))
int PhysBinCompareFunc(void *bin1, void *bin2)
{
   return BINS_EQUAL(bin1, bin2);
}

////////////////////////////////////////

#define BIN_UPDATE_FUNC PhysBinUpdateFunc
#define BIN int
#include <objupd.h>
#undef BIN_UPDATE_FUNC
#undef BIN
#undef BINS_EQUAL

#pragma off(unreferenced)
void PhysBinUpdateFuncDummy(ObjID obj, int refsys, void *binmem, int num_bins)
{
   // See the comment in PhysBinComputeFuncDummy
}
#pragma on(unreferenced)

////////////////////////////////////////

void PhysBinPrintFunc(void *bin, char *str, int maxstrlen)
{
   int n = *(int *)bin;

   if(maxstrlen < 8)
   {
      str = NULL;
      return;
   }

   sprintf(str, "[%d]", n);
}

////////////////////////////////////////

#define OUR_MAX_REFS 64

#pragma off(unreferenced)
int PhysBinComputeFuncDummy(ObjID obj, BoundingPrism *bound, void *bins)
{
   // This is the function that the refsystem calls when it's doing an
   // ObjUpdateLocs.  We want to explicitly update physics refs when the
   // physics model has enough information to determine its end-of-frame
   // position, so we don't need this guy to do anything.

   return 0;
}
#pragma on(unreferenced)

////////////////////////////////////////

int PhysBinComputeFunc(ObjID obj, BoundingPrism *bound, void *bins)
{
   int *gbins = (int *) bins;
   int x, count = 0;
   ObjPos* pos = ObjPosGet(obj);

   Location seedLoc;
   mxs_vector seedVec;

   seedVec.x = (bound->xmax + bound->xmin) / 2;
   seedVec.y = (bound->ymax + bound->ymin) / 2;
   seedVec.z = (bound->zmax + bound->zmin) / 2;
  
   MakeHintedLocationFromVector(&seedLoc, &seedVec, &pos->loc);

   x = CellFromLoc(&seedLoc);
   if (x == CELL_INVALID)
   {
      seedLoc = pos->loc;
      x = CellFromLoc(&pos->loc);
   }
   if (x != CELL_INVALID) {
      mxs_vector bmin, bmax;
      bmin.x = bound->xmin;
      bmin.y = bound->ymin;
      bmin.z = bound->zmin;
      bmax.x = bound->xmax;
      bmax.y = bound->ymax;
      bmax.z = bound->zmax;

      count = PortalCellsInArea(gbins, OUR_MAX_REFS, &bmin, &bmax, &seedLoc);
   }

   return count;
}

////////////////////////////////////////////////////////////////////////////////

// copied from refsys.c
#define OBJ_BINS_SPACE 1024

int PhysRefSystemID = -1;

int PhysUpdateRefsCore(cPhysModel *pModel, ObjID objID)
{
   BoundingPrism prism;   
   mxs_vector    minvec, maxvec;
   mxs_vector    delta_pos;

   if ((PhysRefs.Size() == 0) || (wr_num_cells == 0))
      return FALSE;

   // Compute movement-based bounding box
   PhysGetAABBox(objID, &minvec, &maxvec);

   mx_sub_vec(&delta_pos, &pModel->GetEndLocationVec(), &pModel->GetLocationVec());
   for (int i=0; i<3; i++)
   {
      if (delta_pos.el[i] < 0)
         minvec.el[i] += delta_pos.el[i];
      else
         maxvec.el[i] += delta_pos.el[i];
   }

   prism.xmin = minvec.el[0];
   prism.xmax = maxvec.el[0];
   prism.ymin = minvec.el[1];
   prism.ymax = maxvec.el[1];
   prism.zmin = minvec.el[2];
   prism.zmax = maxvec.el[2];

   void *bins = new uchar[OBJ_BINS_SPACE];

   int num_bins = PhysBinComputeFunc(objID, &prism, bins);
   PhysBinUpdateFunc(objID, PhysRefSystemID, bins, num_bins);

   delete bins;

   return TRUE;
}

int PhysUpdateRefs(cPhysModel *pModel)
{
   return PhysUpdateRefsCore(pModel, pModel->GetObjID());
}

int PhysUpdateRefsObj(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.GetActive(objID);

   if (pModel != NULL)
      return PhysUpdateRefsCore(pModel, objID);
   else
      return FALSE;
}

////////////////////////////////////////

int PhysRefSystemInit()
{
   PhysRefSystemClear();

   PhysRefSystemID = ObjRefSystemRegister(sizeof(int), 
                               PhysRefListFunc, 
                               PhysRefListClearFunc,
                               PhysBinCompareFunc,
                               PhysBinUpdateFuncDummy,
                               PhysBinPrintFunc,
                               PhysBinComputeFuncDummy);
   return PhysRefSystemID;
}

////////////////////////////////////////

void PhysRefSystemClear()
{
   if (PhysRefSystemID == -1)
      return;

   // Delete any physics refs on any objects
   for (int i=0; i<gMaxObjID; i++)
      ObjDelRefsOfSystem(i, PhysRefSystemID);

   PhysRefs.SetSize(0);
   
}

void PhysRefSystemResize()
{
   if (PhysRefs.Size() >= wr_num_cells)
      return; 
   
   if (PhysRefs.Size() > 0)
      PhysRefSystemClear(); 

   LGALLOC_PUSH_CREDIT();
   PhysRefs.SetSize(wr_num_cells);
   LGALLOC_POP_CREDIT();

   for (int i=0; i<wr_num_cells; i++)
      PhysRefs[i] = NULL;
}

void PhysRefSystemRebuild()
{

   // Re-ref all physical objects
   cPhysModel *pModel;

   pModel = g_PhysModels.GetFirstActive();
   while (pModel != NULL)
   {
      ObjDelRefsOfSystem(pModel->GetObjID(), PhysRefSystemID);

      PhysUpdateRefs(pModel);

      pModel = g_PhysModels.GetNextActive(pModel);
   }
}

////////////////////////////////////////

void PhysRefSystemTerm()
{
   PhysRefs.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////






