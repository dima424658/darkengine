/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/ged_win.h,v 1.3 2000/01/31 10:04:57 adurant Exp $
// windows UI utility elements of Gedit
#pragma once

#ifndef __GED_WIN_H
#define __GED_WIN_H

// get a y or n 
EXTERN BOOL ged_winui_GetYorN(char *msg);

// display a text message
EXTERN void ged_winui_Text(char *msg);

#endif  // __GED_WIN_H

