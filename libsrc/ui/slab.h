// $Header: x:/prj/tech/libsrc/ui/RCS/slab.h 1.4 1998/06/18 13:29:18 JAEMZ Exp $

#ifndef __SLAB_H
#define __SLAB_H
#pragma once


// A slab is a collection of information about where to send input
// events, and where to look for mouse cursors.  Every slab comes complete with: 
//  1) a root cursor region, which will be traversed upon to 
//     find regional mouse cursors.  
//  2) A default cursor stack.  If the mouse is in a region
//     which doesn't specify a mouse cursor, the top of the slab's cursor stack
//     is used instead.  
//  3) A focus chain.  This determines which regions have input focus.  

// Only one slab is active at one time, and that slabbed is looked at
// by uiPoll.    



// Includes
#include <lg.h>  // every file should have this
#include <error.h>
#include <array.h>
#include <region.h>
#include <cursors.h>

// Defines

typedef struct _ui_slab
{
   Region* creg;  // cursor region.
   struct _focus_chain
   {
      Array chain;
      int curfocus;
   } fchain;  // focus chain
   cursor_stack cstack;
} uiSlab;


// Prototypes

EXTERN errtype uiMakeSlab(uiSlab* slab,Region* cursor_reg, Cursor* default_cursor);
// Initialize a region with the specified cursor region, default cursor. 
// the initial focus is usually the root region.  

EXTERN errtype uiSetCurrentSlab(uiSlab* slab);
// Sets the current active slab.  

EXTERN errtype uiGetCurrentSlab(uiSlab** slab);
// Gets the current active slab;

EXTERN errtype uiDestroySlab(uiSlab* slab);
// shuts down a slab, freeing any satellite data. 


// Globals

#endif // __SLAB_H
