//      ResCum.c        Resource System Cumulative Stats
//      Rex E. Bradford
/*
* $Header: x:/prj/tech/libsrc/res/RCS/rescum.cpp 1.5 1997/08/15 10:23:59 JAEMZ Exp $
* $Log: rescum.cpp $
 * Revision 1.5  1997/08/15  10:23:59  JAEMZ
 * *** empty log message ***
 * 
 * Revision 1.4  1996/10/10  16:13:32  TOML
 * msvc port
 * 
 * Revision 1.3  1996/09/14  16:09:54  TOML
 * Prepared for revision
 *
 * Revision 1.2  1996/09/14  14:12:49  TOML
 * Made C++ parser friendly
 *
 * Revision 1.1  1994/02/17  11:23:19  rex
 * Initial revision
 *
*/

#include <string.h>

#include <res.h>
#include <res_.h>

#ifdef DBG_ON

ResCumStat *pCumStatId;                          // ptr to cumulative stats by id
ResCumStat cumStatType[NUM_RESTYPENAMES];        // table of cum. stats by type

//  Internal routines

void TruncName(char *dest, char *src, int len);
//  Macro to multiply two longs and divide, and return full long

#ifdef __WATCOMC__
ulong resstat_mul_div(long m0, ulong m1, ulong d);
#pragma aux resstat_mul_div =\
   "mul    edx"     \
   "div    ebx"     \
   parm [eax] [edx] [ebx]  \
   modify [eax edx];
#else
__inline ulong resstat_mul_div(long m0, ulong m1, ulong d)
{
	__asm
	{
		mov	eax,m0
		mov	edx,m1
		mov	ebx,d
		mul	edx
		div	ebx
	}
}
#endif

//  --------------------------------------------------------------
//      CUMULATIVE STATS
//  --------------------------------------------------------------
//
//  ResAllocCumStatTable() allocates cum. stat table

void ResAllocCumStatTable()
{
   if (pCumStatId == NULL)
      pCumStatId = (ResCumStat *) Calloc((resDescMax + 1) * sizeof(ResCumStat));
}
//  --------------------------------------------------------------
//
//  ResSpewCumStats() spews out cumulative stats.

void ResSpewCumStats()
{
#ifdef DBG_ON
   static ulong totGets, totLocks, totExtracts, totLoads, totOverwrites, totPageouts;
   ResCumStat *prcs;
   char buff4[5];
   char pctBuff[8];
   ulong numAcc;
   Id id;
   int itype;
#endif

//  Put out individual stats

   DBG(DSRC_RES_CumStat,
   {
      Spew(DSRC_RES_CumStat, ("\nINDIVIDUAL RES STATS\n\n"));
      Spew(DSRC_RES_CumStat, ("\
  ID     #Gets    #Locks #Extracts #Loads #Overwrites #Pageouts PctAvail\n\
------------------------------------------------------------------------\n"));
      for (id = 0, prcs = pCumStatId; id <= resDescMax; id++, prcs++)
      {
         numAcc = prcs->numGets + prcs->numLocks;
         if ((numAcc + prcs->numExtracts) != 0)
         {
            if (numAcc == 0)
               strcpy(pctBuff, "****");
            else
               sprintf(pctBuff, ".%3d",
                       resstat_mul_div((numAcc - prcs->numLoads), 1000, numAcc));
            Spew(DSRC_RES_CumStat, ("%4x %9d %9d %9d %6d   %9d %9d   %s\n",
                                    id, prcs->numGets, prcs->numLocks, prcs->numExtracts,
                                    prcs->numLoads, prcs->numOverwrites, prcs->numPageouts,
                                    pctBuff));
         }
      }
   });

//  Put out type-based stats

   DBG(DSRC_RES_CumStat,
   {
      Spew(DSRC_RES_CumStat, ("\nTYPE-BASED RES STATS\n\n"));
      Spew(DSRC_RES_CumStat, ("\
   TYPE     #Gets    #Locks #Extracts #Loads #Overwrites #Pageouts PctAvail\n\
---------------------------------------------------------------------------\n"));
      for (itype = 0, prcs = cumStatType; itype < NUM_RESTYPENAMES; itype++,
           prcs++)
      {
         TruncName(buff4, resTypeNames[itype], 4);
         numAcc = prcs->numGets + prcs->numLocks;
         if ((numAcc + prcs->numExtracts) != 0)
         {
            if (numAcc == 0)
               strcpy(pctBuff, "****");
            else
               sprintf(pctBuff, ".%3d",
                       resstat_mul_div((numAcc - prcs->numLoads), 1000, numAcc));
            Spew(DSRC_RES_CumStat, ("%2d %s %9d %9d %9d %6d   %9d %9d   %s\n",
                                    itype, buff4, prcs->numGets, prcs->numLocks,
                                    prcs->numExtracts, prcs->numLoads, prcs->numOverwrites,
                                    prcs->numPageouts, pctBuff));
         }
      }
   });

//  Put out overall stats

   DBG(DSRC_RES_CumStat,
   {
      numAcc = 0;
      for (id = 0, prcs = pCumStatId; id <= resDescMax; id++, prcs++)
      {
         totGets += prcs->numGets;
         totLocks += prcs->numLocks;
         totExtracts += prcs->numExtracts;
         totLoads += prcs->numLoads;
         totOverwrites += prcs->numOverwrites;
         totPageouts += prcs->numPageouts;
         numAcc += prcs->numGets + prcs->numLocks;
      }
      if (numAcc == 0)
         strcpy(pctBuff, "****");
      else
         sprintf(pctBuff, ".%3d",
                 resstat_mul_div((numAcc - totLoads), 1000, numAcc));
      Spew(DSRC_RES_CumStat, ("\nOVERALL RES STATS\n\n\
                              # Gets:       %d\n\
                              # Locks:      %d\n\
                              # Extracts:   %d\n\
                              # Loads:      %d\n\
                              # Overwrites: %d\n\
                              # Pageouts:   %d\n\
                              PctAvail:     %s\n",
                              totGets, totLocks, totExtracts, totLoads, totOverwrites, totPageouts,
                              pctBuff));
   });

//  Put out paging stats

   Spew(DSRC_RES_CumStat, ("\nPAGING STATS\n\n\
                           # calls to page out: %d\n\
                           avg bytes needed:    %d\n\
                           avg bytes paged out: %d\n\
                           avg items paged out: %d\n",
                           resPageStats.numPageouts,
                           resPageStats.numPageouts ? (resPageStats.totSizeNeeded / resPageStats.numPageouts) : 0,
                           resPageStats.numPageouts ? (resPageStats.totSizeGotten / resPageStats.numPageouts) : 0,
                           resPageStats.numPageouts ? (totPageouts / resPageStats.numPageouts) : 0
                           ));

}
//  -----------------------------------------------------------
//      INTERNAL ROUTINES
//  -----------------------------------------------------------

static void TruncName(char *dest, char *src, int len)
{
   int i, j;
//  Copy from src to dest, truncate

   strncpy(dest, src, len);
   dest[len] = 0;

//  Now see if too short, if so pad with spaces

   for (i = 0; i < len; i++)
   {
      if (dest[i] == 0)
      {
         for (j = i; j < len; j++)
            dest[j] = ' ';
         return;
      }
   }
}
#endif
