//      RES_.H  Resource System internal header file
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/res_.h 1.22 1997/07/15 21:29:42 TOML Exp $
* $Log: res_.h $
 * Revision 1.22  1997/07/15  21:29:42  TOML
 * *** empty log message ***
 * 
 * Revision 1.21  1997/05/17  17:44:24  JAEMZ
 * Added ResDoTypeCallbacks
 *
 * Revision 1.20  1997/02/24  23:49:18  TOML
 * New resource cache logic
 *
 * Revision 1.19  1997/02/12  00:30:48  TOML
 * partial lock support
 *
 * Revision 1.18  1997/01/20  14:24:21  TOML
 * removed memreqs to lg.lib
 *
 * Revision 1.15  1996/11/21  17:12:52  mahk
 * Added ResInitInternal
 *
 * Revision 1.14  1996/11/19  17:10:15  TONY
 * Added ResViewCache().
 *
 * Revision 1.13  1996/11/19  17:05:37  TONY
 * Added ResValidateList().
 *
 * Revision 1.12  1996/10/23  11:30:30  TOML
 * *** empty log message ***
 *
 * Revision 1.11  1996/10/22  14:20:20  TOML
 * Removed ARQ dependence from resacc
 *
 * Revision 1.10  1996/10/10  16:13:21  TOML
 * msvc port
 *
 * Revision 1.9  1996/09/15  18:21:12  TOML
 * Added stronger debugging code
 *
 * Revision 1.8  1996/09/14  16:10:02  TOML
 * Prepared for revision
 *
 * Revision 1.7  1996/09/14  14:12:51  TOML
 * Made C++ parser friendly
 *
 * Revision 1.6  1996/08/14  15:57:46  PATMAC
 * Added ResRetrievePartial
 *
 * Revision 1.5  1995/01/13  12:08:23  jak
 * moved ResDefaultPager() proto to res.h,
 * making it public
 *
 * Revision 1.4  1994/11/30  20:39:39  xemu
 * cd spoof drive structs
 *
 * Revision 1.3  1994/09/22  10:47:01  rex
 * Got rid of offset alignment stuff (offset field no longer needs shifting).
 *
 * Revision 1.2  1994/05/26  13:54:52  rex
 * Added stuff for installable & default pager
 *
 * Revision 1.1  1994/02/17  11:22:56  rex
 * Initial revision
 *
*/

#ifndef __RES__H
#define __RES__H

#ifndef __RES_H
#include <res.h>
#endif
#ifndef ___RES_H
#include <_res.h>
#endif


// ----------------------------------------------------------
// The Watcom C++ parser is verbose in warning of possible integral
// truncation, even in cases where for the given native word size
// there is no problem (i.e., assigning longs to  ints).  Here, we
// quiet the warnings, although it wouldn't be bad for someone to
// evaluate them. (toml 09-14-96)
//
#ifdef __WATCOMC__
#pragma warning 389 9
#endif

#ifdef __cplusplus
extern "C"
{
#endif

//  ----------------------------------------------------------
//      FOR RESOURCE SYSTEM INTERNAL USE - DON'T BE BAD!
//  ----------------------------------------------------------

BOOL DoResDrop(Id id);

//  New validation
#ifndef SHIP

// Watcom debugger has trouble with these, so they are off by default
__declspec(dllimport) BOOL __stdcall IsBadReadPtr(const void *lp, unsigned ucb);
__declspec(dllimport) BOOL __stdcall IsBadWritePtr(void *lp, unsigned ucb);

#ifdef VALIDATE_POINTERS
#define ResIsBadReadPtr(p, s) IsBadReadPtr(p, s)
#define ResIsBadWritePtr(p, s) IsBadWritePtr(p, s)
#else
#define ResIsBadReadPtr(p, s) FALSE
#define ResIsBadWritePtr(p, s) FALSE
#endif

#define ValidateRes(id) \
    do \
        { \
        AssertMsg1((id) >= ID_MIN, "Invalid resource id 0x%x", (id)); \
        AssertMsg1((id) <= resDescMax, "Invalid or out of range resource id 0x%x", (id)); \
        if ((id) >= ID_MIN && (id) <= resDescMax) \
            { \
            AssertMsg1(!ResPtr((id)) ||  \
                      (!ResIsBadReadPtr(ResPtr((id)), ResSize((id))) &&  \
                       !ResIsBadWritePtr(ResPtr((id)), ResSize((id)))),  \
                      "Data associated with resource is invalid (0x%x)", (id)); \
            } \
        } \
    while (0)

#define ValidateRef(ref) \
    do \
        { \
        Id __RefValidation_Id = REFID((ref)); \
        ResDesc * __RefValidation_prd; \
        RefTable * __RefValidation_prt; \
        ValidateRes(__RefValidation_Id); \
        AssertMsg1(ResFlags(__RefValidation_Id) & RDF_COMPOUND, "Expected resource 0x%x to be compound", __RefValidation_Id); \
        __RefValidation_prd = RESDESC(REFID(ref)); \
        Assert_(__RefValidation_prd); \
        __RefValidation_prt = (RefTable *) __RefValidation_prd->ptr; \
        AssertMsg1(!__RefValidation_prt ||  \
                  (!ResIsBadReadPtr(__RefValidation_prt, ResSize(__RefValidation_Id)) &&  \
                   !ResIsBadWritePtr(__RefValidation_prt, ResSize(__RefValidation_Id))),  \
                  "Data associated with resource is invalid (0x%x)", __RefValidation_Id); \
        if (__RefValidation_prt) \
            AssertMsg1(RefIndexValid(__RefValidation_prt, REFINDEX(ref)), "Bad index for reference 0x%x", ref); \
        } \
    while (0)

#else
#define ValidateRes(id)
#define ValidateRef(ref)
#endif

//  Memory management
   void ResMemInit();
   void ResMemTerm();

//  Memory limits management

   ulong ResPickAllocCap();

//  Checking id's and ref's (resacc.c and refacc.c)

   bool ResCheckId(Id id);                       // returns TRUE if id ok, else FALSE + warns
   bool RefCheckRef(Ref ref);                    // returns TRUE if ref ok, else FALSE & warns

//  Resource loading (resload.c)

   enum eResLoadFlags
   {
      kResLoadPartialRef = 0x01
   };

#ifdef __cplusplus
   void *ResLoadResource(Id id, unsigned flags = 0, unsigned partialRefIndex = 0);
#endif
   bool ResRetrieve(Id id, void *buffer);
// retrieve part of a resource (must not be compressed or compound)
   bool ResRetrievePartial(Id id, void *buffer, uint32 offset, uint32 nBytes);

//  Resource paging (resmem.c)
   extern BOOL g_fPagingEnabled;

//  ARQ Nofication function
   typedef void (LGAPI * tResARQClearPreloadFunc)(Id id);
   extern tResARQClearPreloadFunc g_pfnResARQClearPreloadFunc;

   // Internal init (res.cpp)

   extern void ResInitInternal(void);

//  Grow descriptor table (res.c)

   void ResGrowResDescTable(Id id);

#define ResExtendDesc(id) {if ((id) > resDescMax) ResGrowResDescTable(id);}

#define DEFAULT_RES_GROWDIRENTRIES 128      // must be power of 2

//  Data alignment aids

#define RES_OFFSET_ALIGN(offset) (((offset)+3)&0xFFFFFFFCL)
#define RES_OFFSET_PADBYTES(size) ((4-(size))&3)

#define RES_OFFSET_REAL2DESC(offset) (offset)
#define RES_OFFSET_DESC2REAL(offset) (offset)

//#define RES_OFFSET_REAL2DESC(offset) ((offset)>>2)
//#define RES_OFFSET_DESC2REAL(offset) ((offset)<<2)

#define RES_OFFSET_PENDING 1    // offset of resource not yet written

//  LRU chain link management

void ResCacheRemove(Id id);
void ResCacheAdd(Id id);
void ResCacheTouch(Id id);

// Internal call to do type installed load callbacks
void ResDoTypeCallbacks(Id id);

//  Statistics tables

#ifdef DBG_ON

   typedef struct
   {
      ulong numGets;                             // # ResGet()'s or RefGet()'s
      ulong numLocks;                            // # ResLock()'s or RefLock()'s
      ushort numExtracts;                        // # ResExtract()'s or RefExtract()'s
      ushort numLoads;                           // # ResLoad()'s
      ushort numOverwrites;                      // # times resource overwritten by one in new file
      ushort numPageouts;                        // # times paged out of ram
   } ResCumStat;

   extern ResCumStat *pCumStatId;                // ptr to cumulative stats by id
   extern ResCumStat cumStatType[NUM_RESTYPENAMES];     // table of cum. stats by type

#define CUMSTATS(id,field) DBG(DSRC_RES_CumStat, { \
    ResCumStat *prcs;               \
    if (pCumStatId == NULL)     \
        ResAllocCumStatTable(); \
    prcs = pCumStatId + (id);   \
    prcs->field++;                  \
    prcs = &cumStatType[RESDESC2(id)->type]; \
    prcs->field++;                  \
    })

   typedef struct
   {
      ushort numPageouts;                        // # times ResPageOut() called
      ulong totSizeNeeded;                       // total # bytes asked for across calls
      ulong totSizeGotten;                       // total # bytes paged out across calls
   } ResPageStats;

   extern ResPageStats resPageStats;             // paging statistics

   void ResAllocCumStatTable();                  // internal stat routine prototypes
   void ResSpewCumStats();                       // these are in rescum.c

#else

#define CUMSTATS(id,field)

#endif

   typedef struct
   {
      char *path;
      void (*callback) (int size, Id id);
   } ResSpoofDrive;

   extern ResSpoofDrive gSpoofDrive;

   extern void ResValidateList();
   extern long ResViewCache(bool bOnlyLocks);

#ifdef __cplusplus
};
#endif

#endif
