/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicmds.cpp,v 1.59 2000/03/21 12:24:12 bfarquha Exp $
//
// @TBD (toml 06-05-98): ifndef SHIP most of this
//

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>
#include <str.h>
#include <hashfns.h>
#include <rnd.h>
#include <random.h>

#include <command.h>
#include <objpos.h>

#include <ai.h>
#include <aiapi.h>
#include <aiapiiai.h>
#include <aiapisns.h>
#include <aicmds.h>
#include <aiman.h>
#include <aipathdb.h>
#include <aidbgcmp.h>
#include <aidebug.h>
#include <aipthdbg.h>
#include <aipthloc.h>
#include <aiprconv.h>
#include <aiprcore.h>
#include <aicbhtoh.h>
#include <aisched.h>
#include <aisignal.h>
#include <aisound.h>
#include <aiutils.h>

#include <link.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <aiactloc.h>

// for the CheckLight hack
#ifndef SHIP
#include <objedit.h>
#include <aiwr.h>
#include <playrobj.h>
#include <aivision.h>
#include <objlight.h>
#endif

#ifdef EDITOR
#include <viewmgr.h>
#endif

#include <playtest.h>

// Must be last header
#include <dbmem.h>

extern void AIDebugDumpCellVertexInfo(int cell_id);
extern void AIDebugDumpCellConnectionInfo(int cell_id);

extern BOOL g_fAIForgetPlayer;
extern BOOL g_fAIUsePathZones;

#ifndef SHIP
///////////////////////////////////////////////////////////////////////////////

// if you add another "thing you can objId watch", add it to this array
// you will have to do a custom command
// we might want to add a "set_debug_watch name objID" and then a string mapping to
//  this array, but not needed for now, clearly
static ObjID *_all_debug_watches[]=
 { &g_AIAlertWatchObj, &g_AIFlowWatchObj, &g_AISoundWatchObj, &g_AICbtWatchObj,
   &g_AIPathWatchObj, &g_AIDeathWatchObj, &g_AISignalWatchObj, &g_AIHearWatchObj,
   &g_AIRangedWatchObj, &g_AIInformWatchObj, &g_AISightWatchObj };

static void AIToggleWatchAll(int objId)
{
   int i, cnt=sizeof(_all_debug_watches)/sizeof(_all_debug_watches[0]);
   BOOL fWatching = FALSE;
   ObjID use_obj;

   for (i=0; i<cnt; i++)
      fWatching |= (*_all_debug_watches[i])!=OBJ_NULL;

   if (fWatching)
   {
      mprintf("Disabled watch on %d\n", objId);
      use_obj = OBJ_NULL;
   }
   else
   {
      mprintf("Enabling full watch on %d\n", objId);
      use_obj = objId;
   }

   for (i=0; i<cnt; i++)
      *_all_debug_watches[i] = use_obj;
}

///////////////////////////////////////////////////////////////////////////////

static void AIDumpComps(int objId)
{
   AIBeginDump(objId);

   tAIIter        iter;
   IAIComponent * pComponent = AIDumpSrc()->GetFirstComponent(&iter);

   if (pComponent)
      AIDump("Components:");

   while (pComponent)
   {
      AIDump1("  %s", pComponent->GetName());

      pComponent->Release();
      pComponent = AIDumpSrc()->GetNextComponent(&iter);
   }

   AIDumpSrc()->GetComponentDone(&iter);

   AIEndDump();
}

///////////////////////////////////////////////////////////////////////////////

static void AIDumpAll(int objId)
{
   AIDumpComps(objId);
}

///////////////////////////////////////////////////////////////////////////////

extern ObjID g_AIBreakID;

static void AIBreak(int objId)
{
   g_AIBreakID = objId;
}

///////////////////////////////////////////////////////////////////////////////

static void AISetSchedule(int budget)
{
   g_AIScheduleSettings.budget = budget;
   g_AIScheduleSettings.fActive = !!g_AIScheduleSettings.budget;
}

///////////////////////////////////////////////////////////////////////////////

static void AIStandardDebugMode()
{
   CommandExecute("ai_aware_of_player");
   CommandExecute("light_bright");
   CommandExecute("ai_draw");
}

///////////////////////////////////////////////////////////////////////////////

static void AISleepAll()
{
   const cAIArray & AIs = g_pAIManager->AccessAIs();
   for (int i = 0; i < AIs.Size(); i++)
   {
      g_pAIModeProperty->Set(AIs[i]->GetID(), kAIM_Asleep);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void AIWakeAll()
{
   const cAIArray & AIs = g_pAIManager->AccessAIs();
   for (int i = 0; i < AIs.Size(); i++)
   {
      if (AIGetMode(AIs[i]->GetID()) == kAIM_Asleep)
      {
         g_pAIModeProperty->Set(AIs[i]->GetID(), kAIM_Normal);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

static void AIWake(int obj)
{
   cAI * pAI = g_pAIManager->AccessAI(obj);
   if (pAI && AIGetMode(obj) == kAIM_Asleep)
   {
      g_pAIModeProperty->Set(obj, kAIM_Normal);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void AIPlayerSound(int type)
{
//   mprintf("Player makes noise of type %d\n", type);
   AutoAppIPtr(AIManager);
   if (!!pAIManager)
   {
      //pAIManager->NotifySound((sSoundInfo *)type);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void AISignal(const char * pszSignal)
{
   mprintf("Sending signal %s to all AIs\n", pszSignal);

   sAISignal signal;
   signal.name = pszSignal;
   const cAIArray & AIs = g_pAIManager->AccessAIs();
   for (int i = 0; i < AIs.Size(); i++)
   {
      AIs[i]->NotifySignal(&signal);
   }
}

///////////////////////////////////////////////////////////////////////////////

static void AICheckLight(int obj)
{
   if (obj==0) obj=PlayerObject();
   if (obj==OBJ_NULL)
   {
      mprintf("No object to get data for\n");
      return;
   }
   else
      mprintf("We got raw %d for obj %s\n",(int)(100*AIGetObjectLighting(obj)),ObjEditName(obj));
   sAIVisibility * pVisibility = AIGetVisibility(obj);
   if (!pVisibility)
      mprintf("No current visibility data\n");
   else
      mprintf("current property says light %d move %d expose %d total %d\n",
              pVisibility->lighting, pVisibility->movement, pVisibility->exposure, pVisibility->level);
}

static void AIBlameLight(int obj)
{
   if (obj==0) obj=PlayerObject();
   if (obj==OBJ_NULL)
   {
      mprintf("No object to get data for\n");
      return;
   }
   blame_object_lighting(obj);
}

///////////////////////////////////////////////////////////////////////////////

static void AIConvStart(ObjID objID)
{
   g_pAIManager->StartConversation(objID);
}

///////////////////////////////////////////////////////////////////////////////

EXTERN Label g_TestMotionName;

static void AITestMotion(char *str)
{
   if(str)
   {
      strncpy(g_TestMotionName.text,str,15);
      g_TestMotionName.text[15]='\0';
   } else
   {
      g_TestMotionName.text[0]='\0';
   }
}

///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG


static void TestRand()
{
}
#endif // DEBUG

///////////////////////////////////////////////////////////////////////////////

static void AIAwareToggle(char *str)
{
   CheatCheckToggle(&g_fAIAwareOfPlayer,str,"AI Aware");
}

static void AIForgetToggle(char *str)
{
   CheatCheckToggle(&g_fAIForgetPlayer,str,"AI Forget");
}
#endif // !SHIP

#ifdef EDITOR
static void AICamToCell(int cell)
{
   if (cell < 1 || cell > g_AIPathDB.m_nCells)
   {
      mprintf("Invalid cell\n");
      return;
   }
   AIDebugMagicLocatorCell = cell;

   mxs_vector *pos;
   mxs_angvec *ang;

   if (vm_spotlight_loc(&pos,&ang))
   {
      if ((pos!=NULL)&&(ang!=NULL))
      {
         *pos=g_AIPathDB.GetCenter(cell);
         pos->z += 1.0;

         vm_redraw_from_camera();
      }
   }
}

static void AIPrintCamCell()
{
   mxs_vector *pos;
   mxs_angvec *ang;

   if (vm_spotlight_loc(&pos,&ang))
   {
      if ((pos!=NULL)&&(ang!=NULL))
      {
         mprintf("Camera is in cell %d\n", AIFindClosestCell(*pos, 0));
         return;
      }
   }

   mprintf("Could not obtain camera location\n");
}


#endif


#ifndef SHIP

static void AIPrintIffyCells(float testVal)
{
   cDynArray<tAIPathCellID> failures;

   if (testVal == 0)
      testVal = 0.666;

   ValidateAllCellSpaces(testVal, TRUE, &failures);
   if (failures.Size())
   {
      mprintf("The following AI cells look unsafe for AI passage:\n");
      for (int i = 0; i < failures.Size(); i++)
      {
         mprintf("   %d (%d, %d, %d)\n", failures[i], (int)g_AIPathDB.GetCenter(failures[i]).x, (int)g_AIPathDB.GetCenter(failures[i]).y, (int)g_AIPathDB.GetCenter(failures[i]).z);
      }
   }
   else
      mprintf("There are no questionable AI cells\n");

   failures.SetSize(0);
   ValidateAllCellRooms(TRUE, &failures);
   if (failures.Size())
   {
      mprintf("The following AI cells are not in the room database:\n");
      for (int i = 0; i < failures.Size(); i++)
      {
         mprintf("   %d (%d, %d, %d)\n", failures[i], (int)g_AIPathDB.GetCenter(failures[i]).x, (int)g_AIPathDB.GetCenter(failures[i]).y, (int)g_AIPathDB.GetCenter(failures[i]).z);
      }
   }
   else
      mprintf("All AI cells are in the room database\n");
}


ObjID g_AIRecoverTestPost;
ObjID g_AIRecoverTestDest;
ObjID g_AIRecoverTestResult;

void AITestRecover()
{
   if (g_AIRecoverTestPost && g_AIRecoverTestDest && g_AIRecoverTestResult)
   {
      tAIPathCellID resultCell;
      mxs_vector resultLoc;
      if (AIFindClosestConnectedCell(AIFindClosestCell(*GetObjLocation(g_AIRecoverTestPost), 0),
                                      *GetObjLocation(g_AIRecoverTestPost),
                                      *GetObjLocation(g_AIRecoverTestDest),
                                      &resultCell,
                                      &resultLoc,
                                      0))
      {
         mprintf("Result: %d, [%d, %d, %d]\n", resultCell, (int)resultLoc.x, (int)resultLoc.y, (int)resultLoc.z);
         ObjTranslate(g_AIRecoverTestResult, &resultLoc);
      }
      else
         mprintf("Test failed\n");
   }
   else
      mprintf("Cannot test\n");
}

#endif



///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP

#ifdef EDITOR
// from aipathdb:
extern BOOL g_fAIPDB_fUseLVLs;
extern BOOL g_fAIPDB_EnableHighStrike;

// from aipthfnd:
extern BOOL g_fAIPDB_UseDoubleFind;
#endif

EXTERN IIntProperty *g_pAIFleePointProperty;
static void AIValidateFleePoints()
{
   sPropertyObjIter iter;
   ObjID iterObj;
   mxs_vector iterPoint;
   tAIPathCellID hint;

   g_pAIFleePointProperty->IterStart(&iter);
   while (g_pAIFleePointProperty->IterNext(&iter, &iterObj))
   {
      if (!OBJ_IS_CONCRETE(iterObj))
         continue;

      GetObjLocation(iterObj, &iterPoint);
      if ((fabs(iterPoint.x) < 0.00001) &&
          (fabs(iterPoint.y) < 0.00001) &&
          (fabs(iterPoint.z) < 0.00001))
      {
         mprintf("FleePoint %d at position %d, %d, %d is likely not in portal database\n", iterObj, iterPoint.x, iterPoint.y, iterPoint.z);
         continue;
      }

      hint = 0;
      if (!AIFindClosestCell(iterPoint, hint, iterObj))
      {
         mprintf("*** FleePoint %d is NOT in pathfinding database\n", iterObj);
         continue;
      }

      mprintf("FleePoint %d PASSES TEST\n", iterObj);
   }

   g_pAIFleePointProperty->IterStop(&iter);
}



#include <aipthfnd.h>
static void GenericTest()
{
   mxs_vector fromLocation, toLocation;
   float fDist;
   tAIPathCellID startCell;
   tAIPathCellID endCell;
   tAIPathOkBits OkBits;

   fromLocation.x = -58.566;
   fromLocation.y = -132.013;
   fromLocation.z = -3.516;
   startCell = 0;

   toLocation.x = -94.1700;
   toLocation.y = -142.670;
   toLocation.z = 3.0000;
   endCell = 0;

   OkBits = kAIOK_Walk | kAIOKCOND_Stressed;

   IAIPath *pPath = AIPathfind(fromLocation, startCell,
                     toLocation, endCell,
                     OkBits,
                     (IAIPathfindControl *)NULL);
}

// from aipathdb.cpp:
extern float * g_pLargeDoorSize;

static void AIReportLargeDoorSize()
{
	mprintf("Large Door Size = %g\n", *g_pLargeDoorSize);
}


#ifdef EDITOR
extern BOOL g_bDoLinkOptimize;
#endif


#ifndef SHIP
// from AI.CPP
extern eAIMode g_ForceAIMode;
static void ForceAIMode(int nMode)
{
   if (nMode < 0)
      g_ForceAIMode = kAIM_Asleep;
   else if (nMode > kAIM_Num)
      g_ForceAIMode = kAIM_Num;
   else
      g_ForceAIMode = (eAIMode)nMode;
}
#endif

#ifndef SHIP
// From aiactloc.cpp:
extern BOOL g_bForcePathfind;
#endif

// from
static Command g_AICommands[] =
{
#ifdef EDITOR
   { "ai_build_path_database", FUNC_VOID, AIPathFindDBBuild, "Update the AI path database",          HK_ALL},
   { "ai_use_zones",           TOGGLE_BOOL, &g_fAIUsePathZones,  "Toggle use of AI zones",          HK_ALL},

   // pathdb building and traversing:
   { "ai_toggle_db_lvl", TOGGLE_BOOL, &g_fAIPDB_fUseLVLs,
     "Toggle LVL path db building", HK_EDITOR },

   { "ai_toggle_db_highstrike", TOGGLE_BOOL, &g_fAIPDB_EnableHighStrike,
     "Toggle HighStrike path db building", HK_EDITOR },

   { "ai_toggle_use_lvl", TOGGLE_BOOL, &g_fAIPDB_UseDoubleFind,
     "Toggle LVL path db usage", HK_ALL },

   { "ai_toggle_linkopt", TOGGLE_BOOL, &g_bDoLinkOptimize,
     "Toggle link optimization", HK_ALL },

//   { "ai_large_door_size", VAR_FLOAT, &g_fLargeDoorSize,
//     "Set large door size", HK_EDITOR },

   { "ai_report_large_door_size", FUNC_VOID, AIReportLargeDoorSize,
     "Report large door size", HK_ALL },

#endif

#ifndef SHIP
   { "ai_repath",          TOGGLE_BOOL, &g_bForcePathfind, "Force repath", HK_ALL},

   { "ai_mode",            FUNC_INT,    ForceAIMode,  "Set AI mode", HK_ALL},
#endif

   { "ai_draw",            TOGGLE_BOOL, &g_AIDrawAIs,          "Draw AI debug info", HK_ALL},
   { "ai_draw_in_game",    TOGGLE_BOOL, &gAIDebugDrawInGame,    "Draw AI debug info in 3D view", HK_ALL},

   { "ai_draw_move_goal",  TOGGLE_BOOL, &g_AIDebugDrawMoveGoal,    "", HK_ALL},
   { "ai_draw_suggestions",TOGGLE_BOOL, &g_AIDebugDrawSuggs,    "", HK_ALL},
   { "ai_draw_paths",      TOGGLE_BOOL, &g_AIDebugDrawPaths,    "", HK_ALL},

   { "ai_aware_of_player", FUNC_STRING, AIAwareToggle, "Toggle AI awareness of player", HK_ALL},
   { "aiawareofplayer",    FUNC_STRING, AIAwareToggle, "Toggle AI awareness of player", HK_ALL},
   { "aiforgetplayer",     FUNC_STRING, AIForgetToggle, "Toggle AI forget of player", HK_ALL},
   { "ai_forget_player",   FUNC_STRING, AIForgetToggle, "Toggle AI forget of player", HK_ALL},

   { "aidebugmode",        FUNC_VOID, AIStandardDebugMode, "Tom's standard AI debug mode", HK_ALL},

   { "ai_sleep_all",       FUNC_VOID, AISleepAll, "Put all AIs to sleep", HK_ALL},
   { "ai_wake_all",        FUNC_VOID,  AIWakeAll, "Wake all sleeping AIs", HK_ALL},
   { "ai_wake",            FUNC_INT,  AIWake, "Wake a sleeping AI", HK_ALL},

   { "ai_check_lighting",  FUNC_INT, AICheckLight, "Check AI Raw Lighting on an Obj", HK_ALL },
   { "blame_lighting",     FUNC_INT, AIBlameLight, "Mprint causes of lighting on an Obj", HK_ALL },

   // Watches
   { "ai_alert_watch",    VAR_INT, &g_AIAlertWatchObj,   "Watch an AIs sense/awareness", HK_ALL},
   { "ai_flow_watch",     VAR_INT, &g_AIFlowWatchObj,    "Watch the decision flow of an AI", HK_ALL},
   { "ai_sound_watch",    VAR_INT, &g_AISoundWatchObj,   "Watch the sound/broadcast of an AI", HK_ALL},
   { "ai_hear_watch",     VAR_INT, &g_AIHearWatchObj,    "See what an AI is hearing", HK_ALL},
   { "ai_combat_watch",   VAR_INT, &g_AICbtWatchObj,     "Watch AI Combat", HK_ALL},
   { "ai_path_watch",     VAR_INT, &g_AIPathWatchObj,    "Watch AI pathfind and path progression", HK_ALL},
   { "ai_death_watch",    VAR_INT, &g_AIDeathWatchObj,   "Watch AI death event and cause", HK_ALL},
   { "ai_signal_watch",   VAR_INT, &g_AISignalWatchObj,  "Watch AI signals", HK_ALL},
   { "ai_ranged_watch",   VAR_INT, &g_AIRangedWatchObj,  "Watch AI ranged combat", HK_ALL},
   { "ai_inform_watch",   VAR_INT, &g_AIInformWatchObj,  "Watch AI inform stream", HK_ALL},
   { "ai_sight_watch",    VAR_INT, &g_AISightWatchObj,   "Watch AI inform stream", HK_ALL},
   { "ai_pulse_watch",    VAR_INT, &g_AISensePulseWatchObj,    "Watch AI sensory pulses", HK_ALL},
   { "ai_recover_watch",  VAR_INT, &g_AIRecoverPathWatchObj,    "Watch AI path recovery", HK_ALL},
   { "ai_ranged_mode_watch",  VAR_INT, &g_AIRangedModeWatchObj,    "Watch AI ranged modes", HK_ALL},
   { "ai_watch",          FUNC_INT, AIToggleWatchAll,    "Toggle all watches on an AI", HK_ALL},

   // Traces
   { "ai_trace_schedule",  TOGGLE_BOOL, &g_AIScheduleTrace, "Trace AI scheduled run", HK_ALL},

   // Schedule commands
   { "ai_schedule",       FUNC_INT, AISetSchedule,    "Set AI frame schedule", HK_ALL},

   // Dump commands
   { "ai_dump_components", FUNC_INT, AIDumpComps, "Dump the list of components an AI is made of", HK_ALL},
   { "ai_dump_all",        FUNC_INT, AIDumpAll,   "Dump all info in an AI", HK_ALL},

   { "ai_break",           FUNC_INT, AIBreak,   "Break in the debugger on run of a specific AI", HK_ALL},

   // Test probes
   { "ai_player_sound",    FUNC_INT,    AIPlayerSound, "Simulate player sound broadcast of specified type", HK_ALL},
   { "ai_signal",          FUNC_STRING, AISignal,      "Simulate signal broadcast of specified type", HK_ALL},

   { "ai_set_recover_test_post",     VAR_INT, &g_AIRecoverTestPost,    "", HK_ALL},
   { "ai_set_recover_test_dest",     VAR_INT, &g_AIRecoverTestDest,    "", HK_ALL},
   { "ai_set_recover_test_result",   VAR_INT, &g_AIRecoverTestResult,    "", HK_ALL},

   { "ai_test_recover",   FUNC_VOID, AITestRecover, "", HK_ALL},

   // drawing pathfinding db
   { "ai_draw_cells", FUNC_VOID, AIPathDrawCellsToggle,
     "show AI path cells in wireframe", HK_EDITOR },
   { "ai_draw_cellids", FUNC_VOID, AIPathDrawCellIdsToggle,
     "show AI path cell ids", HK_EDITOR },
   { "ai_draw_links", FUNC_INT, AIPathDrawLinksToggle,
     "show AI path cell links in wireframe", HK_EDITOR },
   { "ai_draw_zone", VAR_INT, &g_AIPathDBDrawZone,
     "show AI path zone", HK_EDITOR },
   { "ai_draw_zone_type", VAR_INT, &g_AIPathDBDrawZoneType,
     "show AI path zone type (0 N, 1 N+L, 2 H, 3 H+L)", HK_EDITOR },
   { "ai_draw_room", VAR_INT, &g_AIPathDBDrawRoom,
     "show AI path cells in room", HK_EDITOR },
   { "ai_spew_zone", FUNC_VOID, AIPathFindSpewZone,
     "spew AI path zone", HK_EDITOR },
   { "ai_spew_zones", FUNC_VOID, AIPathFindSpewZones,
     "spew all AI path zones", HK_EDITOR },

   { "ai_val_fpts", FUNC_VOID, AIValidateFleePoints,
     "validate flee points", HK_ALL},

   // This is drawn in a different color.
   { "ai_draw_one_cell", VAR_INT, &AIDebugMagicLocatorCell,     "path cell to highlight (0 for none)", HK_EDITOR },
   { "ai_draw_cell_centers", TOGGLE_BOOL, &AIDebugCenters,     "", HK_EDITOR },

   { "ai_dump_cell_connection", FUNC_INT, AIDebugDumpCellConnectionInfo, "mono print links from cell" },
   { "ai_dump_cell_vertex_data", FUNC_INT, AIDebugDumpCellVertexInfo, "show cell vertex list on mono" },

   // conversations
   { "ai_conv_start", FUNC_INT, AIConvStart, "start a conversation (by objID)", HK_ALL },

#ifdef EDITOR
   { "ai_cam_to_cell", FUNC_INT, AICamToCell, "Send camera to AI cell", HK_ALL },
   { "ai_print_cam_cell", FUNC_VOID, AIPrintCamCell, "", HK_ALL },
#endif

   { "ai_test_cells", FUNC_FLOAT, AIPrintIffyCells, "", HK_ALL },

#ifdef DEBUG
   { "test_rand", FUNC_VOID, TestRand, "", HK_ALL },
#endif

   // play motion
   { "ai_test_motion", FUNC_STRING, AITestMotion, "play motion by name",HK_ALL },

   { "ai_test", FUNC_VOID, GenericTest, "", HK_ALL },
};
#endif

void AIInstallCommands()
{
#ifndef SHIP
   COMMANDS(g_AICommands, HK_ALL);
#endif
}

///////////////////////////////////////////////////////////////////////////////
