/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/command.c,v 1.28 2000/02/19 12:29:40 toml Exp $

// Command storage manager/parser

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <lg.h>
#include <mprintf.h>

#include <command.h>
#include <status.h>

#include <prompts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// maximum number of registerable commands sets
#define MAX_COMMAND_SETS  256

static Command *command_list[MAX_COMMAND_SETS];
static int command_count[MAX_COMMAND_SETS];
static int command_list_size;

ulong  command_context = 0xffffffff;
ulong *command_context_ptr = &command_context;

void CommandRegister(Command *cmd_set, int count, ulong contexts)
{
   int i, j;

   if (command_list_size == MAX_COMMAND_SETS)
      Error(1, "CommandRegister: Too many independent definitions.\n");

   if (count <= 0)
      return;

   for (i=0; i < command_list_size; ++i)
      if (command_list[i] == cmd_set) {
         Warning(("CommandRegister: Registered set again (includes cmd %s)\n",
                  cmd_set->name));
         return;
      }

   i = command_list_size;

   command_list[i] = cmd_set;
   command_count[i] = count;

   for (j=0; j < count; ++j)
      if (!cmd_set[j].contexts)
         cmd_set[j].contexts = contexts;

   ++command_list_size;
}

Command *CommandFind(const char *s, int n)
{
   int i,j, count;
   Command *set;

   for (j=0; j < command_list_size; ++j) {
      set = command_list[j];
      count = command_count[j];

      for (i=0; i < count; ++i)
         if (strlen(set[i].name) == n && !strnicmp(set[i].name, s, n)) {
            return &set[i];
         }
   }
   return 0;
}

Command *CommandFindString(const char *s)
{
   return CommandFind(s, (int)strlen(s));
}

void CommandForEach(CommandIterateCallback callback, void *data)
{
   int i, j;

   if (!callback)
      return;

   for (j=0; j < command_list_size; ++j)
      for (i=0; i < command_count[j]; ++i)
         if (!callback(&command_list[j][i], data))
            return;
}

bool CommandParse(const char *inp, Command **res_1, const char **res_2)
{
   // parse out to the first blank
   const char *s;
   int n;
   Command *cmd;

     // skip leading whitespace
   while (*inp && isspace(*inp)) ++inp;

     // if empty it's a no-op
   if (!*inp) return FALSE;

   s = inp;
   while (*s && !isspace(*s)) ++s;

   n = (int)(s - inp);

   cmd = CommandFind(inp, n);

   if (!cmd) return FALSE;

   *res_1 = cmd;
   *res_2 = s;

   return TRUE;
}

static char fail_return[] = "No such Command";
static char context_fail_return[] = "Command is not available in this mode";
char *CommandExecute(const char *inp)
{
   // parse out to the first blank
   const char *s;
   Command *cmd;

   if ((inp==NULL)||(inp[0]=='\0')||(inp[0]==';'))
      return fail_return;  // bonus null command fun
   if (CommandParse(inp, &cmd, &s)) {
      if (!(cmd->contexts & *command_context_ptr)) {
         Warning(("CommandExecute: Invalid context for '%s'\n", cmd->name));
         return context_fail_return;
      }
      CommandExecuteParsed(cmd, s);
      return NULL;
   } else {
      Warning(("Unknown command %s\n", inp));
      return fail_return;
   }
}

bool CommandExecuteParam(const char *inp, const char *parm)
{
   Command *cmd = CommandFindString(inp);
   if (cmd) {
      if (!(cmd->contexts & *command_context_ptr)) {
         Warning(("CommandExecuteParam: Invalid context for '%s'\n", cmd->name));
         return FALSE;
      }
      CommandExecuteParsed(cmd, parm);
      return TRUE;
   } else {
      Warning(("Unknown command %s\n", inp));
      return FALSE;
   }
}

bool atobool(const char *s)
{
   if (!stricmp(s, "false")) return FALSE;
   if (!stricmp(s, "true" )) return TRUE;
   return atoi(s);
}

static float atofloat(const char *s)
{
   return strtof(s, NULL);
}

//////////////////
// actual execute of the parsed command

// @TODO: the s && *s logic is still wrong!
#define get_val_and_exec(f,s,cvrt,prmpt,def) \
   if (do_prompt) f(prmpt(s)); else if ((s)&&(*s)) f(cvrt(s)); else f(def)

void CommandExecuteParsed(Command *cmd, const char *s)
{
   BOOL do_prompt;
   
   if (!(cmd->contexts & *command_context_ptr)) {
      Warning(("CommandExecuteParsed: Invalid context for '%s'\n", cmd->name));
      return;
   }

   while (*s && isspace(*s)) ++s;

   do_prompt=(strncmp("@@",s,2)==0);
   if (do_prompt) s+=2;

   //  mprintf("type %d, s:%x (%c)\n",cmd->type,s,s?*(char *)s:0);

   switch(cmd->type) {
      case FUNC_VOID:
      {
         void (*func)(void);
         func = (void (*)(void)) cmd->val;
         func();
         break;
      }
      case FUNC_BOOL:
      {
         void (*func)(bool);
         func = (void (*)(bool)) cmd->val;
         get_val_and_exec(func,s,atobool,prompt_bool,TRUE);
         break;
      }
      case FUNC_INT: 
      {
         void (*func)(int);
         func = (void (*)(int)) cmd->val;
         get_val_and_exec(func,s,atoi,prompt_int,0);
         break;
      }
      case FUNC_FLOAT:
      {
         void (*func)(float);
         func = (void (*)(float)) cmd->val;
         if (do_prompt) func((float)prompt_double(s));
         else if ((s)&&(*s)) func(atofloat(s));
         else func(0.0f);
         break;
      }
      case FUNC_DOUBLE:
      {
         void (*func)(double);
         func = (void (*)(double)) cmd->val;
         get_val_and_exec(func,s,atof,prompt_double,0.0);
         break;
      }
      case FUNC_STRING: 
      {
         void (*func)(char *);
         func = (void (*)(char *)) cmd->val;
         // perhaps allocate then deallocate the memory
         // get_val_and_exec(func,s,(char *),prompt_string,s);
         // ok, strings are funky so do the following "special" get_val_and_exec
         if (do_prompt)
         {
            char *tmp=Malloc(PROMPT_STR_LEN*sizeof(char *));
            func(prompt_string(s,tmp));
            Free(tmp);
         }
         else //  if ((s)&&(*s)) - since for strings both elses are the same, ie call with s
            func((char *)s);
         break;
      }
      case VAR_BOOL:
      {
         bool *var = (bool *) cmd->val;
         if (*s)
            *var = atobool(s);
         else
            mprintf("%s=%d\n", cmd->name, (int) *var);
         // vm_redraw(); 
         break;
      }
      case VAR_INT:
      {
         int *var = (int *) cmd->val;
         if (*s)
            *var = atoi(s);
         else
            mprintf("%s=%d\n", cmd->name, (int) *var);
         // vm_redraw(); 
         break;
      }
      case VAR_STRING:
      {
         char *var = (char *) cmd->val;
         strncpy(var, s, CMD_STRING_LEN);
         // vm_redraw(); 
         break;
      }
      case VAR_INT_ARRAY:
      {  // TODO: DO THIS! argh
#ifndef SHIP
         mprintf("Can't set arrays yet, har!\n");
#endif
         // vm_redraw(); 
         break;
      }
      case VAR_FLOAT:
      {
         float *var = (float *) cmd->val;
         if (*s)
            *var = strtof(s,NULL);
         else
            mprintf("%s=%g\n", cmd->name, (float) *var);
         break;
      }
      case TOGGLE_BOOL:
      {
         bool *var = (bool *) cmd->val;
         if (*s)
            *var = atobool(s);
         else {
            char str[80];
            *var = !*var;
            sprintf(str,"%s=%d\n", cmd->name, (int) *var);
            mprintf(str);
            Status(str);
         }
         // vm_redraw(); 
         break;
      }
      case TOGGLE_INT:
      {
         int *var = (int *) cmd->val;
         if (*s)
            *var = atoi(s);
         else
            *var = !*var;
         // vm_redraw(); 
         break;
      }
   }
}

///////////////////// built-in commands ////////////////////

static const char *type_name[] =
{
   "function",
   "bool function",
   "int function",
   "float function",
   "double function",
   "string function",
   "boolean variable",
   "integer variable",
   "string variable",
   "integer array",
   "float variable",
   "boolean toggle",
   "integer toggle"
};

static bool help_text_contains(const char *text, const char *query,
                               int query_len)
{
   int text_len;
   int i;

   if (!text || !query || query_len <= 0)
      return FALSE;
      text_len = (int)strlen(text);
   if (query_len > text_len)
      return FALSE;

   for (i = 0; i <= text_len - query_len; ++i)
      if (!strnicmp(text + i, query, query_len))
         return TRUE;
   return FALSE;
}

enum eHelpGroupMask
{
   kHelpMission     = 1 << 0,
   kHelpPlayer      = 1 << 1,
   kHelpRender      = 1 << 2,
   kHelpAI          = 1 << 3,
   kHelpObjects     = 1 << 4,
   kHelpPhysics     = 1 << 5,
   kHelpScripts     = 1 << 6,
   kHelpAudio       = 1 << 7,
   kHelpSimulation  = 1 << 8,
   kHelpNetwork     = 1 << 9,
   kHelpDiagnostics = 1 << 10,
   kHelpSystem      = 1 << 11
};

typedef struct sHelpGroup
{
   const char *name;
   const char *description;
   uint mask;
} sHelpGroup;

static const sHelpGroup help_groups[] =
{
   { "mission", "mission selection, loading, saving, and objectives", kHelpMission },
   { "player", "player state, movement, inventory, weapons, and HUD", kHelpPlayer },
   { "render", "rendering, lighting, textures, portals, sky, and display", kHelpRender },
   { "ai", "AI state, senses, pathfinding, and behavior", kHelpAI },
   { "objects", "objects, properties, links, traits, and containment", kHelpObjects },
   { "physics", "physics, gravity, velocity, and collision", kHelpPhysics },
   { "scripts", "scripts, messages, OSMs, and Squirrel", kHelpScripts },
   { "audio", "sounds, schemas, music, and ambient audio", kHelpAudio },
   { "simulation", "simulation, time, difficulty, and quest state", kHelpSimulation },
   { "network", "networking and ghost state", kHelpNetwork },
   { "debug", "diagnostics, reports, dumps, tests, and visualization", kHelpDiagnostics },
   { "system", "console, bindings, configuration, history, and process control", kHelpSystem },
   { "other", "uncategorized legacy commands", 0 }
};

static bool help_query_is(const char *query, int query_len, const char *text)
{
   int text_len = (int)strlen(text);
   return query_len == text_len && !strnicmp(query, text, query_len);
}

static bool help_name_contains(const Command *cmd, const char *text)
{
   return help_text_contains(cmd->name, text, (int)strlen(text));
}

static uint help_command_group_mask(const Command *cmd)
{
   uint mask = 0;

   if (help_name_contains(cmd, "mission") || help_name_contains(cmd, "miss_") ||
       help_name_contains(cmd, "objective") || help_name_contains(cmd, "campaign") ||
       help_name_contains(cmd, "save_game") || help_name_contains(cmd, "load_game"))
      mask |= kHelpMission;
   if (help_name_contains(cmd, "player") || help_name_contains(cmd, "plyr") ||
       help_name_contains(cmd, "immunity") || help_name_contains(cmd, "flying") ||
       help_name_contains(cmd, "invisible") || help_name_contains(cmd, "weapon") ||
       help_name_contains(cmd, "inventory") || help_name_contains(cmd, "hud") ||
       help_name_contains(cmd, "reticle") || help_name_contains(cmd, "frob"))
      mask |= kHelpPlayer;
   if (help_name_contains(cmd, "render") || help_name_contains(cmd, "rend_") ||
       help_name_contains(cmd, "draw") || help_name_contains(cmd, "fog_") ||
       help_name_contains(cmd, "light_") || help_name_contains(cmd, "texture") ||
       help_name_contains(cmd, "tex_") || help_name_contains(cmd, "portal") ||
       help_name_contains(cmd, "cell") || help_name_contains(cmd, "poly") ||
       help_name_contains(cmd, "mip") || help_name_contains(cmd, "sky") ||
       help_name_contains(cmd, "gamma") || help_name_contains(cmd, "screen"))
      mask |= kHelpRender;
   if (!strnicmp(cmd->name, "ai", 2) || help_name_contains(cmd, "_ai") ||
       help_name_contains(cmd, "pathfind"))
      mask |= kHelpAI;
   if (help_name_contains(cmd, "obj") || help_name_contains(cmd, "prop") ||
       help_name_contains(cmd, "link") || help_name_contains(cmd, "trait") ||
       help_name_contains(cmd, "archetype") || help_name_contains(cmd, "metaprop") ||
       help_name_contains(cmd, "contain"))
      mask |= kHelpObjects;
   if (help_name_contains(cmd, "phys") || help_name_contains(cmd, "gravity") ||
       help_name_contains(cmd, "velocity") || help_name_contains(cmd, "collision"))
      mask |= kHelpPhysics;
   if (help_name_contains(cmd, "script") || help_name_contains(cmd, "scr_") ||
       help_name_contains(cmd, "osm") || help_name_contains(cmd, "squirrel"))
      mask |= kHelpScripts;
   if (help_name_contains(cmd, "sound") || help_name_contains(cmd, "snd_") ||
       help_name_contains(cmd, "schema") || help_name_contains(cmd, "music") ||
       help_name_contains(cmd, "ambient"))
      mask |= kHelpAudio;
   if (help_name_contains(cmd, "sim_") || help_name_contains(cmd, "time") ||
       help_name_contains(cmd, "clock") || help_name_contains(cmd, "difficulty") ||
       help_name_contains(cmd, "quest"))
      mask |= kHelpSimulation;
   if (help_name_contains(cmd, "net_") || help_name_contains(cmd, "network") ||
       help_name_contains(cmd, "ghost"))
      mask |= kHelpNetwork;
   if (help_name_contains(cmd, "debug") || help_name_contains(cmd, "dump") ||
       help_name_contains(cmd, "spew") || help_name_contains(cmd, "stats") ||
       help_name_contains(cmd, "report") || help_name_contains(cmd, "watch") ||
       help_name_contains(cmd, "test") || help_name_contains(cmd, "show_"))
      mask |= kHelpDiagnostics;
   if (help_name_contains(cmd, "console") || help_name_contains(cmd, "help") ||
       help_name_contains(cmd, "bind") || help_name_contains(cmd, "config") ||
       help_name_contains(cmd, "history") || help_name_contains(cmd, "gamepad") ||
       help_query_is(cmd->name, (int)strlen(cmd->name), "run") ||
       help_query_is(cmd->name, (int)strlen(cmd->name), "quit") ||
       help_query_is(cmd->name, (int)strlen(cmd->name), "exit"))
      mask |= kHelpSystem;

   return mask;
}

static const sHelpGroup *help_find_group(const char *query, int query_len)
{
   int i;
   for (i = 0; i < (int)(sizeof(help_groups) / sizeof(help_groups[0])); ++i)
      if (help_query_is(query, query_len, help_groups[i].name))
         return &help_groups[i];

   if (help_query_is(query, query_len, "rendering"))
      return &help_groups[2];
   if (help_query_is(query, query_len, "object"))
      return &help_groups[4];
   if (help_query_is(query, query_len, "script"))
      return &help_groups[6];
   if (help_query_is(query, query_len, "sound"))
      return &help_groups[7];
   if (help_query_is(query, query_len, "sim"))
      return &help_groups[8];
   return NULL;
}

static const char *help_command_type(const Command *cmd)
{
   return (cmd->type >= FUNC_VOID && cmd->type <= TOGGLE_INT)
        ? type_name[cmd->type] : "unknown";
}

static void help_print_command(const Command *cmd)
{
   const char *type = help_command_type(cmd);
   if (cmd->comment)
      mprintf("%s (%s): %s\n", cmd->name, type, cmd->comment);
   else
      mprintf("%s (%s)\n", cmd->name, type);
}

static void help_commands(const char *s)
{
   int matches = 0;
   int j;
   const char *query = s;
   const char *query_end;
   const sHelpGroup *group;
   int query_len;

   while (query && *query && isspace((unsigned char)*query))
      ++query;
   query_end = query ? query + strlen(query) : query;
   while (query_end && query_end > query &&
          isspace((unsigned char)query_end[-1]))
      --query_end;
   query_len = query_end && query ? (int)(query_end - query) : 0;

#ifdef EDITOR
   // DromEd registers thousands of commands. Its existing searchable command
   // reference is more useful than streaming the entire registry into the
   // embedded console. A filtered query remains available in the console.
   if (!query_len)
   {
      Command *window_help = CommandFindString("show_command_help");
      if (window_help && (window_help->contexts & *command_context_ptr))
      {
         CommandExecute("show_command_help");
         return;
      }
   }
#endif

   if (!query_len)
   {
      int i;
      mprintf("Command groups:\n");
      for (i = 0; i < (int)(sizeof(help_groups) / sizeof(help_groups[0])); ++i)
         mprintf("  %-12s %s\n", help_groups[i].name,
                 help_groups[i].description);
      mprintf("Use help <group> to list that group.\n");
      mprintf("Use help <keyword> to search names and descriptions.\n");
      mprintf("Use help all only when you want the complete registry.\n");
      return;
   }

   group = help_find_group(query, query_len);
   if (group || help_query_is(query, query_len, "all"))
   {
      bool all = help_query_is(query, query_len, "all");
      if (all)
         mprintf("All commands available in this mode:\n");
      else
         mprintf("%s commands:\n", group->name);

      for (j = 0; j < command_list_size; ++j)
      {
         int i;
         for (i = 0; i < command_count[j]; ++i)
         {
            Command *cmd = &command_list[j][i];
            uint mask;
            if (!(cmd->contexts & *command_context_ptr))
               continue;
            mask = help_command_group_mask(cmd);
            if (!all && ((group->mask && !(mask & group->mask)) ||
                         (!group->mask && mask)))
               continue;
            ++matches;
            help_print_command(cmd);
         }
      }
      mprintf("%d matching commands.\n", matches);
      return;
   }

   if (query_len)
   {
      for (j=0; j < command_list_size; ++j) 
      {
         Command* set = command_list[j];
         int count = command_count[j];
         int i;

         for (i=0; i < count; ++i)
         {
            Command *cmd = &set[i];
            if (!(set[i].contexts & *command_context_ptr))
               continue;
            if (!help_text_contains(cmd->name, query, query_len) &&
                !help_text_contains(cmd->comment, query, query_len) &&
                !help_text_contains(help_command_type(cmd), query, query_len))
               continue;

            ++matches;
            help_print_command(cmd);
         }

      }
   }

   if (!matches)
      mprintf("No matching commands are available in this mode.\n");
}

// track command finds
static int which_list=0, which_cmd=0;

const char *command_find(const char *prefix, BOOL restart)
{
   int match_len=0;
   BOOL found_us=FALSE;

   if (restart) which_list=which_cmd=0;
      if (prefix)  match_len=(int)strlen(prefix);

   for (; which_list < command_list_size; which_list++, which_cmd=0)
   {
      Command *set=command_list[which_list];
      for (; which_cmd < command_count[which_list]; which_cmd++)
      {
         if (!(set[which_cmd].contexts & *command_context_ptr))
            continue;
         if (match_len)
         {
            if (strnicmp(set[which_cmd].name,prefix,match_len)==0)
               found_us=TRUE;
         }
         else
            found_us=TRUE;
         if (found_us)
            return set[which_cmd++].name;
      }
   }
   return NULL;
}

char *cur_command_help_str(char *buf)
{
   if (which_cmd>0)
   {
      Command *cmd=&command_list[which_list][which_cmd-1];
      if (cmd->comment)
         sprintf(buf,"%s (%s)",cmd->comment, type_name[cmd->type]);
      else
         sprintf(buf,"command: (%s)", type_name[cmd->type]);
      return buf;
   }
   else
      return NULL;
}

BOOL CommandParseStringArgs(char *args, char *arg1, int arg1_len, 
                            char **arg2)
{
   int i = 0;
   int j = 0;

   // copy out first argument
   while ((args[i] != '\0') && (args[i] != ','))
   {
      if (j<arg1_len-1)
         arg1[j++] = args[i];
      ++i;
   }
   arg1[j] = '\0';
   // point to second argument
   if (args[i] != '\0')
   {
      // skip leading spaces
      ++i;
      while ((args[i] == ' ') && (args[i] != '\0'))
         ++i;
      *arg2 = &(args[i]);
      return TRUE;
   }
   // missing second argument
   *arg2 = NULL;
   return FALSE;
}

static void dump_commands(const char *s)
{
   FILE *f;
   int j,i;

   if (!(s && *s))
      return;

   f = fopen(s, "w");

   if (!f) {
      Warning(("Couldn't open command-dump file '%s'.\n", s));
      return;
   }

   for (j=0; j < command_list_size; ++j)
      for (i=0; i < command_count[j]; ++i)
      {
         Command *cmd = &command_list[j][i];
         fprintf(f, "%-20s:  %s\n", cmd->name,
               cmd->comment ? cmd->comment : type_name[cmd->type]);
      }

   fclose(f);
}

// wouldnt it be cool if this used the "script_path" variable????
// for now, ill do that outside of here, as this is "pure system of 90s"
void CommandRunScript(const char *s)
{
   static char buf[256];
   FILE *f;
   f = fopen(s, "r");
   if (f) {
      while (fgets(buf, 256, f)) {
         buf[255] = 0;
         buf[strlen(buf)-1] = 0;  // clear the \n
         CommandExecute(buf);
         // note that if this recurses, because buf is static,
         // we won't nest buffers down the stack infinitely;
         // we reuse this one, which is safe
      }
      fclose(f);
   }
}

static Command commands[] =
{
   { "help", FUNC_STRING, help_commands, "shows command info" },
   { "run", FUNC_STRING, CommandRunScript, "run commands in file" },
   { "dump_cmds", FUNC_STRING, dump_commands, "dump command list to file" }
};

void CommandInit(void)
{
   COMMANDS(commands,0xffffffff);
}
