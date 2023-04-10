//        ResAcc.c          Resource access
//        Rex E. Bradford
//
// $Header: x:/prj/tech/libsrc/res/RCS/resacc.cpp 1.25 1997/10/06 15:34:54 KEVIN Exp $
//

#include <string.h>

#include <res.h>
#include <resthred.h>
#include <res_.h>

#include <resarq.h>

tResARQClearPreloadFunc g_pfnResARQClearPreloadFunc;


// This is pretty dopey.  Ideally we'd be able to use multiple different 
// callbacks for different resources.  For now, there'e just one callback
// and a flag for whether it should be used.


static tResFreeCallback *m_pResFreeCallback=NULL;

void ResSetFreeCallback(tResFreeCallback *cb)
{
    m_pResFreeCallback = cb;
}


void *DoResLock(Id id);

//  ---------------------------------------------------------
//
//  ResLock() locks a resource and returns ptr.
//
//        id = resource id
//
//  Returns: ptr to locked resource

void *ResLock(Id id)
{
//  Check if valid id

    ValidateRes(id);

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return NULL;
    });

    return DoResLock(id);
}


// Same as ResLock(), but also sets the ResDesc flags field.

void *ResLockSetFlags(Id id, ulong flags)
{
    ResDesc *prd;

//  Check if valid id

    ValidateRes(id);

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return NULL;
    });

    prd = RESDESC(id);
    prd->flags = flags;

    return DoResLock(id);
}

// Do the actual work.  Id is assumed valid.

static void *DoResLock(Id id)
{
    cAutoResThreadLock lock;
    ResDesc *prd;
    ResDesc2 *prd2;

//  Clear preload information from ARQ, if any

    if (g_pfnResARQClearPreloadFunc)
        (*g_pfnResARQClearPreloadFunc)(id);

//  Add to cumulative stats

    CUMSTATS(id, numLocks);

//  If resource not loaded, load it

    prd = RESDESC(id);
    prd2 = RESDESC2(id);

    AssertMsg(prd->lock < RES_MAXLOCK, "Bad resource lock count");

    if (prd->ptr == NULL)
    {
        if (ResLoadResource(id) == NULL)
        {
            return (NULL);
        }
    }
    else if (prd->lock == 0)
        ResCacheRemove(id);

   if (prd2->flags & RDF_PARTIAL)
   {
   //  Load all from disk
       ResRetrieve(id, prd->ptr);
       prd2->flags &= ~RDF_PARTIAL;
   }

//  Tally stats, check for over-lock

    DBG(DSRC_RES_Stat,
    {
        if (prd->lock == 0)
            resStat.numLocked++;
    });

//  Increment lock count, check for overlock

    DBG(DSRC_RES_ChkLock,
    {
        if (prd->lock == RES_MAXLOCK)
            prd->lock--;
    });
    prd->lock++;

    Spew(DSRC_RES_LockAll, ("ResLock: lock: $%x lock count: %d\n", id, prd->lock));

    ValidateRes(id);

//  Return ptr

    return (prd->ptr);
}

//  ---------------------------------------------------------
//
//  ResUnlock() unlocks a resource.
//
//        id = resource id

void ResUnlock(Id id)
{
    ResDesc *prd;
    cAutoResThreadLock lock;

//  Check if valid id

    ValidateRes(id);

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return;
    });

//  Check for under-lock

    prd = RESDESC(id);

    if (prd->lock == 0)
    {
        CriticalMsg1("ResUnlock: id $%x already unlocked", id);
        return;
    }

//  Else decrement lock, if 0 move to tail and tally stats

    prd->lock--;
    Spew(DSRC_RES_LockAll, ("ResUnlock: unlock: $%x lock count: %d\n", id, prd->lock));

    if (prd->lock == 0)
    {
        ResCacheAdd(id);
        DBG(DSRC_RES_Stat,
             {
            resStat.numLocked--;
        });
    }

    ValidateRes(id);
}

//  -------------------------------------------------------------
//
//  ResGet() gets a ptr to a resource
//
//        id = resource id
//
//  Returns: ptr to resource (ptr only guaranteed until next Malloc(),
//                   Lock(), Get(), etc.

void *ResGet(Id id)
{
    ResDesc *prd;
    cAutoResThreadLock lock;

//  Check if valid id

    ValidateRes(id);

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return NULL;
    });

//  Add to cumulative stats

    CUMSTATS(id, numGets);

//  Load resource or move to tail

    prd = RESDESC(id);
    if (prd->ptr == NULL)
    {
        if (ResLoadResource(id) == NULL)
        {
            return (NULL);
        }
        ResCacheAdd(id);
    }
    else if (prd->lock == 0)
        ResCacheTouch(id);

    ValidateRes(id);

//  Return ptr

    return (prd->ptr);
}

//  ---------------------------------------------------------
//
//  ResExtract() extracts a resource from an open resource file.
//
//        id     = id
//        buff = ptr to buffer (use ResSize() to compute needed buffer size)
//
//  Returns: ptr to supplied buffer, or NULL if problem

void *ResExtract(Id id, void *buffer)
{
    cAutoResThreadLock lock;
    ValidateRes(id);

//  Retrieve the data into the buffer, please

    if (ResRetrieve(id, buffer))
    {
        CUMSTATS(id, numExtracts);
        return (buffer);
    }

    ValidateRes(id);

//  If ResRetreive failed, return NULL ptr

    return (NULL);
}

//  ---------------------------------------------------------
//
//  ResExtractPartial() extracts part of a resource from an open resource file.
//
//        id     = id
//        buff = ptr to buffer (use ResSize() to compute needed buffer size)
//      offset = offset in bytes to skip at start of resource
//      nBytes = number of bytes to read into buff
//
// NOTE: should only be used for resources which are not LZW encoded,
//  and which are not compound resources (use RefExtractPartial)
//
//  Returns: ptr to supplied buffer, or NULL if problem

void *ResExtractPartial(Id id, void *buffer, uint32 offset, uint32 nBytes)
{

    cAutoResThreadLock lock;

    ValidateRes(id);

//  Retrieve the data into the buffer, please

    if (ResRetrievePartial(id, buffer, offset, nBytes))
    {
        CUMSTATS(id, numExtracts);
        return (buffer);
    }

    ValidateRes(id);

//  If ResRetrievePartial failed, return NULL ptr

    return (NULL);
}

//  ----------------------------------------------------------
//
//  ResDrop() drops a resource from memory for awhile.
//
//        id = resource id


// Internal version does not manipulate the resource cache
BOOL DoResDrop(Id id)
{
    ResDesc *prd;
    cAutoResThreadLock lock;

//  Check for locked

    ValidateRes(id);

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return FALSE;
    });

    prd = RESDESC(id);
    DBG(DSRC_RES_ChkLock,
    {
        if (prd->lock)            \
                Warning(("DoResDrop: Block $%x is locked, dropping anyway\n", id));
    });

/*****
    DBG(DSRC_RES_ChkLock, {if (prd->flags & RDF_NODROP) \
        Warning(("ResDrop: Block $%x has NODROP flag set, dropping anyway\n", id));});
******/

    if (prd->ptr == NULL)
    {
        Warning(("DoResDrop: Block $%x not in memory, ignoring request\n", id));
        return FALSE;
    }

    Spew(DSRC_RES_DelDrop, ("ResDrop: dropping $%x\n", id));

//  Remove from LRU chain

    if (prd->lock != 0)
    {
        Warning(("DoResDrop: Dropping resource 0x%x that's in use.\n",id));
        prd->lock = 0;
    }

//  Tally stats

    DBG(DSRC_RES_Stat,
    {
        resStat.totMemAlloc -= prd->size;
        resStat.numLoaded--;
        Spew(DSRC_RES_Stat, ("DoResDrop: free %d, total now %d bytes\n",
                                    prd->size, resStat.totMemAlloc));
    });

//  Free memory and set ptr to NULL

    if (prd->ptr != NULL)
    {
        if ((m_pResFreeCallback!=NULL)&&(prd->flags & RF_USEFREECALLBACK)) 
        {
            ResDesc2 *prd2 = RESDESC2(id);
            if (prd2->flags & RDF_COMPOUND) {
                RefTable *prt = (RefTable *)prd->ptr;
                for (int i=0; i<prt->numRefs; ++i)
                    m_pResFreeCallback(REFPTR(prt,i), prd->flags, id);
            } else
                m_pResFreeCallback(prd->ptr, prd->flags, id);
        }
        Free(prd->ptr);
        prd->ptr = NULL;
    }

    ValidateRes(id);
    return TRUE;
}

void ResDrop(Id id)
{
    BOOL locked = (RESDESC(id)->lock != 0);
    if (DoResDrop(id) && (!locked))
        ResCacheRemove(id);
}

//  -------------------------------------------------------
//
//  ResDelete() deletes a resource forever.
//
//        Id = id of resource

void ResDelete(Id id)
{
    ResDesc *prd;
    cAutoResThreadLock lock;

//  If locked, issue warning

    DBG(DSRC_RES_ChkIdRef,
    {
        if (!ResCheckId(id))
            return;
    });

    prd = RESDESC(id);
    DBG(DSRC_RES_ChkLock,
    {
        if (prd->lock)            \
                Warning(("ResDelete: Block $%x is locked!\n", id));
    });

//  If in use: if in ram, free memory & LRU, then in any case zap entry

    if (prd->offset)
    {
        Spew(DSRC_RES_DelDrop, ("ResDelete: deleting $%x\n", id));
        if (prd->ptr)
        {
            Spew(DSRC_RES_DelDrop, ("ResDelete: freeing memory for $%x\n", id));
            DBG(DSRC_RES_Stat,
                 {
                resStat.totMemAlloc -= prd->size;
                resStat.numLoaded--;
                Spew(DSRC_RES_Stat, ("ResDelete: free %d, total now %d bytes\n",
                                            prd->size, resStat.totMemAlloc));
            });
            ResDrop(id);
        }
        memset(prd, 0, sizeof(ResDesc));
    }

//  Else if not in use, spew to whoever's listening

    else
    {
        Spew(DSRC_RES_DelDrop, ("ResDelete: $%x not in use\n", id));
    }
}

//  --------------------------------------------------------
//        INTERNAL ROUTINES
//  --------------------------------------------------------
//
//  ResCheckId() checks if id valid.
//
//        id = id to be checked
//
//  Returns: TRUE if id ok, FALSE if invalid & prints warning

bool ResCheckId(Id id)
{
    cAutoResThreadLock lock;

    if (id < ID_MIN)
    {
        Warning(("ResCheckId: id $%x invalid\n", id));
        return FALSE;
    }
    if (id > resDescMax)
    {
        Warning(("ResCheckId: id $%x exceeds table\n", id));
        return FALSE;
    }
    return TRUE;
}
