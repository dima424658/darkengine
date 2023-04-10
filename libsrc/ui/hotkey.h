// $Header: x:/prj/tech/libsrc/ui/RCS/hotkey.h 1.9 1998/06/18 13:28:44 JAEMZ Exp $

#ifndef __HOTKEY_H
#define __HOTKEY_H
#pragma once


// Includes
#include <lg.h>  // every file should have this

// C Library Includes

// System Library Includes
#include <error.h>
#include <hash.h> 
#include <kbcook.h> 
#include <array.h>

// Master Game Includes

// Game Library Includes

// Game Object Includes


// Defines

#define HOTKEY_HELP  1

#define HKSORT_NONE     0
#define HKSORT_KEYCODE  1
#define HKSORT_ASCII    2

typedef bool (*hotkey_callback)(short keycode, ulong context, void* state); 

typedef struct _hotkey_entry
{
   ushort key;
   Array keychain;
   int first;
} hotkey_entry;

typedef struct _hotkey_link
{
   ulong context;
#ifdef HOTKEY_HELP
   hotkey_callback func;
#endif
   void* state;
   char *help_text;
   int next;
} hotkey_link;

#ifdef __HOTKEY_SRC
Hashtable hotkey_table;
#else
extern Hashtable hotkey_table;
#endif

// Prototypes

EXTERN errtype hotkey_init(int tblsize);
// Initialize hotkey table, giving an initial context and table size.  

EXTERN errtype hotkey_add(short keycode, ulong context_mask, hotkey_callback func, void* state);
// installs a hotkey handler for a specific cooked keycode in the set of contexts described by context_mask.  
// This handler will take precidence over previously-installed handlers.  

#ifdef HOTKEY_HELP
EXTERN errtype hotkey_add_help(short keycode, ulong context_mask, hotkey_callback func, void* state, char *help_text);
// like hotkey_add, but also takes a help string which it stores
// for later reference.

EXTERN char *hotkey_help_text(short keycode, ulong contexts, hotkey_callback func);
// looks up the help string for a given hotkey

#endif

EXTERN errtype hotkey_remove(short keycode, ulong context_mask, hotkey_callback func);
// delete all hotkey handlers with the specified keycode and callback function 
// from the contexts specified by the context_mask. 

EXTERN errtype hotkey_replace(short keycode,
    ulong old_contexts, hotkey_callback old_func, void *old_state,
    ulong new_contexts, hotkey_callback new_func, void *new_state);
// replace a single instance of a hotkey handler fully identified
// by old_contexts,old_func,old_state with a new handler with
// new_contexts, new_func, new_state.  This is added to support
// late binding by the command-key binding system, but might be
// generally useful for other data-driven systems.

EXTERN errtype hotkey_dispatch(short keycode);
// dispatches the keycode to the highest-priority key handler for that 
// keycode that has any set bits in common with HotkeyContext.  

EXTERN void hotkey_set_shutdown_callback(hotkey_callback func);
// install a function to be called to clean up state on hotkey_shutdown()

EXTERN errtype hotkey_shutdown(void);
// shut down the hotkey system.

#ifdef GODDAMN_THIS_MESS_IS_IMPOSSIBLE
EXTERN bool hotkey_list(char **item, int sort_type);
// stores in item a string that is the next hotkey string off of the
// list, along with it's help text.  Returns whether or not there
// are more hotkeys to list out.  sort_type determines what sorting
// method is used.

EXTERN errtype hotkey_list_clear();
// Starts hotkey listing at the beginning.
#endif

// Globals

EXTERN ulong HotkeyContext;  

#endif // __HOTKEY_H
