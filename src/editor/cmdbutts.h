/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cmdbutts.h,v 1.2 2000/01/29 13:11:21 adurant Exp $
#pragma once

#ifndef __CMDBUTTS_H
#define __CMDBUTTS_H

#include <gadget.h>
#include <gadblist.h>

//
// Simple function to create a buttonlist where each button signals a command
// from the config file.
//


typedef struct _CmdButtonListDesc 
{
   int num_buttons;
   DrawElement* appearance; // appearance of each button (array)
   Rect* rects; // array of rectangles
   char** commands; // commands to execute for each button
} CmdButtonListDesc;


EXTERN LGadButtonList* CreateCmdButtonList(LGadButtonList* list, LGadRoot* root, CmdButtonListDesc* desc);

//
// Tool for laying out rectangles
// 
// fill rvec with a matrix of rectangles whose matrix dimensions are dims
// with spacing between each rect.

EXTERN void LayoutRectangles(Rect* bounds, Rect rvec[], int num_rects, Point dims, Point spacing);

//
// Fill in a desc given a number of buttons and a config variable prefix
//
// uses <prefix>_name_<n> for the name of button n and 
//      <prefix>_cmd_<n> for the command of button n
// malloc's a whole bunch of stuff.  don't destroy the desc until after
// you've destroyed the buttonlist you make with it.


EXTERN void CreateConfigButtonDesc(CmdButtonListDesc* desc, int num_buttons, Rect* bounds, char* var_prefix, Point dims, Point spacing);

EXTERN void DestroyConfigButtonDesc(CmdButtonListDesc *desc);

#endif // __CMDBUTTS_H
