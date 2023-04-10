/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/robjbase.h,v 1.3 1998/05/22 19:32:53 dc Exp $

#pragma once

#ifndef __ROBJBASE_H
#define __ROBJBASE_H

#include <objtype.h>

//// MODEL FUNCTIONS:  
//    TO RENDER THE MODEL, JUST CALL SETUP, RENDER, AND FINISH


//// RENDER FUNCTION
//  do model index, if not there, draw wedge of color color
//  color 0 means base it on index, color -1 means do nothing if the model isnt there

//// SETUP FUNCTION
// do the startobjectangles blah blah

//// FINISH FUNCTION
// do any finish for the object
// also holds the code for naming objects on screen, which should get moved

//// ENTER LOCAL SPACE FUNCTION
// sets up r3 context so in object's coordinate system.  
// do not use this when rendering the object!  only used for hacks etc
// where app wants to get into object's frame of reference.
// this is only guaranteed to work if called between setup and finish calls

//// LEAVE LOCAL SPACE FUNCTION
// undoes enter space stuff

//// QUEUE STORE FUNCTION
// transforms object and stores it in model queue.  
// Used for multi-pass rendering for object sorting in world. 
// This function is optional.

//// QUEUE RENDER FUNCTION
// render queued, pre-transformed model. 
// Also optional, but must be provided if queue storage function provided.

struct model_funcs
{
   BOOL (*render)(int index, ObjID obj, int fragment, int color,
                  uchar *clut);
   void (*setup)(ObjID);
   void (*finish)(ObjID);
   void (*enterLocalSpace)(ObjID);
   void (*leaveLocalSpace)(ObjID); // only valid if called between setup and finish
   BOOL (*queueStore)(int index, ObjID obj, int fragment,
                      int color, uchar *clut);
   void (*queueRender)(void *queue, uchar *clut);
};

#endif

