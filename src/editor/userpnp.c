/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/userpnp.c,v 1.2 2000/02/19 13:13:41 toml Exp $
#include <config.h>

#include <userpnp.h>
#include <imgui_pnp.h>
#include <gadblist.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// USER-DEFINED PNP
// fills the area with a config-driven menu
////////////////////////////////////////////////////////////

#define USER_PNP_PREFIX "user_menu"

static _PnP_GadgData user_panel;
static int menu_size = 0;

#define MAX_MENU_LINES 7
#define USER_MENU_SPACING MakePoint(1,1)

void CreateUserPNP(Rect* bounds, editBrush* brush)
{
   menu_size = 0;
   config_get_int(USER_PNP_PREFIX"_size",&menu_size);
   if (menu_size > 0)
   {
      Point dims = MakePoint(1,MAX_MENU_LINES); 
      dims.x = (menu_size+MAX_MENU_LINES-1)/MAX_MENU_LINES;
      PnP_GadgStart(&user_panel);
      PnP_CommandButtons(bounds, USER_PNP_PREFIX, menu_size, dims);
   }
}


void UpdateUserPNP(GFHUpdateOp op, editBrush* brush)
{
   if (menu_size > 0 && op == GFH_FORCE)
      PnP_FullRedraw(&user_panel);
}

void DestroyUserPNP(void)
{
   if (menu_size > 0)
   {
      PnP_GadgFree(&user_panel);
   }
   menu_size = 0;
}
