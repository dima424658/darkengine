/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/playtest.cpp,v 1.27 2000/02/24 23:33:43 patmac Exp $

#ifdef PLAYTEST

#include <time.h>
#include <objbase.h>
#include <playtest.h>
#include <command.h>
#include <ctype.h>
#include <config.h>

#include <appagg.h>
#include <status.h>
#include <appname.h>
#include <allocapi.h>
#include <dispapi.h>
#include <timer.h>
#include <simtime.h>

#include <objpos.h>
#include <wrtype.h>

#include <propman.h>
#include <propstat.h>
#include <property.h>
#include <trcachui.h>
#include <propinst.h>
#include <propsprs.h>
#include <prpstats.h>
#include <propbase.h>
#include <version.h>
#include <vernum.h>

#include <mprintf.h>

#include <report.h>
#include <miscrprt.h>

#include <loopapi.h>
#include <resapi.h>
#include <cacheapi.h>
#include <objmodel.h>

#include <litprop.h>

#include <dbmem.h>   // woo-woo

#ifdef DEBUG
EXTERN void DumpDynarraySummary();
#endif //DEBUG

//////////////////////////////
// malloc fun stuff

#ifndef SHIP
#define ShowK(x) ((x+1023)/1024)
void dump_alloc_cap()
{
   sAllocLimits tmp;
   AllocGetLimits(&tmp);
   mprintf("allocCap total %dk, cap %dk, init %dk, peak %dk\n",
           ShowK(tmp.totalAlloc),ShowK(tmp.allocCap),ShowK(tmp.initAllocCap),ShowK(tmp.peakAlloc));
}

void dump_heap_stats()
{
   g_pMalloc->DumpStats();
   dump_alloc_cap();
}

void dump_heap_modules()
{
   g_pMalloc->DumpModules();
}

void dump_heap_blocks()
{
   g_pMalloc->DumpBlocks();
}

void test_heap()
{
   g_pMalloc->VerifyHeap();
}

void dump_heap()
{
   g_pMalloc->DumpHeapInfo();
}
#else
#define dump_heap_stats()
#endif

///////////////////////////////////////////////////////////
// This propcedure displays the time in milliseconds
// that have been accumulated for each property by
// the property methods
///////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IPropertyStats);

static const char* prop_statnames[] =
{
   "Copy",
   "Donor",
   "Exemplar",
   "IterStart",
   "IterNext",
   "IterStop",
   "Create",
   "Delete",
   "Get",
   "Set",
   "Relevant",
   "Listeners",
   "Touch",
   "Requires",
   "AutoCreate",
   "Rebuild",
};

#define NUM_STATS (sizeof(prop_statnames)/sizeof(prop_statnames[0]))

static void dump_time_stats(const sPropTimeStats* stats, BOOL columns = FALSE)
{
   for (int i = 0; i < NUM_STATS; i++)
   {
      if (columns)
         mprintf("%8d",stats->time[i]);
      else
         mprintf("%s: %d\n",prop_statnames[i],stats->time[i]);
   }
   mprintf("\n");
}

static void dump_prop_stats(IProperty* pNamedProp)
{
   // If this property collects stats on itself, then it
   // has an IPropertyStats interface.
   IPropertyStats*  pStats;
   HRESULT result = pNamedProp->QueryInterface(IID_IPropertyStats,(void**)&pStats);

   // Check that the Query is valid.
   if (SUCCEEDED(result))
   {
      mprintf("Timing stats for property %s:\n",pNamedProp->Describe()->name);
      sPropTimeStats* TimeStats = pStats->GetTimeStats();
      dump_time_stats(TimeStats);
   }

   SafeRelease(pStats);
}

static ulong prop_stat_time = 0;

void GetStoreTime(char * arg)
{
   // punt trailing whitespace
   char* s;
   for (s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0';

   AutoAppIPtr_(PropertyManager,pMan);

   ulong time = tm_get_millisec_unrecorded();

   mprintf("Prop stats gathered over %ld msec\n",time-prop_stat_time);

   BOOL all = strcmp(arg,"all") == 0;

   if (*arg && !all)
   {
      IProperty* pNamedProp = pMan->GetPropertyNamed(arg);
      if (pNamedProp->GetID() != PROPID_NULL)
         dump_prop_stats(pNamedProp);
      else
         mprintf("No property named '%s'\n",arg);
   }
   else // no property specified, compute totals
   {
      sPropTimeStats totals;
      memset(&totals,0,sizeof(totals));

      if (all)
      {
         mprintf("%20s","Name");
         for (int i =0; i < NUM_STATS; i++)
         {
            char buf[9];
            strncpy(buf,prop_statnames[i],sizeof(buf));
            buf[sizeof(buf)-1] = '\0';
            mprintf("%8s",buf);
         }
         mprintf("\n");
      }

      sPropertyIter iter;
      pMan->BeginIter(&iter);
      IProperty* prop = pMan->NextIter(&iter);
      for (; prop != NULL; prop = pMan->NextIter(&iter))
      {
         IPropertyStats* pStats;
         HRESULT result = prop->QueryInterface(IID_IPropertyStats,(void**)&pStats);
         if (SUCCEEDED(result))
         {
            const sPropTimeStats* stats = pStats->GetTimeStats();
            for (int i = 0; i < kPropNumTimeStats; i++)
               totals.time[i] += stats->time[i];

            if (all)
            {
               mprintf("%20s",prop->Describe()->name);
               dump_time_stats(stats,TRUE);
            }

         }

         SafeRelease(prop);

      }
      mprintf("Total property times stats:\n");
      dump_time_stats(&totals);
   }

}

static void clear_prop_stats()
{
   AutoAppIPtr_(PropertyManager,pMan);
   sPropertyIter iter;
   pMan->BeginIter(&iter);
   IProperty* prop = pMan->NextIter(&iter);
   for (; prop != NULL; prop = pMan->NextIter(&iter))
   {
      IPropertyStats* pStats;
      HRESULT result = prop->QueryInterface(IID_IPropertyStats,(void**)&pStats);
      if (SUCCEEDED(result))
      {
         sPropTimeStats* stats = pStats->GetTimeStats();
         memset(stats,0,sizeof(*stats));
      }
      SafeRelease(prop);
   }

   prop_stat_time = tm_get_millisec_unrecorded();
}

////////////////////////////////////////

static void sparse_hash_stats(void)
{
   cObjPropDatumHashTable& table = cObjPropDatumHashTable::DefaultTable();
   mprintf("Sparse hash has %d entries out of %d\n",table.nElems(),table.TableSize());
}

extern "C" void hello_debugger(void)
{
   DebugBreak();
}

void hello_ms_profiler(void)
{
   mprintf("ms profiler wakeup");
}

void simpdbg_mclear(void)
{
   mono_clear();
}

void simpdbg_mprint(char *str)
{
   mprintf("%s\n",str);
}

// this actually requires some code
void simpdbg_mlog(char *str)
{
   if (strnicmp(str,"close",strlen("close"))==0)
      mono_logoff();
   else if (str)
      mono_logon(str,MONO_LOG_NEW,MONO_LOG_ALLWIN);
}

void _MallocPushCredit(const char * pszFile, int line)
{
   g_pMalloc->PushCredit(pszFile, line);
}

void _MallocPopCredit()
{
   g_pMalloc->PopCredit();
}


#ifndef SHIP
/////////////////////
// info on looptimings

static void looptime_cmd(int what_to_do)
{
   ILoop* pLoop = AppGetObj(ILoop);
   ILoopDispatch* pDisp = pLoop->GetCurrentDispatch();
   tLoopMessageSet msgs;
   unsigned diags;

   // NOTE: we have to do the crazy ClearTimers if we are disabling
   //  since otherwise you get spew next mode change (on destructors)
   pLoop->GetDiagnostics(&diags,&msgs);
   switch (what_to_do)
   {
      case 0: pDisp->DumpTimerInfo(); break;
      case 1: pDisp->ClearTimers(); break;
      case 2: pDisp->DumpTimerInfo(); pDisp->ClearTimers(); break;
      case 3: diags |= kLoopDiagTimings; break; // on
      case 4: diags &= ~kLoopDiagTimings; pDisp->ClearTimers(); break; // off
      case 5: // toggle
         if (diags&kLoopDiagTimings)
         {
            diags &= ~kLoopDiagTimings;
            pDisp->ClearTimers();  // so we dont get tons of spew
         }
         else
            diags |=  kLoopDiagTimings;
         break;
   }
   pLoop->SetDiagnostics(diags,msgs);
}

/////////////////////
// info on resource system usage/times

void resstats_cmd(int what_to_do)
{
   AutoAppIPtr(ResMan);
   IResStats *pResStats;
   Verify(SUCCEEDED(pResMan->QueryInterface(IID_IResStats, (void **) &pResStats)));
   switch (what_to_do)
   {
      case 0: // dump stats
         pResStats->Dump(NULL);
         break;
      case 1: // activate stats - does this need to be on startup?
         pResStats->SetMode(kResStatMode_Summary, TRUE);
         pResStats->SetMode(kResStatMode_Types, TRUE);
         break;
      case 2: // call into res
         ResSetPrintAccesses(1);
         ResSetPrintDrops(1);
         break;
      case 3: // call into res
         ResSetPrintAccesses(0);
         ResSetPrintDrops(0);
         break;
      case 4: // dump snapshot
         pResStats->DumpSnapshot(NULL);
         break;
      case 5: // flush then dump snapshot
         {
            AutoAppIPtr(SharedCache);
            BOOL old_drops=g_fResPrintAccesses;
            ResSetPrintDrops(1);
            pSharedCache->FlushAll();
            pResStats->DumpSnapshot(NULL);
            ResSetPrintDrops(old_drops);
            dump_heap_stats();
            break;
         }
      case 6: // just flush
         {
            AutoAppIPtr(SharedCache);
            pSharedCache->FlushAll();
         }
   }
   pResStats->Release();
}
#else
#define looptime_cmd(x)
#define resstats_cmd(x)
#endif

//////////////////////
// obj tracker

#define OBJTRACK_NO_SLOT (-1)
#define MAX_OBJS_TRACKED 16

static int _objsTrackedCount=0;
static int _objsTrackedIds[MAX_OBJS_TRACKED];

static int getTrackedObjSlot(int objid)
{
   int i;
   for (i=0; i<_objsTrackedCount; i++)
      if (objid==_objsTrackedIds[i])
         return i;
   return OBJTRACK_NO_SLOT;
}

static void AddTrackedObj(int objid)
{
   if (_objsTrackedCount==MAX_OBJS_TRACKED)
      Warning(("Out of object tracking slots\n"));
   else
      _objsTrackedIds[_objsTrackedCount++]=objid;
}

static void RemTrackedObj(int objid)
{
   int slot=getTrackedObjSlot(objid);
   if (slot!=OBJTRACK_NO_SLOT)
   {
      if (slot!=_objsTrackedCount-1)
         _objsTrackedIds[slot]=_objsTrackedIds[_objsTrackedCount-1];
      _objsTrackedCount--;
   }
   else
      Warning(("ObjID %d not in any tracking slot\n",objid));
}

static void ToggleTrackedObj(int objid)
{
   if (getTrackedObjSlot(objid)!=OBJTRACK_NO_SLOT)
      RemTrackedObj(objid);
   else
      AddTrackedObj(objid);
}

static void _showObjsTracked(void)
{
   int i, cur_time=GetSimTime();
   for (i=0; i<_objsTrackedCount; i++)
   {  // go find location for this object, other useful status pings
      ObjPos *p=ObjPosGet(_objsTrackedIds[i]);
      if (p)
      {
         char buf[256];
         mxs_vector *loc=&p->loc.vec;
         mxs_angvec *fac=&p->fac;
         sprintf(buf,"%3.3d %8.8d Tracked@ %g %g %g %x %x %x\n",_objsTrackedIds[i],cur_time,
                 loc->x,loc->y,loc->z,(ushort)(uint)fac->tx,(ushort)(uint)fac->ty,(ushort)(uint)fac->tz);
         mprint(buf);
      }
   }
}

//////////////////////
// timing tester

static int _timeHeartbeatFreq = 0;
static int _timeHeartbeatFrameCnt = 0;

static void _showNetTimes(void)
{
   if (_timeHeartbeatFreq)
      if (((++_timeHeartbeatFrameCnt)%_timeHeartbeatFreq)==0)
         mprintf("Times: %d %d %d\n",GetSimTime(),tm_get_millisec(),time(NULL));
   // should use QueryPerfomanceCounter, and get better data
}

//////////////////////
// per frame callback for stuff
#define kFrameStatsHeap 1
#define kFrameStatsRes  2
#define kFrameStatsLoop 4

static int _frameStatsLastTime=0;
static int _frameStatsHowLong=0;
static int _frameStatsWhich=1;

void PlaytestFrameReal()
{
   if (_frameStatsHowLong)
      if (_frameStatsLastTime+_frameStatsHowLong<tm_get_millisec_unrecorded())
      {
         if (_frameStatsWhich&kFrameStatsHeap)
            dump_heap_stats();
         if (_frameStatsWhich&kFrameStatsRes)
            resstats_cmd(0);
         if (_frameStatsWhich&kFrameStatsLoop)
            looptime_cmd(0);
         _frameStatsLastTime=tm_get_millisec_unrecorded();
      }
   _showObjsTracked();
   _showNetTimes();
}

//////////////////////
// idiocy helper function for CheatToggleBools

void CheatCheckToggleBool(bool *var, char *cmd, char *name)
{
   BOOL new_val=!(*var);
   if (!CheatsActive())
      return;
   if (cmd&&(cmd[0]))
      if (cmd[0]=='0')
         new_val=FALSE;
      else if (cmd[0]=='1')
         new_val=TRUE;
   *var=new_val;
   mprintf("Set %s to %d\n",name?name:"Unknown",new_val);
}

/////////////////////
// cheat idiocy

#ifdef EDITOR
BOOL cheats_active=TRUE;
#else
BOOL cheats_active=FALSE;
#endif

static int ComputeCheatCode(void)
{
   const Version *ver=AppVersion();
   int vtotal=(ver->num.minor+1)*(ver->num.major+1);
   int utotal=0;

#ifdef HARD_CODE
   time_t now = time(NULL);
   struct tm *cur_tm=localtime( &now );
   vtotal=ver->num.minor+1;
   utotal=cur_tm->tm_mday+1;
#else
   char *username=getenv("USER");
   if (username)
      utotal=strlen(username);
   else
      utotal=0;
#endif
   return (utotal*vtotal)+1000;
}

static void EnableCheats(int val)
{
   if (val==ComputeCheatCode())
   {
      mprintf("Cheats Enabled\n");
      // clearly need a sound effect here!
      cheats_active=TRUE;
   }
   else
      cheats_active=FALSE;
}

static void DoCheatCompute(void)
{
   mprintf("Cheat for you this version is %d\n",ComputeCheatCode());
}

extern "C" bool show_stats, show_full_stats, time_stats, tmgr_stats;
extern "C" bool ambient_mono, ambient_heartbeat;
extern "C" BOOL g_ShowAmbients;

EXTERN void AmbSoundDump(void);
EXTERN void ResetObjTimerStats();

static void
toggle_show_stats( void ) {
#ifdef SHIP
   if ( config_is_defined( "call_me_fishmael" ) )
#endif
      show_stats = !show_stats;
}

#define CHEAT_STRING "txm_toggle"

#if !defined(SHIP) && defined(DARK)
EXTERN BOOL listenFade;  // from drksound
#endif

//
// Commands
//

static Command commands[] =
{
   { "flip_highpoly", FUNC_BOOL, objmodelDetailSetCommand },

   // for now i want this in in case we have more light wackiness
   { "anim_light_reset", FUNC_VOID, ObjAnimLightReset },

#if !defined(SHIP) && defined(DARK)
   { "listen_no_fades", TOGGLE_BOOL, &listenFade },
#endif

#ifndef SHIP
   { "heap_alloc_cap", FUNC_VOID, dump_alloc_cap, "dump alloc cap", HK_ALL},
   { "heap_dump_stats", FUNC_VOID, dump_heap_stats, "dump heap stats", HK_ALL},
   { "heap_dump_modules", FUNC_VOID, dump_heap_modules, "dump heap usage by module", HK_ALL},
   { "heap_dump_blocks", FUNC_VOID, dump_heap_blocks, "dump all allocated heap blocks", HK_ALL},
   { "heap_dump_all", FUNC_VOID, dump_heap, "dump all allocated heap blocks", HK_ALL},
   { "heap_test", FUNC_VOID, test_heap, "validate the heap", HK_ALL},
   { "obj_time_stats", FUNC_VOID, ResetObjTimerStats, "Clear object creation time stats, and dump them to mono.", HK_ALL },
#endif

   { "ambient_spew", TOGGLE_BOOL, &ambient_mono },
   { "ambient_heartbeat", TOGGLE_BOOL, &ambient_heartbeat },
   { "ambient_show", TOGGLE_BOOL, &g_ShowAmbients },
   { "ambient_dump", FUNC_VOID, AmbSoundDump },

   { "track_obj", FUNC_INT, ToggleTrackedObj, "toggle tracking on a given obj" },
   { "track_add_obj", FUNC_INT, AddTrackedObj },
   { "track_rem_obj", FUNC_INT, RemTrackedObj },
   { "time_print", VAR_INT, &_timeHeartbeatFreq, "set freq of time mprints" },


   { "trait_cache_stats", FUNC_VOID, DumpDonorCacheStats, "Dump trait cache stats to mono", HK_ALL },
   { "trait_id_stats", FUNC_INT, GetTraitStats, "Get trait statistics", HK_ALL },
   { "trait_name_stats", FUNC_STRING, GetNamedStats, "Get trait statistics by name", HK_ALL },
   { "trait_max_id", FUNC_VOID, GetMaxID, "Get max trait ID", HK_ALL },
   { "trait_cache_fullness", FUNC_VOID, GetCacheEntryCount, "Get cache entry count", HK_ALL },
   { "prop_time_stats", FUNC_STRING, GetStoreTime, "Get property storage timing statistics.", HK_ALL },
   { "prop_time_stats_clear", FUNC_STRING, clear_prop_stats, "Clear property timing statistics.", HK_ALL },
   { "prop_blame", TOGGLE_BOOL, &gBlameProperties, "Blame properties for memory", HK_ALL },
   { "sparse_hash_stats", FUNC_VOID, sparse_hash_stats, "List sparse hash stats", HK_ALL },
   { "start_ms_profile", FUNC_VOID, hello_ms_profiler },
   { "hello_debugger", FUNC_VOID, hello_debugger, "Hard-coded breakpoint" },
   { "mclear", FUNC_VOID, simpdbg_mclear, "clear mono" },
   { "mprint", FUNC_STRING, simpdbg_mprint, "print mono string" },
   { "mlog", FUNC_STRING, simpdbg_mlog, "monolog state: close closes, name opens" },
   { "show_stats", FUNC_VOID, toggle_show_stats, "frame rate/scene info" },
   { "stats_full", TOGGLE_BOOL, &show_full_stats, "stat verbosity" },
   { "time_stats", TOGGLE_BOOL, &time_stats, "more stat fun, mprint timings" },
   { "tmgr_stats", TOGGLE_BOOL, &tmgr_stats, "mprint texture manager stats" },

   { "playtest_stats", VAR_INT, &_frameStatsHowLong, "set frequency for per frame stats" },
   { "playtest_which", VAR_INT, &_frameStatsWhich, "which: 1 heap, 2 resource, 4 looptime" },

#ifndef SHIP
   { "looptime_cmd", FUNC_INT, looptime_cmd, "0 dump, 1 clear, 2 both, 3 on, 4 off, 5 toggle" },
   { "resstats_cmd", FUNC_INT, resstats_cmd, "0 dump, 2 tracking, 3 cleartrack, 4 dump, 5 flush/dump" },
#endif

#ifdef DEBUG
   { "dynarr_sum", FUNC_VOID, DumpDynarraySummary, ""},
#endif

   { CHEAT_STRING, FUNC_INT, EnableCheats, ""},
#ifdef EXPOSE_CHEAT
   { "xyzzy", FUNC_VOID, DoCheatCompute, ""},
#endif
};

void PlaytestInitReal()
{
   COMMANDS(commands,HK_ALL);
   ReportInit();
   MiscReportFuncsInit();
   if (config_is_defined(CHEAT_STRING))
   {
      int cheat_val=0;
      config_get_int(CHEAT_STRING,&cheat_val);
      EnableCheats(cheat_val);
   }
   if (config_is_defined("language"))
   {
      char buf[128];
      config_get_raw("language",buf,128);
      if (stricmp(buf,"english")!=0)
         cheats_active=TRUE;
   }
}

void PlaytestTermReal()
{
   MiscReportFuncsTerm();
   ReportTerm();
}
#endif
