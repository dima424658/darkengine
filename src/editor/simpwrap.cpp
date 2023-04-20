/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/simpwrap.cpp,v 1.2 2000/02/19 13:11:25 toml Exp $
// wrappers around darkdlgs dll "simple" menus and other ui tools

#include <wtypes.h>
#include <dynfunc.h>

#include <simpwrap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// here we go, copied in dll hookup zaniness

//////////////
// simplemenu

static int simpmenu_woe(const char*, const menustring*, int )
{
//   CriticalMsg("Could not load dialog!");
   return -1; 
}

DeclDynFunc_(int, LGAPI, DoSimpleMenu, (const char*, const menustring*, int));
ImplDynFunc(DoSimpleMenu, "darkdlgs.dll", "_DoSimpleMenu@12", simpmenu_woe);

#define SimpleMenu (DynFunc(DoSimpleMenu).GetProcAddress())

int PickFromStringList(const char *title, const menustring* list, int cnt)
{
   return SimpleMenu(title,list,cnt);
}
