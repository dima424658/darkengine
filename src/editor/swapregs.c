/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swapregs.c,v 1.11 2000/02/19 13:11:35 toml Exp $
// swap button list for the create tool

#include <string.h>

#include <lg.h>
#include <mprintf.h>

#include <swaptool.h>
#include <brushgfh.h>
#include <pnptools.h>

#include <status.h>

#include <command.h>
#include <editbr.h>
#include <viewmgr.h>
#include <vmwincfg.h>
#include <brrend.h>
#include <gedit.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////
// Generically useful swap stuff

#define strlist_len(x) (sizeof(x)/sizeof(x[0]))

static void *curSwap=NULL;

void _generic_swap_destroy(void)
{
   if (curSwap==NULL)
      Warning(("Hey! dont have a swap"));
   SwapDestroyList(curSwap);
   curSwap=NULL;
}

///////////////////////////////
// Create Swap Region

static char *createNames[]={"Brush", "Light", "Area", "Object", "Flow", "Room"};

static void _crCallBack(int idx)
{
   if (idx==-1)
      CommandExecute("insert_brush");
   else
   {
      char buf[80];
      PnP_ExecCommandInt("set_brush_type",idx);
      sprintf(buf,"Create type now %s",createNames[idx]);
      Status(buf);
   }
}

void Create_crSwap(LGadRoot *root, Rect *bounds, editBrush *br)
{
   if (curSwap!=NULL)
      Warning(("Hey! already have a swap"));
   curSwap=SwapBuildList(root,bounds,"Create",strlist_len(createNames),createNames,_crCallBack,BUTTONLIST_RADIO_FLAG);
}

void Destroy_crSwap(void)
{
   _generic_swap_destroy();
}

void Update_crSwap(GFHUpdateOp op, editBrush *br)
{
   int type = gedit_default_brush_type();
   if (curSwap == NULL) return;
   if (SwapListSelection(curSwap) != type)
   {
      SwapListSelect(curSwap,type);
      SwapRedrawList(curSwap);
   }
}

//////////////////////////////
// Scroll subregion
static char *scrollNames[]={"Next", "Prev", "Up", "Left", "Right", "Down"};

static void _scrollCallBack(int idx)
{
   float scale_f=1.0;
   //   mprintf("CrCallBack %d\n",idx);
   switch (idx)
   {
      case 0: vm_cycle_camera(TRUE);  break;
      case 1: vm_cycle_camera(FALSE); break;
      case 2: vm_cur_scroll( 0*scale_f,-1*scale_f); break;
      case 3: vm_cur_scroll(-1*scale_f, 0*scale_f); break;
      case 4: vm_cur_scroll( 1*scale_f, 0*scale_f); break;
      case 5: vm_cur_scroll( 0*scale_f, 1*scale_f); break;
   }
}

void Create_scrollSwap(LGadRoot *root, Rect *bounds, editBrush *br)
{
   if (curSwap!=NULL)
      Warning(("Hey! already have a swap"));
   curSwap=SwapBuildList(root,bounds,"Scroll",strlist_len(scrollNames),scrollNames,_scrollCallBack,0);
}

void Destroy_scrollSwap(void)
{
   _generic_swap_destroy();
}

void Update_scrollSwap(GFHUpdateOp op, editBrush *br)
{

}

//////////////////////////////
// Layout subregion
static char *layoutNames[]={"2by2", "3x1 Vert", "3x1 Horz", "Center", "Solo"};

static void _layoutCallBack(int idx)
{
   idx++;   // -1 (off top) maps to 0 (cycle), which is fine
   if ((idx>=vmWINCFG_CYCLE)||(idx<=vmWINCFG_SOLO))
      vmWinCfgCycleLayout(idx);
}

void Create_layoutSwap(LGadRoot *root, Rect *bounds, editBrush *br)
{
   if (curSwap!=NULL)
      Warning(("Hey! already have a swap"));
   curSwap=SwapBuildList(root,bounds,"Layout",strlist_len(layoutNames),layoutNames,_layoutCallBack,0);
}

void Destroy_layoutSwap(void)
{
   _generic_swap_destroy();
}

void Update_layoutSwap(GFHUpdateOp op, editBrush *br)
{
// i tried this with radio buttons but was mocked, so, ah, who knows, i guess
#ifdef THIS_TOTALLY_FAILED
   int cur_ly=vmWinCfgGetLayout();
   if (curSwap == NULL) return;
   if (SwapListSelection(curSwap) != cur_ly-1)
   {
      SwapListSelect(curSwap,cur_ly-1);
      SwapRedrawList(curSwap);
   }
#endif   
}

//////////////////////////////
// Filters subregion
static char *filterNames[]={"Terr","Light","Area","Object","Flow","Room"};

// this is a dorky hideous hack
static void _filterSetSelections(void)
{
   int i;

   for (i=0; i<sizeof(filterNames)/sizeof(char *); i++)
   {
      if (editbr_filter&brFlag_GetFilterType(i))
         SwapListSelect(curSwap,i);
      else
         SwapListDeSelect(curSwap,i);
   }
   SwapRedrawList(curSwap);
}

static void _filterCallBack(int idx)
{
   int bit=1<<(idx+brFlag_FILTERSHIFT);
   if (idx==-1)              // secret behavior for clicking on menu title
      if (editbr_filter)
         editbr_filter=0;
      else               
         editbr_filter=brFlag_FILTERMASK;
   else
      if (editbr_filter&bit) // already on
         editbr_filter&=~bit;
      else
         editbr_filter|=bit;
   _filterSetSelections();
   gedit_full_redraw();      // hey, who are we kidding
}

void Create_filterSwap(LGadRoot *root, Rect *bounds, editBrush *br)
{
   if (curSwap!=NULL)
      Warning(("Hey! already have a swap"));
   curSwap=SwapBuildList(root,bounds,"Filter",strlist_len(filterNames),filterNames,_filterCallBack,0);
   _filterSetSelections();
}

void Destroy_filterSwap(void)
{
   _generic_swap_destroy();
}

void Update_filterSwap(GFHUpdateOp op, editBrush *br)
{
   //   if (SwapListSelection(curSwap) != cur_ly-1)
   //      SwapListSelect(curSwap,cur_ly-1);
   //   SwapRedrawList(curSwap);
}

//////////////////////////////
// Test subregion
static char *testNames[]={"Scroll"};

static void _testCallBack(int idx)
{
   switch (idx)
   {
   case 0: vmWinCfgSetMode(vmWINCFG_MODE_SCROLL); break;
   }
}

void Create_testSwap(LGadRoot *root, Rect *bounds, editBrush *br)
{
   if (curSwap!=NULL)
      Warning(("Hey! already have a swap"));
   curSwap=SwapBuildList(root,bounds,"Test",strlist_len(testNames),testNames,_testCallBack,0);
}

void Destroy_testSwap(void)
{
   _generic_swap_destroy();
}

void Update_testSwap(GFHUpdateOp op, editBrush *br)
{
}
