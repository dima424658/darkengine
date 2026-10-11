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

EXTERN ulong  command_context;
EXTERN ulong *command_context_ptr;

  // setup command system and add internal commands
EXTERN void CommandInit(void);

  // parse & execute a particular command
  // returns NULL if command existed, an error msg otherwise
EXTERN char *CommandExecute(const char *cmd);

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

#ifdef __cplusplus
class cCommandValue
{
public:
   typedef void (*tErasedFunction)();

   cCommandValue(decltype(nullptr)) : m_data(NULL) {}
   cCommandValue(void *data) : m_data(data) {}

   template <class Return, class... Args>
   cCommandValue(Return (*function)(Args...))
      : m_function(reinterpret_cast<tErasedFunction>(function))
   {
   }

   operator void *() const { return m_data; }

private:
   union
   {
      void *m_data;
      tErasedFunction m_function;
   };
};
#endif

typedef struct
{
   const char *name;
   enum CommandType type;
#ifdef __cplusplus
   cCommandValue val;
#else
   void *val;
#endif
   const char *comment;
   ulong contexts;
} Command;

// Enumerate the registered command definitions without disturbing the
// command-completion iterator used by the command entry box.  Returning FALSE
// from the callback stops enumeration early.
typedef BOOL (LGAPI *CommandIterateCallback)(const Command *command,
                                              void *data);
EXTERN void CommandForEach(CommandIterateCallback callback, void *data);

EXTERN bool CommandParse(const char *inp, Command **res_1, const char **res_2);
EXTERN void CommandExecuteParsed(Command *, const char *parms);
EXTERN bool CommandExecuteParam(const char *cmd, const char *param);

  // register an array of commands
EXTERN void CommandRegister(Command *, int count, ulong contexts);

  // register a statically defined array
#define COMMANDS(x,y)  CommandRegister(x, sizeof(x)/sizeof(x[0]), y)

  // lookup a command, with explicit length
EXTERN Command *CommandFind(const char *, int len);
EXTERN Command *CommandFindString(const char *s);

// Run a command script
EXTERN void CommandRunScript(const char* filename);

// for command completion/tab in cmdterm
EXTERN const char *command_find(const char *prefix, BOOL restart);
EXTERN char *cur_command_help_str(char *buf);

// for commands with two string args, here's a function to parse
// them from a single comma separated string
// arg1 should be a string of length arg1_len - its filled with
// the first argument string (which needs to be copied to be zero
// terminated) and arg2 is set to point at the second argument
EXTERN BOOL CommandParseStringArgs(char *arg, char *arg1, int arg1_len, 
                              char **arg2);
#endif
