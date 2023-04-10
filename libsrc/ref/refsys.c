/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/ref/refsys.c,v 1.3 1998/09/21 20:35:05 mahk Exp $

#include <lg.h>
#include <refsys.h>
#include <string.h>

int               mNumRefSystems;     // The number of RefSystems that exist
int               mMaxRefSystems;     // The number of RefSystems that space is
                                      // allocated for
ObjRefSystem      *mRefSystems;       // Pointer to the array of RefSystems
BoundingPrismFunc gBoundingPrismFunc; // How to compute a bounding prism
ObjPosFunc        gObjPosFunc;        // How to find out an object's position 

int               mNumRefs;           // How many RefInfos are allocated
ObjRefID          mRefFirstFree;      // The first RefInfo in the free chain

ObjRefID**        gFirstRefs;         // 
int               gMaxRefs;           // Maximum number of refs allowed
ObjRefInfo        *ObjRefInfos;       // ObjRef information, gMaxRefs big



//////////////////////////////
//
// Get space for an ObjRef of the given RefSystem
//
ObjRef *ObjRefMalloc (int refsys)
{
   return (ObjRef *) Malloc(mRefSystems[refsys].ref_size);
}

//////////////////////////////
//
// Free up an ObjRef's space
//
BOOL ObjRefFree (ObjRef *refp)
{
   Free(refp); 
   return TRUE;
}

//////////////////////////////
//
// Initialize the ObjRefSystem system, so that it can hold
// num RefSystems by default.
//
BOOL ObjRefSystemInit (int               max_objs, 
                       int               max_refs, 
                       int               num_systems,
                       BoundingPrismFunc bounding_prism_func,
                       ObjPosFunc obj_pos_func)
{
   int i; 

   mMaxRefSystems = num_systems; 
   mRefSystems = Malloc (num_systems * sizeof (ObjRefSystem));
   ObjRefInfos = Malloc (max_refs*sizeof(ObjRefInfos[0])); 
   gFirstRefs = Malloc(num_systems*sizeof(gFirstRefs[0])); 
   for (i = 0; i < num_systems; i++)
   {
      gFirstRefs[i] = Malloc(max_objs*sizeof(gFirstRefs[i][0]));
      memset(gFirstRefs[i],0,max_objs*sizeof(gFirstRefs[i][0]));
   }
   
   gBoundingPrismFunc = bounding_prism_func;
   gObjPosFunc = obj_pos_func; 
   gMaxRefs = max_refs; 

   return TRUE;
}

BOOL ObjRefSystemTerm(void)
{
   int i; 
   Free(mRefSystems);
   mRefSystems = NULL; 
   for (i = 0; i < mMaxRefSystems; i++)
      Free(gFirstRefs[i]); 
   Free(gFirstRefs); 
   gFirstRefs = NULL;
   Free(ObjRefInfos);
   ObjRefInfos = NULL;

   return TRUE; 
}

//////////////////////////////
//
// Returns the ID of the RefSystem you've made
//
RefSystemID ObjRefSystemRegister (char              bin_size,
                                  RefListFunc       ref_list_func,
                                  RefListClearFunc  ref_list_clear_func,
                                  BinCompareFunc    bin_compare_func,
                                  BinUpdateFunc     bin_update_func,
                                  BinPrintFunc      bin_print_func,
                                  BinComputeFunc    bin_compute_func)
{
   ObjRefSystem *pors;

   if (mNumRefSystems == mMaxRefSystems)
   {
      Realloc (mRefSystems, (++mMaxRefSystems) * sizeof (ObjRefSystem));
   }
   
   pors                      = &mRefSystems[mNumRefSystems];
   pors->ref_size            = sizeof (ObjRef) - 1 + bin_size; // ugh
   pors->bin_size            = bin_size;
   pors->ref_list_func       = ref_list_func;
   pors->ref_list_clear_func = ref_list_clear_func;
   pors->bin_compare_func    = bin_compare_func;
   pors->bin_print_func      = bin_print_func;
   pors->bin_compute_func    = bin_compute_func;
   pors->bin_update_func     = bin_update_func;


   return mNumRefSystems++;
}


//////////////////////////////
//
// Initialize the refs
//
void ObjsInitRefs (void)
{
   int i;

   mRefFirstFree = 1;

   for (i = 0; i < gMaxRefs - 1; i++)
   {
      ObjRefInfos[i].next = i+1;
   }
   ObjRefInfos[gMaxRefs - 1].next = 0;
}

//////////////////////////////
//
// Get a free ObjRef of the given RefSystem
//
ObjRefID ObjRefGrab (RefSystemID system)
{
   ObjRefID  ref = mRefFirstFree;
   ObjRef   *refp;

   if (ref == 0)
   {
      Warning (("ObjRefGrab: Ran out of object memory\n"));
      return 0;
   }

   // The order here is important because next & ref are a union
   refp = ObjRefMalloc (system);          // get the space we need
   mRefFirstFree = ObjRefInfos[ref].next; // remove us from free chain
   ObjRefInfos[ref].ref = refp;           // point us at the space

   // Null out some stuff
   refp->refsys      = system;
   refp->obj         = 0;
   refp->next_in_bin = 0;
   refp->next_of_obj = 0;

   return ref;
}

//////////////////////////////
//
// Analogous to ObjReturnToStorage
//
void ObjRefReturnToStorage (ObjRefID ref)
{
   ObjRef *refp = OBJREFID_TO_PTR (ref);

   // Free up the space
   ObjRefFree (refp);

   // Put us back in the free chain
   ObjRefInfos[ref].next = mRefFirstFree;
   mRefFirstFree = ref;
}

//////////////////////////////
//
// Link up an object to a ref
//
BOOL ObjLinkMake (ObjID obj, ObjRefID ref)
{
   ObjRef      *refp   = OBJREFID_TO_PTR (ref);
   RefSystemID  refsys = refp->refsys;

   if (!OBJ_FIRST_REF(obj,refsys))
   {
      // We're the first ObjRef of this system to refer to obj
      OBJ_FIRST_REF(obj,refsys) = ref;
      refp->next_of_obj = ref;
   }
   else
   {
      // There are other references already
      ObjRef *refp2 = OBJREFID_TO_PTR(OBJ_FIRST_REF(obj,refsys));

      refp->next_of_obj  = refp2->next_of_obj;
      refp2->next_of_obj = ref;
   }

   // Now hook it up
   refp->obj = obj;
   return TRUE;
}

//////////////////////////////
//
// Unlink a ref from its object
//
BOOL ObjLinkDel (ObjRefID ref)
{
   ObjRef      *refp   = OBJREFID_TO_PTR(ref);
   RefSystemID  refsys = refp->refsys;
   ObjID        obj    = refp->obj;

   // ROBUSTIFY: Check that objp is valid

   if (refp->next_of_obj == ref)    // last one
   {
      OBJ_FIRST_REF(obj,refsys) = 0;
      return TRUE;
   }
   else
   {
      // Run around the circular list until we reach ourselves,
      // and splice ourselves out
      ObjRefID curref  = ref;
      ObjRef  *currefp = refp;
      while (currefp->next_of_obj != ref)
      {
         curref  = currefp->next_of_obj;
         currefp = OBJREFID_TO_PTR(curref);
      }
      currefp->next_of_obj = refp->next_of_obj;
      OBJ_FIRST_REF(obj,refsys) = curref;
   }

   refp->obj = 0;
   return TRUE;
}

//////////////////////////////
//
// Put a ref in the world at the given spot
// It must already refer to an object.
//
BOOL ObjRefAdd (ObjRefID ref, void *bin)
{
   ObjRef      *refp   = OBJREFID_TO_PTR(ref); // ObjRef of me
   RefSystemID  refsys = refp->refsys;         // RefSystem I'm in
   ObjRefID    *refhead;                       // ptr start of the ref list in
                                               //   this bin

   // ROBUSTIFY: Check that we refer to an obj

   // Find the head of the ref list for this bin
   refhead = mRefSystems[refsys].ref_list_func (bin, TRUE);

   // Insert us
   refp->next_in_bin = *refhead;
   *refhead = ref;

   // Copy in the appropriate bin information
   memcpy (refp->bin, bin, mRefSystems[refsys].bin_size);

   return TRUE;
}

//////////////////////////////
//
// Remove a ref from the world
//
void ObjRefRem (ObjRefID ref)
{
   ObjRef      *refp   = OBJREFID_TO_PTR(ref); // ObjRef of me
   RefSystemID  refsys = refp->refsys;         // RefSystem I'm in
   ObjRefID    *refhead;                       // Address of what points to
                                               //   head of the reflist
   ObjRefID    *prevref; 


   // ROBUSTIFY: Check that objp is valid

   refhead  = mRefSystems[refsys].ref_list_func (refp->bin, FALSE);
   prevref = refhead;

   // ROBUSTIFY: Check that refhead is valid

   if (*refhead == ref)
   {
      // We're the first in this bin
      if (refp->next_in_bin == 0)
      {
         // We're the only ref in this bin
         mRefSystems[refsys].ref_list_clear_func (refp->bin);
         return;
      }
   }
   else
   {
      ObjRef* nextrefp = OBJREFID_TO_PTR(*prevref);
      while (*prevref != ref)
      {
         prevref = &nextrefp->next_in_bin;
         if (*prevref == 0) // we're not in the reflist at all, what gives? 
            return; 
         nextrefp = OBJREFID_TO_PTR(*prevref);
      }
   }

   // Now we're pointing at the right thing, so we can splice our ref out.
   *prevref = refp->next_in_bin;
   refp->next_in_bin = 0;
}

//////////////////////////////
//
// Gets a free ObjRef, assigns it to the given Obj,
// and puts in the world in the given place.
//
// Returns the ObjRefID, or 0 if it failed.
//
ObjRefID ObjRefMake (ObjID obj, RefSystemID refsys, void *bin)
{
   ObjRefID ref;
   BOOL    ok;

   if ((ref = ObjRefGrab (refsys)) == 0) return 0;
   ok = ObjLinkMake (obj, ref); if (!ok) return 0;   // ROBUSTIFY: Check ok
   ok = ObjRefAdd (ref, bin);   if (!ok) return 0;   // ROBUSTIFY: Check ok

   return ref;
}

//////////////////////////////
//
// Removes an ObjRef from the map, destroys its reference to its Obj,
// and destroys it.
//
BOOL ObjRefDel (ObjRefID ref)
{
   ObjRefRem (ref);
   ObjLinkDel (ref);
   ObjRefReturnToStorage (ref);
   return TRUE;
}

//////////////////////////////
//
// Deletes all references to a given object in a given refsystem
//
BOOL ObjDelRefsOfSystem (ObjID obj, RefSystemID refsys)
{
   ObjRefID ref;
   ObjRef  *refp;
   ObjRefID nextref = 0;


   if ((ref = OBJ_FIRST_REF(obj,refsys)) == 0) return TRUE; // all done
   refp = OBJREFID_TO_PTR(ref);

   // Go through the circular list deleting refs.  Slower than it has
   // to be because ObjRefRem cleans up after itself each time.
   while (TRUE)
   {
      nextref = refp->next_of_obj;
      ObjRefRem (ref);
      ObjLinkDel (ref);
      ObjRefReturnToStorage (ref);
      if (ref == nextref) break;
      ref = nextref;
      refp = OBJREFID_TO_PTR(ref);
   }
   
   OBJ_FIRST_REF(obj,refsys) = 0;
   return TRUE;
}

//////////////////////////////
//
// Deletes all references to a given object in all refsystems
//
BOOL ObjDelRefs (ObjID obj)
{
   RefSystemID sys;
   BOOL ok;

   for (sys = 0; sys < mNumRefSystems; sys++)
      if (!(ok = ObjDelRefsOfSystem (obj, sys))) return FALSE;

   return TRUE;
}


//
// Cannibalized from eos
//

// Size of space to store bins this obj extends into
#define OBJ_BINS_SPACE 1000

//////////////////////////////
//
// Updates the location of an object, returns success
// You have already updated the object's position.
// In fact, you should have called the macros ObjSetLocation or
// ObjSetPosition, which will call this automatically.
//


BOOL ObjUpdateLocs (ObjID obj)
{
   RefSystemID    sys;
   BoundingPrism  prism;
   int            num_bins;     // how many bins for this RefSystem
   void          *pos;          // position of object
   void          *bins; 

   Assert_(obj > 0); 

   pos = gObjPosFunc(obj);
   if (obj < 0 || !pos)
      return FALSE; 
      
   gBoundingPrismFunc (obj, pos, &prism);

   bins = Malloc (OBJ_BINS_SPACE);
   for (sys = 0; sys < mNumRefSystems; sys++)
   {
      num_bins = mRefSystems[sys].bin_compute_func (obj, &prism, bins);
      mRefSystems[sys].bin_update_func (obj, sys, bins, num_bins);
   }

   Free (bins);

   return TRUE;
}


/*
Local Variables:
typedefs:("BOOL" "BinCompareFunc" "BinComputeFunc" "BinPrintFunc" "BinUpdateFunc" "BoundingPrism" "BoundingPrismFunc" "Obj" "ObjID" "ObjLoc" "ObjRef" "ObjRefChain" "ObjRefFromBinFunc" "ObjRefHashFunc" "ObjRefID" "ObjRefSystem" "Phylum" "RefListClearFunc" "RefListFunc" "RefSystemID" "fix" "pointing" "uchar")
End:
*/
