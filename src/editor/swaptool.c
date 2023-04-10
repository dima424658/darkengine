/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swaptool.c,v 1.11 2000/02/19 13:11:36 toml Exp $
// utilities for PnP swap area

#include <string.h>

#include <lg.h>
#include <mprintf.h>
#include <guistyle.h>

#include <swaptool.h>
#include <cmdbutts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////
typedef struct {
   LGadButtonListDesc desc;
   LGadButtonList     list;
   LGadButton title;
   LGadRoot* root;
} SwapButtonList;

static void (*listShoot)(int idx);

static bool swap_list_callback(ushort action, int button, void *data, LGadBox *vb)
{
   if (!(action&BUTTONGADG_LCLICK))
      return FALSE;
   // int blah=*(int *)data;
   // blah seems to never have any relevance to anything?
   //   mprintf("Yo here for %d, data %x\n",button,blah);
   if (listShoot!=NULL)
      (*listShoot)(button);
   return TRUE;
}

static void SwapBuildElement(DrawElement *elem, int i, char *text)
{
   ElementClear(elem);
   elem->draw_type=DRAWTYPE_TEXT;
   elem->draw_data=text;
   elem->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
}

static void SwapBuildTitle(Rect* area, LGadRoot* root,  LGadButton* butt, char* title)
{
   DrawElement draw;
   short h;

   // figure out the string height
   guiStyleSetupFont(NULL,StyleFontNormal);
   h = gr_string_height(title);
   guiStyleCleanupFont(NULL,StyleFontNormal);

   ElementClear(&draw);
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = title;

   LGadCreateButtonArgs(butt,root,area->ul.x,area->ul.y,
      (short)RectWidth(area),(short)(h+2),&draw,NULL,0);
   LGadButtonSetFlags(butt,LGADBUTT_FLAG_STATELESS);
}


void *SwapBuildList(LGadRoot *root, Rect *area, char* title, int n, char *strings[], void (*shoot)(int idx), ulong flags)
{
   SwapButtonList *SwapList=(SwapButtonList *)Malloc(sizeof(SwapButtonList));
   LGadButtonListDesc *SwapPtr;
   int i;
   Rect bounds = *area;
   
   if (SwapList==NULL) return NULL;      // didnt work, sad day for us

   memset(SwapList,0,sizeof(*SwapList));

   SwapList->root = root;

   // build the title 
   SwapBuildTitle(area,root,&SwapList->title,title);
   // exclude the title area from our buttonlist. 
   bounds.ul.y = LGadBoxRect(&SwapList->title)->lr.y;

   
   SwapPtr=&SwapList->desc;
   SwapPtr->num_buttons=n;
   SwapPtr->button_rects=(Rect *)Malloc(n*sizeof(Rect));

   // hey doug, I took out swaprect fill, because I already have LayoutRectangles
   //   SwapRectFill(SwapPtr->button_rects,area,n);
   LayoutRectangles(&bounds,SwapPtr->button_rects,n,MakePoint(1,n),MakePoint(2,2));
   
   // build the buttonlist proper
   SwapPtr->button_elems=(DrawElement *)Malloc(n*sizeof(DrawElement));
   for (i=0; i<n; i++)
      SwapBuildElement(&SwapPtr->button_elems[i],i,strings[i]);
   SwapPtr->cb=swap_list_callback;
   SwapPtr->paltype=0;
   SwapPtr->flags = flags;
   LGadCreateButtonListDesc(&SwapList->list,root,SwapPtr);
   listShoot=shoot;
   return SwapPtr;
}

// call this will the void * returned to you by BuildList
void SwapDestroyList(void *data)
{
   SwapButtonList *SwapList=(SwapButtonList *)data;
   LGadDestroyButtonList(&SwapList->list);
   LGadDestroyBox((LGadBox*)&SwapList->title,FALSE);
   Free(SwapList->desc.button_rects);
   Free(SwapList->desc.button_elems);
   Free(SwapList);
}


void SwapRedrawList(void* data)
{
   SwapButtonList* list = (SwapButtonList*)data;
   LGadDrawBox(VB(list->root),NULL);
}

// well, these are great, if you only have radio buttons
// so, perhaps they should get smarter

int SwapListSelection(void* data)
{
   SwapButtonList* swaplist = (SwapButtonList*)data;
   if (swaplist->desc.flags&BLIST_RADIO_FLAG)
      return LGadRadioButtonSelection(&swaplist->list);
   else
      Warning(("Not really relevant for non-button list\n"));
   return -1; // ?? some way to say "no button, maybe ?? - not sure
}

void SwapListSelect(void* data,int sel)
{
   SwapButtonList* swaplist = (SwapButtonList*)data;
   if (swaplist->desc.flags&BLIST_RADIO_FLAG)
      LGadRadioButtonSelect(&swaplist->list,sel);
   else
      swaplist->desc.button_elems[sel].fcolor=guiStyleGetColor(NULL,StyleColorBright);
}

void SwapListDeSelect(void* data,int sel)
{
   SwapButtonList* swaplist = (SwapButtonList*)data;
   if (swaplist->desc.flags&BLIST_RADIO_FLAG)
      Warning(("Cant deselect a Radio button\n"));
   else
      swaplist->desc.button_elems[sel].fcolor=guiStyleGetColor(NULL,StyleColorDim);
}
