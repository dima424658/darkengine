/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkbndui.h,v 1.1 1998/11/02 02:59:14 mahk Exp $
#pragma once  
#ifndef __DRKBNDUI_H
#define __DRKBNDUI_H

#include <str.h>
////////////////////////////////////////////////////////////
// DARK KEY BINDING UI 
//

//Will stuff retrieve_num many controls bound to cmd into the dest
//string array, and will return how many actually got stuffed 
extern int GetCmdBinds (const char *cmd, cStr *dest, int retrieve_num); 

#endif // __DRKBNDUI_H
