/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidebug.h,v 1.18 1999/04/09 11:30:33 TOML Exp $
//
//
//

#ifndef __AIDEBUG_H
#define __AIDEBUG_H

#pragma once

#include <comtools.h>
#ifdef __cplusplus
#include <str.h>
#endif

// #include <_ai.h>

F_DECLARE_INTERFACE(IInternalAI);

///////////////////////////////////////////////////////////////////////////////

extern ObjID g_AICurrentAI;

#ifndef SHIP
extern BOOL g_AIEfficiencyDebug;
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Watch Routines
//

#ifndef SHIP

#define AIIsWatched(kind, id) (g_AI##kind##WatchObj == -1 || id == g_AI##kind##WatchObj)

#define AIWatch(kind, id, msg)                           do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind); } while (0)
#define AIWatch1(kind, id, msg, a)                       do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a); } while (0)
#define AIWatch2(kind, id, msg, a, b)                    do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b); } while (0)
#define AIWatch3(kind, id, msg, a, b, c)                 do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c); } while (0)
#define AIWatch4(kind, id, msg, a, b, c, d)              do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d); } while (0)
#define AIWatch5(kind, id, msg, a, b, c, d, e)           do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e); } while (0)
#define AIWatch6(kind, id, msg, a, b, c, d, e, f)        do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f); } while (0)
#define AIWatch7(kind, id, msg, a, b, c, d, e, f, g)     do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g); } while (0)
#define AIWatch8(kind, id, msg, a, b, c, d, e, f, g, h)  do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g, h); } while (0)
#define AIWatch9(kind, id, msg, a, b, c, d, e, f, g, h, i)  do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g, h, i); } while (0)

#define AIWatchRaw(kind, id, msg)                          do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg); } while (0)
#define AIWatchRaw1(kind, id, msg, a)                      do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a); } while (0)
#define AIWatchRaw2(kind, id, msg, a, b)                   do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b); } while (0)
#define AIWatchRaw3(kind, id, msg, a, b, c)                do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c); } while (0)
#define AIWatchRaw4(kind, id, msg, a, b, c, d)             do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c, d); } while (0)
#define AIWatchRaw5(kind, id, msg, a, b, c, d, e)          do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c, d, e); } while (0)
#define AIWatchRaw6(kind, id, msg, a, b, c, d, e, f)       do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c, d, e, f); } while (0)
#define AIWatchRaw7(kind, id, msg, a, b, c, d, e, f, g)    do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c, d, e, f, g); } while (0)
#define AIWatchRaw8(kind, id, msg, a, b, c, d, e, f, g, h) do { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf(msg, a, b, c, d, e, f, g, h); } while (0)

#define AIWatchTrue(test, kind, id, msg)                          do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind); } } while (0)
#define AIWatchTrue1(test, kind, id, msg, a)                      do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a); } } while (0)
#define AIWatchTrue2(test, kind, id, msg, a, b)                   do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b); } } while (0)
#define AIWatchTrue3(test, kind, id, msg, a, b, c)                do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c); } } while (0)
#define AIWatchTrue4(test, kind, id, msg, a, b, c, d)             do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d); } } while (0)
#define AIWatchTrue5(test, kind, id, msg, a, b, c, d, e)          do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e); } } while (0)
#define AIWatchTrue6(test, kind, id, msg, a, b, c, d, e, f)       do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f); } } while (0)
#define AIWatchTrue7(test, kind, id, msg, a, b, c, d, e, f, g)    do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g); } } while (0)
#define AIWatchTrue8(test, kind, id, msg, a, b, c, d, e, f, g, h) do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g, h); } } while (0)
#define AIWatchTrue9(test, kind, id, msg, a, b, c, d, e, f, g, h, i) do { if (test) { if (g_AI##kind##WatchObj != -1 && id != g_AI##kind##WatchObj) ; else mprintf("[AI(%3d) Watch:%5s] " msg "\n", id, #kind, a, b, c, d, e, f, g, h, i); } } while (0)

#define AITrace(kind, msg)                           do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind); } while (0)
#define AITrace1(kind, msg, a)                       do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a); } while (0)
#define AITrace2(kind, msg, a, b)                    do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b); } while (0)
#define AITrace3(kind, msg, a, b, c)                 do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c); } while (0)
#define AITrace4(kind, msg, a, b, c, d)              do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d); } while (0)
#define AITrace5(kind, msg, a, b, c, d, e)           do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e); } while (0)
#define AITrace6(kind, msg, a, b, c, d, e, f)        do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f); } while (0)
#define AITrace7(kind, msg, a, b, c, d, e, f, g)     do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g); } while (0)
#define AITrace8(kind, msg, a, b, c, d, e, f, g, h)  do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g, h); } while (0)
#define AITrace9(kind, msg, a, b, c, d, e, f, g, h, i)  do { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g, h, i); } while (0)

#define AITraceRaw(kind, msg)                           do { if (!g_AI##kind##Trace) ; else mprintf(msg); } while (0)
#define AITraceRaw1(kind, msg, a)                       do { if (!g_AI##kind##Trace) ; else mprintf(msg, a); } while (0)
#define AITraceRaw2(kind, msg, a, b)                    do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b); } while (0)
#define AITraceRaw3(kind, msg, a, b, c)                 do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c); } while (0)
#define AITraceRaw4(kind, msg, a, b, c, d)              do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d); } while (0)
#define AITraceRaw5(kind, msg, a, b, c, d, e)           do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d, e); } while (0)
#define AITraceRaw6(kind, msg, a, b, c, d, e, f)        do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d, e, f); } while (0)
#define AITraceRaw7(kind, msg, a, b, c, d, e, f, g)     do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d, e, f, g); } while (0)
#define AITraceRaw8(kind, msg, a, b, c, d, e, f, g, h)  do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d, e, f, g, h); } while (0)
#define AITraceRaw9(kind, msg, a, b, c, d, e, f, g, h, i)  do { if (!g_AI##kind##Trace) ; else mprintf(msg, a, b, c, d, e, f, g, h, i); } while (0)

#define AITraceTrue(test, kind, msg)                          do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind); } } while (0)
#define AITraceTrue1(test, kind, msg, a)                      do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a); } } while (0)
#define AITraceTrue2(test, kind, msg, a, b)                   do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b); } } while (0)
#define AITraceTrue3(test, kind, msg, a, b, c)                do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c); } } while (0)
#define AITraceTrue4(test, kind, msg, a, b, c, d)             do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d); } } while (0)
#define AITraceTrue5(test, kind, msg, a, b, c, d, e)          do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e); } } while (0)
#define AITraceTrue6(test, kind, msg, a, b, c, d, e, f)       do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f); } } while (0)
#define AITraceTrue7(test, kind, msg, a, b, c, d, e, f, g)    do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g); } } while (0)
#define AITraceTrue8(test, kind, msg, a, b, c, d, e, f, g, h) do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g, h); } } while (0)
#define AITraceTrue9(test, kind, msg, a, b, c, d, e, f, g, h, i) do { if (test) { if (!g_AI##kind##Trace) ; else mprintf("[AI Trace:%5s] " msg "\n", #kind, a, b, c, d, e, f, g, h, i); } } while (0)

#else

#define AIWatch(kind, id, msg)
#define AIWatch1(kind, id, msg, a)
#define AIWatch2(kind, id, msg, a, b)
#define AIWatch3(kind, id, msg, a, b, c)
#define AIWatch4(kind, id, msg, a, b, c, d)
#define AIWatch5(kind, id, msg, a, b, c, d, e)
#define AIWatch6(kind, id, msg, a, b, c, d, e, f)
#define AIWatch7(kind, id, msg, a, b, c, d, e, f, g)
#define AIWatch8(kind, id, msg, a, b, c, d, e, f, g, h)
#define AIWatch9(kind, id, msg, a, b, c, d, e, f, g, h, i)

#define AIWatchRaw(kind, id, msg)
#define AIWatchRaw1(kind, id, msg, a)
#define AIWatchRaw2(kind, id, msg, a, b)
#define AIWatchRaw3(kind, id, msg, a, b, c)
#define AIWatchRaw4(kind, id, msg, a, b, c, d)
#define AIWatchRaw5(kind, id, msg, a, b, c, d, e)
#define AIWatchRaw6(kind, id, msg, a, b, c, d, e, f)
#define AIWatchRaw7(kind, id, msg, a, b, c, d, e, f, g)
#define AIWatchRaw8(kind, id, msg, a, b, c, d, e, f, g, h)

#define AIWatchTrue(test, kind, id, msg)
#define AIWatchTrue1(test, kind, id, msg, a)
#define AIWatchTrue2(test, kind, id, msg, a, b)
#define AIWatchTrue3(test, kind, id, msg, a, b, c)
#define AIWatchTrue4(test, kind, id, msg, a, b, c, d)
#define AIWatchTrue5(test, kind, id, msg, a, b, c, d, e)
#define AIWatchTrue6(test, kind, id, msg, a, b, c, d, e, f)
#define AIWatchTrue7(test, kind, id, msg, a, b, c, d, e, f, g)
#define AIWatchTrue8(test, kind, id, msg, a, b, c, d, e, f, g, h)
#define AIWatchTrue9(test, kind, id, msg, a, b, c, d, e, f, g, h, i)

#define AITrace(kind, msg)                           
#define AITrace1(kind, msg, a)                       
#define AITrace2(kind, msg, a, b)                    
#define AITrace3(kind, msg, a, b, c)                 
#define AITrace4(kind, msg, a, b, c, d)              
#define AITrace5(kind, msg, a, b, c, d, e)           
#define AITrace6(kind, msg, a, b, c, d, e, f)        
#define AITrace7(kind, msg, a, b, c, d, e, f, g)     
#define AITrace8(kind, msg, a, b, c, d, e, f, g, h)  
#define AITrace9(kind, msg, a, b, c, d, e, f, g, h, i)  

#define AITraceRaw(kind, msg)                           
#define AITraceRaw1(kind, msg, a)                       
#define AITraceRaw2(kind, msg, a, b)                    
#define AITraceRaw3(kind, msg, a, b, c)                 
#define AITraceRaw4(kind, msg, a, b, c, d)              
#define AITraceRaw5(kind, msg, a, b, c, d, e)           
#define AITraceRaw6(kind, msg, a, b, c, d, e, f)        
#define AITraceRaw7(kind, msg, a, b, c, d, e, f, g)     
#define AITraceRaw8(kind, msg, a, b, c, d, e, f, g, h)  
#define AITraceRaw9(kind, msg, a, b, c, d, e, f, g, h, i)  

#define AITraceTrue(test, kind, msg)                          
#define AITraceTrue1(test, kind, msg, a)                      
#define AITraceTrue2(test, kind, msg, a, b)                   
#define AITraceTrue3(test, kind, msg, a, b, c)                
#define AITraceTrue4(test, kind, msg, a, b, c, d)             
#define AITraceTrue5(test, kind, msg, a, b, c, d, e)          
#define AITraceTrue6(test, kind, msg, a, b, c, d, e, f)       
#define AITraceTrue7(test, kind, msg, a, b, c, d, e, f, g)    
#define AITraceTrue8(test, kind, msg, a, b, c, d, e, f, g, h) 
#define AITraceTrue9(test, kind, msg, a, b, c, d, e, f, g, h, i) 

#endif

///////////////////////////////////////
//
// Watches
//

#ifndef SHIP
EXTERN ObjID g_AIAlertWatchObj;
EXTERN ObjID g_AIFlowWatchObj;
EXTERN ObjID g_AISoundWatchObj;
EXTERN ObjID g_AIHearWatchObj;
EXTERN ObjID g_AICbtWatchObj;
EXTERN ObjID g_AIPathWatchObj;
EXTERN ObjID g_AIDeathWatchObj;
EXTERN ObjID g_AISignalWatchObj;
EXTERN ObjID g_AIRangedWatchObj;
EXTERN ObjID g_AIInformWatchObj;
EXTERN ObjID g_AISightWatchObj;
EXTERN ObjID g_AISensePulseWatchObj;
EXTERN ObjID g_AIRecoverPathWatchObj;
EXTERN ObjID g_AICombatWatchObj;
EXTERN ObjID g_AIRangedModeWatchObj;

EXTERN BOOL g_AIScheduleTrace;

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Dump Routines
//

#ifdef __cplusplus
extern cStr          g_AIDumpStr;
extern IInternalAI * g_pAIDumpAI;

#define AIBeginDump(objId) \
   { \
      AutoAppIPtr_(AIManager, pAIManager_Dump); \
      if (!!pAIManager_Dump) \
      { \
         g_pAIDumpAI = (IInternalAI *)pAIManager_Dump->GetAI(objId); \
         if (g_pAIDumpAI) \
         {

#define AIDumpSrc() g_pAIDumpAI

#define AIDump(fmt)                         { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID()); mprint(g_AIDumpStr); }
#define AIDump1(fmt, a)                     { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a); mprint(g_AIDumpStr); }
#define AIDump2(fmt, a, b)                  { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b); mprint(g_AIDumpStr); }
#define AIDump3(fmt, a, b, c)               { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b, c); mprint(g_AIDumpStr); }
#define AIDump4(fmt, a, b, c, d)            { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b, c, d); mprint(g_AIDumpStr); }
#define AIDump5(fmt, a, b, c, d, e)         { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b, c, d, e); mprint(g_AIDumpStr); }
#define AIDump6(fmt, a, b, c, d, e, f)      { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b, c, d, e, f); mprint(g_AIDumpStr); }
#define AIDump7(fmt, a, b, c, d, e, f, g)   { g_AIDumpStr.FmtStr("[AI(%3d) Dump]        " fmt "\n", g_pAIDumpAI->GetObjID(), a, b, c, d, e, f, g); mprint(g_AIDumpStr); }

#define AIEndDump() \
            g_pAIDumpAI->Release(); \
         } \
      } \
   }
#endif
///////////////////////////////////////////////////////////////////////////////
//
// Spew
//

#define AISpew(x,y)         DBGS(x, Spew(x,y))
#define AISpewID(x, a)      Spew(x, ("AI %d: ", a));
#define AISpewWithID(a,x,s) DBGS(x, {AISpewID((x),(a)); Spew((x), s);})

// pathfinding data
#define AISpewPathFind(x)          AISpewWithID(g_AICurrentAI, DSRC_AI_PATHFIND,x)

#ifndef SHIP
#define AIDebugSet(obj) g_AICurrentAI = obj
#define AIDebugGet() g_AICurrentAI
#else
#define AIDebugSet(obj)
#define AIDebugGet() OBJ_NULL
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Rendered debug info
//

EXTERN BOOL gAIDebugDrawInGame;
EXTERN BOOL g_AIDrawAIs;
EXTERN BOOL g_AIDebugDrawMoveGoal;
EXTERN BOOL g_AIDebugDrawSuggs;
EXTERN BOOL g_AIDebugDrawPaths;

// call this per render frame per AI in game you want to see info on
EXTERN void AIDebugDrawObjAIInfo(ObjID obj);


///////////////////////////////////////////////////////////////////////////////
#endif /* !__AIDEBUG_H */
