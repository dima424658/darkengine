/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/areapnp.c,v 1.25 2000/02/19 12:27:39 toml Exp $
// HotRegion/Time Filter ("Area") PnP gadget

#include <gadblist.h>
#include <lgsprntf.h>
#include <mprintf.h>

#include <command.h>
#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <brlist.h>
#include <brinfo.h>
#include <brrend.h>
#include <brquery.h>
#include <vbrush.h>
#include <status.h>

#include <brushGFH.h>
#include <PnPtools.h>

#include <areapnp.h>
#include <namenum.h>
#include <prompts.h>
#include <ged_rmap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// global Named Hot Region system pointer
sNameNum *pNN_HotRegions=NULL;

/* artists conception
 *
 * 0  [show hot/show all]   (tog bool renderHotsOnly (comm "hots_filter"))
 * 1 [activate/deactivate]  (tog this hot region ("hots_state" 0 or 1))
 * 2       [me only]        (toggle this brush being ME, ("hots_state" 2)
 * 3 make/add to multibrush
 * 4      Current Name
 *    5 Name   7 Search
 *  6 time lo  8 time hi    (slider times, 0 is none, "time_filter_lo/hi")
 */

#define NUM_BUTTONS (0x9)
static Rect areaPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (areaPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 5
#define BTN_BELOW 2
#define VERT_DIV  5
#define HORIZ_DIV (0.5)

void areaPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   for (i=0; i<BTN_ABOVE; i++)
      buildYLineRect(curRect++,whole,i,0,1,2,2);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,0,2,2);   
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,1,2,2);
}

static bool actv_hot=FALSE, me_only=FALSE;
static editBrush *areaPnP_br;

static char *unnamed="UnNamed";
static char *areaPnP_brush_name=NULL;

#define BRUSH_NAME 0
#define NUM_OWNED (BRUSH_NAME+1)
static int pnpOwners[NUM_OWNED];

static _PnP_GadgData areaPnP;

#define HOTS_OR_ALL  1
#define ACTIVATE     2
#define DO_MEONLY    3
#define DO_MULTI_ADD 4

#define NAME_CHOSE   6
#define NAME_SEARCH  7

#define TIME_LO     1
#define TIME_HI     2
static int time_max;

// update is TRUE to mean the globals just changed
void areaPnP_setvars(editBrush *br, BOOL update)
{
   areaPnP_br=br;       // silly, since we do it a lot, but oh well
   if (areaPnP_br==NULL) return;
   if (brushGetType(br)!=brType_HOTREGION)
      actv_hot=me_only=0;
   else
   {
      actv_hot=((brHot_Status(br)&brHot_ACTIVE)>0);
      me_only=(brHot_IsMEONLY(br)>0);
   }
   areaPnP_brush_name=NamenumFetchByTag(pNN_HotRegions,areaPnP_br->br_id);
   if (areaPnP_brush_name==NULL)
      areaPnP_brush_name=unnamed;
   if (update)
   {
      PnP_SetCurrent(&areaPnP);
      PnP_Redraw(pnpOwners[BRUSH_NAME],areaPnP_brush_name);
   }
   time_max=blistCount();
}

static void areaPnP_UpdateFlag(Rect *where, bool val, int data)
{
   switch (data)
   {
   case HOTS_OR_ALL:
      if (renderHotsOnly) Status("Hot regions on"); else Status("Hot regions off");
      break;
   case ACTIVATE:
      PnP_ExecCommandInt("hots_state",brHot_TOGACTIVE);
      Status(brHot_Status(areaPnP_br)&brHot_ACTIVE?"area now Active":"area now Inactive");
      break;
   case DO_MEONLY:
      PnP_ExecCommandInt("hots_state",brHot_TOGMEONLY);
      if (brHot_IsMEONLY(areaPnP_br))
         renderHotsOnly=TRUE;  // is this right
      Status(brHot_IsMEONLY(areaPnP_br)?"area MeOnly-ed, back off":"area now normal");
      break;
   }
   areaPnP_setvars(areaPnP_br,TRUE);
   PnP_FullRedraw(&areaPnP);
   gedit_full_redraw();  // you have changed what hotregions are active, lets just go redraw this sucker
}

static void areaPnP_Names(int which)
{
   switch (which)
   {
   case NAME_CHOSE:
      {
         char buf[PROMPT_STR_LEN], def[64], *val;
         BOOL inited=FALSE;
         int hnd;

         val=NamenumFetchByTag(pNN_HotRegions,areaPnP_br->br_id);
         if (val==NULL)
         {
            strcpy(def,"brush_");
            itoa(areaPnP_br->br_id,def+strlen(def),10);
         }
         else
         {
            strcpy(def,val);
            inited=TRUE;
         }
         val=prompt_string(def,buf);
         if (inited)
            NamenumClearByTag(pNN_HotRegions,areaPnP_br->br_id);
         hnd=NamenumStore(pNN_HotRegions,val,areaPnP_br->br_id);
         areaPnP_brush_name=NamenumFetch(pNN_HotRegions,hnd);
         break;
      }
   case NAME_SEARCH:
      {
         int picked_tag=NamenumSimpleMenuChooseTag(pNN_HotRegions);
         if (picked_tag!=NAMENUM_NO_TAG)
            vBrush_SelectBrush(brFind(picked_tag));
         break;
      }
   }
   areaPnP_setvars(areaPnP_br,TRUE);
   PnP_FullRedraw(&areaPnP);
}

// simple functions to change state of brushes inside of current region
editBrush *gedit_get_meonly_brush(void);
static int cnts[2];
static void areaPnP_addtoMulti(editBrush *us)
{                                            // lets try not putting ourselves in it
   if ((brushGetType(us)!=brType_HOTREGION)) // ||(us==areaPnP_br)) // hmmm, does this work
      if (vBrush_editBrush_Op(us,vBrush_OP_ADD))
         cnts[0]++;
      else
         cnts[1]++;
}

#ifdef NEED_REM_FROM_MULTI
static void areaPnP_remfromMulti(editBrush *us)
{
   if (brushGetType(us)!=brType_HOTREGION)
      vBrush_editBrush_Op(us,vBrush_OP_ADD);
}
#endif

// really want a custom RunOnActive for current hotregion only, somehow....

static void areaPnP_OneShots(int lid)
{
   editBrush *old_meonly=gedit_get_meonly_brush(), *init_brush=areaPnP_br;
   BOOL old_rhots=renderHotsOnly;
   int our_old_state=brHot_Status(areaPnP_br);
   char buf[100];

   if (init_brush==NULL) return;
   
   // setup for oneshot trigger
   if (old_meonly)
      brHot_Status(old_meonly)&=~brHot_MEONLY;
   brHot_Status(areaPnP_br)|=brHot_MEONLY;
   
   renderHotsOnly=TRUE;
   cnts[0]=cnts[1]=0;

   // now do the work
   switch (lid)
   {
   case DO_MULTI_ADD:
      brushRunOnActive(areaPnP_addtoMulti);
      vBrush_getToCurGroup();
      break;
   }

   // now cleanup after ourselves
   brHot_Status(init_brush)=our_old_state;
   renderHotsOnly=(our_old_state==0)?FALSE:old_rhots;
   if (old_meonly)
      brHot_Status(old_meonly)|=brHot_MEONLY;

   // and tell/show the world what we did
   sprintf(buf,"Changed %d of %d",cnts[0],cnts[0]+cnts[1]);
   Status(buf);
   gedit_full_redraw();  // who knows how many brushes we just changed representations for
}

static void areaPnP_time(PnP_SliderOp op, Rect *where, int val, int data)
{
   if (op != PnP_SliderUpdateOp)
      return;
   switch (data)
   {
   case TIME_LO:     break;
   case TIME_HI:
      if (val==time_max+1)
         editbr_filter_time_hi=0;
      break;
   }
   gedit_full_redraw();  // just made changes to what brushes are visible
}

//////////////
// per frame control

void Create_areaPnP(LGadRoot* root, Rect* bounds, editBrush *br)
{
   GFHSetCoordMask(GFH_ALL_COORDS);
   areaPnP_buildRects(bounds);
   PnP_GadgStart(&areaPnP,root);
   areaPnP_setvars(br,FALSE);
   PnP_ButtonToggle(getRect(0),"Show Hot","Show All",&renderHotsOnly,areaPnP_UpdateFlag,HOTS_OR_ALL);
   PnP_ButtonToggle(getRect(1),"Activate","DeActivate",&actv_hot,areaPnP_UpdateFlag,ACTIVATE);
   PnP_ButtonToggle(getRect(2),"Me Only","Not Me Only",&me_only,areaPnP_UpdateFlag,DO_MEONLY);
   PnP_ButtonOneShot(getRect(3),"multi-brush-me",areaPnP_OneShots,DO_MULTI_ADD);
   pnpOwners[BRUSH_NAME]=PnP_TextBox(getRect(4),areaPnP_brush_name);
   PnP_ButtonOneShot(getRect(5),"Name",areaPnP_Names,NAME_CHOSE);
   PnP_ButtonOneShot(getRect(7),"Search",areaPnP_Names,NAME_SEARCH);   
   PnP_SliderInt(getRect(6),"Lo",0,time_max,1,&editbr_filter_time_lo,areaPnP_time,TIME_LO,PNP_SLIDER_VSLIDE);
   PnP_SliderInt(getRect(8),"Hi",0,time_max+1,1,&editbr_filter_time_hi,areaPnP_time,TIME_HI,PNP_SLIDER_VSLIDE);
}

void Destroy_areaPnP(void)
{
   PnP_GadgFree(&areaPnP);
   areaPnP_br=NULL;
}

void Update_areaPnP(GFHUpdateOp op, editBrush *br)
{
   areaPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)           
      PnP_FullRedraw(&areaPnP);
}

/////////////////
// maintain pNN_HotRegions

#define MAX_AREAS_NAMED 256
#define NAME_AREA_NAMES "HotRegions"

// reset the names to nothing
void AreaPnP_Names_Reset(void)
{
   if (pNN_HotRegions)
      NamenumFree(pNN_HotRegions);
   pNN_HotRegions=NamenumInit(NAME_AREA_NAMES,MAX_AREAS_NAMED);
}

// load a new set from the tagfile
BOOL AreaPnP_Names_Load(ITagFile *file)
{
   if (pNN_HotRegions)
      NamenumFree(pNN_HotRegions);
   pNN_HotRegions=NamenumLoad(NAME_AREA_NAMES,file);
   if (pNN_HotRegions)
      return TRUE;
   pNN_HotRegions=NamenumInit(NAME_AREA_NAMES,MAX_AREAS_NAMED);
   return FALSE;
}

// fixup overused tags, out of date tags, from old bad delete code days
void AreaPnP_Names_Fixup(void)
{
   int *tags, cnt;
   char **names;
   cnt=NamenumAll(pNN_HotRegions,&names,&tags);
   if (cnt)
   {
      int *toast, toasted=0, i, j;
      toast=(int *)Malloc(cnt*sizeof(int));
      for (i=0; i<cnt; i++)
      {
         editBrush *us=brFind(tags[i]);
         if ((us==NULL)||(brushGetType(us)!=brType_HOTREGION))
         {
            Warning(("AreaPnPFixup: no brush or not area on tag %d (%s)\n",tags[i],names[i]));
            toast[toasted++]=tags[i];
         }
         else
            for (j=i+1; j<cnt; j++)
               if (tags[i]==tags[j])
               {
                  Warning(("AreaPnPFixup: Later match on %d (%s) to %d (%s) [%d,%d]\n",tags[i],names[i],tags[j],names[j],i,j));
                  toast[toasted++]=tags[i];
                  break;
               }
      }
      for (i=0; i<toasted; i++)
         NamenumClearByTag(pNN_HotRegions,toast[i]);
   }
   Free(names);
   Free(tags);
}

// delete this hotregions name tags
void AreaPnP_Delete_Brush_Tags(editBrush *us)
{
   if (brushGetType(us)==brType_HOTREGION)
      if (NamenumFetchByTag(pNN_HotRegions,us->br_id))
         NamenumClearByTag(pNN_HotRegions,us->br_id);
}

// go save them off
BOOL AreaPnP_Names_Save(ITagFile *file)
{
   if (pNN_HotRegions!=NULL)
      return NamenumSave(pNN_HotRegions,file);
   return FALSE;
}

// really just needs to retag the list
void AreaPnP_Names_Remap(int *mapping_array)
{
   if (pNN_HotRegions)
      NamenumRetag(pNN_HotRegions,mapping_array);
}

// for app init
void AreaPnP_Names_Init(void)
{
   ged_remap_add_callback(AreaPnP_Names_Remap);
   pNN_HotRegions=NULL;
}

// and app close
void AreaPnP_Names_Term(void)
{
   ged_remap_rem_callback(AreaPnP_Names_Remap);
   if (pNN_HotRegions)   
      NamenumFree(pNN_HotRegions);
   pNN_HotRegions=NULL;
}
