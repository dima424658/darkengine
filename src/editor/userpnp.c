/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/userpnp.c,v 1.2 2000/02/19 13:13:41 toml Exp $
#include <config.h>

#include <userpnp.h>
#include <cmdbutts.h>
#include <gadblist.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// USER-DEFINED PNP
// fills the area with a config-driven menu
////////////////////////////////////////////////////////////

#define USER_PNP_PREFIX "user_menu"

static CmdButtonListDesc user_desc;
static LGadButtonList user_list;
static int menu_size = 0;

#define MAX_MENU_LINES 7
#define USER_MENU_SPACING MakePoint(1,1)

#pragma off(unreferenced)
void CreateUserPNP(LGadRoot* root, Rect* bounds, editBrush* brush)
{
   menu_size = 0;
   config_get_int(USER_PNP_PREFIX"_size",&menu_size);
   if (menu_size > 0)
   {
      Point dims = MakePoint(1,MAX_MENU_LINES); 
      dims.x = (menu_size+MAX_MENU_LINES-1)/MAX_MENU_LINES;
      CreateConfigButtonDesc(&user_desc,menu_size,bounds,USER_PNP_PREFIX,dims,USER_MENU_SPACING);
      CreateCmdButtonList(&user_list,root,&user_desc);
   }
}


void UpdateUserPNP(GFHUpdateOp op, editBrush* brush)
{
   if (menu_size > 0 && op == GFH_FORCE)
      LGadDrawBox(VB(&user_list),NULL);
}

void DestroyUserPNP(void)
{
   if (menu_size > 0)
   {
      LGadDestroyButtonList(&user_list);
      DestroyConfigButtonDesc(&user_desc);
   }
   menu_size = 0;
}



#pragma on(unreferenced)
