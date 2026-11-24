/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/simpwrap.cpp,v 1.2 2000/02/19 13:11:25 toml Exp $
// Wrapper for the DARKDLGS simple-list dialog.

#include <simpwrap.h>
#include <darkdlgs/darkdlgs.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

int PickFromStringList(const char *title, const menustring* list, int cnt)
{
   return DoSimpleMenu(title,list,cnt);
}
