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

unsigned long  command_context = 0xffffffff;
unsigned long *command_context_ptr = &command_context;

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

Command *CommandFind(char *s, int n)
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

Command *CommandFindString(char *s)
{
   return CommandFind(s, strlen(s));
}

bool CommandParse(char *inp, Command **res_1, char **res_2)
{
   // parse out to the first blank
   char *s;
   int n;
   Command *cmd;

     // skip leading whitespace
   while (*inp && isspace(*inp)) ++inp;

     // if empty it's a no-op
   if (!*inp) return FALSE;

   s = inp;
   while (*s && !isspace(*s)) ++s;

   n = s - inp;

   cmd = CommandFind(inp, n);

   if (!cmd) return FALSE;

   *res_1 = cmd;
   *res_2 = s;

   return TRUE;
}

static char *fail_return = "No such Command";
char *CommandExecute(char *inp)
{
   // parse out to the first blank
   char *s;
   Command *cmd;

   if ((inp==NULL)||(inp[0]=='\0')||(inp[0]==';'))
      return fail_return;  // bonus null command fun
   if (CommandParse(inp, &cmd, &s)) {
      CommandExecuteParsed(cmd, s);
      return NULL;
   } else {
      Warning(("Unknown command %s\n", inp));
      return fail_return;
   }
}

bool CommandExecuteParam(char *inp, char *parm)
{
   Command *cmd = CommandFindString(inp);
   if (cmd) {
      CommandExecuteParsed(cmd, parm);
      return TRUE;
   } else {
      Warning(("Unknown command %s\n", inp));
      return FALSE;
   }
}

bool atobool(char *s)
{
   if (!stricmp(s, "false")) return FALSE;
   if (!stricmp(s, "true" )) return TRUE;
   return atoi(s);
}

//////////////////
// actual execute of the parsed command

// @TODO: the s && *s logic is still wrong!
#define get_val_and_exec(f,s,cvrt,prmpt,def) \
   if (do_prompt) f(prmpt(s)); else if ((s)&&(*s)) f(cvrt(s)); else f(def)

void CommandExecuteParsed(Command *cmd, char *s)
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
         get_val_and_exec(func,s,atof,prompt_double,0.0);
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
            func(s);
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
            *var = atof(s);
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

static char *type_name[] =
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

static void help_commands(char *s)
{
   if (s && *s) 
   {
      int j;

      for (j=0; j < command_list_size; ++j) 
      {
         Command* set = command_list[j];
         int count = command_count[j];
         int i;

         for (i=0; i < count; ++i)
         {
            int k;
            int len = strlen(set[i].name);
            int n = strlen(s);
            for(k = 0; k <= len - n; k++)
               if (strnicmp(set[i].name+k,s,n) == 0)
               {
                  Command *cmd = &set[i];
                  mprintf("%s ",set[i].name);
                  if (cmd->comment)
                     mprintf("(%s): %s\n", type_name[cmd->type], cmd->comment);
                  else
                     mprintf("(%s)\n", type_name[cmd->type]);
               }

         }

      }
   }  
   else 
   {
      int j,i,cnt=0;
      char holding_pen[21];
      holding_pen[19]='\0';
      holding_pen[20]='\0';
      for (j=0; j < command_list_size; ++j)
         for (i=0; i < command_count[j]; ++i)
         {
            strncpy(holding_pen,command_list[j][i].name,19);
            if ((++cnt)&3)  // ie line 0,1,2...
               mprintf("%020s", holding_pen);
            else
            {
               holding_pen[18]='\0';  // replace final char with a \n for monolog
               mprintf("%019s\n", holding_pen);
            }
         }
      if (cnt&3) mprintf("\n"); // clean up with a return
   }
}

// track command finds
static int which_list=0, which_cmd=0;

char *command_find(char *prefix, BOOL restart)
{
   int match_len=0;
   BOOL found_us=FALSE;

   if (restart) which_list=which_cmd=0;
   if (prefix)  match_len=strlen(prefix);

   for (; which_list < command_list_size; which_list++, which_cmd=0)
   {
      Command *set=command_list[which_list];
      for (; which_cmd < command_count[which_list]; which_cmd++)
      {
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

static void dump_commands(char *s)
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
void CommandRunScript(char *s)
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
