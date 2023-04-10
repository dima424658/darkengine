/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swaptool.h,v 1.5 2000/01/29 13:13:17 adurant Exp $
// tools for swap region buttons
#pragma once

#ifndef __SWAPTOOL_H
#define __SWAPTOOL_H

#include <rect.h>
#include <gadblist.h>

// build a button list for the Swappable region, use flags in gadblist.h
void *SwapBuildList(LGadRoot *root, Rect *area, char* title, int n, char *strings[], void (*shoot)(int idx),ulong flags);

// call this will the void * returned to you by BuildList
void SwapDestroyList(void *data);

// call this to redraw your list
void SwapRedrawList(void *list);

// void call this to set and get the "current button" if any
int SwapListSelection(void* list);

// for radio buttons, select sets selection
// for normal lists. select/deselect colors/uncolors that button (according to guiStyle)
void SwapListSelect(void* list, int selection);
void SwapListDeSelect(void* list, int selection);

#endif // __SWAPTOOL_H
