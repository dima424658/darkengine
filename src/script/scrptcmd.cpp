/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptcmd.cpp,v 1.1 1998/02/10 09:51:50 MAT Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   editor commands for the script system

   Those script commands which use the script property--load and
   dropping modules and a few others--are in scrptprp.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

// @TODO: Some of these were copied in from scrptprp, where the
// commands used to be, and may not be needed here.

#include <scrptapi.h>
#include <scrptcmd.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <objnotif.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <dynarray.h>

#include <traitman.h>
#include <trait.h>
#include <objquery.h>

#include <edittool.h>

#include <listset.h>
#include <lststtem.h>

#include <vernum.h>
#include <tagfile.h>

#include <mprintf.h>

#include <traitman.h>
#include <traitbas.h>

// For script datapath
#include <datapath.h>
#include <config.h>

// Command stuff
#include <ctype.h>
#include <command.h>
#include <status.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// SCRIPT COMMANDS
//

static void script_test_message(char* arg)
{
   // Munch trailing whitespace
   for (char* s = arg + strlen(arg) - 1; s >= arg && isspace(*s); s++)
      *s = '\0';

   AutoAppIPtr(ScriptMan);
   AutoAppIPtr_(EditTools,pTools);

   sScrMsg * pMsg = new sScrMsg(pTools->GetObjNamed(arg),"Test");

   pScriptMan->SendMessage(pMsg);

   SafeRelease(pMsg);
}


//////////////////////////////////////////////////////////////////////
//
// message tracing
//
static char g_aWhitespace[] = { " \t" };

// <object> <message> <action> <line>
static void trace_add(char *arg)
{
   char aLine[256];
   strcpy (aLine, arg);

   char *pObjectStr = strtok(aLine, g_aWhitespace);
   char *pMessageStr = strtok(NULL, g_aWhitespace);
   char *pActionStr = strtok(NULL, g_aWhitespace);
   char *pLineStr = strtok(NULL, g_aWhitespace);

   if (!pObjectStr || !pMessageStr || !pActionStr || !pLineStr) {
      Warning(("The format you want is <object> <message> <action> <line>\n"));
      return;
   }

   AutoAppIPtr_(EditTools, pTools);
   ObjID obj = strtol(pObjectStr, NULL, 10);
   if (!obj)
      obj = pTools->GetObjNamed(pObjectStr);
   if (!obj) {
      Warning(("Object not found: %s.\n", pObjectStr));
      return;
   }

   eScrTraceAction Action;
   if (pActionStr[0] == 'b' || pActionStr[0] == 'B')
      Action = kBreak;
   else if (pActionStr[0] == 's' || pActionStr[0] == 'S')
      Action = kSpew;
   else {
      Warning(("Action should be either break or spew."
               "  We only check the first letter.\n"));
      return;
   }

   int iTraceLine = strtol(pLineStr, NULL, 10) - 1;
   if (iTraceLine < 0 || iTraceLine > 31) {
      Warning(("Trace lines are numbered 1-32.\n"));
      return;
   }

   AutoAppIPtr(ScriptMan);
   pScriptMan->AddTrace(obj, pMessageStr, Action, iTraceLine);
}


// <object> <message>
static void trace_remove(char *arg)
{
   char aLine[256];
   strcpy (aLine, arg);

   char *pObjectStr = strtok(aLine, g_aWhitespace);
   char *pMessageStr = strtok(NULL, g_aWhitespace);

   if (!pObjectStr || !pMessageStr) {
      Warning(("The format you want is <object> <message>\n"));
      return;
   }

   AutoAppIPtr_(EditTools, pTools);
   ObjID obj = strtol(pObjectStr, NULL, 10);
   if (!obj)
      obj = pTools->GetObjNamed(pObjectStr);
   if (!obj) {
      Warning(("Object not found: %s.\n", pObjectStr));
      return;
   }

   AutoAppIPtr(ScriptMan);
   pScriptMan->RemoveTrace(obj, pMessageStr);
}


// BOOL to toggle line
static void trace_line(int iLine)
{
   iLine--;

   if (iLine < 0 || iLine > 31) {
      Warning(("Trace lines are numbered 1-32."));
   }

   AutoAppIPtr(ScriptMan);
   BOOL bCurrentState = pScriptMan->GetTraceLine(iLine);
   if (bCurrentState) {
      pScriptMan->SetTraceLine(iLine, FALSE);
      mprintf("Trace line %d is now OFF.\n", iLine + 1);
   } else {
      pScriptMan->SetTraceLine(iLine, TRUE);
      mprintf("Trace line %d is now ON.\n", iLine + 1);
   }
}


static char *dump_flag(int iFlag)
{
   if (iFlag)
      return "ON ";
   else
      return "OFF";
}


static char *dump_action(eScrTraceAction Action)
{
   if (Action == kBreak)
      return "Break";
   else if (Action == kSpew)
      return "Spew ";
   else {
      Warning(("Unknown kind of trace action: %d\n", Action));
      return "HUH? ";
   }
}


static void dump_one_trace(const cScrTrace *pTrace, int iMask)
{
   char Name[40];
   pTrace->MessageNameToString(Name);
   AutoAppIPtr_(EditTools, pTools);
   
   mprintf("%s    %s    %s    %d    %s\n",
           pTools->ObjName(pTrace->m_HostObjID),
           Name,
           dump_action(pTrace->m_TraceAction),
           pTrace->m_iTraceLine + 1,
           dump_flag((1 << pTrace->m_iTraceLine) & iMask));
}


static void trace_dump()
{
   mprintf("object       message   action   line   line status\n");

   AutoAppIPtr(ScriptMan);
   const cScrTrace *pTrace;
   int iMask = pScriptMan->GetTraceLineMask();
   tScrIter i;

   pTrace = pScriptMan->GetFirstTrace(&i);
   while (pTrace) {
      dump_one_trace(pTrace, iMask);
      pTrace = pScriptMan->GetNextTrace(&i);
   }
}


static void trace_dump_active()
{
   mprintf("object       message   action   line   line status\n");

   AutoAppIPtr(ScriptMan);
   const cScrTrace *pTrace;
   int iMask = pScriptMan->GetTraceLineMask();
   tScrIter i;

   pTrace = pScriptMan->GetFirstTrace(&i);
   while (pTrace) {
      if (iMask & (1 << pTrace->m_iTraceLine))
         dump_one_trace(pTrace, iMask);
      pTrace = pScriptMan->GetNextTrace(&i);
   }
}


static void trace_line_dump()
{
   AutoAppIPtr(ScriptMan);
   int iMask = pScriptMan->GetTraceLineMask();

   for (int i = 0; i < 32; i += 4)
      mprintf("%2d: %s    %2d: %s    %2d: %s    %2d: %s\n",
              i + 1, dump_flag(pScriptMan->GetTraceLine(i)),
              i + 2, dump_flag(pScriptMan->GetTraceLine(i + 1)),
              i + 3, dump_flag(pScriptMan->GetTraceLine(i + 2)),
              i + 4, dump_flag(pScriptMan->GetTraceLine(i + 3)));
}


static Command commands[] =
{
   // debugging
   { "script_test", FUNC_STRING, script_test_message,
     "Send a 'test' message to an object", HK_ALL },

   // tracing/breakpoints
   { "trace_add", FUNC_STRING, trace_add,
     "<object> <message> <action> <line>", HK_ALL },
   { "trace_remove", FUNC_STRING, trace_remove,
     "<object> <message>", HK_ALL },
   { "trace_line", FUNC_INT, trace_line,
     "toggle a script trace line", HK_ALL },

   // trace mono spew
   { "trace_dump", FUNC_VOID, trace_dump,
     "show all traces", HK_ALL },
   { "trace_dump_active", FUNC_VOID, trace_dump_active,
     "show all traces on active lines", HK_ALL },
   { "trace_line_dump", FUNC_VOID, trace_line_dump,
     "show status of all trace lines", HK_ALL },
};


void ScriptCommandsInit(void)
{
   COMMANDS(commands, HK_ALL);
}
