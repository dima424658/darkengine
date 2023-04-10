/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover binds processing
#include <stdlib.h>
#include <command.h>
#include <config.h>
#include <contexts.h>
#include <dpccntxt.h>
#include <gen_bind.h>
#include <cfgtool.h>

static char *EdgeTriggerProc (char *cmd, char *val, BOOL)
{  
   double dval = atof (val);
   //for these, start is 1 and end is 0
   char str[64];
   sprintf(str,"%s %d",cmd,(int)dval);
   //strcpy (str, cmd);
   //strcat (str, (dval == 0.0) ? " 1" : " 0");
   CommandExecute (str);

   return NULL;
}             


IB_var g_dpc_ib_vars[] = {
//{var name, starting var val, flags, game callback to be called, agg callback to resolve conflicts, always NULL},
   {"fire_weapon",        "0", 0, EdgeTriggerProc, NULL, NULL},
   {"query",              "0", 0, EdgeTriggerProc, NULL, NULL},
   {"split",              "0", 0, EdgeTriggerProc, NULL, NULL},
   {"drag_and_drop",      "0", 0, EdgeTriggerProc, NULL, NULL},
   {"drag_and_drop_frob", "0", 0, EdgeTriggerProc, NULL, NULL},
   {"drag_and_drop_mode", "0", 0, EdgeTriggerProc, NULL, NULL},
   //{"frob_toggle", "0", 0, EdgeTriggerProc, NULL, NULL},
   NULL
};

void InitDPCIBVars ()
{
   char def_path[256];
   Verify (find_file_in_config_path (def_path, "default.bnd", "include_path")); 

   //input binding stuff
   g_pInputBinder->VarSet (g_dpc_ib_vars);

   // There isn't anything bound in chat mode; it deals with its own
   // keystrokes.
   g_pInputBinder->SetContext (HK_CHAT_MODE, TRUE);
   g_pInputBinder->SetContext (HK_RECORD_MODE, TRUE);
   g_pInputBinder->LoadBndFile (def_path, HK_RECORD_MODE, "all");
   g_pInputBinder->LoadBndFile (def_path, HK_RECORD_MODE, "record");
}

