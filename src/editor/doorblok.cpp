/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/doorblok.cpp,v 1.9 2000/03/06 12:31:20 ccarollo Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   doorblok.cpp

   This is the editor-side code which enforces door boundaries during
   portalization.  It does this by generating temporary terrain
   brushes matching the OBBs of the doors, using the media_op for
   blocking.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#include <mprintf.h>
#include <cfgdbg.h>
#include <dynarray.h>
#include <brinfo.h>
#include <brlist.h>
#include <doorprop.h>
#include <doorphys.h>
#include <editbr.h>
#include <editbr_.h>
#include <wrtype.h>
#include <objpos.h>
#include <phprop.h>
#include <primal.h>
#include <propbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// These are in ged_csg.c.
extern "C" char *mediaop_names[];
extern "C" int num_media_ops;


// We keep track of our brushes so we can destroy them after
// portalization.
static cDynArray<editBrush *> g_BrushList;


// When we make a terrain brush to match a door, we want to make sure
// it's completely inside the door and extends well into the door
// frame.  So we treat it as having a constant thinness on its
// smallest axis and enlarge it a little on the other two.
#define kfDoorThickness .04
#define kfDoorGrowth .1

#define kMinorAxis 0
#define kMajorAxis 1

// The brush system's notion of size is really the distance from the
// center to each face, so it's half the kind of size an object has.
static float RescaleAxis(float fSize, int iSize)
{
   if (iSize == kMinorAxis)
      return kfDoorThickness;
   else
      return fSize * .5 + kfDoorGrowth;
}


// rescale a brush's OBB as described above
static void RescaleOBB(mxs_vector *pDims, int iXSize, int iYSize, int iZSize)
{
   pDims->x = RescaleAxis(pDims->x, iXSize);
   pDims->y = RescaleAxis(pDims->y, iYSize);
   pDims->z = RescaleAxis(pDims->z, iZSize);
}


// creates the appropriate brush, if appropriate
static void GenerateBrush(ObjID obj, sDoorProp *pDoor, mxs_angvec *pOrient,
                          int iMedium)
{
   if (OBJ_IS_ABSTRACT(obj))
      return;
   if (!pDoor->vision_blocking)
      return;

   cPhysDimsProp *pDimsProp;
   if (!g_pPhysDimsProp->Get(obj, &pDimsProp))
   {
      mprintf("Door %d has no OBB--cannot block terrain\n", obj);
      return;
   }

   editBrush *pBrush = brushInstantiate(PRIMAL_CUBE_IDX);
   brushSetType(pBrush, brType_TERRAIN);

   mxs_vector obj_offset, world_offset;
   mxs_matrix rotation;

   mx_copy_vec(&obj_offset, &pDimsProp->offset[0]);
   mx_ang2mat(&rotation, pOrient);
   mx_mat_mul_vec(&world_offset, &rotation, &obj_offset);

   mx_add_vec(&pBrush->pos, &pDoor->base_closed_location, &world_offset);
   pBrush->ang = *pOrient;
   pBrush->media = iMedium;

   // find minor axis & set our dimensions
   pBrush->sz = pDimsProp->size;

   if (pBrush->sz.x < pBrush->sz.y && pBrush->sz.x < pBrush->sz.z)
      RescaleOBB(&pBrush->sz, kMinorAxis, kMajorAxis, kMajorAxis);
   else if (pBrush->sz.y < pBrush->sz.x && pBrush->sz.y < pBrush->sz.z)
      RescaleOBB(&pBrush->sz, kMajorAxis, kMinorAxis, kMajorAxis);
   else
      RescaleOBB(&pBrush->sz, kMajorAxis, kMajorAxis, kMinorAxis);

   blistSeek(0, SEEK_END);
   blistInsert(pBrush);

   // keep track so we can nuke 'em when we're done portalizing
   g_BrushList.Append(pBrush);
}


extern "C" void DrBlkGenerateBrushes(void)
{
   // We'd rather do this than have a magic number for our media op.
   int iOpIndex;
   for (iOpIndex = 0; iOpIndex < num_media_ops; ++iOpIndex)
      if (strcmp(mediaop_names[iOpIndex], "blockable") == 0)
         break;

   Assert_ (iOpIndex < num_media_ops);

   sPropertyObjIter iter;
   ObjID obj;
   sRotDoorProp *pRotDoor;
   sTransDoorProp *pTransDoor;

   // For rotating doors we can pull the orientation straight out of
   // the property.
   g_pRotDoorProperty->IterStart(&iter);

   while (g_pRotDoorProperty->IterNext(&iter, &obj)) {
      g_pRotDoorProperty->Get(obj, &pRotDoor);
      GenerateBrush(obj, pRotDoor, &pRotDoor->base_closed_facing, iOpIndex);
   }

   g_pRotDoorProperty->IterStop(&iter);

   // The property for translating doors does not have an orientation
   // since that's constant--so we get it from the Position property.
   g_pTransDoorProperty->IterStart(&iter);

   while (g_pTransDoorProperty->IterNext(&iter, &obj)) {
      g_pTransDoorProperty->Get(obj, &pTransDoor);

      ObjPos *pPos = ObjPosGet(obj);
      if (!pPos) {
         ConfigSpew("BlockSpew", 
                    ("Door %d has no position--cannot block terrain\n", obj));
         break;
      }

      GenerateBrush(obj, pTransDoor, &pPos->fac, iOpIndex);
   }

   g_pTransDoorProperty->IterStop(&iter);
}


extern "C" void DrBlkDestroyBrushes(void)
{
   int iSize = g_BrushList.Size();

   for (int i = 0; i < iSize; ++i) {
      blistSetPostoBrush(g_BrushList[i]);
      blistDelete();
   }

   g_BrushList.SetSize(0);
}


// This is in preparation for lighting the level.  We don't want the
// doors to interfere with how we find the cells affected by each
// light, since the boundary between affected and unaffeted cells
// looks goofy.
extern "C" void DrBlkUnblockAll(void)
{
   sPropertyObjIter iter;
   ObjID obj;

   g_pRotDoorProperty->IterStart(&iter);
   while (g_pRotDoorProperty->IterNext(&iter, &obj))
      if (OBJ_IS_CONCRETE(obj))
         RemoveDoorBlocking(obj);
   g_pRotDoorProperty->IterStop(&iter);

   g_pTransDoorProperty->IterStart(&iter);
   while (g_pTransDoorProperty->IterNext(&iter, &obj))
      if (OBJ_IS_CONCRETE(obj))
         RemoveDoorBlocking(obj);
   g_pTransDoorProperty->IterStop(&iter);
}


// This is the opposite of the above.  All doors which are supposed to
// be blocking are set to do so.
extern "C" void DrBlkReblockAll(void)
{
   sPropertyObjIter iter;
   ObjID obj;

   g_pRotDoorProperty->IterStart(&iter);
   while (g_pRotDoorProperty->IterNext(&iter, &obj))
      if (OBJ_IS_CONCRETE(obj))
         UpdateDoorBrush(obj);
   g_pRotDoorProperty->IterStop(&iter);

   g_pTransDoorProperty->IterStart(&iter);
   while (g_pTransDoorProperty->IterNext(&iter, &obj))
      if (OBJ_IS_CONCRETE(obj))
         UpdateDoorBrush(obj);
   g_pTransDoorProperty->IterStop(&iter);
}
