/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/gadtext.c,v 1.4 1997/02/20 17:31:04 mahk Exp $

#include <string.h>

#include <gadget.h>
#include <gadtext.h>
#include <uigadget.h>



static bool textbox_event_handler(uiEvent* ev, Region* reg, void* data)
{
   LGadTextBox* box = (LGadTextBox*)reg;
   TextGadgEvent* tev = (TextGadgEvent*)ev;
   
   if (tev->signaller != TEXTED_SIGNALLER)
      return FALSE;

   if (box->cb != NULL)
      return box->cb(box,tev->action,tev->data,box->data);
   return FALSE;
}

LGadTextBox* LGadCreateTextBoxDesc(LGadTextBox* box, LGadRoot* root, LGadTextBoxDesc* desc)
{
   int cookie;
   if (root == NULL)
   {
      root = LGadCurrentRoot();
      AssertMsg(root != NULL, "New LGadTextBox has no root\n");
   }
   if (box == NULL)
   {
      box = Malloc(sizeof(LGadTextBox));
      AssertMsg(box != NULL,"Could not allocate an LGadTextBox");
      if (box == NULL) return NULL;
      memset(box,0,sizeof(*box));
      box->free_self = TRUE;
   }
   else
      box->free_self = FALSE;
   TextGadgInit(&VB(root)->r,&box->gadg,&desc->bounds,0,desc->editbuf,desc->editbuflen,desc->flags);
   uiInstallRegionHandler(&VB(box)->r,UI_EVENT_USER_DEFINED,textbox_event_handler,NULL,&cookie);
   VB(box)->style = desc->style;
   box->cb = desc->cb;
   box->data = desc->data;
   return box;
}

void LGadDestroyTextBox(LGadTextBox* box)
{
   TextGadgDestroy(&box->gadg);
   if (box->free_self)
      Free(box);
}
