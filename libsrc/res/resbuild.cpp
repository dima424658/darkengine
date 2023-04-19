//      RESBUILD.C      Resource-file building routines
//      Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/res/RCS/resbuild.cpp 1.17 1997/01/16 14:50:50 TOML Exp $
* $Log: resbuild.cpp $
 * Revision 1.17  1997/01/16  14:50:50  TOML
 * stronger error checking
 * 
 * Revision 1.16  1997/01/14  16:37:10  TOML
 * Initial revision
 *
 * Revision 1.15  1996/10/10  16:13:36  TOML
 * msvc port
 *
 * Revision 1.14  1996/09/14  16:09:57  TOML
 * Prepared for revision
 *
 * Revision 1.13  1996/09/14  14:12:45  TOML
 * Made C++ parser friendly
 *
 * Revision 1.12  1994/09/22  10:48:52  rex
 * Modified access to resdesc flags and type, which have moved
 *
 * Revision 1.11  1994/09/20  17:43:25  xemu
 * ability to use a pcompbuff
 * ResKill now works on non/zero file IDs
 * ResWrite now has a return value
 *
 * Revision 1.10  1994/06/16  11:06:30  rex
 * Got rid of RDF_NODROP flag
 *
 * Revision 1.9  1994/02/17  11:25:32  rex
 * Moved some stuff out to resmake.c and resfile.c
 *
*/

#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <res.h>
#include <res_.h>
#include <lzw.h>
#include <pkzip.h>

#define CTRL_Z 26       // make sure comment ends with one, so can type a file

//  Internal prototypes

bool ResEraseIfInFile(Id id);                    // erase item from file
void ResCopyBytes(int fd, long writePos, long readPos, long size);
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

//  -------------------------------------------------------
//
//  ResSetComment() sets comment in res header.

void ResSetComment(int filenum, const char *comment)
{
   ResFileHeader *phead;
   DBG(DSRC_RES_ChkIdRef,
   {
      if (resFile[filenum].pedit == NULL)
      {         \
            Warning(("ResSetComment: file %d not open for writing\n", filenum));         \
            return;
      }
   });

   Spew(DSRC_RES_General,
        ("ResSetComment: setting comment for filenum %d to:\n%s\n",
         filenum, comment));

   phead = &resFile[filenum].pedit->hdr;
   memset(phead->comment, 0, sizeof(phead->comment));
   strncpy(phead->comment, comment, sizeof(phead->comment) - 2);
   phead->comment[strlen(phead->comment)] = CTRL_Z;
}
//  -------------------------------------------------------
//
//  ResWrite() writes a resource to an open resource file.
//  This routine assumes that the file position is already set to
//  the current data position.
// Returns the total number of bytes written out, or -1 if there
// was a writing error.
//
//      id = id to write

extern uchar *restemp_buffer;
extern int restemp_buffer_size;

int ResWrite(Id id)
{
#define EXTRA 250
   static uchar pad[] = {0, 0, 0, 0, 0, 0, 0, 0};
   int retval = 0, old_retval = 0;
   ResDesc *prd;
   ResDesc2 *prd2;
   ResFile *prf;
   ResDirEntry *pDirEntry;
   uchar *p;
   void *pcompbuff;
   long size, sizeTable;
   long compsize;
   int padBytes;
   bool buffer_alloc = FALSE;
//  Check for errors

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return (-1);
   });

   prd = RESDESC(id);
   prd2 = RESDESC2(id);
   prf = &resFile[prd->filenum];

   if (prf->pedit == NULL)
   {
      CriticalMsg("File not opened!");
      return (-1);
   }

//  Check if item already in directory, if so erase it

   ResEraseIfInFile(id);

//  If directory full, grow it

   if (prf->pedit->pdir->numEntries == prf->pedit->numAllocDir)
   {
      Spew(DSRC_RES_Write, ("ResWrite: growing directory of filenum %d\n",
                            prd->filenum));

      prf->pedit->numAllocDir += DEFAULT_RES_GROWDIRENTRIES;
      prf->pedit->pdir = (ResDirHeader *) Realloc(prf->pedit->pdir,
                                                  sizeof(ResDirHeader) + (sizeof(ResDirEntry) * prf->pedit->numAllocDir));
   }

//  Set resource's file offset

   prd->offset = RES_OFFSET_REAL2DESC(prf->pedit->currDataOffset);

//  Fill in directory entry

   pDirEntry = ((ResDirEntry *) (prf->pedit->pdir + 1)) +
      prf->pedit->pdir->numEntries;

   pDirEntry->id = id;
   pDirEntry->flags = prd2->flags;
   pDirEntry->type = prd2->type;
   pDirEntry->size = prd->size;

   Spew(DSRC_RES_Write, ("ResWrite: writing $%x\n", id));

//  If compound, write out reftable without compression

   lseek(prf->fd, prf->pedit->currDataOffset, SEEK_SET);
   p = (uchar *) prd->ptr;
   sizeTable = 0;
   size = prd->size;
   if (prd2->flags & RDF_COMPOUND)
   {
      sizeTable = REFTABLESIZE(((RefTable *) p)->numRefs);
      old_retval = retval;
      retval += write(prf->fd, p, sizeTable);
      if (old_retval + sizeTable != retval)
      {
         Warning(("ResWrite ID %x (RDF_COMPOUND) only wrote %d bytes instead of %d!\n", id, retval - old_retval, sizeTable));
         return (-1);
      }
      p += sizeTable;
      size -= sizeTable;
   }

//  If compression, try it (may not work out)

   if (pDirEntry->flags & (RDF_LZW | RDF_PKZIP))
   {
      if ((restemp_buffer == NULL) || (restemp_buffer_size < size))
      {
         buffer_alloc = TRUE;
         pcompbuff = (void *) Malloc(size + EXTRA);
      }
      else
         pcompbuff = (void *) restemp_buffer;
      if (pDirEntry->flags & RDF_LZW)
          compsize = LzwCompressBuff2Buff(p, size, pcompbuff, size);
      else
          compsize = PkImplodeMemToMem(p, size, pcompbuff, size);
      if (compsize < 0)
      {
         pDirEntry->flags &= ~(RDF_LZW | RDF_PKZIP);
      }
      else
      {
         pDirEntry->csize = sizeTable + compsize;
         old_retval = retval;
         retval += write(prf->fd, pcompbuff, compsize);
         if (old_retval + compsize != retval)
         {
            Warning(("ResWrite ID %x (RDF_LZW | RDF_PKZIP) only wrote %d bytes instead of %d!\n", id, retval - old_retval, compsize));
            return (-1);
         }
      }
      if (buffer_alloc)
         Free(pcompbuff);
   }

//  If no compress (or failed to compress well), just write out

   if (!(pDirEntry->flags & (RDF_LZW | RDF_PKZIP)))
   {
      pDirEntry->csize = prd->size;
      old_retval = retval;
      retval += write(prf->fd, p, size);
      if (old_retval + size != retval)
      {
         Warning(("ResWrite ID %x (uncompressed) only wrote %d bytes instead of %d!\n", id, retval - old_retval, size));
         return (-1);
      }
   }

//  Pad to align on data boundary

   padBytes = RES_OFFSET_PADBYTES(pDirEntry->csize);
   if (padBytes)
   {
      old_retval = retval;
      retval += write(prf->fd, pad, padBytes);
      if (old_retval + padBytes != retval)
      {
         Warning(("ResWrite ID %x (pad) only wrote %d bytes instead of %d!\n", id, retval - old_retval, padBytes));
         return (-1);
      }
   }

   if (tell(prf->fd) & 3)
      Warning(("ResWrite: misaligned writing!\n"));

//  Advance dir num entries, current data offset

   prf->pedit->pdir->numEntries++;
   prf->pedit->currDataOffset =
      RES_OFFSET_ALIGN(prf->pedit->currDataOffset + pDirEntry->csize);
   return (retval);
}
//  -------------------------------------------------------------
//
//  ResKill() not only deletes a resource from memory, it removes it
//  from the file too.

void ResKill(Id id)
{
   ResDesc *prd;
//  Check for valid id

   DBG(DSRC_RES_ChkIdRef,
   {
      if (!ResCheckId(id))
         return;
   });

   Spew(DSRC_RES_Write, ("ResKill: killing $%x\n", id));

   ResDrop(id);

   prd = RESDESC(id);

//  Make sure file is writeable

   DBG(DSRC_RES_Write,
   {
      if (resFile[prd->filenum].pedit == NULL)
      {         \
            Warning(("ResKill: file %d not open for writing\n", prd->filenum));         \
            return;
      }
   });

//  If so, erase it

   ResEraseIfInFile(id);
}
//  -------------------------------------------------------------
//
//  ResPack() removes holes from a resource file.
//
//      filenum = resource filenum (must already be open for create/edit)
//
//  Returns: # bytes reclaimed

long ResPack(int filenum)
{
   ResFile *prf;
   ResDirEntry *pDirEntry;
   long numReclaimed, sizeReclaimed;
   long dataRead, dataWrite;
   int i;
   ResDirEntry *peWrite;
//  Check for errors

   prf = &resFile[filenum];
   if (prf->pedit == NULL)
   {
      Warning(("ResPack: filenum %d not open for editing\n"));
      return (0);
   }

//  Set up

   sizeReclaimed = numReclaimed = 0;
   dataRead = dataWrite = prf->pedit->pdir->dataOffset;

//  Scan thru directory, copying over all empty entries

   pDirEntry = (ResDirEntry *) (prf->pedit->pdir + 1);
   for (i = 0; i < prf->pedit->pdir->numEntries; i++)
   {
      if (pDirEntry->id == 0)
      {
         numReclaimed++;
         sizeReclaimed += pDirEntry->csize;
      }
      else
      {
         if (gResDesc[pDirEntry->id].offset > RES_OFFSET_PENDING)
            gResDesc[pDirEntry->id].offset = RES_OFFSET_REAL2DESC(dataWrite);
         if (dataRead != dataWrite)
            ResCopyBytes(prf->fd, dataWrite, dataRead, pDirEntry->csize);
         dataWrite = RES_OFFSET_ALIGN(dataWrite + pDirEntry->csize);
      }
      dataRead = RES_OFFSET_ALIGN(dataRead + pDirEntry->csize);
      pDirEntry++;
   }

//  Now pack directory itself

   pDirEntry = (ResDirEntry *) (prf->pedit->pdir + 1);
   peWrite = pDirEntry;
   for (i = 0; i < prf->pedit->pdir->numEntries; i++)
   {
      if (pDirEntry->id)
      {
         if (pDirEntry != peWrite)
            *peWrite = *pDirEntry;
         peWrite++;
      }
      pDirEntry++;
   }
   prf->pedit->pdir->numEntries -= numReclaimed;

//  Set new current data offset

   prf->pedit->currDataOffset = dataWrite;
   lseek(prf->fd, dataWrite, SEEK_SET);
   prf->pedit->flags &= ~RFF_NEEDSPACK;

//  Truncate file to just header & data (will be extended later when
//  write directory on closing)

   chsize(prf->fd, dataWrite);

//  Return # bytes reclaimed

   Spew(DSRC_RES_Write, ("ResPack: reclaimed %d bytes\n", sizeReclaimed));

   return (sizeReclaimed);
}
#define SIZE_RESCOPY 32768
uchar *restemp_buffer = NULL;
int restemp_buffer_size = 0;

static void ResCopyBytes(int fd, long writePos, long readPos, long size)
{
   long sizeCopy;
   uchar *buff;
   bool alloc_buff = FALSE;
   if ((restemp_buffer == NULL) || (restemp_buffer_size < SIZE_RESCOPY))
   {
      buff = (uchar *) Malloc(SIZE_RESCOPY);
      alloc_buff = TRUE;
   }
   else
      buff = restemp_buffer;

   while (size > 0)
   {
      sizeCopy = min(SIZE_RESCOPY, size);
      lseek(fd, readPos, SEEK_SET);
      read(fd, buff, sizeCopy);
      lseek(fd, writePos, SEEK_SET);
      write(fd, buff, sizeCopy);
      readPos += sizeCopy;
      writePos += sizeCopy;
      size -= sizeCopy;
   }

   if (alloc_buff)
      Free(buff);
}
//  --------------------------------------------------------
//      INTERNAL ROUTINES
//  --------------------------------------------------------
//
//  ResEraseIfInFile() erases a resource if it's in a file's directory.
//
//      id = id of item
//
//  Returns: TRUE if found & erased, FALSE otherwise

bool ResEraseIfInFile(Id id)
{
   ResDesc *prd;
   ResFile *prf;
   ResDirEntry *pDirEntry;
   int i;
   prd = RESDESC(id);
   prf = &resFile[prd->filenum];
   pDirEntry = (ResDirEntry *) (prf->pedit->pdir + 1);

   for (i = 0; i < prf->pedit->pdir->numEntries; i++)
   {
      if (id == pDirEntry->id)
      {
         Spew(DSRC_RES_Write, ("ResEraseIfInFile: $%x being erased\n", id));
         pDirEntry->id = 0;
         prf->pedit->flags |= RFF_NEEDSPACK;
         if (prf->pedit->flags & RFF_AUTOPACK)
            ResPack(prd->filenum);
         return TRUE;
      }
      pDirEntry++;
   }

   return FALSE;
}
