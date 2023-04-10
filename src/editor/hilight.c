/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/hilight.c,v 1.17 2000/03/24 14:33:19 adurant Exp $
// brush highlighting system

#include <string.h>

#include <command.h>
#include <status.h>

#include <property.h>

#include <csgbrush.h>
#include <hilight.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brquery.h>
#include <brrend.h>
#include <gedit.h>
#include <vbrush.h>
#include <objpos.h>
#include <editobj.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define HILIGHT_NUM_BITS 8

// global highlight controls
uchar hilight_array[MAX_CSG_BRUSHES];
uchar hilight_active=0;               // which hilights im actually showing

// are we hilighting everything, or just rendered brushes
static BOOL hilight_global=TRUE;

//////////////////
// common core add/remove of various types
static uchar hilight_cur=1;           // what hilight im currently adding
static bool  hilight_autoclear=TRUE;  // do i reclear every time i add

BOOL hilightAddByBrushId(int brush_id)
{
   hilight_array[brush_id]|=hilight_cur;
   return TRUE;
}

BOOL hilightAddByObjId(int obj_id)
{
   editBrush *us;
   us=editObjGetBrushFromObj(obj_id);
   if (us)
      hilightAddByBrushId(us->br_id);
   return TRUE;
}

BOOL hilightAddByBrush(editBrush *br)
{
   return hilightAddByBrushId(br->br_id);
}

BOOL hilightRemoveByBrushId(int brush_id)
{
   if (hilight_array[brush_id]&hilight_cur)
   {
      hilight_array[brush_id]&=~hilight_cur;
      return TRUE;
   }
   return FALSE;
}

BOOL hilightRemoveByObjid(int obj_id)
{
   editBrush *us;
   us=editObjGetBrushFromObj(obj_id);
   if (us)
      return hilightRemoveByBrushId(us->br_id);
   return FALSE;
}

BOOL hilightRemoveByBrush(editBrush *br)
{
   return hilightRemoveByBrushId(br->br_id);
}

static void hilight_clear_all(void)
{
   memset(hilight_array,0,MAX_CSG_BRUSHES);
}

void hilightClearBits(int bitmask)
{
   int i;
   for (i=0; i<MAX_CSG_BRUSHES; i++)
      hilight_array[i]&=~bitmask;
}

//////////////////////////
// common setup/operation code for highlights

void hilight_begin(void)
{
   if ((hilight_cur&hilight_active)&&(hilight_autoclear))
   {
      hilightClearBits(hilight_cur);
      hilight_active=0;
   }
}

void hilight_end(ulong flags)
{
   if (flags & kHilightDone)
   {
      hilight_active|=hilight_cur;
      gedit_full_redraw();
   }
}

static BOOL (*_hilight_cur_check)(editBrush *br)=NULL;
static int    _hilight_cur_cnt=0;
void hilight_active_check(editBrush *br)
{
   if (_hilight_cur_check)
      if ((*_hilight_cur_check)(br))
      {
         hilightAddByBrush(br);
         _hilight_cur_cnt++;
      }
}
   
static BOOL hilight_simple_run(BOOL (*br_check)(editBrush *br))
{
   BOOL rv=TRUE;

   hilight_begin();
   if (hilight_global)
   {
      if (!brFilter(br_check,hilightAddByBrush))
      {
         Status("No matching brushes");
         rv=FALSE;
      }
   }
   else
   {
      _hilight_cur_check=br_check;
      _hilight_cur_cnt=0;
      brushRunOnActive(hilight_active_check);
      rv=_hilight_cur_cnt>0;
   }
   hilight_end((rv)? kHilightDone : kHilightCancel);

   return rv;
}

BOOL highlight_check(editBrush *br)
{
   return isActiveHighlight(br->br_id);
}

////////////////
// obj highlighting - wants to scan through all objects, checking for this property

static IProperty *pTestProp;

static BOOL property_check(editBrush *br)
{
   if (brushGetType(br)==brType_OBJECT)
      return IProperty_IsRelevant(pTestProp,brObj_ID(br));
   return FALSE;
}

static BOOL property_check_direct(editBrush *br)
{
   if (brushGetType(br)==brType_OBJECT)
      return IProperty_IsSimplyRelevant(pTestProp,brObj_ID(br));
   return FALSE;
}

static void hilight_obj_with_property(char *str)
{
   BOOL haveProp;
   if ((str==NULL)||(str[0]=='\0'))
   {
      Status("Prop menu not yet supported here");
      return;  // want to bring up dialog with which to choose a property
   }
   haveProp = GetPropertyInterfaceNamed(str,IProperty,&pTestProp);
   if (!haveProp||(pTestProp==NULL))
   {
      Status("Dont know about that property");
      return;
   }
   hilight_simple_run(property_check);
   SafeRelease(pTestProp);
}

static void hilight_obj_with_property_direct(char *str)
{
   BOOL haveProp;
   if ((str==NULL)||(str[0]=='\0'))
   {
      Status("Prop menu not yet supported here");
      return;  // want to bring up dialog with which to choose a property
   }
   haveProp = GetPropertyInterfaceNamed(str,IProperty,&pTestProp);
   if (!haveProp||(pTestProp==NULL))
   {
      Status("Dont know about that property");
      return;
   }
   hilight_simple_run(property_check_direct);
   SafeRelease(pTestProp);
}

// needs to do wacky stuff, basically
static void hilight_split_obj(void)
{
   extern BOOL hilight_split_objs;  // in eosapp, really
   if ((hilight_cur&hilight_active)&&(hilight_autoclear))
   {
      hilightClearBits(hilight_cur);
      hilight_active=0;
   }
   hilight_split_objs=TRUE;
   ObjDeleteAllRefs();     // this will cause it to call out 
   ObjBuildAllRefs();      //   since split_objs is true..
   hilight_split_objs=FALSE;
   hilight_active|=hilight_cur;
   gedit_full_redraw();
}

////////////////
// terrain highlighting
// find nonaxial stuff
static BOOL nonaxial_check(editBrush *br)
{
   if (brushGetType(br)==brType_TERRAIN)
      return ((br->ang.el[0]&0x3fff)||(br->ang.el[1]&0x3fff)||(br->ang.el[2]&0x3fff));
   return FALSE;
}

static void hilight_nonaxial_terrain(void)
{
   hilight_simple_run(nonaxial_check);
}

// find given media
static int  terr_media_target=0;
static BOOL terr_media_check(editBrush *br)
{
   if (brushGetType(br)==brType_TERRAIN)
      return br->media==terr_media_target;
   return FALSE;
}

static void hilight_media_type(int media_id)
{
   terr_media_target=media_id;
   hilight_simple_run(terr_media_check);
}

// check for terrain texture id
static int  terr_texture_target=0;
static BOOL terr_texture_check(editBrush *br)
{
   if (brushGetType(br)==brType_TERRAIN)
   {
      int i, use_default=0;
      for (i=0; i<br->num_faces; i++)
         if (br->txs[i].tx_id==-1)
            use_default++;
         else if (br->txs[i].tx_id==terr_texture_target)
            return TRUE;
      return (use_default && (br->tx_id==terr_texture_target));
   }
   return FALSE;
}

static void hilight_texture_id(int texture_id)
{
   terr_texture_target=texture_id;
   hilight_simple_run(terr_texture_check);
}

// for snap checking
static BOOL any_snap_check(editBrush *br)
{
   if (brushGetType(br)==brType_TERRAIN)
      if (!br->grid.grid_enabled)
      {
         BOOL need_snap=FALSE;
         float tmp;
         int i;
         for (i=0; i<3; i++)
         {
            tmp=br->pos.el[i]; need_snap|=_gedit_float_snap(&tmp);
            tmp=br->sz.el[i];  need_snap|=_gedit_float_snap(&tmp);
         }
         return need_snap;
      }
   return FALSE;
}

static BOOL ortho_snap_check(editBrush *br)
{
   if (brushGetType(br)==brType_TERRAIN)   
      if (!nonaxial_check(br))
         return any_snap_check(br);
   return FALSE;
}

static void hilight_check_snap(int cntrl)
{
   hilight_simple_run(cntrl?any_snap_check:ortho_snap_check);
}

static BOOL do_snap_op(editBrush *us)
{
   return gedit_snap_brush(us);
}

static void hilight_do_snap(void)
{
   brFilter(highlight_check,do_snap_op);
   gedit_full_redraw();
}

////////////////
// multibrush interface

static BOOL add_to_mbrush(editBrush *br)
{
   return vBrush_editBrush_Op(br,vBrush_OP_ADD);
}

static void multibrush_the_highlight(void)
{
   brFilter(highlight_check,add_to_mbrush);
   vBrush_getToCurGroup();
   gedit_full_redraw();
}

static IProperty* pCreateProp;

static BOOL propertize(editBrush* br)
{
   if (brushGetType(br)==brType_OBJECT)
   {
      IProperty_Create(pCreateProp,brObj_ID(br));
      return TRUE;
   }
   return FALSE;
}

static void propertize_the_highlight(char* propname)
{
   pCreateProp = GetPropertyNamed(propname);
   brFilter(highlight_check,propertize);
   SafeRelease(pCreateProp);
}

static BOOL unpropertize(editBrush* br)
{
   if (brushGetType(br)==brType_OBJECT)
   {
      IProperty_Delete(pCreateProp,brObj_ID(br));
      return TRUE;
   }
   return FALSE;
}

static void unpropertize_the_highlight(char* propname)
{
   pCreateProp = GetPropertyNamed(propname);
   brFilter(highlight_check,unpropertize);
   SafeRelease(pCreateProp);
}

//////////////////////////
// dumb really basic UI
static void hilight_use(int use)
{
   int i, bitval, hits=0;

   for (i=0, bitval=1; i<HILIGHT_NUM_BITS; i++, bitval<<=1)
      if (bitval&use)
         hits++;
   if (hits==1)
      hilight_cur=use;
   else
      Status("Poorly formed use bits");
}

static void hilight_clear(int which)
{
   if (which==0)
      hilight_clear_all();
   else
      hilightClearBits(which);
   gedit_full_redraw();
}

static void hilight_activate(int which)
{
   hilight_active|=which;
   gedit_full_redraw();   
}

static void hilight_deactivate(int which)
{
   hilight_active&=~which;
   gedit_full_redraw();   
}

static void hilight_brush(int which)
{
   BOOL full_redraw=FALSE;
   if (which==0)
   {
      editBrush *us=vBrush_GetSel();
      which=us->br_id;
      // perhaps should check if vbrush
      // if so, do fancy stuff (tm)
      full_redraw=TRUE;  // for now, since hilight doesnt seem to work right with incremental update      
   }
   else
      full_redraw=TRUE;
   hilightAddByBrushId(which);
   hilight_active|=hilight_cur;
   if (full_redraw) gedit_full_redraw();
}

static void reinstantiate_and_highlight(char *str)
{
  hilight_obj_with_property(str);
  unpropertize_the_highlight(str);
  propertize_the_highlight(str);
}

//////////////////////////
// misc ui stuff

static BOOL _list_br(editBrush* br)
{
   mprintf("Brush %d time %d\n",br->br_id,br->timestamp);
   return TRUE;
}

static BOOL _list_obj(editBrush* br)
{
   if (brushGetType(br)==brType_OBJECT)
   {
      mprintf("Obj %d brush %d\n",brObj_ID(br),br->br_id);
      return TRUE;
   }
   return FALSE;
}

static void hilight_list(BOOL all_br)
{
   brFilter(highlight_check,all_br?_list_br:_list_obj);
}

//////////////////////////
// actual key commands for highlight system
Command hilight_keys[] =
{
   { "hilight_by_prop", FUNC_STRING, hilight_obj_with_property, "give property name, highlights obj's with it" },
   { "hilight_by_prop_direct", FUNC_STRING, hilight_obj_with_property_direct, "give property name, highlights obj's with it specifically on them (not inherited)" },
   { "hilight_nonaxial", FUNC_VOID, hilight_nonaxial_terrain, "highlights any terrain with non-90 angles" },
   { "hilight_media", FUNC_INT, hilight_media_type, "highlight terrain w/media_op of type <arg>" },
   { "hilight_texture", FUNC_INT, hilight_texture_id, "highlight terrain w/texture id <arg>" },
   { "hilight_split_obj", FUNC_VOID, hilight_split_obj, "highlight objects crossing a portal" },
   { "multibrush_the_highlight", FUNC_VOID, multibrush_the_highlight, "make hilight objs the multibrush" },
   { "hilight_brush", FUNC_INT, hilight_brush, "hilight current (0) or brush_id" },
   { "hilight_check_snap", FUNC_INT, hilight_check_snap, "hilight unangled unsnapped brushes, or if (1) all unsnapped brushes" },
   { "hilight_do_snap", FUNC_INT, hilight_do_snap, "grid snap all hilight brushes..." },
   { "hilight_list", FUNC_BOOL, hilight_list, "list objs, or, if arg 1, all brush ids" },
   { "hilight_global", TOGGLE_BOOL, &hilight_global, "if true, we will hilight everything, else just active" },
   
   { "hilight_autoclear", TOGGLE_BOOL, &hilight_autoclear, "do we autoclear old hilight and make it only active" },
   { "hilight_use", FUNC_INT, hilight_use, "set which hilight bit to use (bitfield, must be just 1)" },
   { "hilight_clear", FUNC_INT, hilight_clear, "clear current highlights (all if 0, else bitfields)" },
   { "hilight_activate", FUNC_INT, hilight_activate, "turn on hilight bits" },
   { "hilight_deactivate", FUNC_INT, hilight_deactivate, "turn off hilight bits" },
   { "hilight_add_prop", FUNC_STRING, propertize_the_highlight, "Add a named property to all hilit objects"},
   { "hilight_rem_prop", FUNC_STRING, unpropertize_the_highlight, "Add a named property to all hilit objects"},

   { "hilight_render", TOGGLE_BOOL, &renderHilightOnly },
   { "hilight_reinstantiate", FUNC_STRING, reinstantiate_and_highlight, "Hilight all objects with property, then remove and readd property"},
}; // all take bitfields since while ugly it is consistent

void hilightCommandRegister(void)
{
   hilight_clear_all();
   COMMANDS(hilight_keys, HK_BRUSH_EDIT);
}
