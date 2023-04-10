//      RefAcc.c        Resource reference access
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/refacc.cpp 1.17 1997/03/07 17:21:35 JON Exp $
* $Log: refacc.cpp $
 * Revision 1.17  1997/03/07  17:21:35  JON
 * Expanded warning for exceeding max lock count.
 * 
 * Revision 1.16  1997/02/24  23:49:16  TOML
 * New resource cache logic
 * 
 * Revision 1.15  1997/02/12  00:30:45  TOML
 * partial lock support
 *
 * Revision 1.14  1997/01/14  16:36:55  TOML
 * Initial revision
 *
 * Revision 1.13  1996/09/15  18:21:10  TOML
 * Added stronger debugging code
 *
 * Revision 1.12  1996/09/14  16:10:36  TOML
 * Prepared for revision
 *
 * Revision 1.11  1996/09/14  14:12:54  TOML
 * Made C++ parser friendly
 *
 * Revision 1.10  1996/08/30  10:24:43  TOML
 * *** empty log message ***
 *
 * Revision 1.9  1996/06/28  18:46:58  REX
 * Removed unnecc. Warning() from RefExtract() & RefExtractPartial()
 *
 * Revision 1.8  1996/06/28  15:53:06  REX
 * Added RefExtractPartial()
 *
 * Revision 1.7  1994/12/07  14:14:40  mahk
 * Fixed warning.
 *
 * Revision 1.6  1994/11/18  13:37:02  mahk
 * ResNumRefs, NULL reftable extract, and other fun stuff.
 *
 * Revision 1.5  1994/09/22  10:47:38  rex
 * Changed references to 'flags' field of resdesc, which has moved
 *
 * Revision 1.4  1994/08/30  15:18:38  rex
 * Made sure RefGet() returns NULL if ResLoadResource() did
 *
 * Revision 1.3  1994/08/30  15:15:22  rex
 * Put in check for NULL return from ResLoadResource
 *
 * Revision 1.2  1994/06/16  11:05:17  rex
 * Modified RefGet() to handle LRU list better (keep locked guys out)
 *
 * Revision 1.1  1994/02/17  11:23:16  rex
 * Initial revision
 *
*/

#include <string.h>
#include <io.h>

#include <res.h>
#include <res_.h>
#include <resthred.h>
#include <lzw.h>
#include <pkzip.h>

//  ---------------------------------------------------------
//
//  DoRefLock() locks a compound resource and returns ptr to item.
//
//      ref = resource reference
//
//  Returns: ptr to item within locked compound resource.

void * DoRefLock(Ref ref, BOOL fPartial = FALSE, unsigned partialIndex = 0)
{
   Id id = REFID(ref);
   ResDesc *prd;
   ResDesc2 *prd2;
   RefTable *prt;
   RefIndex index;
   ResThreadLock();
//  Check for valid ref

   ValidateRef(ref);

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return NULL;
   });

//  Add to cumulative stats

   CUMSTATS(id, numLocks);

//  Load block if not in ram

   prd = RESDESC(id);
   prd2 = RESDESC2(id);

   AssertMsg3(prd->lock < RES_MAXLOCK, 
             "Bad ref lock count (%d) for res %d (%x)\n",
             prd->lock, id, id);

   if (prd->ptr == NULL)
   {
      unsigned fResLoad = (fPartial) ? kResLoadPartialRef : 0;
      if (ResLoadResource(id, fResLoad, partialIndex) == NULL)
      {
         ResThreadUnlock();
         return (NULL);
      }
   }
   else if (prd->lock == 0)
      ResCacheRemove(id);

   if (!fPartial && (prd2->flags & RDF_PARTIAL))
   {
   //  Load all from disk
       ResRetrieve(id, prd->ptr);
       prd2->flags &= ~RDF_PARTIAL;
   }

//  Tally stats

   DBG(DSRC_RES_Stat,
   {
      if (prd->lock == 0)
         resStat.numLocked++;
   });

//  Bump lock count

   DBG(DSRC_RES_ChkLock,
   {
      if (prd->lock == RES_MAXLOCK)
         prd->lock--;
   });
   prd->lock++;

//  Index into ref table

   prt = (RefTable *) prd->ptr;
   index = REFINDEX(ref);
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefIndexValid(prt, index))         \
            Warning(("RefLock: reference: $%x bad, index out of range\n", ref));
   });

   ValidateRef(ref);

   ResThreadUnlock();

//  Return ptr
   return (((uchar *) prt) + (prt->offset[index]));
}

//  ---------------------------------------------------------
//
//  RefLock[Exact]()
//

void * RefLock(Ref ref)
{
    return DoRefLock(ref);
}

void * RefLockExact(Ref ref)
{
    return DoRefLock(ref, TRUE, REFINDEX(ref));
}

//  ---------------------------------------------------------
//
//  RefGet() gets a ptr to an item in a compound resource (ref).
//
//      ref = resource reference
//
//  Returns: ptr to item (ptr only guaranteed until next Malloc(),
//              Lock(), Get(), etc.

void *RefGet(Ref ref)
{
   Id id;
   ResDesc *prd;
   RefTable *prt;
   RefIndex index;
   ResThreadLock();
//  Check for valid ref

   ValidateRef(ref);

   id = REFID(ref);

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return NULL;
   });

//  Add to cumulative stats

   CUMSTATS(REFID(ref), numGets);

//  Get hold of ref

   prd = RESDESC(id);
   if (prd->ptr == NULL)
   {
      if (ResLoadResource(id) == NULL)
      {
         ResThreadUnlock();
         return (NULL);
      }
      ResCacheAdd(id);
   }
   else if (prd->lock == 0)
      ResCacheTouch(id);

//  Index into ref table

   prt = (RefTable *) prd->ptr;
   index = REFINDEX(ref);
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefIndexValid(prt, index))         \
            Warning(("RefGet: reference: $%x bad, index out of range\n", ref));
   });

   AssertMsg1(RefIndexValid(prt, index), "Bad index for reference 0x%x", ref);
   ResThreadUnlock();

   ValidateRef(ref);

//  Return ptr
   return (((uchar *) prt) + (prt->offset[index]));
}
//  ---------------------------------------------------------
//
//  ResReadRefTable() reads a compound resource's ref table.
//
//      id = id of compound resource
//
//  Returns: ptr to reftable allocated with Malloc(), or NULL

RefTable *ResReadRefTable(Id id)
{
   ResDesc *prd;
   int fd;
   RefIndex numRefs;
   RefTable *prt;
   ResThreadLock();
//  Check id and file number and make sure compound

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return (NULL);
   });
   prd = RESDESC(id);
   fd = resFile[prd->filenum].fd;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (fd < 0)
      {         \
            Warning(("ResReadRefTable: id $%x doesn't exist\n", id));         \
            ResThreadUnlock();         \
            return (NULL);         \
      }
   });
   if ((ResFlags(id) & RDF_COMPOUND) == 0)
   {
      DBG(DSRC_RES_ChkIdRef,
          {         \
            Warning(("ResReadRefTable: id $%x is not compound\n", id));         \
      });
      ResThreadUnlock();
      return (NULL);
   }

//  Seek to data, read numrefs, allocate table, read in offsets

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
   read(fd, &numRefs, sizeof(RefIndex));
   prt = (RefTable *) Malloc(REFTABLESIZE(numRefs));
   prt->numRefs = numRefs;
   read(fd, &prt->offset[0], sizeof(long) * (numRefs + 1));

   ResThreadUnlock();
   return (prt);
}
//  ---------------------------------------------------------
//
//  ResExtractRefTable() extracts a compound res's ref table.
//
//      id   = id of compound resource
//      prt  = ptr to ref table
//      size = size of ref table in bytes
//
//  Returns: 0 if ok, -1 if error

int ResExtractRefTable(Id id, RefTable * prt, long size)
{
   ResDesc *prd;
   int fd;
   ResThreadLock();
//  Check id and file number and make sure compound

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return (-1);
   });
   prd = RESDESC(id);
   fd = resFile[prd->filenum].fd;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (fd < 0)
      {         \
            Warning(("ResExtractRefTable: id $%x doesn't exist\n", id));         \
            ResThreadUnlock();         \
            return (-1);         \
      }
   });
   if ((ResFlags(id) & RDF_COMPOUND) == 0)
   {
      DBG(DSRC_RES_ChkIdRef,
          {         \
            Warning(("ResExtractRefTable: id $%x is not compound\n", id));         \
      });
      ResThreadUnlock();
      return (-1);
   }

//  Seek to data, read numrefs, check table size, read in offsets

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
   read(fd, &prt->numRefs, sizeof(RefIndex));
   if (REFTABLESIZE(prt->numRefs) > size)
   {
      Warning(("ResExtractRefTable: ref table too large for buffer\n"));
      ResThreadUnlock();
      return (-1);
   }
   read(fd, &prt->offset[0], sizeof(long) * (prt->numRefs + 1));

   ResThreadUnlock();
   return (0);
}

// return number of refs, or -1 if error
int ResNumRefs(Id id)
{
   ResDesc *prd;
   ResThreadLock();
//  Check id and file number and make sure compound

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return (-1);
   });
   if ((ResFlags(id) & RDF_COMPOUND) == 0)
   {
      DBG(DSRC_RES_ChkIdRef,
          {         \
            Warning(("ResNumRefs: id $%x is not compound\n", id));         \
      });
      ResThreadUnlock();
      return (-1);
   }
   prd = RESDESC(id);
   if (prd->ptr != NULL)
   {
      ResThreadUnlock();
      return ((RefTable *) prd->ptr)->numRefs;
   }
   else
   {
      int fd = resFile[prd->filenum].fd;
      RefIndex result;
      DBG(DSRC_RES_ChkIdRef,
          {
         if (fd < 0)
         {            \
               Warning(("ResNumRefs: id $%x doesn't exist\n", id));            \
               ResThreadUnlock();            \
               return (-1);            \
         }
      });
      lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
      read(fd, &result, sizeof(RefIndex));
      ResThreadUnlock();
      return result;
   }
}
//  ---------------------------------------------------------
//
//  RefExtract() extracts a ref item from a compound resource.
//
//      prt  = ptr to ref table
//      ref  = ref
//      buff = ptr to buffer (use RefSize() to compute needed buffer size)
//
//  Returns: ptr to supplied buffer, or NULL if problem

void *RefExtract(RefTable * prt, Ref ref, void *buff)
{
   RefIndex index;
   ResDesc *prd;
   int fd;
   long refsize;
   RefIndex numrefs;
   long offset;
   ResThreadLock();
//  Check id, get file number

   ValidateRef(ref);

   prd = RESDESC(REFID(ref));
   fd = resFile[prd->filenum].fd;
   index = REFINDEX(ref);

   // get reftable date from rt or by seeking.
   if (prt != NULL)
   {
      refsize = RefSize(prt, index);
      numrefs = prt->numRefs;
      offset = prt->offset[index];
   }
   else
   {
      // seek into the file and find the stuff.
      lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
      read(fd, &numrefs, sizeof(RefIndex));
      lseek(fd, index * sizeof(long), SEEK_CUR);
      read(fd, &offset, sizeof(long));
      read(fd, &refsize, sizeof(long));
      refsize -= offset;
//      Warning(("Null reftable size = %d offset = %d numrefs = %d\n",refsize,offset,numrefs));
   }
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return (NULL);
   });
   DBG(DSRC_RES_ChkIdRef,
   {
      if (index >= numrefs)
      {         \
            Warning(("RefExtract: ref $%x index too large\n", ref));         \
            ResThreadUnlock();         \
            return (NULL);         \
      }
   });

//  Add to cumulative stats

   CUMSTATS(REFID(ref), numExtracts);

//  Seek to start of all data in compound resource

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
         SEEK_SET);

//  If LZW, extract with skipping, else seek & read

   if (ResFlags(REFID(ref)) & RDF_LZW)
   {
      LzwExpandFd2Buff(fd, buff,
                       offset - REFTABLESIZE(numrefs),  // skip amt
                       refsize);                 // data amt
   }
   else if (ResFlags(REFID(ref)) & RDF_PKZIP)
   {
      PkExplodeFileToMem(fd, buff,
                         offset - REFTABLESIZE(numrefs),  // skip amt
                         refsize);                 // data amt
   }
   else
   {
      lseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
      read(fd, buff, refsize);
   }

   ValidateRef(ref);

   ResThreadUnlock();
   return (buff);
}


//  ---------------------------------------------------------
//
//  RefExtractPartial() extracts a portion of a ref item from a
// compound resource.
//
//      prt  = ptr to ref table
//      ref  = ref
//      buff = ptr to buffer
//    size = size of portion to read, in bytes
//
//  Returns: ptr to supplied buffer, or NULL if problem

void *RefExtractPartial(RefTable * prt, Ref ref, void *buff, long size)
{
// @Note (toml 02-11-97): if the resource is locked or in lru, should do a memcpy?
   RefIndex index;
   ResDesc *prd;
   int fd;
   long refsize;
   RefIndex numrefs;
   long offset;
   ResThreadLock();
   ValidateRef(ref);

//  Check id, get file number

   prd = RESDESC(REFID(ref));
   fd = resFile[prd->filenum].fd;
   index = REFINDEX(ref);

   // get reftable date from rt or by seeking.
   if (prt != NULL)
   {
      refsize = RefSize(prt, index);
      numrefs = prt->numRefs;
      offset = prt->offset[index];
   }
   else
   {
      // seek into the file and find the stuff.
      lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
      read(fd, &numrefs, sizeof(RefIndex));
      lseek(fd, index * sizeof(long), SEEK_CUR);
      read(fd, &offset, sizeof(long));
      read(fd, &refsize, sizeof(long));
      refsize -= offset;
//      Warning(("Null reftable size = %d offset = %d numrefs = %d\n",refsize,offset,numrefs));
   }
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return (NULL);
   });
   DBG(DSRC_RES_ChkIdRef,
   {
      if (index >= numrefs)
      {         \
            Warning(("RefExtract: ref $%x index too large\n", ref));         \
            ResThreadUnlock();         \
            return (NULL);         \
      }
   });

//  Add to cumulative stats

   CUMSTATS(REFID(ref), numExtracts);

//  Seek to start of all data in compound resource

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
         SEEK_SET);

// Check size

   if (size > refsize)
   {
      Warning(("RefExtractPartial: size %d > refsize: %d (ref: $%x)\n",
               size, refsize, ref));
      size = refsize;
   }

//  If LZW, extract with skipping, else seek & read

   if (ResFlags(REFID(ref)) & RDF_LZW)
   {
      LzwExpandFd2Buff(fd, buff,
                       offset - REFTABLESIZE(numrefs),  // skip amt
                       size);                    // data amt
   }
   else if (ResFlags(REFID(ref)) & RDF_PKZIP)
   {
      PkExplodeFileToMem(fd, buff,
                         offset - REFTABLESIZE(numrefs),  // skip amt
                         size);                 // data amt
   }
   else
   {
      lseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
      read(fd, buff, size);
   }

   ValidateRef(ref);

   ResThreadUnlock();
   return (buff);
}
//  ---------------------------------------------------------

int RefInject(RefTable * prt, Ref ref, void *buff)
{
   RefIndex index;
   ResDesc *prd;
   int fd;
   long refsize;
   RefIndex numrefs;
   long offset;
   ResThreadLock();
//  Check id, get file number

   if ((ResFlags(REFID(ref)) & RDF_LZW) || (ResFlags(REFID(ref)) & RDF_PKZIP))
   {
      ResThreadUnlock();
      return 0;
   }


   prd = RESDESC(REFID(ref));
   fd = resFile[prd->filenum].fd;
   index = REFINDEX(ref);

   // get reftable date from rt or by seeking.
   if (prt != NULL)
   {
      refsize = RefSize(prt, index);
      numrefs = prt->numRefs;
      offset = prt->offset[index];
   }
   else
   {
      // seek into the file and find the stuff.
      lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
      read(fd, &numrefs, sizeof(RefIndex));
      lseek(fd, index * sizeof(long), SEEK_CUR);
      read(fd, &offset, sizeof(long));
      read(fd, &refsize, sizeof(long));
      refsize -= offset;
      Warning(("Null reftable size = %d offset = %d numrefs = %d\n", refsize, offset, numrefs));
   }
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return (NULL);
   });
   DBG(DSRC_RES_ChkIdRef,
   {
      if (index >= numrefs)
      {         \
            Warning(("RefExtract: ref $%x index too large\n", ref));         \
            ResThreadUnlock();         \
            return (NULL);         \
      }
   });

//  Add to cumulative stats

   CUMSTATS(REFID(ref), numExtracts);

//  Seek to start of all data in compound resource

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
         SEEK_SET);


   lseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
   ResThreadUnlock();
   return write(fd, buff, refsize);

}
//  ---------------------------------------------------------
//      INTERNAL ROUTINES
//  ---------------------------------------------------------
//
//  RefCheckRef() checks if ref valid.
//
//      ref = ref to be checked
//
//  Returns: TRUE if ref ok, FALSE if invalid & prints warning

bool RefCheckRef(Ref ref)
{
   Id id;
   ResThreadLock();
   id = REFID(ref);
   if (!ResCheckId(id))
   {
      ResThreadUnlock();
      return FALSE;
   }

   if ((ResFlags(id) & RDF_COMPOUND) == 0)
   {
      Warning(("RefCheckRef: id $%x is not a compound resource\n", id));
      ResThreadUnlock();
      return FALSE;
   }

   ResThreadUnlock();
   return TRUE;
}
