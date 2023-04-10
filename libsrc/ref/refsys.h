/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* refsys.h
**
** ObjRefs, ObjRefSystems
**
** $Header: r:/t2repos/thief2/libsrc/ref/refsys.h,v 1.1 1998/03/14 22:05:08 mahk Exp $
 * 
*/

#ifndef _REFSYS_H
#define _REFSYS_H

typedef int ObjID; 
typedef uint ObjRefID; 
typedef uint RefSystemID; 

// Locations in the world are called `bins.'  For example, in Terra
// Nova, each 6m-by-6m square of the world is a bin.  We get to
// objects from the world by looking up what objects extend into a
// particular bin.  Since any one object can extend into more than one
// bin at once, we need an extra data structure, called an `ObjRef',
// to represent the presence of a given object in a given bin.

typedef struct ObjRef
{
   RefSystemID  refsys;         // What RefSystem I belong to
   ObjID        obj;            // What object I refer to
   ObjRefID     next_in_bin;    // The next ObjRef in this bin (non-circular)
   ObjRefID     next_of_obj;    // The next ObjRef to refer to my obj (circular)
   char         bin[1];            // What bin am I in?  Length depends on refsys
} ObjRef;

// If the following diagram doesn't make the pointer situation
// clear, just ignore it.  One thing that is not made explicit
// here is that each Obj also has a pointer back to one of its
// ObjRefs.
//
//                          +-----+
//           +------------->|obj 1|<-------------------+
//           |              +-----+                    |
//    ObjRef |                                         |
//        +-----+     +-----+           +-----+     +-----+
// ------>| obj |     | obj |    ------>| obj |     | obj |
// bin1   +-----+     +-----+    bin2   +-----+     +-----+
//        | nib |---->| nib |->X        | nib |---->| nib |->X
//        +-----+     +-----+           +-----+     +-----+
//        | noo |     | noo |<--------->| noo |     | noo |
//        +-----+     +-----+           +-----+     +-----+
//           ^           |                 |           ^
//           |           |                 |           |
//           +---------- | --------------- | ----------+
//                       |                 |
//                       |  +-----+        |
//                       +->|obj 2|<-------+
//                          +-----+
//
// The arrows labeled `bin1' and `bin2' signify, as you might expect,
// that the bin structure has a pointer to the `ref list' in it.  In
// reality, we use a function to map from a bin to the head of the
// ref list.
//
// The problem with this system is that the number of bins in the
// world might be prohibitively high (in Terra Nova, there are 256K of
// them).  However, in such a situation, it is unlikely that more than
// a small fraction of these bins will contain refs.  Our solution is
// to create a hash function which maps a bin to a chain of ref lists.
// We'd like to map to a ref list directly, but our hash function is
// likely to have collisions.  By using a chain, we can have multiple
// bins hash to the same value and not die horribly.
//
// The ---///---> arrows in the following diagram mean that the pointer
// is generated through a function, rather than existing already.  The
// ---|||---> arrows stand for a correspondence rather than a direct
// pointer; the indentifying information for the bin may be stored in
// the ObjRef directly.
//
// +-----+                +------+
// |bin 1|---///--------->| next |-->X
// +-----+                +------+
//    ^                   | refl |
//    |                   +------+
//    |                      |
//    |                      V
//    |                   +------+
//    +-------|||---------| bin  |
//                        +------+
// +-----+
// |bin 2|---///---+
// +-----+         |  ObjRefChain
//    ^            |      +------+       +------+
//    +----\       +----->| next |------>| next |-->X
//          \      |      +------+       +------+
// +-----+   \     |      | refl |       | refl |
// |bin 3|---///---+      +------+       +------+
// +-----+     \             |              |
//    ^         \     ObjRef V              V
//    |          \        +------+       +------+
//    |           \-------| bin  |       | bin  |
//    |                   +------+       +------+
//    |                                     |
//    +--------|||--------------------------+
//
// What does this mean to the engine object system?  Actually, not a
// lot, because we're making the app do all the work.  (In the future,
// I will probably provide a library to do a lot of the hashing work
// automatically.)
//
// We just need two functions, which can be provided under both of the
// above systems:
//
// 1. Given a bin, give me a pointer to the memory that holds the
//    ObjRefID at the beginning of that bin.  We need a pointer so
//    that we can change it.  If bins are hashed, it is possible that
//    there is no such pointer (if there currently is no ref list
//    associated with that bin).  We pass in a bool to specify
//    whether, in this case, a null ref list should be created, since
//    presumably we're about to add ot it.  If this argument is FALSE,
//    then we assume the bin exists already; if it doesn't, NULL
//    should be returned.

// ObjRefID *FUNC (void *bin, BOOL create) {}
typedef ObjRefID * (* RefListFunc) (void *bin, BOOL create);

// 2. Clear out a given bin.  If bins are hashed, this probably
//    involves some work to clear out the hash table.  It also will
//    involve duplicating some work that you probably just did in a
//    RefListFunc, since chances are that we called that function just
//    before we called this one.  So I recommend that during
//    RefListFunc, you memoize some information to make it easy to
//    find that bin again.

// void FUNC (void *bin) {}
typedef void (* RefListClearFunc) (void *bin);


// A bin is a black box to us, since we don't know anything a priori
// about the world representation.  It seems like it would be handy
// to have a function to compare two bins, although this may turn out
// not to be necessary.

// int FUNC (void *b1, void *b2) {}
typedef int (* BinCompareFunc) (void *b1, void *b2);


// For debugging, it's nice to have a function to print out a
// string representation of a bin.  This can be NULL (e.g., in
// a shipping version).

// void FUNC (void *bin, char *str, int maxstrlen) {}
typedef void (* BinPrintFunc) (void *bin, char *str, int maxstrlen);


// Although there may be many RefSystems, there should be one basic
// Cartesian coordinate system that they all understand.  This is the
// coordinate system used for locations in the objects, and for
// physics.
//
// Each RefSystem has a function that enumerates the bins into which a
// given object extends.  Since most of these functions (we imagine)
// would benefit from knowing the bounding prism around the object, we
// move that work out into a separate, global function.

// The 'pos' that is passed to your function is a pointer to a
// "Position" as currently defined in wrtype.h.  We just call it a
// void * here because we don't know anything about the world
// representation.

// This is an aligned prism; all edges on it are parallel to one of
// the x, y, or z axes.
typedef struct
{
   float xmin, xmax;
   float ymin, ymax;
   float zmin, zmax;
}
BoundingPrism;

// puts output into bound
// void FUNC (ObjID obj, void *pos, BoundingPrism *bound) {}
typedef void (* BoundingPrismFunc) (ObjID obj, void *pos, BoundingPrism *bound);

// Get the position of an object 
typedef void* (* ObjPosFunc)(ObjID obj); 


// Each RefSystem needs a function that, with the aid of the bounding
// prism, computes the bins into which an object extends.  We don't
// know at compile time how big the bins are, so we just give the
// function a chunk of memory to write its array to.

// Returns number of bins
// int FUNC (ObjID obj,
//           BoundingPrism *bound,
//           void *bins);          // output
typedef int (* BinComputeFunc) (ObjID obj, BoundingPrism *bound,
                                void *bins);          // output


// One thing we need to do often is change the bins an object extends
// into.  While we could theoretically do this with generic code that
// utilized the above functions, it would be really slow; we'd have to
// be calling a BinCompareFunc all the time.  Instead we make the app
// provide it, but we do provide a template for writing it; see
// objupd.h.

// void FUNC (ObjID obj, int refsys, void *binmem, int num_bins);

typedef void (* BinUpdateFunc) (ObjID obj, int refsys,
                                void *binmem, int num_bins);



// We may want multiple notions of bins in a single game.  For
// example, the optimal bin size may differ between physics and the
// renderer.  In Terra Nova, physics and renderer used the same bins,
// but two additional types of bins were used for rendering objects in
// the strip region.
//
// We need to package everything up about a given bin/ref system so that
// we can instantiate multiple instances of them.  We call this package
// an ObjRefSystem.

typedef struct ObjRefSystem
{
   char              ref_size;            // size in bytes of an ObjRef
   char              bin_size;            // size in bytes of a bin
   RefListFunc       ref_list_func;       // function to provide head of ref list
   RefListClearFunc  ref_list_clear_func; // function to clear ref list
   BinCompareFunc    bin_compare_func;    // function for comparing bins
   BinUpdateFunc     bin_update_func;     // function for updating extent
   BinPrintFunc      bin_print_func;      // function to print a bin (NULL is ok)
   BinComputeFunc    bin_compute_func;    // compute bins for object
} ObjRefSystem;

// ObjRefs are referred to by ObjRefID.  The actual ObjRefs are
// scattered throughout memory, so there are accessing macros to get
// to the actual ObjRef.  You shouldn't keep an ObjRef around past a
// frame, but I doubt anyone will want to anyway.

typedef struct
{
   // Note that ObjRefInfos have no active field.
   union
   {
      ObjRef   *ref;            // What ObjRef this ID refers to
      ObjRefID  next;           // If unused, the next free ObjRefInfo.
   };
}
ObjRefInfo;

#define OBJREFID_TO_PTR(id) (ObjRefInfos[id].ref)
#define OBJ_FIRST_REF(obj,refsys)  (gFirstRefs[refsys][obj])


// Variables defined in refsys.c

EXTERN int               mNumRefSystems;     // The number of RefSystems that exist
                                             // space is allocated for
EXTERN ObjRefSystem      *mRefSystems;       // Pointer to the array of RefSystems
EXTERN BoundingPrismFunc gBoundingPrismFunc; // How to compute a bounding prism
EXTERN ObjPosFunc        gObjPosFunc;
EXTERN ObjRefID          mRefFirstFree;      // The first RefInfo in the free chain
EXTERN ObjRefID**        gFirstRefs;          // 
EXTERN int               gMaxRefs;           // maximum number of refs allowed
EXTERN ObjRefInfo*       ObjRefInfos;      // ref information, gMaxRefs big

// Returns the ID of the RefSystem you've made
EXTERN RefSystemID ObjRefSystemRegister (char              bin_size,
                                  RefListFunc       ref_list_func,
                                  RefListClearFunc  ref_list_clear_func,
                                  BinCompareFunc    bin_compare_func,
                                  BinUpdateFunc     bin_update_func,
                                  BinPrintFunc      bin_print_func,
                                  BinComputeFunc    bin_compute_func);

// Initialize the ObjRefSystem system, so that it can hold
// num RefSystems by default.
EXTERN BOOL ObjRefSystemInit (int max_objs,
                              int max_refs, 
                              int num_systems,
                              BoundingPrismFunc bounding_prism_func, ObjPosFunc obj_pos_func);

EXTERN BOOL ObjRefSystemTerm(void);

EXTERN void     ObjsInitRefs (void);
EXTERN ObjRefID ObjRefGrab (RefSystemID refsys);
EXTERN void     ObjRefReturnToStorage (ObjRefID ref);
EXTERN BOOL     ObjLinkMake (ObjID obj, ObjRefID ref);
EXTERN BOOL     ObjLinkDel (ObjRefID ref);
EXTERN BOOL     ObjRefAdd (ObjRefID ref, void *bin);
EXTERN void     ObjRefRem (ObjRefID ref);
EXTERN ObjRefID ObjRefMake (ObjID obj, RefSystemID refsys, void *bin);
EXTERN BOOL     ObjRefDel (ObjRefID ref);
EXTERN BOOL     ObjDelRefsOfSystem (ObjID obj, RefSystemID refsys);
EXTERN BOOL     ObjDelRefs (ObjID obj);
EXTERN BOOL     ObjUpdateLocs (ObjID obj);

#endif

/*
Local Variables:
typedefs:("BOOL" "BinCompareFunc" "BinComputeFunc" "BinPrintFunc" "BinUpdateFunc" "BoundingPrism" "BoundingPrismFunc" "ObjID" "ObjLoc" "ObjRef" "ObjRefChain" "ObjRefFromBinFunc" "ObjRefHashFunc" "ObjRefID" "ObjRefInfo" "ObjRefSystem" "RefListClearFunc" "RefListFunc" "RefSystemID" "fix" "uchar" "ushort")
End:
*/
