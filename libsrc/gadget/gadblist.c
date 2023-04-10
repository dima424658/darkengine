/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/gadblist.c,v 1.6 1997/11/06 17:05:13 mahk Exp $
#include <string.h>

#include <uigadget.h>
#include <gadblist.h>
#include <gadget.h>


#pragma off(unreferenced)
bool gadblist_handler(uiEvent* ev, Region* reg, void* data)
{
   LGadButtonList* blist = (LGadButtonList*)reg;
   ButtonListEvent* lev = (ButtonListEvent*)ev;
   if (lev->signaller != BUTTONLIST_SIGNALLER)
      return FALSE;
   blist->call(lev->action,lev->button,reg->user_data,VB(blist));

   return FALSE;
}
#pragma on(unreferenced)


LGadButtonList* LGadCreateButtonListDesc(LGadButtonList* list, LGadRoot* root,
   LGadButtonListDesc* desc)
{
   int id;
   if (list == NULL)
   {
      list = Malloc(sizeof(LGadButtonList));
      AssertMsg(list != NULL, "Could not allocate LGadButtonlist");
      memset(list,0,sizeof(*list));
      list->free_self = TRUE;
   }
   else
      list->free_self = FALSE;
   ButtonListInit(&VB(root)->r, &list->blist, 0, desc->num_buttons,
      desc->button_rects,desc->button_elems,desc->flags);
   list->call = desc->cb;

   VB(list)->paltype = desc->paltype;
   uiInstallRegionHandler(&VB(list)->r,UI_EVENT_USER_DEFINED,gadblist_handler,NULL,&id);
                          
   return list;
}


void LGadDestroyButtonList(LGadButtonList* list)
{
   ButtonListDestroy(&list->blist);
   if (list->free_self)
      Free(list);
}

