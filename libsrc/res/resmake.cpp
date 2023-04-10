//      ResMake.c       Resource making
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/resmake.cpp 1.5 1996/09/14 16:10:34 TOML Exp $
* $Log: resmake.cpp $
 * Revision 1.5  1996/09/14  16:10:34  TOML
 * Prepared for revision
 * 
 * Revision 1.4  1996/09/14  14:12:43  TOML
 * Made C++ parser friendly
 *
 * Revision 1.3  1994/09/22  10:48:17  rex
 * Modified access to resdesc flags, which have moved
 *
 * Revision 1.2  1994/06/16  11:08:04  rex
 * Modified LRU list handling, lock resource made with ResMake() instead of
 * setting RDF_NODROP flag
 *
 * Revision 1.1  1994/02/17  11:23:57  rex
 * Initial revision
 *
*/

#include <string.h>

#include <res.h>
#include <res_.h>

//  --------------------------------------------------------
//
//  ResMake() makes a resource from a data block.
//
//      Id      = id of resource
//      ptr     = ptr to memory block (resource is not copied; this should
//                  point to storage where the resource can live indefinitely)
//      size    = size of resource in bytes
//      type    = resource type (RTYPE_XXX)
//      filenum = file number
//      flags   = flags (RDF_XXX)

void ResMake(Id id, void *ptr, long size, uchar type, int filenum, uchar flags)
{
   ResDesc *prd;
   ResDesc2 *prd2;
//  Extend res desc table if need to

   ResExtendDesc(id);

//  Check for resource at that id, if so delete it

   Spew(DSRC_RES_Make, ("ResMake: making resource $%x\n", id));

   prd = RESDESC(id);
   if (prd->offset)
      ResDelete(id);

//  Add us to the soup, set lock so doesn't get swapped out

   prd->ptr = ptr;
   prd->size = size;
   prd->filenum = filenum;
   prd->lock = 1;
   prd->offset = RES_OFFSET_PENDING;

   prd2 = RESDESC2(id);
   prd2->flags = flags;
   prd2->type = type;
}
//  ---------------------------------------------------------------
//
//  ResMakeCompound() makes an empty compound resource
//
//      id      = id of resource
//      type    = resource type (RTYPE_XXX)
//      filenum = file number
//      flags   = flags (RDF_XXX, RDF_COMPOUND automatically added)

void ResMakeCompound(Id id, uchar type, int filenum, uchar flags)
{
   RefTable *prt;
   long sizeTable;
//  Build empty compound resource in allocated memory

   Spew(DSRC_RES_Make, ("ResMake: making compound resource $%x\n", id));

   sizeTable = REFTABLESIZE(0);
   prt = (RefTable *) Malloc(sizeTable);
   prt->numRefs = 0;
   prt->offset[0] = sizeTable;

//  Make a resource out of it

   ResMake(id, prt, sizeTable, type, filenum, flags | RDF_COMPOUND);
}
//  ---------------------------------------------------------------
//
//  ResAddRef() adds an item to a compound resource.
//
//      ref      = reference
//      pitem    = ptr to item's data (copied from here, unlike simple resource)
//      itemSize = size of item

void ResAddRef(Ref ref, void *pitem, long itemSize)
{
   ResDesc *prd;
   RefTable *prt;
   RefIndex index,
    i;
   long sizeItemOffsets,
    oldSize,
    sizeDiff;
//  Error check

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return;
   });

//  Get vital info (and get into memory if not already)

   Spew(DSRC_RES_Make, ("ResAddRef: adding ref $%x\n", ref));

   prd = RESDESC(REFID(ref));
   prt = (RefTable *) prd->ptr;
   if (prt == NULL)
      prt = (RefTable *) RefGet(ref);

//  If index within current range of compound resource, replace or insert

   index = REFINDEX(ref);
   if (index < prt->numRefs)
   {
      oldSize = RefSize(prt, index);

//  If same size, just copy in

      if (itemSize == oldSize)
      {
         Spew(DSRC_RES_Make, ("ResAddRef: replacing same size ref\n"));
         memcpy(REFPTR(prt, index), pitem, itemSize);
      }

//  Else if new item smaller, reduce offsets, shift data, insert new data

      else if (itemSize < oldSize)
      {
         Spew(DSRC_RES_Make, ("ResAddRef: replacing larger ref\n"));
         sizeDiff = oldSize - itemSize;
         for (i = index + 1; i <= prt->numRefs; i++)
            prt->offset[i] -= sizeDiff;
         prd->size -= sizeDiff;
         memmove(REFPTR(prt, index + 1), REFPTR(prt, index + 1) + sizeDiff,
                 prt->offset[prt->numRefs] - prt->offset[index + 1]);
         memcpy(REFPTR(prt, index), pitem, itemSize);
         prd->ptr = Realloc(prd->ptr, prd->size);
      }
      else
      {
         Spew(DSRC_RES_Make, ("ResAddRef: replacing smaller ref\n"));
         sizeDiff = itemSize - oldSize;
         prd->size += sizeDiff;
         prd->ptr = prt = (RefTable *) Realloc(prd->ptr, prd->size);
         memmove(REFPTR(prt, index + 1) + sizeDiff, REFPTR(prt, index + 1),
                 prt->offset[prt->numRefs] - prt->offset[index + 1]);
         for (i = index + 1; i <= prt->numRefs; i++)
            prt->offset[i] += sizeDiff;
         memcpy(REFPTR(prt, index), pitem, itemSize);
      }
   }

//  Else if index exceeds current range, expand

   else
   {
      Spew(DSRC_RES_Make, ("ResAddRef: extending compound resource\n"));

      // Extend resource for new offset(s) and data item

      sizeItemOffsets = sizeof(long) * ((index + 1) - prt->numRefs);
      prd->size += sizeItemOffsets + itemSize;
      prd->ptr = Realloc(prd->ptr, prd->size);
      prt = (RefTable *) prd->ptr;

      // Shift data upwards to make room for new offset(s)

      memmove(REFPTR(prt, 0) + sizeItemOffsets, REFPTR(prt, 0),
              prd->size - REFTABLESIZE(index + 1));

      // Advance old offsets, set new ones

      for (i = 0; i <= prt->numRefs; i++)
         prt->offset[i] += sizeItemOffsets;
      for (i = prt->numRefs + 1; i <= index; i++)
         prt->offset[i] = prt->offset[prt->numRefs];
      prt->offset[index + 1] = prt->offset[index] + itemSize;

      // Copy data into place, set new numRefs

      memcpy(REFPTR(prt, index), pitem, itemSize);
      prt->numRefs = index + 1;
   }
}
//  -------------------------------------------------------------
//
//  ResUnmake() removes a resource from the LRU list and sets its
//      ptr to NULL.  In this way, a program may take over management
//      of the resource data, and the RES system forgets about it.
//      This is typically done when user-managed data needs to be
//      written to a resource file, using ResMake(),ResWrite(),ResUnmake().
//
//      id = id of resource to unmake

void ResUnmake(Id id)
{
   memset(RESDESC(id), 0, sizeof(ResDesc));
}
