/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $header: $
#include <lg.h>
#include <cfgtool.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <cfg.h>
#include <mprintf.h>

#include <command.h>
#include <status.h>
#include <scrnmode.h>
#include <scrnman.h>

#include <datapath.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void set_config_cmd(char* arg)
{
   int n;
   char var[64],*val;
   sscanf(arg,"%s %n",var,&n);
   if (n >= 0 && n < strlen(arg))
      for (val = arg + n; *val != '\0' && isspace(*val); val++)
         ;
   else
      val = "";
   mprintf("setting '%s' to '%s'\n",var,val);
   config_set_string(var,val);
   config_set_priority(var,PRIORITY_TRANSIENT);
   Status("Config variable set.");
}


void get_config_cmd(char* arg)
{
   char var[64];
   char buf[80];

   sscanf(arg,"%s ",var);
   mprintf("getting '%s'\n",var);
   if (config_get_raw(var,buf,sizeof(buf)))
   {
      char outbuf[90];
      sprintf(outbuf,"Value: %s",buf);
      Status(outbuf);
   }
   else
      Status("No such config var");
}


void eval_config_cmd(char* arg)
{
   ulong n;
   char var[64],*val;
   char valbuf[80] = "";
   char cmdbuf[256];

   sscanf(arg,"%s %n",var,&n);
   for (val = arg + n; *val != '\0' && isspace(*val); val++)
      ;
   config_get_raw(var,valbuf,sizeof(valbuf));
   sprintf(cmdbuf,val,valbuf);
   if (CommandExecute(cmdbuf))
      Status("No such command.");
}


void ifdef_config_cmd(char* arg)
{
   ulong n;
   char var[64],*cmd;
   sscanf(arg,"%s %n",var,&n);
   for (cmd = arg + n; *cmd != '\0' && isspace(*cmd); cmd++)
      ;
   if (config_is_defined(var))
      if (CommandExecute(cmd))
         Status("No such command.");
}


void ifndef_config_cmd(char* arg)
{
   ulong n;
   char var[64],*cmd;
   sscanf(arg,"%s %n",var,&n);
   for (cmd = arg + n; *cmd != '\0' && isspace(*cmd); cmd++)
      ;
   if (!config_is_defined(var))
      if (CommandExecute(cmd))
         Status("No such command.");
}


void undef_config_cmd(char* arg)
{
   ulong n = 0;
   char var[64], *cmd;
   sscanf(arg,"%s %n", var, &n);
   for (cmd = arg + n; *cmd != '\0' && isspace(*cmd); cmd++)
      ;
   config_unset(var);
}




////////////////////////////////////////


static const char* iter_prefix;
static char* iter_val;

#define INCLUDE_PATH "include_path"

// @TODO: lift this cap 
#define MAX_FILES 64
static char (*include_files)[80]; 
static int files_found; 

static bool include_iter(char* var)
{
   if (strnicmp(iter_prefix,var,strlen(iter_prefix)) == 0
       && files_found < MAX_FILES)
   {
      
      int i = files_found++; 
      strncpy(include_files[i],iter_val,sizeof(include_files[i]));
   }
   return FALSE;
}

void process_config_includes(const char* prefix)
{
   int i; 
   char buf[256];

   iter_val = buf;
   iter_prefix = prefix;
   files_found = 0;
   include_files = Malloc(sizeof(*include_files)*MAX_FILES); 
 
   config_get_raw_all(include_iter,buf,sizeof(buf));

   for (i = 0; i < files_found; i++)
   {
      char fbuf[256]; 
      find_file_in_config_path(fbuf,include_files[i],INCLUDE_PATH);
      config_load(fbuf);
   }
   Free(include_files);
   include_files = NULL; 
}

////////////////////////////////////////


#define SCRIPT_PATH "script_path"

static bool script_iter(char* var)
{
   if (strnicmp(iter_prefix,var,strlen(iter_prefix)) == 0)
   {
      char buf[256];
      find_file_in_config_path(buf,iter_val,SCRIPT_PATH);
      CommandRunScript(buf);
   }
   return FALSE;
}

void process_config_scripts(const char* prefix)
{
   char buf[256];
   iter_val = buf;
   iter_prefix = prefix;
   config_get_raw_all(script_iter,buf,sizeof(buf));
}


////////////////////////////////////////////////////////////

// @ROBUSTIFY: make args const.
BOOL find_file_in_config_path(char* targ, const char* filename, const char* path_var)
{
   Datapath path;
   char pathbuf[256] = "";
   BOOL result;
   DatapathClear(&path);
   if (!config_get_raw(path_var, pathbuf, sizeof(pathbuf)))
       return FALSE;

   DatapathAdd(&path,pathbuf);
   result = DatapathFind(&path,filename,targ,256);
   DatapathFree(&path);
   return result;
}
