/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* objupd.h
**
** Template for bin-updating functions
**
** $Header: r:/t2repos/thief2/libsrc/ref/objupd.h,v 1.1 1998/03/14 22:05:14 mahk Exp $
** $Log: objupd.h,v $
** Revision 1.1  1998/03/14 22:05:14  mahk
** Initial revision
**
 * Revision 1.1  1996/07/24  12:39:15  DFAN
 * Initial revision
 * 
*/

// THIS IS NOT A REGULAR HEADER FILE!
//
// This is a template for defining a bin-updating function for a RefSystem.
//
// This code needs to be fast; otherwise I'd implement this the "nice" way
// (by using a callback on BINS_EQUAL).  See the discussion of a
// BinUpdateFunc in refsys.h.
//
// #define the following macros:
//
//   BIN_UPDATE_FUNC: the name of this bin-updating function
//   BIN: the name of a bin in your RefSystem
//   BINS_EQUAL(b1,b2): takes pointers to two bins,
//                      returns TRUE if they're equal
//
// and then #include this file.
//
// I recommend that you also put something with the same name there
// which looks like a function, so that this grotesque hackery is
// somewhat easier to track down.


// If we have more than this many refs per obj, we're going to take
// forever anyway
// @TODO: make this a c++ template
#define MAX_REFS_PER_OBJ 256

void BIN_UPDATE_FUNC (ObjID obj, int refsys, void *binmem, int num_bins)
{
   ObjRefID  old_refs[MAX_REFS_PER_OBJ];
   BIN      *new_bins = (BIN *) binmem;
   int       oldcount = 0;
   ObjRefID  ref = OBJ_FIRST_REF(obj,refsys); 
   ObjRefID  firstref = ref;
   ObjRef   *refp;
   int       i, j;

   // ROBUSTIFY: oldcount must be < MAX_REFS_PER_OBJS
   if (firstref != 0)
   {
      do
      {
         refp = OBJREFID_TO_PTR(ref);
         old_refs[oldcount++] = ref;
         ref = refp->next_of_obj;
      }
      while (ref != firstref);
   }
   
   for (i = 0; i < num_bins; i++)
   {
      for (j = 0; j < oldcount; j++)
      {
         if (BINS_EQUAL(&new_bins[i],
                        ((BIN *) (&(OBJREFID_TO_PTR(old_refs[j])->bin)))))
         {
            // no change, forget about it
            old_refs[j] = old_refs[--oldcount];
            goto done_with_bin;
         }
      }

      // A new one!
      ObjRefMake (obj, refsys, &new_bins[i]);

done_with_bin:;
   }

   // We've gone through the new ones, see if there are any old ones left
   for (i = 0; i < oldcount; i++)
   {
      ObjRefDel (old_refs[i]);
   }
}

/*
Local Variables:
typedefs:("BIN" "Obj" "ObjID" "ObjRef" "ObjRefID")
End:
*/
