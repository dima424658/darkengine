/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkdiff.cpp,v 1.10 2000/02/19 12:27:14 toml Exp $
#include <drkdiff.h>
#include <questprp.h>
#include <propbase.h>
#include <objquery.h>
#include <iobjsys.h>
#include <objdef.h>
#include <appagg.h>
#include <contain.h>
#include <scrptapi.h>
#include <lockprop.h>
#include <diffscrm.h>
#include <questapi.h>
#include <doorphys.h>
#include <drkgoalt.h>
#include <config.h>

#include <command.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//starting difficulty
int g_diff = 0;

//
// "Destroy" property 
//

static sPropertyDesc destroy_desc = 
{
   PROP_DIFF_DESTROY_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Destroy", 
   }
}; 

static IQuestVarProperty* gpDestroyProp = NULL;

   // do the destruction thing 
static void do_destroy()
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr(ContainSys); 

   pObjSys->Lock(); 
   cAutoIPtr<IObjectQuery> query( gpDestroyProp->QueryAllMatches(kQVarMatches)); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (OBJ_IS_CONCRETE(obj))
      {
         // Destroy objects in the container too 
         sContainIter* iter = pContainSys->IterStart(obj); 
         for (;!iter->finished; pContainSys->IterNext(iter))
            pObjSys->Destroy(iter->containee); 
         pContainSys->IterEnd(iter); 

         pObjSys->Destroy(obj); 
      }
   }
   pObjSys->Unlock(); 
}

//
// "permit" property 
// just like destroy, but is only destroyed at non-matching difficulty levels
//

static sPropertyDesc permit_desc = 
{
   PROP_DIFF_PERMIT_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Permit", 
   }
}; 

static IQuestVarProperty* gpPermitProp = NULL;

   // do the destruction thing 
static void do_permit()
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr(ContainSys); 

   pObjSys->Lock(); 
   cAutoIPtr<IObjectQuery> query( gpPermitProp->QueryAllMatches(kQVarNonMatches)); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (OBJ_IS_CONCRETE(obj))
      {
         // permit objects in the container too 
         sContainIter* iter = pContainSys->IterStart(obj); 
         for (;!iter->finished; pContainSys->IterNext(iter))
            pObjSys->Destroy(iter->containee); 
         pContainSys->IterEnd(iter); 

         pObjSys->Destroy(obj); 
      }
   }
   pObjSys->Unlock(); 
}

//
// "Lock" property 
//

static sPropertyDesc lock_desc = 
{
   PROP_DIFF_LOCK_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Lock (Unlock)", 
   }
}; 

static IQuestVarProperty* gpLockProp = NULL;

// do the lock thing 
static void do_lock()
{
   sPropertyObjIter iter; 
   ObjID obj; 

   gpLockProp->IterStart(&iter); 
   while (gpLockProp->IterNext(&iter,&obj))
      if (OBJ_IS_CONCRETE(obj))
         ObjSetSelfLocked(obj, gpLockProp->Matches(obj)); 
   gpLockProp->IterStop(&iter); 
}

//
// "Close Door" property 
//

static sPropertyDesc close_door_desc = 
{
   PROP_DIFF_CLOSE_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Close (Open) Door",  // @TODO: change this 
   }
}; 

static IQuestVarProperty* gpCloseDoorProp = NULL;


static void set_door_state(ObjID obj, BOOL closed)
{
   if (closed)
      DoorSlamClosed(obj); 
   else
      DoorSlamOpen(obj); 
}

// do the close door thing 
static void do_close_door()
{
   sPropertyObjIter iter; 
   ObjID obj; 

   gpCloseDoorProp->IterStart(&iter); 
   while (gpCloseDoorProp->IterNext(&iter,&obj))
      if (OBJ_IS_CONCRETE(obj))
         set_door_state(obj,gpCloseDoorProp->Matches(obj)); 
   gpCloseDoorProp->IterStop(&iter); 
}

//
// "Lock" property 
//

static sPropertyDesc turnon_desc = 
{
   PROP_DIFF_TURNON_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Turn On (Off)", 
   }
}; 

static IQuestVarProperty* gpTurnonProp = NULL;

// do the turnon thing 
static void do_turnon()
{
   sPropertyObjIter iter; 
   ObjID obj; 

   gpTurnonProp->IterStart(&iter); 
   while (gpTurnonProp->IterNext(&iter,&obj))
      if (OBJ_IS_CONCRETE(obj))
      {
         BOOL matches = gpTurnonProp->Matches(obj); 
         AutoAppIPtr(ScriptMan); 
         sScrMsg msg(obj,matches ? "TurnOn" : "TurnOff" ); 
         pScriptMan->SendMessage(&msg); 
      }
   gpTurnonProp->IterStop(&iter); 
}

//
// "Script" property 
//

static sPropertyDesc script_desc = 
{
   PROP_DIFF_SCRIPT_NAME, 
   kPropertyInstantiate|kPropertyNoInherit, 
   NULL, // constraints
   0, 0, // version 
   {
      "Difficulty",
      "Script", 
   }
}; 

static IQuestVarProperty* gpScriptProp = NULL;

   // do the destruction thing 
static void do_script()
{
   AutoAppIPtr(ScriptMan); 
   AutoAppIPtr(QuestData); 
   int diff = pQuestData->Get(DIFF_QVAR); 

   cAutoIPtr<IObjectQuery> query( gpScriptProp->QueryAllMatches(kQVarMatches)); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (OBJ_IS_CONCRETE(obj))
      {
         sDiffScrMsg msg(obj,diff); 
         pScriptMan->SendMessage(&msg); 
      }
   }
}

//
// Prep objectives
//
static BOOL in_range(IQuestData* pQuestData, int i, int diff)
{
   cStr minvar = GoalMinDiffVarName(i); 
   if (pQuestData->Exists(minvar))
   {
      int m = pQuestData->Get(minvar); 
      if (diff < m)
         return FALSE; 
   }

   cStr maxvar = GoalMaxDiffVarName(i); 
   if (pQuestData->Exists(maxvar))
   {
      int m = pQuestData->Get(maxvar); 
      if (diff > m)
         return FALSE; 
   }
   return TRUE; 

}


static void do_objectives(void)
{
   AutoAppIPtr(QuestData); 
   int diff = pQuestData->Get(DIFF_QVAR); 

   
   BOOL done = FALSE; 
   for (int i = 0; !done ;i++)
   {
      cStr statevar = GoalStateVarName(i); 
      if (!pQuestData->Exists(statevar))
         break; 

      if (!in_range(pQuestData,i,diff))
      {
         pQuestData->Set(statevar,kGoalInactive); 
         pQuestData->Set(GoalVisibleVarName(i),0); 
      }

   }
   
}


//
// Init/Term 
//

void init_commands(); 

void DarkDifficultyInit()
{
   init_commands(); 
   gpDestroyProp = CreateQuestVarProperty(DIFF_QVAR,&destroy_desc,kPropertyImplLlist); 
   gpPermitProp = CreateQuestVarProperty(DIFF_QVAR,&permit_desc,kPropertyImplLlist); 
   gpLockProp = CreateQuestVarProperty(DIFF_QVAR,&lock_desc,kPropertyImplLlist); 
   gpCloseDoorProp = CreateQuestVarProperty(DIFF_QVAR,&close_door_desc,kPropertyImplLlist); 
   gpTurnonProp = CreateQuestVarProperty(DIFF_QVAR,&turnon_desc,kPropertyImplLlist); 
   gpScriptProp = CreateQuestVarProperty(DIFF_QVAR,&script_desc,kPropertyImplLlist); 


}

void DarkDifficultyTerm()
{
   SafeRelease(gpScriptProp); 
   SafeRelease(gpDestroyProp); 
   SafeRelease(gpPermitProp); 
   SafeRelease(gpLockProp); 
   SafeRelease(gpTurnonProp); 
   SafeRelease(gpCloseDoorProp); 
}

void DarkPrepLevelForDifficulty()
{

   do_destroy(); 
   do_permit();
   do_lock(); 
   do_close_door(); 
   do_turnon(); 
   do_script(); 
   do_objectives(); 
}

BOOL DarkDifficultyIsAllowed(ObjID obj, int diff)
{
   // permit and destroy are the only relevant things to check here
   if (gpDestroyProp->IsRelevant(obj) && gpDestroyProp->Matches(obj,diff))
      return(FALSE);
   if (gpPermitProp->IsRelevant(obj) && !gpPermitProp->Matches(obj,diff))
      return(FALSE);

   return(TRUE);
}

static Command commands[] = 
{
   { "process_difficulty", FUNC_VOID, DarkPrepLevelForDifficulty, "Prep level for difficulty.  Warning: This may destroy some objects", HK_ALL }, 
};

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

