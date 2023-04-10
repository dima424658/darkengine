//      Res.H       Resource Manager header file
//      Rex E. Bradford (REX)
//
// $Header: x:/prj/tech/libsrc/res/RCS/res.h 1.43 1999/04/12 14:08:24 BFarquha Exp $
//

#ifndef __RES_H
#define __RES_H

#include <lg.h>
#include <comtools.h>
#include <prikind.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef DATAPATH_H
#include <datapath.h>
#endif
#ifndef __RESTYPES_H
#include <restypes.h>
#endif

//  ---------------------------------------------------------
//      ID AND REF DEFINITIONS AND MACROS
//  ---------------------------------------------------------

//  Id's refer to resources, Ref's refer to items in compound resources

typedef ushort Id;                               // ID of a resource
typedef ulong Ref;                               // high word is ID, low word is index
typedef ushort RefIndex;                         // index part of ref

// This is a type installed callback on load time
typedef void (*fResCallback)(uchar *bits,int size);

typedef void (tResFreeCallback)(void *ptr, ulong flags, Id id);

//  Here's how you get parts of a ref, or make a ref

#define REFID(ref)      ((Id)((ref)>>16))         // get id from ref
#define REFINDEX(ref)   ((ref)&0xFFFF)            // get index from ref
#define MKREF(id,index) ((((long)id)<<16)|(index))// make ref

#define ID_NULL 0           // null resource id
#define ID_HEAD 1           // holds head ptr for LRU chain
#define ID_TAIL 2           // holds tail ptr for LRU chain
#define ID_MIN 3            // id's from 3 and up are valid

//  ---------------------------------------------------------
//      ACCESS TO RESOURCES (ID'S)  (resacc.c)
//  ---------------------------------------------------------

void * ResLock(Id id);                           // lock resource & get ptr
void * ResLockSetFlags(Id id, ulong flags);      // lock resource; get ptr; set flags
void   ResUnlock(Id id);                         // unlock resource
void * ResGet(Id id);                            // get ptr to resource (dangerous!)
void * ResExtract(Id id, void *buffer);          // extract resource into buffer
void   ResDrop(Id id);                           // drop resource from immediate use
void   ResDelete(Id id);                         // delete resource forever

// extract part of (uncompressed, non-compound) resource into buffer
void * ResExtractPartial(Id id, void *buffer, uint32 offset, uint32 nBytes);

#define ResLockString(id) ((const char *)ResLock(id))
#define ResGetString(id)  ((const char *)ResGet(id))

void ResSetWarnOnLoad(BOOL);                     // use to track non-preloaded resources in time/look-critical code

// This how you install a callback on a specific type
void ResInstallTypeCallback(int type,fResCallback cback);

void ResSetFreeCallback(tResFreeCallback *);

//  ------------------------------------------------------------
//      ACCESS TO ITEMS IN COMPOUND RESOURCES (REF'S)  (refacc.c)
//  ------------------------------------------------------------

//  Each compound resource starts with a Ref Table

typedef struct
{
   RefIndex numRefs;                             // # items in compound resource
   long     offset[1];                           // offset to each item (numRefs + 1 of them)
} RefTable;

void *  RefLock(Ref ref);                        // lock compound res, get ptr to item
void *  RefLockExact(Ref ref);
#define RefUnlock(ref) ResUnlock(REFID(ref))     // unlock compound res item
void *  RefGet(Ref ref);                         // get ptr to item in comp. res (dangerous!)

RefTable * ResReadRefTable(Id id);               // alloc & read ref table
#define    ResFreeRefTable(prt) (Free(prt))      // free ref table
int        ResExtractRefTable(Id id, RefTable *prt, long size); // extract reftable
void *     RefExtract(RefTable *prt, Ref ref, void *buff); // extract ref
void *     RefExtractPartial(RefTable *prt, Ref ref, void *buff, long size);
#define    RefIndexValid(prt,index) ((index) < (prt)->numRefs)
#define    RefSize(prt,index) (prt->offset[(index)+1]-prt->offset[index])

int        ResNumRefs(Id id);                           // returns the number of refs in a resource, extracting if necessary.

#define    REFTABLESIZE(numrefs) (sizeof(RefIndex) + (((numrefs)+1) * sizeof(long)))
#define    REFPTR(prt,index) (((uchar *) prt) + prt->offset[index])

#define    RefLockString(ref) ((const char *)RefLock(ref))
#define    RefGetString(ref)  ((const char *)RefGet(ref))

//  -----------------------------------------------------------
//      BLOCK-AT-A-TIME ACCESS TO RESOURCES  (resexblk.c)
//  -----------------------------------------------------------

void ResExtractInBlocks(Id id, void *buff, long blockSize,
                        long (*f_ProcBlock) (void *buff, long numBytes, long iblock));
void RefExtractInBlocks(RefTable * prt, Ref ref, void *buff, long blockSize,
                        long (*f_ProcBlock) (void *buff, long numBytes, long iblock));
#define REBF_FIRST 0x01     // set for 1st block passed to f_ProcBlock
#define REBF_LAST  0x02     // set for last block (may also be first!)

//  -----------------------------------------------------------
//      IN-MEMORY RESOURCE DESCRIPTORS, AND INFORMATION ROUTINES
//  -----------------------------------------------------------

//  Each resource id gets one of these resource descriptors

typedef struct
{
   void *ptr;                                    // ptr to resource in memory, or NULL if on disk
   ulong lock: 8;                                // lock count
   ulong size: 24;                               // size of resource in bytes (1 Mb max)
   ulong offset;                                 // offset in file
#if 0
   Id next;                                      // next resource in LRU order
   Id prev;                                      // previous resource in LRU order
#else
   ushort filenum;
   ushort flags;
#endif
} ResDesc;

#define RF_USEFREECALLBACK 0x0001                // flag to indicate Free callback should be
                                                 // used when resource is dropped

typedef struct
{
   ushort flags: 8;                              // misc flags (RDF_XXX, see below)
   ushort type: 8;                               // resource type (RTYPE_XXX, see restypes.h)
} ResDesc2;

#define RESDESC(id) (&gResDesc[id])              // convert id to resource desc ptr
#define RESDESC_ID(prd) ((prd)-gResDesc)         // convert resdesc ptr to id

#define RESDESC2(id) (&gResDesc2[id])            // convert id to rd2 ptr
#define RESDESC2_ID(prd) ((prd)-gResDesc2)       // convert rd2 ptr to id

#define RDF_LZW         0x01                     // if 1, LZW compressed
#define RDF_COMPOUND    0x02                     // if 1, compound resource
#define RDF_VIRTALLOC   0x04                     // if 1, allocated fromn virtual memory
#define RDF_LOADONOPEN  0x08                     // if 1, load block when open file
#define RDF_CDSPOOF     0x10                     // is this resource on a virtual CD rom drive?
#define RDF_PKZIP       0x20                     // if 1, PKZIP compressed
#define RDF_PARTIAL     0x40                     // if 1, only partially there
#define RDF_UNUSED1     0x80

#define RES_MAXLOCK 255             // max locks on a resource

extern ResDesc *  gResDesc;                      // ptr to big array of ResDesc's
extern ResDesc2 * gResDesc2;                     // ptr to array of ResDesc2 (shared buff with resdesc)

//  Information about resources

#define ResInUse(id) (gResDesc[id].offset)
#define ResPtr(id) (gResDesc[id].ptr)
#define ResSize(id) (gResDesc[id].size)
#define ResExists(id) (gResDesc[id].size != 0)
#define ResLocked(id) (gResDesc[id].lock)
#define ResFilenum(id) (gResDesc[id].filenum)
#define ResType(id) (gResDesc2[id].type)
#define ResFlags(id) (gResDesc2[id].flags)
#define ResCompressed(id) (gResDesc2[id].flags & RDF_LZW)
#define ResZipped(id) (gResDesc2[id].flags & RDF_PKZIP)
#define ResIsCompound(id) (gResDesc2[id].flags & RDF_COMPOUND)

//  ------------------------------------------------------------
//      RESOURCE MANAGER GENERAL ROUTINES  (res.c)
//  ------------------------------------------------------------

void ResInit();                                  // init Res, allocate initial ResDesc[]
void ResTerm();                                  // term Res (done auto via atexit)

EXTERN void LGAPI ResARQInit();
EXTERN void LGAPI ResARQTerm();

#define ResSharedCacheCreate() \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _ResSharedCacheCreate(IID_TO_REFIID(IID_ISharedCache), NULL, pAppUnknown); \
    COMRelease(pAppUnknown); \
}

F_DECLARE_INTERFACE(ISharedCache);
DEFINE_LG_GUID(GUID_ResCache, 0x8b);

EXTERN HRESULT LGAPI _ResSharedCacheCreate(REFIID, ISharedCache ** ppSharedCache, IUnknown * pOuter);

//  ------------------------------------------------------------
//      RESOURCE FILE ACCESS (resfile.c)
//  ------------------------------------------------------------

typedef enum
{
   ROM_READ,                                     // open for reading only
   ROM_EDIT,                                     // open for editing (r/w) only
   ROM_EDITCREATE,                               // open for editing, create if not found
   ROM_CREATE                                    // open for creation (deletes existing)
} ResOpenMode;

void ResAddPath(char *path);                     // add search path for resfiles
int  ResOpenResFile(char *fname, ResOpenMode mode, bool auxinfo);        // openfile
void ResCloseFile(int filenum);                  // close res file
void ResSetCDSpoof(char *path, void (*spoof_cb) (int size, Id id));
#define ResOpenFile(fname) ResOpenResFile(fname, ROM_READ, FALSE)
#define ResEditFile(fname,creat) ResOpenResFile(fname, \
    (creat) ? ROM_EDITCREATE : ROM_EDIT, TRUE)
#define ResCreateFile(fname) ResOpenResFile(fname, ROM_CREATE, TRUE)

#define MAX_RESFILENUM 40           // maximum file number

extern Datapath gDatapath;                       // res system's datapath (others may use)

//  ---------------------------------------------------------
//      RESOURCE MEMORY MANAGMENT ROUTINES  (resmem.c)
//  ---------------------------------------------------------

void * ResMalloc(size_t size);
void * ResRealloc(void *p, size_t newsize);
void   ResFree(void *p);
size_t ResMSize(void *p);
void * ResPage(long size);
void   ResCompact(void);
void   ResEnablePaging(BOOL);
size_t ResMsize(void * p);
#ifndef SHIP
BOOL   ResVerifyAlloc(void * p);
void   ResVerifyAllAllocs();
#else
#define ResVerifyAlloc(p) (TRUE)
#define ResVerifyAllAllocs()
#endif

void * ResDefaultPager(long size);
void   ResInstallPager(void *f(long size));

extern ulong g_ResAllocCap;
extern ulong g_ResInitAllocCap;
extern ulong g_ResTotalAlloc;

#define ResCountAlloc(sz)   g_ResTotalAlloc+=(sz)
#define ResCountFree(sz)    g_ResTotalAlloc-=(sz)
#define ResGetTotalAlloc()  ((const ulong)g_ResTotalAlloc)
#define ResSetAllocCap(cap) g_ResAllocCap=(cap)
#define ResResetAllocCap()  g_ResAllocCap=g_ResInitAllocCap
#define ResGetAllocCap()    ((const ulong)g_ResAllocCap)

typedef struct sResMemStats
{
    ulong reserved;

    // From Windows:
    ulong memoryLoad;    // percent of memory in use
    ulong totalPhys;     // bytes of physical memory
    ulong availPhys;     // free physical memory bytes
    ulong totalPageFile; // bytes of paging file
    ulong availPageFile; // free bytes of paging file
    ulong totalVirtual;  // user bytes of address space
    ulong availVirtual;  // free user bytes

    // From Resource system
    ulong allocCap;
    ulong totalMalloc;
    ulong lockedMalloc;
    ulong cachedMalloc;

} sResMemStats;

void ResGetMemStats(sResMemStats *);

void ResCheckCache(int maxLocks);

//  ---------------------------------------------------------
//      RESOURCE STATS - ACCESSIBLE AT ANY TIME
//  ---------------------------------------------------------

typedef struct
{
   ushort numLoaded;                             // # resources loaded in ram
   ushort numLocked;                             // # resources locked
   long totMemAlloc;                             // total memory alloted to resources
} ResStat;

extern ResStat resStat;                          // stats computed if proper DBG bit set

//  ----------------------------------------------------------
//      PUBLIC INTERFACE FOR CREATORS OF RESOURCES
//  ----------------------------------------------------------

//  ----------------------------------------------------------
//      RESOURCE MAKING  (resmake.c)
//  ----------------------------------------------------------

void ResMake(Id id, void *ptr, long size, uchar type, int filenum,
              uchar flags);                      // make resource from data block
void ResMakeCompound(Id id, uchar type, int filenum,
                      uchar flags);              // make empty compound resource
void ResAddRef(Ref ref, void *pitem, long itemSize);    // add item to compound
void ResUnmake(Id id);                           // unmake a resource

//  ----------------------------------------------------------
//      RESOURCE FILE LAYOUT
//  ----------------------------------------------------------

//  Resource-file disk format:  header, data, dir

typedef struct
{
   char  signature[16];                          // "LG ResFile v2.0\n",
   char  comment[96];                            // user comment, terminated with '\z'
   uchar reserved[12];                           // reserved for future use, must be 0
   long  dirOffset;                              // file offset of directory
} ResFileHeader;                                 // total 128 bytes (why not?)

typedef struct
{
   ushort numEntries;                            // # items referred to by directory
   long   dataOffset;                            // file offset at which data resides
                                                 // directory entries follow immediately
                                                 // (numEntries of them)
} ResDirHeader;

typedef struct
{
   Id    id;                                     // resource id (if 0, entry is deleted)
   ulong size: 24;                               // uncompressed size (size in ram)
   ulong flags: 8;                               // resource flags (RDF_XXX)
   ulong csize: 24;                              // compressed size (size on disk)
                                                 // (this size is valid disk size even if not comp.)
   long type: 8;                                 // resource type
} ResDirEntry;

//  Active resource file table

typedef struct
{
   ushort         flags;                         // RFF_XXX
   ResFileHeader  hdr;                           // file header
   ResDirHeader * pdir;                          // ptr to resource directory
   ushort         numAllocDir;                   // # dir entries allocated
   long           currDataOffset;                // current data offset in file
} ResEditInfo;

typedef struct
{
   int           fd;                             // file descriptor (from open())
   ResEditInfo * pedit;                          // editing info, or NULL if read-only file
} ResFile;
#define RFF_NEEDSPACK   0x0001                   // resfile has holes, needs packing
#define RFF_AUTOPACK        0x0002               // resfile auto-packs (default TRUE)

extern ResFile resFile[MAX_RESFILENUM + 1];
//  Macros to get ptr to resfile's directory, & iterate across entries

#define RESFILE_HASDIR(filenum) (resFile[filenum].pedit)
#define RESFILE_DIRPTR(filenum) (resFile[filenum].pedit->pdir)
#define RESFILE_DIRENTRY(pdir,n) ((ResDirEntry *)((pdir) + 1) + (n))
#define RESFILE_FORALLINDIR(pdir,pde) for (pde = RESFILE_DIRENTRY(pdir,0); \
    pde < RESFILE_DIRENTRY(pdir,pdir->numEntries); pde++)

extern char resFileSignature[16];                // magic header
extern Id   resDescMax;                          // max id in res desc

//  --------------------------------------------------------
//      RESOURCE FILE BUILDING  (resbuild.c)
//  --------------------------------------------------------

void ResSetComment(int filenum, char *comment);  // set comment
int  ResWrite(Id id);                            // write resource to file
void ResKill(Id id);                             // delete resource & remove from file
long ResPack(int filenum);                       // remove empty entries

#define ResAutoPackOn(filenum) (resFile[filenum].pedit->flags |= RFF_AUTOPACK)
#define ResAutoPackOff(filenum) (resFile[filenum].pedit->flags &= ~RFF_AUTOPACK)
#define ResNeedsPacking(filenum) (resFile[filenum].pedit->flags & RFF_NEEDSPACK)

//---------------------------------------------------------
// the psuedo memory limiter thing
//---------------------------------------------------------

ulong ResMemSetCap(ulong cap);

//---------------------------------------------------------
// ARQ Helper Functions
//---------------------------------------------------------

typedef long HRESULT;

EXTERN BOOL    LGAPI ResAsyncLock(Id id, int priority);
EXTERN BOOL    LGAPI ResAsyncExtract(Id id, int priority, void *buf, long bufSize);
EXTERN BOOL    LGAPI ResAsyncPreload(Id id);
EXTERN BOOL    LGAPI ResAsyncIsFulfilled(Id id);
EXTERN HRESULT LGAPI ResAsyncKill(Id id);
EXTERN HRESULT LGAPI ResAsyncGetResult(Id id, void **);

#ifdef __cplusplus
};
#endif

#endif