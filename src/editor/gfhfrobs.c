/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gfhfrobs.c,v 1.15 2000/02/19 13:10:50 toml Exp $

#include <string.h>

#include <lg.h>
#include <mprintf.h>

#include <cmdbutts.h>
#include <command.h>
#include <pnptools.h>
#include <swappnp.h>
#include <brushgfh.h>
#include <gadblist.h>

#include <brlist.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <brinfo.h>
#include <vbrush.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* top region looks like
 * 0  GridSz   < val >
 * 2  [Show]  3 [Use]
 *
 * bot region
 * 4  Time    < val >   6 [Create] 7 [Select]
 * 8  brid? 9 [toend]   A [swap3]  B [Scroll]
 */

#define NUM_BUTTONS 0xC
static Rect GFHFrobs_rects[NUM_BUTTONS];
#define OUR_RECTS (GFHFrobs_rects)
#define getRect(n) (&OUR_RECTS[n])

#define TOGGLE_GRID_SHOW     1
#define TOGGLE_GRID_USE      2

#define BUTTON_TIME_TOEND    3

#define STRING_BRUSHID       8

#define FROBS_TIME           9
#define FROBS_GRID           10

#define NUM_OWNED           (FROBS_GRID+1)

extern Grid cur_global_grid;
extern void grid_pow2_scale(int pow2);
extern bool vm_show_grid, vm_show_grid_3d;

static int pnpOwners[NUM_OWNED];

// local data
static editBrush *GFHFrobs_br=NULL;
static char brushIDstr[8];
static _PnP_GadgData GFHFrobs;

static int o_time, g_size, o_brid, o_grp;
static bool useGrid, showGrid;
static LGadButtonList swaplist;

static int _get_grid_inv_val(float scale)
{
   int i=0;
   float base=scale_pow2int_to_float(i);
   while ((base<scale)&&(i<32))
   {
      base*=2;
      i++;
   }
   return i;
}

// the update call
// have to get grid/show/use from the world
// and brush id and time
static void GFHFrobs_setvars(editBrush *br, bool update)
{
   BOOL change=FALSE;
   int l_sc;
   
   if (br==NULL)     br=GFHFrobs_br;
   else if (!update) GFHFrobs_br=br;

   PnP_SetCurrent(&GFHFrobs);

   _varforPnP_synch(cur_global_grid.grid_enabled,useGrid,update,pnpOwners[TOGGLE_GRID_USE],change);

   if (showGrid!=(vm_show_grid||vm_show_grid_3d))
   {
      if (update)
         vm_show_grid=vm_show_grid_3d=showGrid;
      else
         showGrid=(vm_show_grid||vm_show_grid_3d);
      PnP_Redraw(pnpOwners[TOGGLE_GRID_SHOW],NULL);
      change=TRUE;
   }
   
   l_sc=_get_grid_inv_val(cur_global_grid.line_spacing);
   if (l_sc!=g_size)
   {
      if (update)
         grid_pow2_scale(g_size);
      else
         g_size=l_sc;
      PnP_Redraw(pnpOwners[FROBS_GRID],NULL);
      change=TRUE;
   }
   
   if (o_time!=blistCheck(br))
   {
      if (update)
         Warning(("Update o_time in setvars? logic bug - %d %d\n",o_time,blistCheck(br)));
      else
         o_time=blistCheck(br);
      PnP_Redraw(pnpOwners[FROBS_TIME],NULL);
      change=TRUE;
   }

   if (brSelect_Flag(br)&brSelect_VIRTUAL)
   {  // -1 check makes sure we get it if we came from a non multi-brush
      if ((brSelect_Group(br)!=o_grp)||(o_brid!=-1))
      {
         o_grp=brSelect_Group(br);
         sprintf(brushIDstr,"G>  %d",o_grp);
         PnP_Redraw(pnpOwners[STRING_BRUSHID],NULL);
      }
      o_brid=-1;
   }
   else if (br->br_id!=o_brid)    // this is a read only thing, so no need to worry
   {
      itoa(br->br_id,brushIDstr,10);
      o_brid=br->br_id;
      PnP_Redraw(pnpOwners[STRING_BRUSHID],NULL);
   }

   if (!update && LGadRadioButtonSelection(&swaplist) != GFHCurrentSwappable())
   {
      LGadRadioButtonSelect(&swaplist,GFHCurrentSwappable());
   }  
   
   if (change&&update)
      gedit_full_redraw();  // well, most of this might be things like adding a grid or something
}

#define END_OF_TIME   (-2)
#define START_OF_TIME (-1)
// n of -2 means go to end of time
static bool do_time_frob(int n)
{
   bool rv;
   extern bool set_brush_to_time_n(editBrush *br, int n);
   int max_time=blistCount();  // wrong, should use the real time
   if (n==END_OF_TIME)
      vBrush_go_EOT();
   else
   {
      if (n>=max_time)
         n=max_time-1;
      rv=set_brush_to_time_n(GFHFrobs_br,n);
   }
   o_time=blistCheck(GFHFrobs_br);
   PnP_Redraw(pnpOwners[FROBS_TIME],NULL);
   return rv;
}

static void GFHFrobs_OneShots(int lid)
{
   if (lid==BUTTON_TIME_TOEND)
      do_time_frob(END_OF_TIME);
   else
      Warning(("GFHFrobs Oneshot %d- what is going on\n",lid));
}

static void GFHFrobs_Tog(Rect *where, bool val, int data)
{
   GFHFrobs_setvars(NULL,TRUE);
}

static void GFHFrobs_Update(PnP_SliderOp op, Rect *where, int val, int data)
{
   if (data==FROBS_TIME)
      do_time_frob(o_time);
   else
      GFHFrobs_setvars(NULL,TRUE);
}

#define MIN_GRID 8
#define MAX_GRID 24

#define BOT_BORDER 3

//////////////////////////////
// actual swap button area

#pragma off(unreferenced)
static bool swap_butt_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action & (BUTTONGADG_LCLICK|BUTTONGADG_RCLICK))
      PnP_ExecCommandInt("start_swap",button);
   return TRUE;
}
#pragma on(unreferenced)

static DrawElement dvec[] = 
{ 
   { DRAWTYPE_TEXT, "Create", NULL, 0,0, BORDER(DRAWFLAG_BORDER_OUTLINE),},
   { DRAWTYPE_TEXT, "Layout", NULL, 0,0, BORDER(DRAWFLAG_BORDER_OUTLINE),},
   { DRAWTYPE_TEXT, "Scroll", NULL, 0,0, BORDER(DRAWFLAG_BORDER_OUTLINE),},
   { DRAWTYPE_TEXT, "Filter", NULL, 0,0, BORDER(DRAWFLAG_BORDER_OUTLINE),},
};

#define NUM_SWAPBUTTS (sizeof(dvec)/sizeof(dvec[0]))

static void CreateSwapButts(LGadRoot* root, Rect* rvec)
{
   LGadButtonListDesc desc;

   memset(&desc,0,sizeof(desc));
   desc.num_buttons = NUM_SWAPBUTTS;
   desc.button_rects = rvec;
   desc.button_elems = dvec;
   desc.cb = swap_butt_cb;
   desc.flags = BUTTONLIST_RADIO_FLAG;
   LGadCreateButtonListDesc(&swaplist,root,&desc);
}

static void DestroySwapButts(void)
{
   LGadDestroyButtonList(&swaplist);
}

////////////////////////
// actually do the deed
void Create_GFHFrobs(LGadRoot *root, Rect *top_r, Rect *bot_r, editBrush *br)
{
   LayoutRectangles(top_r,OUR_RECTS+0,4,MakePoint(2,2),MakePoint(2,2));
   getRect(0)->lr.x=getRect(1)->lr.x;  // horizontal merge the top of top region
   LayoutRectangles(bot_r,OUR_RECTS+4,8,MakePoint(4,2),MakePoint(2,2));
   getRect(4)->lr.x=getRect(5)->lr.x-BOT_BORDER;  // and the top of time area
   getRect(9)->lr.x-=BOT_BORDER;  // add a border tween time and swaps
   getRect(6)->ul.x+=BOT_BORDER; getRect(0xA)->ul.x+=BOT_BORDER;
   
   PnP_GadgStart(&GFHFrobs,root);
   CreateSwapButts(root,OUR_RECTS+6);
   GFHFrobs_setvars(br,FALSE);

   pnpOwners[FROBS_GRID]=PnP_SliderInt(getRect(0),"Grid Sz",MIN_GRID,MAX_GRID,1,&g_size,
                    GFHFrobs_Update,FROBS_GRID,PNP_SLIDER_CLICKS);
   pnpOwners[TOGGLE_GRID_SHOW]=
      PnP_ButtonToggle(getRect(2),"Show",NULL,&showGrid,GFHFrobs_Tog,TOGGLE_GRID_SHOW);
   pnpOwners[TOGGLE_GRID_USE]=   
      PnP_ButtonToggle(getRect(3),"Use",NULL,&useGrid,GFHFrobs_Tog,TOGGLE_GRID_USE);

   pnpOwners[FROBS_TIME]=PnP_SliderInt(getRect(4),"Time",0,65536,1,&o_time,
                    GFHFrobs_Update,FROBS_TIME,PNP_SLIDER_VSLIDE);
   pnpOwners[STRING_BRUSHID]=PnP_TextBox(getRect(8),brushIDstr);
   PnP_ButtonOneShot(getRect(9),"To End",GFHFrobs_OneShots,BUTTON_TIME_TOEND);

   // I wonder if just copying rects around works.
   *getRect(8)=*getRect(0xA);  // That's 0xA, NOT 10!
   *getRect(9)=*getRect(0xB);
}

void Destroy_GFHFrobs(void)
{
   DestroySwapButts();
   PnP_GadgFree(&GFHFrobs);
}

void Update_GFHFrobs(GFHUpdateOp op, editBrush *br)
{
   GFHFrobs_setvars(br,FALSE);   
   if (op==GFH_FORCE)
      PnP_FullRedraw(&GFHFrobs);
}
