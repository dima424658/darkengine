//      ResExBlk.c      Resource block-by-block extraction
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/resexblk.cpp 1.7 1997/01/14 16:37:08 TOML Exp $
* $Log: resexblk.cpp $
 * Revision 1.7  1997/01/14  16:37:08  TOML
 * Initial revision
 * 
 * Revision 1.6  1996/09/14  16:10:32  TOML
 * Prepared for revision
 *
 * Revision 1.5  1996/09/14  14:11:58  TOML
 * Made C++ parser friendly
 *
 * Revision 1.4  1996/08/30  10:24:57  TOML
 * Made thread safe
 *
 * Revision 1.3  1994/09/22  10:49:26  rex
 * Modified access to resdesc flags, which have moved
 *
 * Revision 1.2  1994/03/09  19:30:55  jak
 * Res\RefExtractInBlocks transfers a variable/length
 * block of data in each pass.  The user/defined function
 * returns the amount that should be passed in NEXT time.
 *
 * Revision 1.1  1994/02/17  11:23:07   rex
 * Initial revision
 *
*/

#include <io.h>
#include <string.h>

#include <res.h>
#include <resthred.h>
#include <res_.h>
#include <lzw.h>

typedef struct
{
   uchar *buff;
   long blockSize;
   long sofar;
   long totalSize;
   long (*f_ProcBlock) (void *buff, long numBytes, long iblock);
   long iblock;
   long currIndex;
} BlockedBuffCtrl;

BlockedBuffCtrl bbc;

void LzwBlockedBuffDestCtrl(long buff, LzwCtrl ctrl);
void LzwBlockedBuffDestPut(uchar byte);
void ResGrabInBlocks(int fd, void *buff, long blockSize, long totSize,
                      long (*f_ProcBlock) (void *buff, long blockSize, long iblock));
//  ---------------------------------------------------------
//
//  ResExtractInBlocks() extracts a resource in blocks, passing
//      each block to a user-supplied function.  The user-supplied
//    function should return the number of bytes to be passed NEXT time.
//
//      id              = resource id
//      buff            = ptr to buffer to put each block into
//      blockSize   = initial amout to be passed to user-supplied function
//      f_ProcBlock = ptr to function to call for each block

void ResExtractInBlocks(Id id, void *buff, long blockSize,
                         long (*f_ProcBlock) (void *buff, long numBytes, long iblock))
{
   ResDesc *prd;
   ResDesc2 *prd2;
   int fd;
   ResThreadLock();

//  Error-check id

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return;
   });

//  Add to cumulative stats

   CUMSTATS(id, numExtracts);

// Check for 0-length resource or 0-length block

   if (ResSize(id) == 0)
   {
      Warning(("ResExtractInBlocks: can't extract 0-length res $%x\n", id));
      ResThreadUnlock();
      return;
   }
   if (blockSize == 0)
   {
      Warning(("ResExtractInBlocks: can't use 0-length block for res $%x\n", id));
      ResThreadUnlock();
      return;
   }

//  Get ptr to resource descriptor and file descriptor, seek to resource

   prd = RESDESC(id);
   prd2 = RESDESC2(id);
   fd = resFile[prd->filenum].fd;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (fd < 0)
      {         \
            Warning(("ResRetrieve: id $%x doesn't exist\n", id));
      }
   });
   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);

//  If uncompressed, read each block from disk and pass along

   if ((prd2->flags & (RDF_PKZIP | RDF_LZW)) == 0)
      ResGrabInBlocks(fd, buff, blockSize, prd->size, f_ProcBlock);

//  Else if compressed, first warn & bail out if compound resource
//  Then run thru magic lzw block chunker

   else
   {
      AssertMsg(prd2->flags & RDF_LZW, "Can't extract in blocks pk-compressed resources");
      if (prd2->flags & RDF_COMPOUND)
      {
         Warning(("ResExtractInBlocks: can't extract compound res $%x this way\n",
                  id));
         ResThreadUnlock();
         return;
      }
      bbc.buff = (uchar *) buff;
      bbc.totalSize = ResSize(id);
      bbc.blockSize = blockSize;
      bbc.f_ProcBlock = f_ProcBlock;
      LzwExpand(LzwFdSrcE(fd), LzwBlockedBuffDestCtrl,
                LzwBlockedBuffDestPut, (long) buff, 0, 0);
   }
   ResThreadUnlock();
}
//  ---------------------------------------------------------
//
//  RefExtractInBlocks() extracts a resource item in blocks, passing
//      each block to a user-supplied function.  The user-supplied
//    function should return the number of bytes to be passed NEXT time.
//
//      prt         = ptr to ref table
//      ref         = resource ref
//      buff            = ptr to buffer to put each block into
//      blockSize   = initial amout to be passed to user-supplied function
//      f_ProcBlock = ptr to function to call for each block



void RefExtractInBlocks(RefTable * prt, Ref ref, void *buff, long blockSize,
                         long (*f_ProcBlock) (void *buff, long numBytes, long iblock))
{
   RefIndex index;
   ResDesc *prd;
   int fd;
   long skipAmt,
    refSize;
   ResThreadLock();
//  Check id, get file number

   index = REFINDEX(ref);
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!RefCheckRef(ref))
         return;
   });
   DBG(DSRC_RES_ChkIdRef,
   {
      if (index >= prt->numRefs)
      {         \
            Warning(("RefExtractInBlocks: ref $%x index too large\n", ref));         \
            return;         \
      }
   });
   prd = RESDESC(REFID(ref));
   fd = resFile[prd->filenum].fd;

//  Add to cumulative stats

   CUMSTATS(REFID(ref), numExtracts);

// Check for 0-length resource or 0-length block

   if (RefSize(prt, index) == 0)
   {
      Warning(("RefExtractInBlocks: can't extract 0-length ref $%x\n", ref));
      ResThreadUnlock();
      return;
   }
   if (blockSize == 0)
   {
      Warning(("RefExtractInBlocks: can't use 0-length block for ref $%x\n", ref));
      ResThreadUnlock();
      return;
   }

//  Seek to start of all data in compound resource

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(prt->numRefs),
         SEEK_SET);
   skipAmt = prt->offset[index] - REFTABLESIZE(prt->numRefs);
   refSize = RefSize(prt, index);

//  If uncompressed, seek past reftable, grab in chunks

   if ((ResFlags(REFID(ref)) & (RDF_PKZIP | RDF_LZW)) == 0)
   {
      lseek(fd, skipAmt, SEEK_CUR);
      ResGrabInBlocks(fd, buff, blockSize, refSize, f_ProcBlock);
   }

//  Else if compressed, run thru magic lzw block chunker

   else
   {
      AssertMsg(ResFlags(REFID(ref)) & RDF_LZW, "Can't extract in blocks pk-compressed resources");
      bbc.buff = (uchar *) buff;
      bbc.totalSize = RefSize(prt, index);
      bbc.blockSize = blockSize;
      bbc.f_ProcBlock = f_ProcBlock;
      LzwExpand(LzwFdSrcE(fd), LzwBlockedBuffDestCtrl,
                LzwBlockedBuffDestPut, (long) buff, skipAmt, refSize);
   }
   ResThreadUnlock();
}
//  ---------------------------------------------------------------
//      INTERNAL ROUTINES
//  ---------------------------------------------------------------
//
//  LzwBlockedBuffDestCtrl() is control routine for blocked extract.

#pragma off(unreferenced);

static void LzwBlockedBuffDestCtrl(long buff, LzwCtrl ctrl)
{
   if (ctrl == BEGIN)
   {
      bbc.iblock = 0;
      bbc.sofar = 0;
      bbc.currIndex = 0;

      // Dont run off end
      bbc.blockSize = (bbc.blockSize > bbc.totalSize) ? bbc.totalSize : bbc.blockSize;
   }
}
#pragma on(unreferenced);

//  ---------------------------------------------------------------
//
//  LzwBlockedBuffDestPut() is put routine for blocked extract.

static void LzwBlockedBuffDestPut(uchar byte)
{
   long newBlockSize,
    bytesLeft;
//  Put byte in buffer, decrement total counter

   *(bbc.buff + bbc.currIndex) = byte;

//  If reached block size, send to proc func.    Then bump block counter,
//  compute size of next block, and reset current index

   if (++bbc.currIndex >= bbc.blockSize)
   {
      // Call proc func on accumulated data
      newBlockSize = (*bbc.f_ProcBlock) (bbc.buff, bbc.blockSize, bbc.iblock);

      // Calculate next block size
      bbc.sofar += bbc.blockSize;
      bytesLeft = bbc.totalSize - bbc.sofar;
      bbc.blockSize = newBlockSize > 0 ? newBlockSize : bbc.blockSize;
      bbc.blockSize = (bbc.blockSize > bytesLeft) ? bytesLeft : bbc.blockSize;

      ++bbc.iblock;
      Assrt(bbc.blockSize != 0 || bytesLeft == 0);
      Assrt(bytesLeft >= 0);
      bbc.currIndex = 0;
   }
}
//  -------------------------------------------------------------
//
//  ResGrabInBlocks() grabs uncompressed chunk of res data in blocks.

static void ResGrabInBlocks(int fd, void *buff, long blockSize, long totSize,
                             long (*f_ProcBlock) (void *buff, long blockSize, long iblock))
{
   long iblock,
    n,
    newBlockSize,
    sofar,
    bytesLeft;
   sofar = 0;
   iblock = 0;

// Burn thru data, reading from disk and sending to proc func

   while (sofar < totSize)
   {
      // Dont read off end
      bytesLeft = totSize - sofar;
      n = (blockSize > bytesLeft) ? bytesLeft : blockSize;

      // Read it and call proc func
      read(fd, buff, n);
      newBlockSize = (*f_ProcBlock) (buff, n, iblock);

      // A new blocksize of 0 means use last blocksize.
      blockSize = (newBlockSize > 0) ? newBlockSize : blockSize;

      sofar += n;
      iblock++;
   }
}
