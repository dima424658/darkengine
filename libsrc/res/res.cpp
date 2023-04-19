//      Res.C       Resource Manager primary access routines
//      Rex E. Bradford (REX)
//
//      See the doc RESOURCE.DOC for information.
/*
* $Header: x:/prj/tech/libsrc/res/RCS/res.cpp 1.33 1997/02/24 23:49:27 TOML Exp $
* $Log: res.cpp $
 * Revision 1.33  1997/02/24  23:49:27  TOML
 * New resource cache logic
 * 
 * Revision 1.32  1997/01/20  14:49:41  TOML
 * removed reqs
 *
 * Revision 1.31  1997/01/17  17:30:44  TOML
 * *** empty log message ***
 *
 * Revision 1.30  1996/11/21  17:14:54  mahk
 * Broke out ResCreateInternal
 *
 * Revision 1.29  1996/09/20  15:11:58  TOML
 * Removed ARQ references
 *
 * Revision 1.28  1996/09/14  16:09:58  TOML
 * Prepared for revision
 *
 * Revision 1.27  1996/09/14  14:12:55  TOML
 * Made C++ parser friendly
 *
 * Revision 1.26  1996/08/30  10:24:45  TOML
 * Made thread safe
 *
 * Revision 1.25  1994/09/22  10:47:56  rex
 * Added secondary resdesc table, in shared buffer with first
 *
 * Revision 1.24  1994/07/15  18:19:33  xemu
 * added ResShrinkResDescTable
 *
 * Revision 1.23  1994/05/26  13:51:55  rex
 * Added ResInstallPager(ResDefaultPager) to ResInit()
 *
 * Revision 1.22  1994/02/17  11:24:51  rex
 * Moved most funcs out into other .c files
 *
*/

#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <res.h>
#include <res_.h>
#include <lzw.h>
#include <memall.h>

//  The resource descriptor table

ResDesc *gResDesc;                               // ptr to array of resource descriptors
ResDesc2 *gResDesc2;                             // secondary array, shared buff with resdesc
Id resDescMax;                                   // max id in res desc
#define DEFAULT_RESMAX 1023     // default max resource id
#define DEFAULT_RESGROW 1024        // grow by blocks of 1024 resources
                                            // must be power of 2!
//  Some variables

ResStat resStat;                                 // stats held here

//  ---------------------------------------------------------
//      INITIALIZATION AND TERMINATION
//  ---------------------------------------------------------
//
//  ResInit() initializes resource manager.

void ResInitInternal(void)
{
   char *p;
   int i;

   ResMemInit();
   LzwInit();

//  Allocate initial resource descriptor table, default size (can't fail)

   resDescMax = DEFAULT_RESMAX;
   gResDesc = (ResDesc *) Calloc((DEFAULT_RESMAX + 1) *
                                 (sizeof(ResDesc) + sizeof(ResDesc2)));
   gResDesc2 = (ResDesc2 *) (gResDesc + (DEFAULT_RESMAX + 1));

#if 0
   gResDesc[ID_HEAD].prev = 0;
   gResDesc[ID_HEAD].next = ID_TAIL;
   gResDesc[ID_TAIL].prev = ID_HEAD;
   gResDesc[ID_TAIL].next = 0;
#endif

//  Clear file descriptor array

   for (i = 0; i <= MAX_RESFILENUM; i++)
      resFile[i].fd = -1;

//  Add directory pointed to by RES env var to search path

   p = getenv("RES");
   if (p)
      ResAddPath(p);

   Spew(DSRC_RES_General, ("ResInit: res system initialized\n"));

}

void ResInit(void)
{
   AtExit(ResTerm);
   ResInitInternal();
}

//  ---------------------------------------------------------
//
//  ResTerm() terminates resource manager.

void ResTerm()
{
   int i;

//  Close all open resource files

   for (i = 0; i <= MAX_RESFILENUM; i++)
   {
      if (resFile[i].fd >= 0)
         ResCloseFile(i);
   }

//  Spew out cumulative stats if want them

   DBG(DSRC_RES_CumStat,
   {
      ResSpewCumStats();
   });

//  Free up resource descriptor table

   if (gResDesc)
   {
      Free(gResDesc);
      gResDesc = NULL;
      gResDesc2 = NULL;
      resDescMax = 0;
   }

   ResMemTerm();

//  We're outta here

   Spew(DSRC_RES_General, ("ResTerm: res system terminated\n"));
}
//  ---------------------------------------------------------
//
//  ResGrowResDescTable() grows resource descriptor table to
//  handle a new id.
//
//  This routine is normally called internally, but a client
//  program may call it directly too.
//
//      id = id

void ResGrowResDescTable(Id id)
{
   long newAmt, currAmt;
   ResDesc2 *pNewResDesc2;
//  Calculate size of new table and size of current

   newAmt = (id + DEFAULT_RESGROW) & ~(DEFAULT_RESGROW - 1);
   currAmt = resDescMax + 1;

//  If need to grow, do it

   if (newAmt > currAmt)
   {
      Spew(DSRC_RES_General,
           ("ResGrowResDescTable: extending to $%x entries\n", newAmt));

//  Realloc double-array buffer and check for error

      gResDesc = (ResDesc *) Realloc(gResDesc, newAmt *
                                     (sizeof(ResDesc) + sizeof(ResDesc2)));
      if (gResDesc == NULL)
      {
         Warning(("ResGrowDescTable: RES DESCRIPTOR TABLE BAD!!!\n"));
         return;
      }

//  Compute new location for gResDesc2[] array at top of buffer,
//  and move the gResDesc2[] array up there

      gResDesc2 = (ResDesc2 *) (gResDesc + currAmt);
      pNewResDesc2 = (ResDesc2 *) (gResDesc + newAmt);
      memmove(pNewResDesc2, gResDesc2, currAmt * sizeof(ResDesc2));
      gResDesc2 = pNewResDesc2;

//  Clear extra entries in both tables

      memset(gResDesc + currAmt, 0, (newAmt - currAmt) * sizeof(ResDesc));
      memset(gResDesc2 + currAmt, 0, (newAmt - currAmt) * sizeof(ResDesc2));

//  Set new max id limit

      resDescMax = newAmt - 1;

//  Grow cumulative stats table too

      DBG(DSRC_RES_CumStat,
          {
         if (pCumStatId == NULL)
            ResAllocCumStatTable();
         else
         {
            pCumStatId = (ResCumStat *) Realloc(pCumStatId, newAmt * sizeof(ResCumStat));
            if (pCumStatId == NULL)
            {
               Warning(("ResGrowDescTable: RES CUMSTAT TABLE BAD!!!\n"));
               return;
            }
            memset(pCumStatId + currAmt, 0, (newAmt - currAmt) *
                   sizeof(ResCumStat));
         }
      });
   }
}
//  ---------------------------------------------------------
//
//  ResShrinkResDescTable() resizes the descriptor table to be
//  the minimum allowable size with the currently in-use resources.
//

void ResShrinkResDescTable()
{
   long newAmt, currAmt;
   // id is the largest used ID
   Id id;
   ResDesc2 *pNewResDesc2;
// Calculate largest used ID
   id = resDescMax;
   while ((id > ID_MIN) && (!ResInUse(id)))
      id--;
   Spew(DSRC_RES_General, ("largest ID in use is %x.\n", id));

//  Calculate size of new table and size of current

   newAmt = (id + DEFAULT_RESGROW) & ~(DEFAULT_RESGROW - 1);
   currAmt = resDescMax + 1;

//  If need to shrink do it
// note that we don't decrease the stat table

   if (currAmt > newAmt)
   {
      Spew(DSRC_RES_General,
           ("ResGrowResDescTable: extending to $%x entries\n", newAmt));

//  Move gResDesc2[] array down to new place in buffer

      pNewResDesc2 = (ResDesc2 *) (gResDesc + newAmt);
      memmove(pNewResDesc2, gResDesc2, newAmt * sizeof(ResDesc2));

//  Shrink down double-array buffer

      gResDesc = (ResDesc *) Realloc(gResDesc, newAmt * sizeof(ResDesc));
      if (gResDesc == NULL)
      {
         Warning(("ResGrowDescTable: RES DESCRIPTOR TABLE BAD!!!\n"));
         return;
      }

//  Set new gResDesc2 ptr

      gResDesc2 = (ResDesc2 *) (gResDesc + newAmt);

//  Set new max id limit

      resDescMax = newAmt - 1;
   }
}
