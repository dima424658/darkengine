//      ResLoad.c   Load resource from resfile
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/resload.cpp 1.16 1997/07/15 21:29:41 TOML Exp $
* $Log: resload.cpp $
 * Revision 1.16  1997/07/15  21:29:41  TOML
 * *** empty log message ***
 * 
 * Revision 1.15  1997/05/17  17:45:20  JAEMZ
 * Added type specific install callbacks
 *
 * Revision 1.14  1997/02/12  16:14:50  TOML
 * load warn opn debug only
 *
 * Revision 1.13  1997/02/12  00:30:29  TOML
 * partial lock support
 *
 * Revision 1.12  1997/02/10  13:49:02  TOML
 * added ability to warn of load
 *
 * Revision 1.11  1997/01/14  16:36:50  TOML
 * pk support
 *
 * Revision 1.10  1996/09/14  16:10:33  TOML
 * Prepared for revision
 *
 * Revision 1.9  1996/09/14  14:12:52  TOML
 * Made C++ parser friendly
 *
 * Revision 1.8  1996/08/14  15:57:02  PATMAC
 * Added ResRetrievePartial
 *
 * Revision 1.7  1994/11/30  20:32:32  xemu
 * CD spoofing support
 *
 * Revision 1.6  1994/09/22  10:49:10  rex
 * Modified access to resdesc flags, which have moved
 *
 * Revision 1.5  1994/06/16  11:07:44  rex
 * Took LRU list adding out of ResLoadResource()
 *
 * Revision 1.4  1994/05/26  13:52:32  rex
 * Surrounded Malloc() for loading resource with setting of idBeingLoaded,
 * so installable pager can make use of this.
 *
 * Revision 1.3  1994/04/19  16:40:28  rex
 * Added check for 0-size resource
 *
 * Revision 1.2  1994/03/14  16:10:47  rex
 * Added id to spew in ResLoadResource()
 *
 * Revision 1.1  1994/02/17  11:23:39  rex
 * Initial revision
 *
*/

#include <io.h>

#include <res.h>
#include <res_.h>
#include <lzw.h>
#include <pkzip.h>

#include <_res.h>

//  -----------------------------------------------------------
//
// Warning on load
//

static BOOL g_fWarnOnLoad;

void ResSetWarnOnLoad(BOOL fNew)
{
    g_fWarnOnLoad = fNew;
}

//  -----------------------------------------------------------
//
//  ResLoadResource() loads a resource object, decompressing it if it is
//      compressed.
//
//      id = resource id

void *ResLoadResource(Id id, unsigned flags, unsigned partialRefIndex)
{
   ResDesc *prd;
   ResDesc2 *prd2;
//  If doesn't exit, forget it

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResInUse(id))
         return NULL;
   });
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return NULL;
   });

#ifdef DEBUG
   if (g_fWarnOnLoad)
      Warning(("loading resource $%x during critical code\n", id));
#endif

   Spew(DSRC_RES_Read, ("ResLoadResource: loading $%x\n", id));

//  Check for bad resource

   prd = RESDESC(id);
   prd2 = RESDESC2(id);

   if (prd->size == 0)
   {
      Warning(("ResLoadResource: id $%x does not exist!\n", id));
      return (NULL);
   }

//  Allocate memory, setting magic id so pager can tell who it is if need be.

   prd->ptr = Malloc(prd->size);
   if (prd->ptr == NULL)
      return (NULL);

//  Tally memory allocated to resources

   DBG(DSRC_RES_Stat,
   {
      resStat.totMemAlloc += prd->size;
   });
   Spew(DSRC_RES_Stat, ("ResLoadResource: loading id: $%x, alloc %d, total now %d bytes\n",
                        id, prd->size, resStat.totMemAlloc));

//  Add to cumulative stats

   CUMSTATS(id, numLoads);

//  Load from disk

   if ((flags & kResLoadPartialRef) && partialRefIndex == 0)  // sorry, only do exact for first compound item for now! // @TBD (toml 02-11-97): Is this stuff really helping?
   {
   //  Seek to data, read numrefs, read in offsets, extract ref 0
      int fd = resFile[prd->filenum].fd;

      RefTable *prt = (RefTable *)prd->ptr;
      lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
      read(fd, &prt->numRefs, sizeof(RefIndex));
      read(fd, &prt->offset[0], sizeof(long) * (prt->numRefs + 1));

      RefExtract(prt,
                 MKREF(id, partialRefIndex),
                 ((uchar *)(prd->ptr)) +
                  (sizeof(RefIndex) +
                   sizeof(long) * (prt->numRefs + 1)));

      prd2->flags |= RDF_PARTIAL;
   }
   else
   {
      ResRetrieve(id, prd->ptr);
      prd2->flags &= ~RDF_PARTIAL;
   }

//  Tally stats

   DBG(DSRC_RES_Stat,
   {
      resStat.numLoaded++;
   });

//  Return ptr

   return (prd->ptr);
}
//  ---------------------------------------------------------
//
//  ResRetrieve() retrieves a resource from disk.
//
//      id     = id of resource
//      buffer = ptr to buffer to load into (must be big enough)
//
//  Returns: TRUE if retrieved, FALSE if problem

bool ResRetrieve(Id id, void *buffer)
{
   ResDesc *prd;
   ResDesc2 *prd2;
   int fd;
   uchar *p;
   long size;
   RefIndex numRefs;
//  Check id and file number
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return FALSE;
   });
   prd = RESDESC(id);
   prd2 = RESDESC2(id);
   fd = resFile[prd->filenum].fd;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (fd < 0)
      {         \
            Warning(("ResRetrieve: id $%x doesn't exist\n", id));         \
            return FALSE;         \
      }
   });

//  Seek to data, set up

   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
   p = (uchar *) buffer;
   size = prd->size;

//  If compound, read in ref table

   if (prd2->flags & RDF_COMPOUND)
   {
      read(fd, p, sizeof(short));
      numRefs = *(short *) p;
      p += sizeof(short);
      read(fd, p, sizeof(long) * (numRefs + 1));
      p += sizeof(long) * (numRefs + 1);
      size -= REFTABLESIZE(numRefs);
   }

//  Read in data

   if (prd2->flags & RDF_LZW)
      LzwExpandFd2Buff(fd, p, 0, 0);
   else if (prd2->flags & RDF_PKZIP)
      PkExplodeFileToMem(fd, p, 0, size);
   else
      read(fd, p, size);

// Ooh, now we must pay the piper...
   if (prd2->flags & RDF_CDSPOOF)
   {
      if (gSpoofDrive.callback != NULL)
         gSpoofDrive.callback(size, id);
   }

   // Now do any type installed callbacks
   ResDoTypeCallbacks(id);

   return TRUE;
}
//  ---------------------------------------------------------
//
//  ResRetrievePartial() retrieves part of a resource from disk.
//
//      id     = id of resource
//      buffer = ptr to buffer to load into (must be big enough)
//    offset = # of bytes to skip at start of resource
//    nBytes = # of bytes of resource to copy to buffer
//
// NOTE: should only be used for resources which are not LZW encoded,
//  and which are not compound resources (use RefExtractPartial)
//
//  Returns: TRUE if retrieved, FALSE if problem

bool ResRetrievePartial(Id id, void *buffer, uint32 offset, uint32 nBytes)
{
   ResDesc *prd;
   ResDesc2 *prd2;
   int fd;
   uchar *p;
   long size;
//  Check id and file number
   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return FALSE;
   });
   prd = RESDESC(id);
   prd2 = RESDESC2(id);
   fd = resFile[prd->filenum].fd;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (fd < 0)
      {         \
            Warning(("ResRetrievePartial: id $%x doesn't exist\n", id));         \
            return FALSE;         \
      }
   });

//  Seek to data, set up

   if ((offset + nBytes) > prd->size)
   {
      // data goes beyond end of resource
      return FALSE;
   }
   lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + offset, SEEK_SET);
   p = (uchar *) buffer;
   size = nBytes;

   // bail out if compound or compressed
   if ((prd2->flags & RDF_COMPOUND)
       || (prd2->flags & (RDF_LZW | RDF_PKZIP)))
   {
      // can't deal with compressed or compound resources
      return FALSE;
   }

   read(fd, p, size);

// Ooh, now we must pay the piper...
   if (prd2->flags & RDF_CDSPOOF)
   {
      if (gSpoofDrive.callback != NULL)
         gSpoofDrive.callback(size, id);
   }


   return TRUE;
}
