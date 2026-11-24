/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swapregs.c,v 1.11 2000/02/19 13:11:35 toml Exp $
// swap button list for the create tool

#include <string.h>

#include <lg.h>
#include <mprintf.h>

#include <gadget.h>
#include <cmdbutts.h>
#include <imgui_pnp.h>
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

static _PnP_GadgData swap_panel;
static int owners[6];

static void build_swap(Rect *rect, const char *title, int count, char *const *labels, void (*activate)(int))
{
   Rect heading = *rect;
   Rect fields = *rect;
   Rect buttons[6];
   int i;
   heading.lr.y = heading.ul.y + 13;
   fields.ul.y = heading.lr.y;
   PnP_GadgStart(&swap_panel);
   PnP_HeadingButton(&heading, title, activate, -1);
   LayoutRectangles(&fields, buttons, count, MakePoint(1, count), MakePoint(2, 2));
   for (i = 0; i < count; ++i)
      owners[i] = PnP_ChoiceButton(&buttons[i], labels[i], activate, i);
}

static void destroy_swap(void)
{
   PnP_GadgFree(&swap_panel);
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

void Create_crSwap(Rect *bounds, editBrush *br)
{
   build_swap(bounds, "Create", strlist_len(createNames), createNames, _crCallBack);
}

void Destroy_crSwap(void)
{
   destroy_swap();
}

void Update_crSwap(GFHUpdateOp op, editBrush *br)
{
   int i;
   PnP_SetCurrent(&swap_panel);
   for (i = 0; i < strlist_len(createNames); ++i)
      PnP_SetChoice(owners[i], gedit_default_brush_type() == i);
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

void Create_scrollSwap(Rect *bounds, editBrush *br)
{
   build_swap(bounds, "Scroll", strlist_len(scrollNames), scrollNames, _scrollCallBack);
}

void Destroy_scrollSwap(void)
{
   destroy_swap();
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

void Create_layoutSwap(Rect *bounds, editBrush *br)
{
   build_swap(bounds, "Layout", strlist_len(layoutNames), layoutNames, _layoutCallBack);
}

void Destroy_layoutSwap(void)
{
   destroy_swap();
}

void Update_layoutSwap(GFHUpdateOp op, editBrush *br)
{

}

//////////////////////////////
// Filters subregion
static char *filterNames[]={"Terr","Light","Area","Object","Flow","Room"};

static void select_filters(void)
{
   int i;
   PnP_SetCurrent(&swap_panel);
   for (i = 0; i < strlist_len(filterNames); ++i)
      PnP_SetChoice(owners[i], editbr_filter & brFlag_GetFilterType(i));
}

static void _filterCallBack(int idx)
{
   int bit = idx >= 0 ? brFlag_GetFilterType(idx) : 0;
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
   select_filters();
   gedit_full_redraw();      // hey, who are we kidding
}

void Create_filterSwap(Rect *bounds, editBrush *br)
{
   build_swap(bounds, "Filter", strlist_len(filterNames), filterNames, _filterCallBack);
   select_filters();
}

void Destroy_filterSwap(void)
{
   destroy_swap();
}

void Update_filterSwap(GFHUpdateOp op, editBrush *br)
{
   select_filters();
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

void Create_testSwap(Rect *bounds, editBrush *br)
{
   build_swap(bounds, "Test", strlist_len(testNames), testNames, _testCallBack);
}

void Destroy_testSwap(void)
{
   destroy_swap();
}

void Update_testSwap(GFHUpdateOp op, editBrush *br)
{
}
