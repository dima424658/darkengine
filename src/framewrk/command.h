/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/command.h,v 1.13 2000/01/29 13:20:25 adurant Exp $
#pragma once

#ifndef __COMMAND_H
#define __COMMAND_H

#include <lg.h>

  // anybody who's adding commands needs to see this
#include <contexts.h>

EXTERN unsigned long  command_context;
EXTERN unsigned long *command_context_ptr;

  // setup command system and add internal commands
EXTERN void CommandInit(void);

  // parse & execute a particular command
  // returns NULL if command existed, an error msg otherwise
EXTERN char *CommandExecute(char *cmd);

// types of commands

enum CommandType
{
   FUNC_VOID,
   FUNC_BOOL,
   FUNC_INT,
   FUNC_FLOAT,
   FUNC_DOUBLE,
   FUNC_STRING,

   VAR_BOOL,
   VAR_INT,
   VAR_STRING,
   VAR_INT_ARRAY,
   VAR_FLOAT,

   TOGGLE_BOOL,
   TOGGLE_INT
};

  // length you must provide for a string var
#define CMD_STRING_LEN   32

// command definition structure

typedef struct
{
   char *name;
   enum CommandType type;
   void *val;
   char *comment;
   ulong contexts;
} Command;

EXTERN bool CommandParse(char *inp, Command **res_1, char **res_2);
EXTERN void CommandExecuteParsed(Command *, char *parms);
EXTERN bool CommandExecuteParam(char *cmd, char *param);

  // register an array of commands
EXTERN void CommandRegister(Command *, int count, ulong contexts);

  // register a statically defined array
#define COMMANDS(x,y)  CommandRegister(x, sizeof(x)/sizeof(x[0]), y)

  // lookup a command, with explicit length
EXTERN Command *CommandFind(char *, int len);
EXTERN Command *CommandFindString(char *s);

// Run a command script
EXTERN void CommandRunScript(char* filename);

// for command completion/tab in cmdterm
EXTERN char *command_find(char *prefix, BOOL restart);
EXTERN char *cur_command_help_str(char *buf);

// for commands with two string args, here's a function to parse
// them from a single comma separated string
// arg1 should be a string of length arg1_len - its filled with
// the first argument string (which needs to be copied to be zero
// terminated) and arg2 is set to point at the second argument
EXTERN CommandParseStringArgs(char *arg, char *arg1, int arg1_len, 
                              char **arg2);
#endif
