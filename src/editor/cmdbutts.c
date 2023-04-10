/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cmdbutts.c,v 1.5 2000/02/19 12:27:51 toml Exp $

#include <string.h>
#include <stdlib.h>

#include <lgsprntf.h>
#include <config.h>


#include <cmdbutts.h>
#include <command.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//
// Layout rects
//

void LayoutRectangles(Rect* bounds, Rect* rvec, int n, Point dims, Point sp)
{
   short bw = RectWidth(bounds);
   short bh = RectHeight(bounds);
   Point ul = bounds->ul;
   short w;
   short h;
   int i,j;
   int m;

   if (dims.x <= 0 || dims.y <= 0) 
   {
      Warning(("Tried to lay out rects for a zero-sided matrix\n"));
      return;
   }

   w = (bw - sp.x*(dims.x-1))/dims.x;
   h = (bh - sp.y*(dims.y-1))/dims.y;

   m = 0;
   for (j = 0; j < dims.y; j++)
      for (i = 0; i < dims.x; i++)
      {
         Rect* r = &rvec[m];
         r->ul.x = ul.x;
         if (dims.x > 1)
            r->ul.x += (bw - w)*i/(dims.x-1);
         r->lr.x = r->ul.x + w;
         
         r->ul.y = ul.y;
         if (dims.y > 1)
            r->ul.y += (bh - h)*j/(dims.y-1);
         r->lr.y = r->ul.y + h;

         m++;
         if (m >= n)
            goto out;

      }
out: ;
}

////////////////////////////////////////

static bool cmd_blist_call(ushort action, int button, void* data, LGadBox* vb)
{
   if (action & BUTTONGADG_LCLICK)
   {
      char** cmds = (char**)data;
      CommandExecute(cmds[button]);
      return TRUE;
   }
   return FALSE;
}

////////////////////////////////////////

LGadButtonList* CreateCmdButtonList(LGadButtonList* list, LGadRoot* root, CmdButtonListDesc* desc)
{
   LGadButtonListDesc bdesc;
   LGadButtonList* result;

   memset(&bdesc,0,sizeof(bdesc));
   bdesc.num_buttons = desc->num_buttons;
   bdesc.button_rects = desc->rects;
   bdesc.button_elems = desc->appearance;
   bdesc.cb = cmd_blist_call;
   
   result = LGadCreateButtonListDesc(list,root,&bdesc);
   LGadBoxSetUserData(VB(list),desc->commands);
   return result;
}

#define BUTTNAME_BUFSIZ 32
#define COMMAND_BUFSIZ 64

void CreateConfigButtonDesc(CmdButtonListDesc* desc, int n, Rect* bounds, char* var_prefix, Point dims, Point spacing)
{
   int i;
   if (n < 1)
   {
      Warning(("Tried to create a config-driven button list with %d buttons\n",n));
      return;
   }

   desc->num_buttons = n;
   desc->appearance = Malloc(sizeof(DrawElement)*n);
   Assrt(desc->appearance != NULL);
   desc->rects = Malloc(sizeof(Rect)*n);
   Assrt(desc->rects != NULL);
   desc->commands = Malloc(sizeof(char*)*n);
   Assrt(desc->commands != NULL);
   
   for (i = 0; i < n; i++)
   {
      char vbuf[65];
      DrawElement* de = &desc->appearance[i];
      char** pcmd = &desc->commands[i];

      memset(de,0,sizeof(*de));
      de->draw_type = DRAWTYPE_TEXT;
      lg_sprintf(vbuf,"%s_name_%d",var_prefix,i);
      de->draw_data = Malloc(BUTTNAME_BUFSIZ);  
      Assrt(de->draw_data != NULL);
      *(char*)de->draw_data = '\0';
      config_get_raw(vbuf,(char*)de->draw_data,BUTTNAME_BUFSIZ);
      de->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
      
      lg_sprintf(vbuf,"%s_cmd_%d",var_prefix,i);
      *pcmd = Malloc(COMMAND_BUFSIZ);
      Assrt(*pcmd != NULL);
      **pcmd = '\0';
      config_get_raw(vbuf,*pcmd,COMMAND_BUFSIZ);
   }
   LayoutRectangles(bounds,desc->rects,n,dims,spacing);
}

void DestroyConfigButtonDesc(CmdButtonListDesc* desc)
{
   int n = desc->num_buttons;
   int i;

   for (i = 0; i < n; i++)
   {
      Free(desc->appearance[i].draw_data);
      Free(desc->commands[i]);
   }
   Free(desc->appearance);
   Free(desc->rects);
   Free(desc->commands);
   desc->num_buttons = 0;
}

