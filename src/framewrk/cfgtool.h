/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/cfgtool.h,v 1.12 2000/01/29 13:20:24 adurant Exp $
#pragma once

#ifndef __CFGTOOL_H
#define __CFGTOOL_H
#include <lg.h>

//////////////////////////////////////////////////////////////
// STUPID CONFIG SYSTEM TOOLS
//

//
// commands
// 

//////////////////////
// set <var> <val>
//
EXTERN void set_config_cmd(char* arg);

#define CONFIG_SET_CMD(name) \
   { name, FUNC_STRING, set_config_cmd, "set a config var: set <var> <value>", HK_ALL}

//////////////////////
// get <var>
// 
EXTERN void get_config_cmd(char* arg);

#define CONFIG_GET_CMD(name) \
{ name, FUNC_STRING, get_config_cmd, "Look up a config value: get <var>", HK_ALL}

//////////////////////
// eval <var> <cmd>
// <cmd> has "" in it somewhere, which is replaced by the value of <var> 
// and then evaluated.
//

EXTERN void eval_config_cmd(char* arg); 

#define CONFIG_EVAL_CMD(name) \
{ name, FUNC_STRING, eval_config_cmd, "Exec a command with config substitution: eval <var> <cmd>", HK_ALL}

//////////////////////
// ifdef <var> <cmd>
// iff <var> is defined, execute <cmd>
//

EXTERN void ifdef_config_cmd(char* arg); 

#define CONFIG_IFDEF_CMD(name) \
{ name, FUNC_STRING, ifdef_config_cmd, "exec a command if a config variable is defined: ifdef <var> <cmd>", HK_ALL}

//////////////////////
// ifndef <var> <cmd>
// iff <var> is not defined, execute <cmd>
//

EXTERN void ifndef_config_cmd(char* arg); 

#define CONFIG_IFNDEF_CMD(name) \
{ name, FUNC_STRING, ifndef_config_cmd, "exec a command if a config variable is undefined: ifndef <var> <cmd>", HK_ALL}


//////////////////////
// unset <var> 
// unset a config var
//

EXTERN void undef_config_cmd(char* arg); 

#define CONFIG_UNSET_CMD(name) \
{ name, FUNC_STRING, undef_config_cmd, "Unset a config variable: unset <var>", HK_ALL}

////////////////////////////////////////////////////////////
// NON-COMMANDS
//


//
// Find all config variables that start with prefix, and load their values as
// config files.
//
EXTERN void process_config_includes(const char* prefix);

//
// Find all config variables that start with prefix, and run their values as
// command scripts
//
EXTERN void process_config_scripts(const char* prefix);

//
// Looks up a path in a config var, finds a filename in that path, and 
// fills in targ with the full path
//

EXTERN BOOL find_file_in_config_path(char* targ, char* filename, char* path_var);

#endif // __CFGTOOL_H

