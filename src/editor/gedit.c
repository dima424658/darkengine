/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gedit.c,v 2.10 2000/02/18 17:20:57 MAT Exp $

// gedit.c
// contains functionality for test program

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>
#include <r3d.h>
#include <lgd3d.h>
#include <md.h>
#include <star.h>
#include <dump.h>
#include <event.h>
#include <config.h>
#include <kbcook.h>
#include <appagg.h>

#include <portal.h>
#include <command.h>
#include <cfgtool.h>

#include <objpos.h>
#include <objedit.h>
#include <osysbase.h>

#include <animlgt.h>
#include <cvtbrush.h>
#include <editbr.h>
#include <editbr_.h>
#include <primal.h>
#include <brlist.h>
#include <brinfo.h>
#include <brobjpro.h>
#include <brrend.h>
#include <vbrush.h>
#include <editobj.h>
#include <csg.h>
#include <csgbrush.h>
#include <viewmgr.h>
#include <status.h>
#include <undoredo.h>
#include <brundo.h>
#include <brquery.h>
#include <gridsnap.h>
#include <gedit.h>
#include <ged_rend.h>
#include <ged_undo.h>
#include <ged_csg.h>
#include <ged_line.h>
#include <winui.h>
#include <ged_room.h>
#include <animlit.h>
#include <family.h>     // for texture family loads and unloads, commands
#include <texprop.h>
#include <media.h>
#include <mediaop.h>
#include <editobj.h>
#include <filetool.h>
#include <vumanui.h>
#include <gamecam.h>
#include <appsfx.h>
#include <scrnman.h>
#include <skyrend.h>
#include <texmem.h>
#include <txtpnp.h>
#include <objlight.h>
#include <wrmem.h>
#include <preload.h>
#include <modalui.h>
#include <starhack.h>
#include <bldstair.h>
#include <hilight.h>
#include <gedmedmo.h>
#include <doorphys.h>
#include <iobjsys.h>
#include <physapi.h>
#include <missrend.h>
#include <pgrpprop.h>
#include <saveload.h>

#include <resapi.h>
#include <imgrstyp.h>

#include <prof.h>
#include <objshape.h>
#include <objmodel.h>
#include <editsave.h>

#include <rendobj.h>    // for rendobj_name_color

#include <comtools.h>
#include <traitman.h>

#include <dbmem.h>      // last included file

extern BOOL g_lgd3d;

extern int scale_factor;

int TERRAIN_ID = 0xFFFF;
int MAX_LOCKED_BRUSHES = 50;

ObjID gLockedBrushes [50];
BOOL brushIsLocked (editBrush* brush);
void UpdateMenuCheckmarks (void);

Grid cur_global_grid = { 4.0, { 0,0,0 }, { 0,0,0 }, TRUE};

void gedit_register(void);

// initialize geom editor, specifically brush lists
void gedit_init(void)
{
   gedit_register();
   undoStackInit();
   gedLineInit();
}

void gedit_full_redraw(void)
{
   vm_redraw();
}

void gedit_redraw_3d(void)
{
   vm_redraw_from_new3d();
}

void gedit_term(void)
{
   undoStackFree();
   gedLineTerm();
}

static editBrush *gedit_find_point(int camera, int x, int y)
{
   editBrush *us, *winner=NULL;
   int useFlag=brFlag_COLOR_NONE, hIter;
   float best_dist=DISTANCE_INF;  // bigger than x+y screen coordinates

   if (!vm_get_3d(camera)) useFlag|=brFlag_IsoView;
   brushSetupDraw(useFlag,camera);
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      float dist=brushClickCheck(us,x,y);
      if (dist<best_dist)
      {
         winner=us;
         best_dist=dist;
      }
      us=blistIterNext(hIter);
   }
   return winner;
}

BOOL gedit_click(int flags, int x, int y, int camera)
{
   editBrush *us;
   BOOL rv=FALSE;
   int brush, face;

   if (flags&GEDIT_CLICK_IS3D)
      rv=vm_pick_camera(camera,x,y,&brush,&face);

   if (rv)  // if it wasnt 3d, rv is always false here
   {   // this is only taken on 3d cameras which vm can pick for (ie. solid)
      int cur_txt;
      us=csg_brush[brush];

      if ((us)&&(brushGetType(us)!=brType_HOTREGION))
      {
         BOOL do_select=TRUE;
         // some autotexture if palette up thing here....?
         cur_txt=txtPnP_pal_query();
         if (cur_txt!=TXTPNP_NO_PAL_UP)
         {
            int old_face=us->cur_face;  // save face, cause we dont want to change selection
            us->cur_face=face;          //    in tpal mode
            gedit_texture_brush(us,cur_txt);
            us->cur_face=old_face;
            do_select=config_is_defined("txtrpal_select");
            if (txtPnP_attached())
            {
               if (!config_is_defined("txtrpal_no_detach"))
                  txtPnP_detach(); // people hate this....
               if (txtPnP_attached())
                  do_select=TRUE;
            }

         }
         if (do_select)
         {
				us->cur_face=face;
            flags&=~GEDIT_CLICK_FANCY;  // cant be a fancy click in 3d scene
            flags|=GEDIT_CLICK_NOTOGGLE;
            brSelect_Flag(us)&=~brSelect_COMPLEX;
            vBrush_click(us,flags,x,y);   // needs xy for fancy clicks
         }
      }
      else
         rv=FALSE;
   }                              // or should fancy clicks autotexture?
   else
   {
      vm_set_camera_canvas(camera);
      vm_start_3d(camera);
      us=gedit_find_point(camera,x,y);
      if (us)
      {
         vBrush_click(us,flags,x,y);   // needs xy for fancy clicks
         rv=TRUE;
      }
      vm_end_3d(camera);
      vm_unset_camera_canvas();
   }
   return rv;
}

// get the maximal extent of a level
mxs_vector *gedit_get_extent(BOOL hot_region_p)
{
   static mxs_vector bounds[2];
   int hot_found=0, hIter;
   editBrush *us;

   mx_mk_vec(&bounds[0],0.0,0.0,0.0);
   mx_mk_vec(&bounds[1],0.0,0.0,0.0);
   brushConvertStart();
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if (hot_region_p)
      {
         if (brushGetType(us)==brType_HOTREGION)
         {
            brushCheckExtents(us,bounds);
            hot_found++;
         }
      }
      else
         brushCheckExtents(us,bounds);
      us=blistIterNext(hIter);
   }
   brushConvertEnd();
   if (hot_region_p&&hot_found==0)
   {  // bounds are still 0, since we saw nothing, so lets just go do everything
      brushConvertStart();
      us=blistIterStart(&hIter);
      while (us!=NULL)
      {
         brushCheckExtents(us,bounds);
         us=blistIterNext(hIter);
      }
      brushConvertEnd();
      Warning(("no hot region\n"));
   }
   return &bounds[0];
}

// if you need to take all objects out and put them back in?
static void gedit_replace_all_objs(void)
{
   editobjFullDeref();
   editobjFullReref();
}

static int focus_br_id=-1;

// inside gedit, we want non scripted lights, so this deals
void gedit_enter(void)
{
#ifdef FAST_DYNAMIC_LIGHTING
   gedcsg_relight_level();
#endif
   if (focus_br_id!=-1)
      vBrush_SelectBrush(brFind(focus_br_id));
   editUndoSuspend(FALSE);
   vm_resume(); 
}

void gedit_exit(void)
{
   if (gedit_editted)
   {
      if (winui_GetYorN("Portalization Not Up to Date.  Reportalize?"))
         build_level();
   }
   else if (!gedcsg_lighting_ok)
      if (winui_GetYorN("Lighting Not Up to Date.  Relight?"))
         gedcsg_relight_level();

   if (!g_RoomsOK)
   {
#if 0
      if (winui_GetYorN("Room database Not Up to Date.  Recompile?"))
#endif
         ged_compile_rooms();
   }

#ifdef FAST_DYNAMIC_LIGHTING
   gedcsg_relight_level();
#endif
   editUndoSuspend(TRUE);
   undoClearUndoStack();
   focus_br_id=vBrush_GetSel()->br_id;
   vm_suspend(); 
}

///////////////////
// snapping control

// this is to make following brush getting code easier/smaller (and backwards compatible, to boot)
#define cur vBrush_editBrush_Get

#define FLOAT_SNAP_TOL 0.00001
static float  gedit_snap_pos=0.5, gedit_snap_sz=0.5;
static fixang gedit_snap_ang=0x1000;
static BOOL   gedit_snap_placement=TRUE;

// returns whether or not it snapped, in its mind
BOOL _floatSnap(float *val, float snap)
{
   float newval=floor(((*val)+snap/2) / snap) * snap;
   float oldval=*val;
   *val=newval;
   return newval!=oldval;
}

BOOL _fixangSnap(fixang *val, fixang snap)
{
   fixang toround=(*val)+snap/2;
   fixang newval=toround-(toround%snap);
   fixang oldval=*val;
   *val=newval;
   return newval!=oldval;
}

BOOL _gedit_float_snap(float *val)
{
   return _floatSnap(val,cur_global_grid.line_spacing);
}

static mxs_vector vbrush_hack_delta;

static void delta_snap_op(editBrush *us)
{
   mx_addeq_vec(&us->pos,&vbrush_hack_delta);
}


BOOL snap_brush_by_vertex_hack(editBrush *us)
{
   int i;
   BOOL snapped = FALSE;
   mxs_vector bounds[2];
   mxs_vector size;
   float r;

   brushConvertStart();
   mx_copy_vec(&bounds[0],&us->pos);
   mx_copy_vec(&bounds[1],&us->pos);
   brushCheckExtents(us,bounds);
   brushConvertEnd();
   mx_sub_vec(&size,&bounds[1],&bounds[0]);
   mx_diveq_vec(&size,2.0F);
   for (i=0; i<3; i++)
   {
      // okay, now figure out whether to snap real or half
      r = (float)fmod((double)size.el[i],(double)cur_global_grid.line_spacing);
      // if we were 1.0 aligned, center must also be 1.0 aligned
      if (r <= cur_global_grid.line_spacing / 4) // if exact match, 1.0 aligned
      {
         snapped|=_floatSnap(&us->pos.el[i],cur_global_grid.line_spacing);
      }
      else
      {
         // if we were 0.5 aligned, center must also be 0.5 aligned, but NOT 1.0 aligned
         us->pos.el[i] += cur_global_grid.line_spacing / 2; // first put it off by 0.5
         snapped|=_floatSnap(&us->pos.el[i],cur_global_grid.line_spacing); // then "1.0" align
         us->pos.el[i] -= cur_global_grid.line_spacing / 2; // now restore from offset
      }
   }

   return(snapped);
}

BOOL gedit_snap_brush(editBrush *us)
{
   BOOL snapped=FALSE;
   int i;

   if (!cur_global_grid.grid_enabled)
      return FALSE;
   if (us==NULL)
      us=cur();
   if (brSelect_Flag(us)==brSelect_VIRTUAL)
   {
      // note that vbrush snap only defined for positioning, 
      // not angle or size, is this OK?
      if (config_is_defined("vbrush_snap"))
      {
         editBrush *mbsel;
         mxs_vector oldpos;

         // record old data
         mbsel = vBrush_GetSel();
         mx_copy_vec(&oldpos, &mbsel->pos);

         // snap center point
         snap_brush_by_vertex_hack(mbsel);

         // note delta and restore
         mx_sub_vec(&vbrush_hack_delta,&mbsel->pos,&oldpos);
         mx_copy_vec(&mbsel->pos,&oldpos);

         // apply delta to all brushes individually
         // since we have no mechanism for passing data into this we stuff
         // the translation factor into the aptly named vbrush_hack_delta
         vBrush_GroupOp(FALSE, delta_snap_op);

         return snapped;
      }
      else
         return FALSE;
   }
   if (!((brushGetType(us)==brType_TERRAIN)||(brushGetType(us)==brType_ROOM)))
      return FALSE;

   if (config_is_defined("new_snap"))
   {
      for (i=0; i<3; i++)
      {  
         // note we snap size first to keep the game from resizing already happily sized brushes
         // find the closest size that can be 0.5 or 1.0 aligned
         snapped|=_floatSnap(&us->sz.el[i],cur_global_grid.line_spacing/2);
      }
      snapped |= snap_brush_by_vertex_hack(us);
      for (i=0; i < 3; i++)
         snapped|=_fixangSnap(&us->ang.el[i],gedit_snap_ang);
   }
   else
   {
      for (i=0; i<3; i++) {
         snapped|=_floatSnap(&us->pos.el[i],cur_global_grid.line_spacing);
         snapped|=_floatSnap(&us->sz.el[i],cur_global_grid.line_spacing);
         snapped|=_fixangSnap(&us->ang.el[i],gedit_snap_ang);
      }
   }
   return snapped;
}

void gedit_snap_object (editBrush *us)
{
   int i;
   Location objLoc, newLoc;
   mxs_vector minv, maxv;
   mxs_vector minSnap, maxSnap;
   mxs_vector minOffset, maxOffset;
   
   //  Snap objects by their closest vertex
   //
   ObjGetObjOffsetBBox(brObj_ID(us), &minv, &maxv);
   mx_add_vec (&minv, &minv, &us->pos);
   mx_add_vec (&maxv, &maxv, &us->pos);
   minSnap = minv;
   maxSnap = maxv;

   for (i=0; i<3; i++)
   {  
       _floatSnap (&minSnap.el[i], cur_global_grid.line_spacing);
       _floatSnap (&maxSnap.el[i], cur_global_grid.line_spacing);
   }

   mx_sub_vec (&minOffset, &minSnap, &minv);
   mx_sub_vec (&maxOffset, &maxSnap, &maxv);

   for (i=0; i<3; i++)
   {
       us->pos.el[i] += (fabs(minOffset.el[i]) < fabs(maxOffset.el[i])) ? minOffset.el[i] : maxOffset.el[i];
   }

   objLoc = ObjPosGet(brObj_ID(us))->loc;
   MakeHintedLocationFromVector (&newLoc, &us->pos, &objLoc);
   ObjPosSetLocation (brObj_ID(us), &newLoc);
}

void gedit_set_snaps(float snap_pos, float snap_sz, fixang snap_ang)
{
   if (snap_pos>0) gedit_snap_pos=snap_pos;
   if ( snap_sz>0) gedit_snap_sz =snap_sz;
   if (snap_ang>0) gedit_snap_ang=snap_ang;  // 1 really means no snap
}

////////////////////
/// grid stuff

void grid_get_from_current(void)
{
   cur_global_grid = cur()->grid;
   gedit_full_redraw();
}

void grid_set_from_current(void)
{
   cur()->grid = cur_global_grid;
}

void grid_toggle(void)
{
   cur_global_grid.grid_enabled = !cur_global_grid.grid_enabled;
   gedit_full_redraw();
}

void grid_scale(float scale)
{
   cur_global_grid.line_spacing *= scale;
   gedit_full_redraw();
}

void grid_abs_scale(float scale)
{
   cur_global_grid.line_spacing = scale;
   gedit_full_redraw();
}

void grid_pow2_scale(int pow2)
{
   float real_scale=scale_pow2int_to_float(pow2);
   grid_abs_scale(real_scale);
}

// this, sadly, needs to convert the scale (currently a spacing float) to a pow2 rep
// so we do the stupid thing, cause we are, as they say... stupid
int get_pow2_scale(void)
{
   float val;
   int i;

   for (i=8; i<18; i++)
   {
      val=scale_pow2int_to_float(i);
      if (val>=cur_global_grid.line_spacing)
         return i;
   }
   return 16;  // default, since we dont know
}

//////////////
// brush build/control stuff

int brush_build_type=brType_TERRAIN;

// if build_type is -1, then if old that is type, else type is left alone, else use build_type
// if old is non-Null, then misc_settings (old format) are assumed, and old is passed down to field_update
// at_end means place new brush at end, else place at current
BOOL gedit_full_create_brush(editBrush *new_br, editBrush *old, ulong flags, int build_type)
{
   IObjectSystem *pObjSys = AppGetObj(IObjectSystem);
   BOOL at_end = flags & GEDIT_CREATE_AT_END;
   ulong field_flags = brField_New;

   IObjectSystem_Lock(pObjSys);

   if (build_type!=-1)
      brushSetType(new_br,build_type);
   else if (old)
      brushSetType(new_br,brushGetType(old));

   if (flags & GEDIT_CREATE_CLONE)
      field_flags |= brField_Clone;
   brush_field_update(new_br,old,field_flags);      // copy over relevant settings - we are always a new brush

   blistSeek(0,at_end?SEEK_END:SEEK_CUR);    // now actually put the thing there
   blistInsert(new_br);                      // put us in the list
   editUndoStoreCreate(new_br);
   if (brushGetType(new_br)==brType_TERRAIN)
      gedit_db_update(FALSE);
   gedit_full_redraw();

   IObjectSystem_Unlock(pObjSys);
   SafeRelease(pObjSys);

   return TRUE;
}

void gedit_set_default_brush_type(int br_type)
{
   brush_build_type=br_type;
}

int gedit_default_brush_type(void)
{
   return brush_build_type;
}

///////////////////
// status and control

static BOOL auto_portalize;
static BOOL auto_roombuild;

BOOL gedit_editted=TRUE;

void gedit_db_update(BOOL redraw)
{
   if (redraw)
      gedit_full_redraw();
   if (auto_portalize)
   {
      build_level();
      gedit_redraw_3d();
   }
   else
   {
      gedit_editted=TRUE;
      StatusField(SF_FILESTATE,"*");
   }
}

void gedit_light_update(BOOL redraw)
{
   if (redraw)
      gedit_full_redraw();
   if (auto_portalize)
   {
      gedcsg_relight_level();
      gedit_redraw_3d();
   }
   else
   {
      gedcsg_lighting_ok=FALSE;
      StatusField(SF_FILESTATE,"*");
   }
}

void gedit_room_update(BOOL redraw)
{
   if (redraw)
      gedit_full_redraw();
   if (auto_roombuild)
      ged_compile_rooms();
   else
   {
      g_RoomsOK = FALSE;
      StatusField(SF_FILESTATE,"*");
   }
}

// @VBRUSH
// fully general find a brush, move it to n
// kinda icky since all brush ops op on current, so it is a mess
// i should just go do this right
bool set_brush_to_time_n(editBrush *br, int n)
{
   editBrush *old_Curs=blistGet();
   int oldPos=blistCheck(br);
   bool is_us=(br==old_Curs), rv=TRUE;

   if (oldPos==-1) return FALSE;
   if (oldPos==n)  return TRUE;
   if (!is_us)
      blistSetPostoBrush(br);
#ifdef DBG_ON
   if (br!=blistDelete())
      Warning(("Hey delete didnt delete us???"));
#else
   blistDelete();
#endif
   //   if ((n<oldPos)&&(n>0)) n--;   // ????
   if (n>0) n--;
   if (!blistSeek(n,SEEK_SET))
   {
      rv=FALSE;
      Warning(("Hey, couldnt seek to new time"));
   }
   blistInsert(br);
   blistSetPostoBrush(is_us?br:old_Curs);
   if (brushGetType(br)==brType_TERRAIN)   // if we are terrain, the db needs to rebuild now
      gedit_db_update(FALSE);
   return rv;
}

// @TODO: undo support for these!!
static void move_brush_to_end_of_time(void)
{
   editBrush *modBrush=blistDelete();          // delete the existing place for the brush
   blistSeek(0,SEEK_END);                      //   get to the end of time
   blistInsert(modBrush);                      //   put us back in the list
   brush_field_update(modBrush,NULL,brField_Old);    // update any field data
   if (brushGetType(modBrush)==brType_TERRAIN) // if we are terrain, the db needs to rebuild now
      gedit_db_update(FALSE);
}

static void move_brush_to_time_n(int n)
{
   set_brush_to_time_n(cur(),n);
}

/////////////////////////////
// texture control

short *_geditGetTxPtr(editBrush *br)
{
   short *tx;
   if (br->cur_face == -1)     // global texture, or per face
      tx=&br->tx_id;
   else
      tx=&br->txs[br->cur_face].tx_id;
   return tx;
}

// this is incomplete
// it needs to have real code for multiwallset
bool geditTxCycle(editBrush *br, int cycle)
{
   short *tx=_geditGetTxPtr(br);

   if (*tx+cycle>=texmemGetMax())
      *tx=0;
   else if (*tx+cycle<0)
      *tx=texmemGetMax()-1;
   else
      *tx=*tx+cycle;

   return TRUE;
}

// returns if it made the change
BOOL gedit_reassign_texture(editBrush *br, BOOL texture_only)
{
   if (gedit_editted)
      Status("Must reportalize to see texture.");
   else
      ReassignTexture(br->br_id,texture_only);
   return !gedit_editted;
}

static void _tex_finish(editBrush *br)
{
   if (gedit_reassign_texture(br,TRUE))
      gedit_redraw_3d();
   editUndoStoreBrush(br);
}

static void cycle_tex(int dir)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   geditTxCycle(modBrush, dir);
   _tex_finish(modBrush);     // does the undo
}


void gedit_texture_brush(editBrush *br, int new_tx)
{
   short *tx;
   editUndoStoreStart(br);
   tx=_geditGetTxPtr(br);
   *tx=new_tx;
   _tex_finish(br);     // does the undo
}

static void gedit_set_brush_tx(int new_tx)
{
   gedit_texture_brush(cur(),new_tx);
}

static void gedit_reset_br_textures(int new_tx)
{
   editBrush *modBrush=cur();
   short *tx;
   int i;

   if (new_tx==-1) { Warning(("Given -1 as new_tx\n")); new_tx=0; }
   editUndoStoreStart(modBrush);
   modBrush->cur_face=-1;     // set to default face
   tx=_geditGetTxPtr(modBrush);
   *tx=new_tx;
   for (i=0; i<modBrush->num_faces; i++)
   {
      modBrush->cur_face=i;
      tx=_geditGetTxPtr(modBrush);
      *tx=-1;
      if (gedit_reassign_texture(modBrush,TRUE))
         gedit_redraw_3d();
   }
   //   _tex_finish(modBrush);     // does the undo - but a reassign as well?
   editUndoStoreBrush(modBrush);
}

static void sky_tex(void)
{
   editBrush *modBrush=cur();
   short *tx=_geditGetTxPtr(modBrush);
   editUndoStoreStart(modBrush);
   *tx = 249;
   _tex_finish(modBrush);     // does the undo
}

static void hack_align_texture(void)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   if (modBrush->cur_face == -1)
   {
      int i;
      for (i=0; i<modBrush->num_faces; i++)
         modBrush->txs[i].tx_rot = TEXINFO_HACK_ALIGN;
   } else
      modBrush->txs[modBrush->cur_face].tx_rot = TEXINFO_HACK_ALIGN;
   _tex_finish(modBrush);
}

//////////////////////
// media control

static void media_change(editBrush *modBrush, int med)
{
   editUndoStoreStart(modBrush);
   modBrush->media = med;
   editUndoStoreBrush(modBrush);
   gedit_db_update(FALSE);
}

static void cycle_media(int dir)
{
   editBrush *modBrush = cur();
   if (brushGetType(modBrush) == brType_TERRAIN)
      media_change(modBrush,(modBrush->media + dir + num_media_ops) % num_media_ops );
}

static void set_medium(int med)
{
   editBrush *modBrush = cur();
   if (brushGetType(modBrush) == brType_TERRAIN)
      media_change(modBrush,med);
}

///////////////
// misc hacks start around here..

#define SKYBOX_VIEWS 5

// arrays for skybox
static mxs_angvec g_aCamAngle[SKYBOX_VIEWS]
= { {0, 0, 32768},
    {0, 0, 16384},
    {0, 0, 0},
    {0, 0, 49152},
    {0, 49152, 16384},
};

static char g_achSuffix[SKYBOX_VIEWS]
= {'n', 'e', 's', 'w', 't'};

void gedit_make_skybox(char *pszArg)
{
   int i, iOldW = grd_bm.w, iOldH = grd_bm.h;

   if (!g_lgd3d)
      return;

   if (!pszArg || !pszArg[0]) {
      mprintf("Please give me a base for the filenames.\n");
      mprintf("You could, for example, type: skybox myname.\n");
      mprintf("            Sincerely and devotedly yours,\n");
      mprintf("                                   Skybox.\n");
      return;
   }

   mprintf("gedit: Starting skybox...\n");

   // cheat wildly: give us a square image
   grd_bm.w = grd_bm.h = 256;

   for (i = 0; i < SKYBOX_VIEWS; ++i) {
      g2s_point pt[4], *vlist[4] = { pt, pt + 1, pt + 2, pt + 3};
      char aszFilename[256];
      mxs_vector DummyPos;
      mx_mk_vec(&DummyPos, 0, 0, 0);

      r3_start_frame();
      r3_set_view_angles(&DummyPos, g_aCamAngle + i, R3_DEFANG);
      r3_set_space(R3_PROJECT_SPACE);

      // clear the rectangle
      pt[0].sx = pt[3].sx = pt[0].sy = pt[1].sy = fix_make(-1, 0);
      pt[1].sx = pt[2].sx = pt[2].sy = pt[3].sy = fix_make(256, 0);
      lgd3d_set_z(1);
      lgd3d_set_texture(NULL);
      lgd3d_g2upoly(4, vlist);

      SkyRendererRender();
      r3_end_frame();

      // make sure all our polygons made it to the screen
      ScrnLockDrawCanvas();
      ScrnUnlockDrawCanvas();

      // set filename & take our shot
      sprintf(aszFilename, "%s%c.bmp", pszArg, g_achSuffix[i]);
      dmp_bmp_dump_screen(aszFilename);
   }

   // try to restore order in wake of wild cheating
   grd_bm.w = iOldW;
   grd_bm.h = iOldH;

   mprintf("gedit: skybox done.\n");
}

// if im a terrain brush, change me to a room
void gedit_brush_to_room(float grow_pct)
{
   editBrush *tmp;
   editBrush *modBrush = cur();
   if ((brushGetType(modBrush)!=brType_TERRAIN)&&(brushGetType(modBrush)!=brType_HOTREGION))
   {
      Status("Not a valid brush to clone to room\n");
      return;
   }
   /*
   // this removed to allow area & funky shaped brushes to room-ize, though they
   // do so a little oddly, it can still be useful.  Xemu 5/14/98
   if (modBrush->primal_id!=PRIMAL_CUBE_IDX)
   {
      Status("Not a valid brush to clone to room\n");
      return;
   }
   */
   if (brSelect_Flag(modBrush)==brSelect_VIRTUAL)
   {
      Status("Not a valid brush to clone to room\n");
      return;
   }

   tmp=brushInstantiate(PRIMAL_CUBE_IDX);
   tmp->pos=modBrush->pos;
   tmp->sz=modBrush->sz;
   tmp->ang=modBrush->ang;
   if (grow_pct>0.01)
      mx_scaleeq_vec(&tmp->sz,1.0+(grow_pct/100.0));
   gedit_full_create_brush(tmp,modBrush,GEDIT_CREATE_AT_END,brType_ROOM);
   vBrush_NewBrush(tmp);
}

// if im a terrain brush, do appropriate parameter adoption to rest of selection
void gedit_brush_adopt(int which_val)
{
   editBrush *modBrush = cur();
   if (brushGetType(modBrush) == brType_TERRAIN)
   {

   }
}

static int prim_cur_sides=4, prim_cur_type=0, prim_cur_face=0;


static char *prim_side_name[USED_PRIMAL_TYPES]
= {
   "Cube", "Cylinder", "Pyramid", "Corner pyramid"
};

static int prim_side_limit[USED_PRIMAL_TYPES]
= { USED_PRIMAL_SIDES,
    USED_PRIMAL_SIDES - 2,
    USED_PRIMAL_SIDES - 1,
    USED_PRIMAL_SIDES - 1
};

// make sure the number of sides is valid for this type of primitive--
// if the number is invalid we slam prim_cur_sides
static void prim_side_constrain(void)
{
   if (prim_cur_sides < 3) {
      Warning(("gedit: Too few sides for shape.  Setting to 3.\n"));
      prim_cur_sides = 3;
   }
   else if (prim_cur_sides > prim_side_limit[prim_cur_type]) {
      Warning(("gedit: %s limited to %d sides.\n",
               prim_side_name[prim_cur_type], prim_side_limit[prim_cur_type]));
      prim_cur_sides=prim_side_limit[prim_cur_type];
   }
}

static void prim_finish(void)
{
   prim_side_constrain();

   if (prim_cur_type==0)
      editbr_default_primal=PRIMAL_CUBE_IDX;
   else
      editbr_default_primal=primalID_FullMake(prim_cur_type,prim_cur_face,prim_cur_sides);
   gedit_set_default_brush_type(brType_TERRAIN);
}

void gedit_set_primal_sides(int n)
{
   prim_cur_sides=n;
   prim_finish();
}

extern void SetRadioCheckmark (int menu, int firstPos, int lastPos, int checkPos);

// 0 is cube, 1 cyl, 2 pyr, 3 corner pyr
void gedit_set_primal_type(int t)
{
    if ((t<0)||(t>=PRIMAL_TYPE_MAX))
        Warning(("Invalid primal type\n"));
    else
    {
        prim_cur_type=t;
        prim_finish();
        
        UpdateMenuCheckmarks();
    }
}

// are we face or vertex aligned
void gedit_set_primal_facevertex(bool use_face)
{
   prim_cur_face=use_face?PRIMAL_ALIGN_FACE:0;
   prim_finish();
}

// hmmmm....
void gedit_set_cube(void)
{
   editbr_default_primal=PRIMAL_CUBE_IDX;
   gedit_set_default_brush_type(brType_TERRAIN);
   
   UpdateMenuCheckmarks();
}

void gedit_set_special(int spectype)
{
   editbr_default_primal=primalID_Make(PRIMAL_TYPE_SPECIAL,spectype);
   gedit_set_default_brush_type(brType_TERRAIN);

   UpdateMenuCheckmarks();
}

static void reset(void)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   modBrush->ang.tx = modBrush->ang.ty = modBrush->ang.tz = 0;
   editUndoStoreBrush(modBrush);
   gedit_db_update(TRUE);
}

void fit_world(bool mode)
{
   mxs_vector *res = gedit_get_extent(mode);
   if (res)
      vm_fit_cameras_to_region(res,res+1);
   gedit_full_redraw();
}

void gedit_cam_to_brush(void)
{
   static int which_ang[]={2,2,2,2,1,1};
   static int what_to_add[]={0x8000,0xC000,0,0x4000,0xC000,0x4000};
   mxs_vector *pos;
   mxs_angvec *ang;
   editBrush *br=blistGet();

   if (br!=NULL)
      if (vm_spotlight_loc(&pos,&ang))
         if ((pos!=NULL)&&(ang!=NULL))
         {
            *pos=br->pos;
            *ang=br->ang;
            if (brushGetType(br)==brType_TERRAIN)
               if (br->cur_face!=-1)
                  ang->el[which_ang[br->cur_face]]+=what_to_add[br->cur_face];
            vm_redraw_from_camera();
         }
}

static bool gedit_allow_lazy_updates=FALSE;
static bool gedit_lazy_oldstate;
static BOOL gedit_drag_happening=FALSE;
static BOOL cur_sel_changed=FALSE;

BOOL gedit_is_drag_in_prog(void)
{
   return gedit_drag_happening;
}

void gedit_setup_check_lazy_drag(void)
{
   if (!gedit_allow_lazy_updates)
   {
      gedrend_show_current=FALSE;
      vm_redraw();
      vm_render_cameras();
      gedrend_show_current=TRUE;
      vm_redraw_from_select();
      vm_render_cameras();
   }
   gedit_lazy_oldstate=gedit_allow_lazy_updates;
   gedit_allow_lazy_updates=TRUE;
   gedit_drag_happening=TRUE;
}

void gedit_finish_check_lazy_drag(void)
{
   gedit_allow_lazy_updates=gedit_lazy_oldstate;
   if (!gedit_allow_lazy_updates)
      gedit_full_redraw();  // really should look at lazy update i guess....?
   gedit_drag_happening=FALSE;
}

// will not bother redrawing, unless user has turned on no_lazy_updates
static void gedit_lazy_update(editBrush *br)
{  // shouldnt be blistGet, should be multibrush compatible!!
   if ((br!=vBrush_editBrush_Get())||!gedit_allow_lazy_updates)
      gedit_full_redraw();      // anal mode, always redraw
   else
      vm_redraw_from_select();  // of course, lazy update may have already flagged it all to redraw...
}

// @TODO: rethink this when i can think
// raw change and refresh
void gedit_raw_change_brush(editBrush *us, BOOL is_new, BOOL change_world, BOOL change_db)
{
   if (brSelect_Flag(us) == brSelect_VIRTUAL)  // do we need this???
   {
      gedit_lazy_update(us);
   }
   else
   if (brushGetType(us) == brType_TERRAIN)
   {
      if (change_db)
         gedit_db_update(FALSE);
   }
#ifdef INCREMENTAL_FLOW_UPDATE
   else
   if (brushGetType(us) == brType_FLOW)
   {
      change_world = TRUE;
      GEdMedMoMarkWaterAllBrushes();
   }
#endif
   else
   if ((brushGetType(us) == brType_LIGHT)||
       ((brushGetType(us) == brType_OBJECT) && (brObjProp_getLightLevel(brObj_ID(us)) >= 0.0)))
   {
      if (!is_new)
      {
         if (change_db)
            gedit_light_update(FALSE);
      }
      else
         change_world=TRUE;
   }
   else
   if ((brushGetType(us) == brType_OBJECT) && IsDoor(brObj_ID(us)))
   {
      UpdateDoorBrush(brObj_ID(us));
      gedit_lazy_update(us);
   }
   else
   if (brushGetType(us) == brType_ROOM)
   {
      change_world = TRUE;
      if (change_db)
         gedit_room_update(FALSE);
   }
   else
      gedit_lazy_update(us);

   //   vm_redraw_from_select();  // of course, lazy update may have already flagged it all to redraw...
   gedit_lazy_update(us);
   if (change_world || (brushGetType(us) == brType_OBJECT))
      gedit_redraw_3d();    // this still isnt quite right - we still do this too much
   cur_sel_changed=TRUE;
}

// @VBRUSH: should this go in vbrush, perhaps?
// call me when you change a brush, really
void i_changed_the_brush(editBrush *us, BOOL snap, BOOL is_new, BOOL change_world)
{
   if (us==NULL)
      return;

	brush_field_update(us,NULL,brField_Old);
	if (snap)
    {
        if (config_is_defined("hens_changes") && brushGetType(us)==brType_OBJECT && is_new)
            gedit_snap_object(us);
        else
            gedit_snap_brush(us);
    }
	gedit_raw_change_brush(us,is_new,change_world,TRUE);
}

void gedit_change_selection(void)
{  // really needs to know if brush selection was changed
   if (cur_sel_changed&&gedit_allow_lazy_updates)
      vm_redraw();
   else
      vm_redraw_from_select();
   cur_sel_changed=FALSE;
}

void gedit_redraw_selection(void)
{
   vm_redraw_from_select();
}

// says - ok - data has changed - set me to changed, but dont do much else
void gedit_change_current_brush(void)
{
   if (gedit_allow_lazy_updates)
   {
      vm_redraw_from_select();
      cur_sel_changed=TRUE;
   }
   else
   {
      vm_redraw();
      cur_sel_changed=FALSE;
   }
}

/////////////////
// moving brushes around

static void stretch(int axis)
{
   editBrush *modBrush=cur();
   float amt = 0.125 * scale_factor;
   if (axis >= 3) { amt = -amt; axis -= 3; }
   if (axis >= 0 && axis < 3) {
      // don't allow a brush to be shrunk to nothing
      if (amt < 0 && modBrush->sz.el[axis] <= -amt)
         return;
      editUndoStoreStart(modBrush);
      modBrush->sz.el[axis] += amt;
      i_changed_the_brush(modBrush,FALSE,FALSE,FALSE);
      editUndoStoreBrush(modBrush);
   }
}

void translate(int axis)
{
   editBrush *modBrush = cur();
   float amt;
   if (config_is_defined("translate_by_grid"))
   {
      amt = cur_global_grid.line_spacing;
      //mprintf("spacing = %g\n",cur_global_grid.line_spacing);
   }
   else
      amt = 0.125 * scale_factor;
   if (axis >= 3) { amt = -amt; axis -= 3; }
   if (axis >= 0 && axis < 3) {
      editUndoStoreStart(modBrush);
      modBrush->pos.el[axis] += amt;
      i_changed_the_brush(modBrush,FALSE,FALSE,FALSE);
      editUndoStoreBrush(modBrush);
   }
}

static void rotate(int axis)
{
   editBrush *modBrush = cur();
   int amt = 0x100 * scale_factor;
	if (axis >= 3) { amt = -amt; axis -= 3; }
   editUndoStoreStart(modBrush);
   switch (axis) {
      case 0: modBrush->ang.tx += amt; break;
      case 1: modBrush->ang.ty += amt; break;
      case 2: modBrush->ang.tz += amt; break;
   }
   i_changed_the_brush(modBrush,FALSE,FALSE,FALSE);
   editUndoStoreBrush(modBrush);
}

// if current brush is an object, floor it correctly
void gedit_floor_brush(editBrush *us, mxs_vector *dir)
{
   Location cur_loc, down_loc, bbox_hit_loc, phys_hit_loc;
   mxs_vector minv,maxv;
   mxs_vector down;
   ObjPos *cur_pos;
   ObjID obj_id;
   mxs_vector new_vec;
   int i;

   BOOL bbox_hit = FALSE;
   BOOL phys_hit = FALSE;

   mxs_vector bbox_offset;
   mxs_vector phys_offset;

   if (brushGetType(us)!=brType_OBJECT) return;   // needs to be an object
   if (brSelect_Flag(us)&brSelect_VIRTUAL) return; // and to be real

   // do the real work
   mx_copy_vec(&down, dir);

   obj_id=brObj_ID(us);
   cur_pos=ObjPosGet(obj_id);
   if (cur_pos==NULL)  // hmmm... cant find us
   {
      Warning(("Tried to floor a non-existant object\n"));
      return;
   }
   cur_loc=cur_pos->loc;
   if ((cur_loc.vec.x!=us->pos.x)||(cur_loc.vec.y!=us->pos.y)||(cur_loc.vec.z!=us->pos.z))
      Warning(("Brush isnt where object is\n"));

   ObjGetObjOffsetBBox(obj_id,&minv,&maxv);
   mx_addeq_vec(&down,&cur_loc.vec);  // bottom of case
   MakeHintedLocationFromVector(&down_loc,&down,&cur_loc);

   if (!PortalRaycast(&cur_loc, &down_loc, &bbox_hit_loc, 0))
   {
      bbox_hit = TRUE;
    
      mx_sub_vec(&bbox_offset, &bbox_hit_loc.vec, &cur_loc.vec);

      for (i=0; i<3; i++)
      {
         if (dir->el[i] > 0)
            bbox_offset.el[i] -= maxv.el[i];
         else
         if (dir->el[i] < 0)
            bbox_offset.el[i] -= minv.el[i];
      }
   }

   if (!PhysObjMoveDir(brObj_ID(us), &cur_loc, &down_loc, &phys_hit_loc))
   {
      phys_hit = TRUE;

      mx_sub_vec(&phys_offset, &phys_hit_loc.vec, &cur_loc.vec);
   }

   if (bbox_hit && phys_hit)
   {
      if (mx_mag2_vec(&bbox_offset) < mx_mag2_vec(&phys_offset))
         mx_add_vec(&new_vec, &cur_loc.vec, &bbox_offset);
      else
         mx_add_vec(&new_vec, &cur_loc.vec, &phys_offset);
   }
   else
   if (bbox_hit)
   {
      mx_add_vec(&new_vec, &cur_loc.vec, &bbox_offset);
   } 
   else
   if (phys_hit)
   {
      mx_add_vec(&new_vec, &cur_loc.vec, &phys_offset);
   }
   else
   {
      Warning(("cound't find the ground!\n"));
      return;
   }

   us->pos = new_vec;
   i_changed_the_brush(us, FALSE, FALSE, TRUE);
   editUndoStoreBrush(us);
}

void gedit_floor_me(void)
{
   mxs_vector down;
   down.x = 0; down.y = 0; down.z = -20.0;
   gedit_floor_brush(cur(),&down);
}

void gedit_ceil_me(void)
{
   mxs_vector up;
   up.x = 0; up.y = 0; up.z = 20.0;
   gedit_floor_brush(cur(),&up);
}

void gedit_wall_me(void)
{
   mxs_vector walldir,unitvec;
   mxs_matrix rotmat;
   ObjID obj_id;
   ObjPos *cur_pos;

   obj_id=brObj_ID(cur());
   if (obj_id == OBJ_NULL)
      return;

   cur_pos=ObjPosGet(obj_id);

   // reverse our orientation vector
   mx_ang2mat(&rotmat, &cur_pos->fac);
   unitvec.x = -20.0F; unitvec.y = 0; unitvec.z = 0;
   mx_mat_mul_vec(&walldir, &rotmat, &unitvec);

   gedit_floor_brush(cur(),&walldir);
}

/////////////////////////

void gedit_info_window(void)
{
   char *info=(char *)Malloc(1024);
   editBrush *loop;
   int br_per_type[brType_ROOM-brType_TERRAIN+1];
   int act_hr=0, me_only=0;
   int hIter;

   // gather brush stats   
   memset(br_per_type,0,sizeof(br_per_type));
   loop=blistIterStart(&hIter);
   while (loop!=NULL)      // go through and make any other MeOnly's active
   {
      br_per_type[brushGetType(loop)-brType_TERRAIN]++;
      if (brushGetType(loop)==brType_HOTREGION)
         if (brHot_IsMEONLY(loop))   // this was a meonly
            me_only++;
         else if (brHot_Status(loop)==brHot_ACTIVE)   // this was a meonly
            act_hr++;
      loop=blistIterNext(hIter);
   }
   strcpy(info,"Brush Counts\n");
   sprintf(info+strlen(info),"%d Terrain ",br_per_type[brType_TERRAIN-brType_TERRAIN]);
   sprintf(info+strlen(info),"%d Object\n",br_per_type[brType_OBJECT-brType_TERRAIN]);
   sprintf(info+strlen(info),"%d Light ",br_per_type[brType_LIGHT-brType_TERRAIN]);
   sprintf(info+strlen(info),"%d Room\n",br_per_type[brType_ROOM-brType_TERRAIN]);
   sprintf(info+strlen(info),"%d Flow ",br_per_type[brType_FLOW-brType_TERRAIN]);
   sprintf(info+strlen(info),"%d Area -",br_per_type[brType_HOTREGION-brType_TERRAIN]);
   if (me_only)
      strcat(info,"MeOnly'd");
   else
      sprintf(info+strlen(info)," %d active\n",act_hr);
   _editsave_text_info(info);
   winui_Text(info);
   Free(info);
}

void gedit_brush_to_mono(int brush_id)
{
   editBrush *br;
   char buf[200];
   if (brush_id==0)
      br=cur();
   else
      br=brFind(brush_id);
   mprintf("Brush %d (0x%x)\n",br->br_id,br->br_id);
   sprintf(buf,"  Pos: %.9f %.9f %.9f\n",br->pos.el[0],br->pos.el[1],br->pos.el[2]);
   mprintf(buf);
   sprintf(buf,"   Sz: %.9f %.9f %.9f\n",br->sz.el[0],br->sz.el[1],br->sz.el[2]);
   mprintf(buf);
   sprintf(buf,"  Ang: %x %x %x\n",br->ang.el[0],br->ang.el[1],br->ang.el[2]);
   mprintf(buf);
}

/////////////////////////

editBrush *gedit_get_meonly_brush(void)
{
   editBrush *loop;
   int hIter;

   loop=blistIterStart(&hIter);
   while (loop!=NULL)      // go through and make any other MeOnly's active
   {
      if (brushGetType(loop)==brType_HOTREGION)
         if (brHot_IsMEONLY(loop))   // this was a meonly
            break;
      loop=blistIterNext(hIter);
   }
   if (loop!=NULL)
      blistIterDone(hIter);
   return loop;
}

static void gedit_go_to_meonly(void)
{
   editBrush *br=gedit_get_meonly_brush();
   if (br)
      vBrush_SelectBrush(br);
   else
      Status("No MeOnly brush");
}

static void gedit_hots_state(int new_state)
{
   editBrush *us=cur();

   if (brushGetType(us)!=brType_HOTREGION)
      return;   // we arent a hot region, so go home
   // parse the toggles first
   if (new_state==brHot_TOGMEONLY)
   {
      if (brHot_IsMEONLY(us))              // we are the meonly, so we just need
         new_state=brHot_GetBase(us);      // to revert to our base type
      else                                 // else we need to go check if there is
      {                                    // a meonly and clear it, then set us
         editBrush *old_meonly=gedit_get_meonly_brush();
         new_state=brHot_GetBase(us)|brHot_MEONLY;
         if (old_meonly)
            brHot_Status(old_meonly)&=brHot_BASEMASK;  // so filter the meonly out
      }
   }
   else if (new_state==brHot_TOGACTIVE)
   {
      new_state=(brHot_GetBase(us)==brHot_ACTIVE)?brHot_INACTIVE:brHot_ACTIVE;
      if (brHot_IsMEONLY(us)) new_state|=brHot_MEONLY;
   }
   brHot_Status(us)=new_state;
   gedit_full_redraw();
}

///////////////////////////
// busywait/full screen mode

// lifted from dragbox, perhaps they should be in some header?
#define ismouse(ev)     ((ev).type & (UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE))
#define iskbd(ev)       ((ev).type == UI_EVENT_KBD_COOKED)

extern void redraw_all_cmd(void);

// do a busywait in modal ui for next event we care about
void gedit_busywait(BOOL clear)
{
   uiEvent ev;

   modal_ui_start(MODAL_CHECK_NONE);
   while (1)
   {
      if (modal_ui_get_event(&ev))
         if (iskbd(ev))
         {
            uiCookedKeyEvent *kev = (uiCookedKeyEvent *) &ev;
            if (kev->code == (27 | KB_FLAG_DOWN))
               break;
         }
      modal_ui_update();
   }
   modal_ui_end();
   if (clear) gr_clear(0);
   redraw_all_cmd();
}

// show an image file
static void gedit_show_image(char *str)
{
   IResMan *pResMan = AppGetObj(IResMan);
   IRes *pRes = IResMan_Bind(pResMan, str, RESTYPE_IMAGE, NULL, NULL, 0);

   if (pRes)
   {  // really should check screen size and mode here, maybe switch in and out
      grs_bitmap *bm=(grs_bitmap *) IRes_Lock(pRes);
      gr_clear(0);
      gr_bitmap(bm, 0, 0);
      gedit_busywait(TRUE);
      IRes_Unlock(pRes);
      SafeRelease(pRes);
   }
   else
      Warning(("Cant find %s\n",str));

   SafeRelease(pResMan);
}

static void gedit_get_pixel_color(char *str)
{
   uiEvent ev;

   if (str && *str!='\0')
   {
      int x=1,y=1, c;
      sscanf(str,"%d %d",&x,&y);
      c=gr_get_pixel(x,y);
      mprintf("Pixel at %d,%d is color %d (0x%x) (%d %d %d)\n",x,y,c,c,grd_pal[c*3],grd_pal[c*3+1],grd_pal[c*3+2]);
      return;
   }
   // unhide mouse here?? - if we support game mode, or something?
   uiShowMouse(NULL);
   modal_ui_start(MODAL_CHECK_NONE);
   while (1)
   {
      if (modal_ui_get_event(&ev))
         if (iskbd(ev))
         {
            uiCookedKeyEvent *kev = (uiCookedKeyEvent *) &ev;
            if (kev->code == (27 | KB_FLAG_DOWN))
               break;
         }
         else if (ismouse(ev))
         {
            uiMouseEvent *mev=(uiMouseEvent *)&ev;
            if (mev->action&(MOUSE_LUP|MOUSE_RUP))
            {
               int x=mev->pos.x, y=mev->pos.y, c;
               c=gr_get_pixel(x,y);
               mprintf("Pixel at %d,%d is color %d (0x%x) (%d %d %d)\n",x,y,c,c,grd_pal[c*3],grd_pal[c*3+1],grd_pal[c*3+2]);
               if (mev->action&MOUSE_LUP)  // left button breaks out
                  break;                   // right button keeps going
            }
         }
      modal_ui_update();
   }
   modal_ui_end();
   uiHideMouse(NULL);
}

// preload a sound
static void gedit_preload_snd(char *str)
{
   preload_data(PRELOAD_SOUND,str,NULL);
}

// preload a sound
static void gedit_preload_mot(char *str)
{
   preload_data(PRELOAD_BIN,str,"motions/");
}

extern void preload_dump_stats(void);

#ifdef DBG_ON

static void showPalette(void)
{
   static bool display=TRUE;
   static LGadRoot *root;
   static Rect *box;
   uchar const *use_ipal=ScrnGetInvPalette();
   int x, y;

   if (display)
   {
      root = vmGrabViews(vmGrabSingle);
      box = LGadBoxRect(root);

      // do the palette first
      for (y=0; y<16; y++)
         for (x=0; x<16; x++)
         {
            gr_set_fcolor(x+y*16);
            gr_rect(x*8+box->ul.x, y*8+box->ul.y, x*8+8+box->ul.x, y*8+8+box->ul.y);
         }

      // next, show the ipal
      for (y=0; y<16; y++)
         for (x=0; x<16; x++)
         {  // get an 888 rgb value
            long rgb = grd_pal[(x+y*16)*3];
            rgb |= grd_pal[(x+y*16)*3+1] << 8;
            rgb |= grd_pal[(x+y*16)*3+2] << 16;
            gr_set_fcolor(use_ipal[gr_index_lrgb(rgb)]);
            gr_rect(x*8+box->ul.x + 150, y*8+box->ul.y, x*8+8+box->ul.x + 150, y*8+8+box->ul.y);
         }

#ifdef SHOW_PCX
      {
         // *If* firetile is already loaded, then fetch it and show it.
         grs_bitmap *bm;
         IResMan *pResMan = AppGetObj(pResMan);
         IRes *pRes = IResMan_Lookup(pResMan, 
                                     "firetile.pcx", 
                                     RESTYPE_IMAGE,
                                     NULL);
         if (pRes)
         {
            bm = (grs_bitmap *) IRes_DataPeek(pRes);
            if (bm)
            {
               gr_bitmap(bm, x*8+box->ul.x, box->ul.y);
            }
         }
      }
#endif
   }
   else
      vmReleaseViews(root);

   display = !display;
}

#endif

void gedit_run_script_hack(char *script_name)
{
   char buf[256];
   find_file_in_config_path(buf,script_name,"script_path");
   CommandRunScript(buf);
}

void set_ambient(char *arg)
{
   sMissionRenderParams parms;
   parms = *GetMissionRenderParams();

   if (arg==NULL || strlen(arg)==0) {
      char s[256];
      sprintf(s, "ambient: %.3f %.3f %.3f", parms.ambient_light.x * 255.0,
              parms.ambient_light.y * 255.0, parms.ambient_light.z * 255.0);
      mprintf("%s\n", s);
   } else {
      mxs_vector rgb;
      mx_mk_vec(&rgb, -11, -11, -11);// try to tell if we've got too few numbers
      sscanf(arg, "%f %f %f", &rgb.x, &rgb.y, &rgb.z);
      if (rgb.x != -11 && rgb.y != -11 && rgb.z != -11) {
         mx_scale_vec(&parms.ambient_light, &rgb, 1.0 / 255.0);
         SetMissionRenderParams(&parms);
      } else
         Warning(("Pssst!  Try typing it like this: ambient r g b\n"));
   }
}

extern BOOL loud_flag_clear;

void gedit_saveload_flag_hack(void)
{
   BOOL old_flag=loud_flag_clear;
   loud_flag_clear=TRUE;
   saveloadFixFlagField();
   loud_flag_clear=old_flag;
}

// split every model into every cell - not sure if still supported
#ifndef SHIP
extern BOOL always_split_all_models;
#endif

// are hotregions actively put in the portalizations, or just a filter
extern bool passive_hotregions;

#ifdef SEAN_STUFF
extern bool debug_cell_traversal;
#endif

Command gedit_edit_only_keys[] =
{
   { "refresh_particle_links", FUNC_VOID, ParticleGroupRefreshLinks, "Update all ParticleAttachement links" },

   { "cycle_tex", FUNC_INT, cycle_tex },
   { "sky_tex", FUNC_VOID, sky_tex },
   { "align_tex", FUNC_VOID, hack_align_texture },
   { "cycle_media", FUNC_INT, cycle_media },
   { "set_debug_tex", FUNC_INT, objmodel_set_debug_tex },

   { "reset_brush", FUNC_VOID, reset },
   { "brush_stretch", FUNC_INT, stretch, "stretch current brush" },
   { "brush_rotate", FUNC_INT, rotate, "rotate current brush" },
   { "brush_translate", FUNC_INT, translate, "translate current brush" },
   { "floor_object", FUNC_VOID, gedit_floor_me },
   { "wall_object", FUNC_VOID, gedit_wall_me },
   { "ceil_object", FUNC_VOID, gedit_ceil_me },
   { "fit_cameras", FUNC_BOOL, fit_world, "fit cameras to world/hot region" },
   { "auto_portalize", TOGGLE_BOOL, &auto_portalize },
   { "auto_roombuild", TOGGLE_BOOL, &auto_roombuild },

   { "go_to_meonly", FUNC_VOID, gedit_go_to_meonly },

   { "brush_to_room", FUNC_FLOAT, gedit_brush_to_room, "terrain->room (grow by arg on all axis)" },
   { "brush_adopt", FUNC_INT, gedit_brush_adopt, "all brushes adopt from us (arg for which parameter)" },

   { "spiral_serf", FUNC_INT, spiral_stair_cmd, "build spiral stair, 0 for dialog" },
   { "stair_serf", FUNC_INT, straight_stair_cmd, "build straight stair, 0 for dialog" },

   { "ambient", FUNC_STRING, set_ambient, "give this three values 0-255: r g b" },
   { "snap_placement", TOGGLE_BOOL, &gedit_snap_placement },
   { "clear_brushes", FUNC_VOID, brushClearAll, "clear all brushes" },
   { "set_medium", FUNC_INT, set_medium, "set brush medium" },

   { "blist_dump", FUNC_VOID, blistDump },

   { "get_grid", FUNC_VOID, grid_get_from_current, "get grid from brush" },
   { "set_grid", FUNC_VOID, grid_set_from_current, "force brush to grid" },
   { "grid_toggle", FUNC_VOID, grid_toggle, "turn grid on off" },
   { "grid_scale", FUNC_FLOAT, grid_scale, "rescale the master grid" },
   { "grid_abs_scale", FUNC_FLOAT, grid_abs_scale, "set absolute scale of master grid" },
   { "brush_go_last", FUNC_VOID, move_brush_to_end_of_time, "move current brush to temporal last" },
   { "brush_set_time", FUNC_INT, move_brush_to_time_n, "move current brush to time n" },
   { "load_object", FUNC_STRING, objmodel_command, "load 3d object" },

   { "preload_sound", FUNC_STRING, gedit_preload_snd },
   { "preload_motion", FUNC_STRING, gedit_preload_mot },
   { "preload_stats", FUNC_VOID, preload_dump_stats },

#ifdef DBG_ON
   { "show_pal", FUNC_VOID, showPalette },
   { "primal_mprint", FUNC_INT, primalBr_mprint },
#endif

   { "brush_color", VAR_INT, &editbr_color_mode },
   { "brush_filter", VAR_INT, &editbr_filter },
   { "hots_filter", TOGGLE_BOOL, &renderHotsOnly },
   { "hots_state", FUNC_INT, gedit_hots_state },
   { "time_filter_lo", VAR_INT, &editbr_filter_time_lo },
   { "time_filter_hi", VAR_INT, &editbr_filter_time_hi },
   { "size_filter", VAR_FLOAT, &editbr_filter_size },

   { "set_brush_tx", FUNC_INT, gedit_set_brush_tx },
   { "set_brush_type", FUNC_INT, gedit_set_default_brush_type },
   { "reset_brush_tx", FUNC_INT, gedit_reset_br_textures },
   { "cam_to_brush", FUNC_VOID, gedit_cam_to_brush, "move camera to look at cur brush" },
   { "obj_ambient", VAR_FLOAT, &mdd_lt_amb },
   { "obj_diffuse", VAR_FLOAT, &mdd_lt_diff },

   { "zap_brush_flags", FUNC_VOID, gedit_saveload_flag_hack, "secret clear flags stuff" },
   { "loud_zap_flags", VAR_INT, &loud_flag_clear, "talk about flags you clear" },

#ifdef SEAN_STUFF
   { "debug_cell", TOGGLE_BOOL, &debug_cell_traversal },
#endif

   { "info_window", FUNC_VOID, gedit_info_window },

   { "lazy_update", TOGGLE_BOOL, &gedit_allow_lazy_updates, "do we update instantly, or only on new brush selection" },
   { "run_cmd_script", FUNC_STRING, gedit_run_script_hack },

   { "set_primal", VAR_INT, &editbr_default_primal, "set default primal" },
   { "prim_sides", FUNC_INT, gedit_set_primal_sides, "# of sides on primals" },
   { "prim_type", FUNC_INT, gedit_set_primal_type, "1=cyl, 2=pyr, 3=cpyr, 0=cube " },
   { "prim_facealign", FUNC_BOOL, gedit_set_primal_facevertex, "are we face aligned" },
   { "cube", FUNC_VOID, gedit_set_cube, "set cube" },
   { "prim_special", FUNC_INT, gedit_set_special, "set 'special' primal type" },
   { "brush_to_mono", FUNC_INT, gedit_brush_to_mono, "show brush info on mono" },

#ifndef SHIP
   { "obj_split", TOGGLE_BOOL, &always_split_all_models, "always portal split objects" },
#endif
   { "obj_re_place", FUNC_VOID, gedit_replace_all_objs, "replace all objects in world" },
   { "passive_hotregions", TOGGLE_BOOL, &passive_hotregions, "are hotregions in the CSG, or just filters" },

   // these want to make it down to the list below - but arent ready for it yet - as does obj_pal
   { "show_image", FUNC_STRING, gedit_show_image, "load image from disk" },
};

// for keys you want to work in game mode as well, but ONLY In editor builds
// really, this should probably go somewhere else at some point
Command gedit_all_mode_keys[] =
{
#ifndef SHIP
   { "skybox", FUNC_STRING, gedit_make_skybox, "make skyhack texures"},
   { "play_sfx", FUNC_STRING, SFX_command },
#endif
   { "get_pixel_color", FUNC_STRING, gedit_get_pixel_color, "click on pixel, get told color" },
   { "heapchk", FUNC_VOID, &_heapchk, "Test the heap, if debugging heap enabled" },
   { "monodebug", TOGGLE_BOOL, &mono_to_debugger, "output mono to Windows debug stream" },
#ifdef RN_STATS
   { "rn_stat_clear", FUNC_VOID, rnStatClear },
   { "rn_stat_show", FUNC_VOID, rnStatShow },
#endif
};


// register all of the subsystems here as well
void gedit_register(void)
{
   COMMANDS(gedit_edit_only_keys, HK_BRUSH_EDIT);
   COMMANDS(gedit_all_mode_keys, HK_ALL);
   gedcsgCommandRegister();
   gedundoCommandRegister();
   hilightCommandRegister();
}

//	Returns whether the given brush has been locked
//
BOOL brushIsLocked (editBrush* brush)
{
	BOOL isLocked = FALSE;
    BOOL unlock = FALSE;
	ITraitManager* traitMan;
    int i;

    if (!config_is_defined ("hens_changes"))
        return FALSE;

	traitMan = AppGetObj (ITraitManager);
    for (i = 0; i < MAX_LOCKED_BRUSHES; i++)
    {
        if (gLockedBrushes[i] != 0)
        {
            if (gLockedBrushes[i] == TERRAIN_ID)
            {
                if (brushGetType(brush) == brType_TERRAIN)
                {
                    isLocked = TRUE;
                    break;
                }
            }
            else if (ITraitManager_ObjHasDonor(traitMan, brObj_ID(brush), gLockedBrushes[i]))
            {
                isLocked = TRUE;
                break;
            }
        }
    }
	SafeRelease (traitMan);
	
    return isLocked;
}

void UpdateMenuCheckmarks (void)
{
    int menu, firstPos, lastPos;
    int type;

    //  Brush shapes
    //
    menu = 4;
    firstPos = 0;
    lastPos = 5;
    
    type = primalID_GetType(editbr_default_primal);
    switch (type)
    {
        case PRIMAL_TYPE_CYLINDER:
        case PRIMAL_TYPE_PYRAMID:
        case PRIMAL_TYPE_CORNERPYR:
            SetRadioCheckmark (menu, firstPos, lastPos, type);
            break;

        case PRIMAL_TYPE_SPECIAL:
        {
            if (editbr_default_primal == PRIMAL_WEDGE_IDX)
                SetRadioCheckmark (menu, firstPos, lastPos, 4);
            else if (editbr_default_primal == PRIMAL_DODEC_IDX)
                SetRadioCheckmark (menu, firstPos, lastPos, 5);
            else
                SetRadioCheckmark (menu, firstPos, lastPos, 0);     //  Cube
            break;
        }
    }

    //  Lighting mode
    //
    menu = 5;
    firstPos = 9;
    lastPos = 11;
    
    if (gedcsg_light_objcast)
        SetRadioCheckmark (menu, firstPos, lastPos, 11);
    else if (gedcsg_light_raycast)
        SetRadioCheckmark (menu, firstPos, lastPos, 10);
    else
        SetRadioCheckmark (menu, firstPos, lastPos, 9);
}

