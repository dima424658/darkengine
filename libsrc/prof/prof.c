/*
 *  Source-based profiler
 *
 *  $Header: x:/prj/tech/libsrc/prof/RCS/prof.c 1.7 1997/08/20 20:21:25 DAVET Exp $
 *  $Log: prof.c $
 * Revision 1.7  1997/08/20  20:21:25  DAVET
 * Added casts to quiet up w110
 * 
 * Revision 1.6  1995/03/07  11:07:32  buzzard
 * *** empty log message ***
 * 
 * Revision 1.5  1995/01/17  07:56:26  buzzard
 * fix typo
 * 
 * Revision 1.4  1995/01/17  07:55:51  buzzard
 * crash proof if not PROFILE_CHILD
 * 
 * Revision 1.3  1995/01/17  07:54:35  buzzard
 * moved analysis to separate tool
 * 
 * Revision 1.2  1995/01/16  15:33:50  buzzard
 * crash-proofed profiling before initialization
 * 
 * Revision 1.1  1995/01/16  14:12:29  buzzard
 * Initial revision
 * 
 *
 */

#define LG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LG
#include <lgsprntf.h>
#else
#define lg_sprintf sprintf
#endif


#define PROFILE_PROTOTYPES
#include <prof.h>


// make this one big since we can't check for overflow
#define MAX_TIMER_STACK		1024     // run-time parent stack, storage is *4

#define MAX_PROFILE_SITES       512      // number of modules, storage is *12
// this must be a power of 2
#define MAX_CALL_SITES          1024

//  Stack used by PROFILE_SELF
long **lPfStack;

//  Stack used by PROFILE_CHILD
ushort *lPfHashStack;

//  Profile site table
typedef struct {
   uchar *filename;
   int line;
   lPfData *timer;
} siteData;

static siteData *site;
static int site_count;

// Call site table
typedef struct {
    ushort child;
    ushort parent;
    long   time;
    long   count;
} callSiteData;

static callSiteData *hashTable;

static uchar initialized;
static long startTime;

void initProfiler(int mode)
{
   if (mode == PROFILE_MODE_NONE)
      return;

   if (initialized) return;

   site = malloc(MAX_PROFILE_SITES * sizeof(site[0]));
   
   if (!site) {
      // not enough memory to profile, oops!
      return;
   }

   if (mode == PROFILE_MODE_SELF) {
      lPfStack = malloc(sizeof(long *) * MAX_TIMER_STACK);         
      if (!lPfStack) {
         free(site);
         return;
      }
   }

   if (mode == PROFILE_MODE_CHILD) {
      lPfHashStack = malloc(sizeof(ushort) * MAX_TIMER_STACK);
      hashTable = malloc(sizeof(callSiteData) * MAX_CALL_SITES);
      if (!lPfHashStack || !hashTable) {
         if (lPfHashStack) free(lPfHashStack);
         if (hashTable) free(hashTable);
         free(site);
         return;
      }
      memset(hashTable, 0, sizeof(callSiteData) * MAX_CALL_SITES);
      *lPfHashStack = 0;
   }

   initialized = mode;
   startTime = RAW_TIME_VALUE;
}

void profileClearData(void)
{
   int i;
   if (initialized == PROFILE_MODE_CHILD) {
      for (i=0; i < MAX_CALL_SITES; ++i) {
         hashTable[i].time = 0;
         hashTable[i].count = 0;
      }
   }
   startTime = RAW_TIME_VALUE;
}

long profileRecordSource(char *name, int line, lPfData *profile)
{
   if (initialized && site_count < MAX_PROFILE_SITES) {
      site[site_count].filename =(uchar *) name;
      site[site_count].line = line;
      site[site_count].timer = profile;
      site_count++;
   }
   return RAW_TIME_VALUE;
}

static int hashSlot(ushort ch, ushort par)
{
   int x;
   x = ((par << 3) ^ ch) & (MAX_CALL_SITES - 1);

   while (hashTable[x].child) {
      if (hashTable[x].child == ch && hashTable[x].parent == par)
          return x;
      x = (x + 1) & (MAX_CALL_SITES-1);
   }

   // allocate a new hash slot

   hashTable[x].child = ch;
   hashTable[x].parent = par;

   hashTable[x].count = hashTable[x].time = 0;
   return x;
}

static long *hashTimerSlot(ushort ch, ushort par)
{
   int x = hashSlot(ch,par);
   return &hashTable[x].time;
}

long profileRecordHashInit(char *name, int line, lPfDataHash *profile)
{
   // Record site information for this guy
   if (initialized && site_count < MAX_PROFILE_SITES) {
      site[site_count].filename = (uchar *) name;
      site[site_count].line = line;
      site_count++;
      profile->hashCode = site_count;
      profile->parentHash = -1;
   }
      
   // now chain to the normal function
   return profileRecordHash(profile);
}

long profileRecordHash(lPfDataHash *profile)
{
   // check if the cached hash slot is still valid

   if (*lPfHashStack == profile->parentHash) {
      // it is, so the cache is set
      ++profile->time_ptr[1];   // increment counter
      *++lPfHashStack = profile->hashCode;
      return RAW_TIME_VALUE;
   }

   // find the right cache entry

   profile->parentHash = *lPfHashStack;
   profile->time_ptr = hashTimerSlot(profile->hashCode, profile->parentHash);
   ++profile->time_ptr[1];
   *++lPfHashStack = profile->hashCode;
   return RAW_TIME_VALUE;
}

void writeProfileRaw(FILE *f)
{
   int i;
   fprintf(f, "S-Prof sample file, mode %d.\n", initialized);
   fprintf(f, "Total ms: %d\n", RAW_TIME_VALUE - startTime);
   for (i=0; i < site_count; ++i) {
      fprintf(f, "%d %d %s\n", i+1, site[i].line, site[i].filename);
   }
   if (initialized == PROFILE_MODE_CHILD) {
      fprintf(f, "Child call data\n");
      for (i=0; i < MAX_CALL_SITES; ++i) {
         if (hashTable[i].child) {
            fprintf(f, "%5d %5d: %14d %8d\n", (int) hashTable[i].child, (int) hashTable[i].parent,
                  hashTable[i].time, hashTable[i].count);
         }
      }
   }
}

void writeProfile(char *name)
{
   FILE *f;
   if (!initialized) return;

   f = fopen(name, "w");
   if (f) {
      writeProfileRaw(f);
      fclose(f);
   }
}
