/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editgeom.c,v 1.130 2000/02/19 13:10:33 toml Exp $

// test code for geometry editor
// parses camera control keys, calls outer 3d stuff
// calls to gedit for most real stuff, i think

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <lg.h>
#include <mprintf.h>
#include <res.h>
#include <2dres.h>
#include <r3d.h>
#include <event.h>
#include <kb.h>
#include <kbcook.h>
#include <config.h>
#include <g2pt.h>

#include <portal.h>
#include <pt.h>

#include <command.h>
#include <status.h>

#include <editbr.h>
#include <brlist.h>
#include <gedit.h>
#include <media.h>

#include <viewmgr.h>

#include <loopapi.h>
#include <dispatch.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <vismsg.h>
#include <loopmsg.h>

#include <scrnman.h>
#include <scrnloop.h>
#include <editgeom.h>
#include <gamecam.h>
#include <palette.h>
#include <brloop.h>
#include <undoredo.h>

#include <editgeo_.h>
#include <uiapp.h>
#include <objloop.h>
#include <mipmap.h>
#include <family.h>
#include <appsfx.h>
#include <plyrloop.h>
#include <prof.h>
#include <editbr_.h> // for creating initial level
#include <ged_room.h>

#include <lresname.h>
#include <texmem.h>

#include <wrloop.h>

#include <dbfile.h>
#include <dbasemsg.h>
#include <render.h>

    // hacked data file
#include <texture.h>

extern BOOL g_null_raster;

mxs_vector player_pos;
static mxs_angvec player_ang;

int scale_factor=4;

bool single_step=TRUE;

bool scene_setup(void)
{
   editBrush* tmp;

   player_pos.el[0]=0; player_pos.el[1]=0; player_pos.el[2]=0;
   player_ang.tx=player_ang.ty=player_ang.tz=0;

   tmp=brushInstantiate(DEF_PRIMAL);
   tmp->sz.el[0]*=4;
   tmp->sz.el[1]*=4;
   tmp->sz.el[2]*=4;
   tmp->tx_id=0;
   blistInsert(tmp);
   return TRUE;
}

mxs_vector last_left, last_up, last_forward;

void print_info(int vol)
{
   char *p;
   while ((p = portal_scene_info(vol)) != 0)
      mprintf("%s\n", p);
}

bool spotlight=FALSE;

#define SF   (scale_factor * 0.125)

static float editor_3d_speed_fac = 1.0;

void slew(int x)
{
   mxs_vector temp;

   temp.x = 0;
   temp.y = 0;
   temp.z = 0;

   ((mxs_real *) &temp) [ x % 3 ] = 3.0 * 0.0625 * editor_3d_speed_fac * scale_factor * (x >= 3 ? -1 : 1);
   vm_cur_slew(&temp);
}

void rot(int x)
{
   mxs_angvec temp;

   temp.tx = 0;
   temp.ty = 0;
   temp.tz = 0;

   ((mxs_ang *) &temp) [ x % 3 ] = 3.0 * 0x0100 * editor_3d_speed_fac * scale_factor * (x >= 3 ? -1 : 1);
   vm_cur_rotate(&temp);
}

void level_cam(void)
{
   vm_cur_level (1);
}

void unroll_cam(void)
{
   vm_cur_level (0);
}

void rescale(bool up)
{
   if (up) {
      if (scale_factor<1024) { scale_factor*=2; vm_scale_all(2.0); }
   } else {
      if (scale_factor>1)    { scale_factor/=2; vm_scale_all(0.5); }
   }
}


void decode_scroll(int x)
{
   switch(x) {
      case 7: vm_cur_scroll(-SF, -SF); break;
      case 8: vm_cur_scroll(  0, -SF); break;
      case 9: vm_cur_scroll( SF, -SF); break;
      case 4: vm_cur_scroll(-SF,   0); break;
      case 5: vm_cur_scroll(  0,   0); break;
      case 6: vm_cur_scroll( SF,   0); break;
      case 1: vm_cur_scroll(-SF,  SF); break;
      case 2: vm_cur_scroll(  0,  SF); break;
      case 3: vm_cur_scroll( SF,  SF); break;
   }
};



#define MAX_FAILURES  8192
Location raycast_dest[MAX_FAILURES], raycast_hit[MAX_FAILURES];
Location fail_source;
int failure_count, min_show = 0, max_show = MAX_FAILURES;
extern bool show_raycasts, show_lightmap, record_movement;

void save_failures(Location *hit, Location *dest)
{
   if (failure_count < MAX_FAILURES) {
      raycast_dest[failure_count] = *dest;
      raycast_hit[failure_count++] = *hit;
   }
}

void render_failures(void)
{
   int i;
   r3s_point source, dest, hit;

   r3_transform_point(&source, &fail_source.vec);

   for (i=0; i < failure_count; ++i) {
      if (i >= min_show && i <= max_show) {
         r3_transform_point(&hit, &raycast_hit[i].vec);
         r3_transform_point(&dest, &raycast_dest[i].vec);
         r3_set_color(1);
         if (!record_movement) {
            r3_draw_line(&source, &hit);
            r3_set_color(4);
         }
         r3_draw_line(&hit, &dest);
      }
   }
}


#if 0
extern void (*failed_light_callback)(Location *hit, Location *dest);

extern void portal_add_omni_light_perfect(float x, float y, float z, float br, LightPermanence dynamic);

void raycast_light(void)
{
   mxs_vector *loc;
   mxs_angvec *ang;

   failed_light_callback = save_failures;
   failure_count = 0;

   if (vm_spotlight_loc(&loc, &ang)) {
      MakeLocationFromVector(&fail_source, loc);
      portal_add_omni_light_perfect(loc->x, loc->y, loc->z, 64, LIGHT_DYNAMIC);
   }

   failed_light_callback = 0;
}
#endif

// stolen from Jaemz
void RenDebugShadTab(uchar *shadtab,int numentries)
{
   int i;
   int col,row;
   int x,y;
   uchar *tab;
   int rowsize;
   int w,h;

   w=grd_canvas->bm.w/64;
   h=grd_canvas->bm.h/(4*numentries);

   rowsize=numentries;
   tab=shadtab;

   for (i=0;i<256*rowsize;++i) {
      col = i%256;
      row = i/256;

      x = (col%64)*w;
      y = (col/64)*(h*rowsize+2) + row*h;

      if (tab) {
         gr_set_fcolor(tab[i]);
      } else {
         gr_set_fcolor(i);
      }
      gr_rect(x,y,x+w-1,y+h-1);
   }
}

extern void gedit_busywait(BOOL clear);
static void show_shad_tab(void)
{
   gr_clear(0);
   RenDebugShadTab(gr_get_light_tab(),16);
   gedit_busywait(TRUE);
}

extern float portal_detail_level;
extern int max_draw_polys;

void PickDistanceFunction(int which);  // Declared right below

static Command parse_keys[]=
{
     // local commands
   { "cam_rotate", FUNC_INT, rot, "rotate current camera" },
   { "cam_slew", FUNC_INT, slew, "slew current camera" },
   { "speed_fac", VAR_FLOAT, &editor_3d_speed_fac, "set speed factor of 3d move in editor" },
   { "cam_level", FUNC_VOID, level_cam, "level current camera" },
   { "cam_unroll", FUNC_VOID, unroll_cam, "unroll current camera" },
   { "num_scroll", FUNC_INT, decode_scroll, "scroll via numeric keypad" },
   { "global_scale", FUNC_BOOL, rescale, "zoom in/out & rescale" },

   { "show_raycasts", TOGGLE_BOOL, &show_raycasts, "display light raycasts" },
   { "min_show", VAR_INT, &min_show, "min id# raycast to show" },
   { "max_show", VAR_INT, &max_show, "max id# raycast to show" },
   { "record_movement", TOGGLE_BOOL, &record_movement, "show rays of samples moving" },

     // imported commands
// The following commands should all be registered
// by the systems that provide them, not here, but
// for now we'll just get it working...

   { "cam_spotlight", TOGGLE_BOOL, &spotlight, "mounted spotlight toggle" },
};

// more imported commands that should be in both editor
// and game mode.  obviously this belongs elsewhere.

extern bool surface_cache, skip_clip;
extern bool surf_256;  // false all surfaces to row == 256
extern bool show_span_lengths, show_render_times;
extern bool portal_clip_poly, portal_clip_fast, poly_clip_fast;
extern palette_light, project_space, cache_feedback;
extern float light_scale;
extern float dynamic_light_min;
extern int player_light;
extern int portal_info_volume;
extern bool keep_all_lit;
extern float max_dist_2;
extern float dot_clamp;
extern bool show_cells;
extern bool linear_map;
extern bool portal_model;
extern bool portal_object_complete_test;
extern bool portal_write_traversal;
extern bool show_split;
extern BOOL show_bbox, show_bbox_2d;
extern bool disable_topsort;

#ifndef SHIP
extern BOOL always_slow_split;
#endif

#ifdef LOUD_D3D
#define d3d_talk(x) mprintf x
#else
#define d3d_talk(x)
#endif

#include <lgd3d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static double z_far = 200.0;
static double z_near = 1.0;

extern void SetZNearFar(double zn, double zf);

extern BOOL g_null_raster;
extern BOOL lgd3d_z_normal;
extern BOOL g_lgd3d;
extern BOOL g_zbuffer; 

static void toggle_null_raster(void)
{
   g_null_raster = !g_null_raster;
   if (g_null_raster)
      r3_use_null();
   else
      if (g_lgd3d)
         r3_use_lgd3d();
      else
         r3_use_g2();
}

extern BOOL pt_aggregate_lightmaps;

static BOOL linear=FALSE;
extern void lgd3d_set_zlinear(BOOL lin);
static void toggle_linear(void)
{
   linear=!linear;
   lgd3d_set_zlinear(linear);
}

static void znup(void)
{
   z_near *= 2.0;
   SetZNearFar(z_near, z_far);
   d3d_talk(("z_near: %g\n", z_near));
}

static void zndown(void)
{
   z_near *= 0.5;
   SetZNearFar(z_near, z_far);
   d3d_talk(("z_near: %g\n", z_near));
}

static void zfup(void)
{
   z_far *= 2.0;
   SetZNearFar(z_near, z_far);
   d3d_talk(("z_far: %g\n", z_far));
}

static void zfdown(void)
{
   z_far *= 0.5;
   SetZNearFar(z_near, z_far);
   d3d_talk(("z_far: %g\n", z_far));
}

extern BOOL lgd3d_punt_d3d;
extern BOOL rendloop_clear;
extern int portal_hack_blend;
extern BOOL portal_test_blend;

#ifndef SHIP
EXTERN void ParticleGroupCountSim(void);
extern int show_particle_counts;
#endif

static Command all_keys[]=
{
   { "blend", VAR_INT, &portal_hack_blend },
   { "test_blend", TOGGLE_BOOL, &portal_test_blend },
   { "traverse_log", TOGGLE_BOOL, &portal_write_traversal,
            "log cell traversal to traverse.log\n" },
   { "clear", TOGGLE_BOOL, &rendloop_clear, "clear framebuffer to pink" },
   { "max_polys", VAR_INT, &max_draw_polys, "maximum polygons to draw" },
   { "show_lightmap", TOGGLE_BOOL, &show_lightmap, "display light sampling" },
   { "render_info", FUNC_INT, print_info, "rendering stats at some volume" },
   { "detail_level", VAR_FLOAT, &portal_detail_level,
           "mipmap detail level (0-1)" },
   { "dot_clamp", VAR_FLOAT, &dot_clamp, "mipmap orientation limit" },
   { "surf_256", TOGGLE_BOOL, &surf_256, "surfaces are all row==256" },
   { "show_span_lengths", TOGGLE_BOOL, &show_span_lengths, 0 },
   { "show_render_times", TOGGLE_BOOL, &show_render_times, "detailed rendering timing info" },
   { "info_volume", VAR_INT, &portal_info_volume, "determine amount of rendering information" },

 // @TODO: optimize
 // when we have shippable levels, test which combination of these three
 // flags is fastest
   { "portal_clip_poly", TOGGLE_BOOL, &portal_clip_poly, 0 },
   { "portal_clip_fast", TOGGLE_BOOL, &portal_clip_fast, 0 },
   { "poly_clip_fast", TOGGLE_BOOL, &poly_clip_fast, 0 },

#ifndef SHIP
   { "always_slow_split", TOGGLE_BOOL, &always_slow_split, "force split objects to do things the hard way" },
   { "show_particle_counts", TOGGLE_INT, &show_particle_counts, "show count of particle sim/render" },
   { "show_particle_sim", FUNC_VOID, ParticleGroupCountSim, "list particle objects set to always_simulate" },
#endif

   { "cache_feedback", TOGGLE_BOOL, &cache_feedback, "get extra surface cache info" },
   { "light_scale", VAR_FLOAT, &light_scale, 0 },
   { "player_light", VAR_INT, &player_light, "attach a light to the player" },
   { "min_light", VAR_FLOAT, &dynamic_light_min, "minimum dynamic light on a surface allowed visible" },
   { "max_dist_2", VAR_FLOAT, &max_dist_2, "maximum distance dynamic light can reach" },

   { "keep_all_lit", TOGGLE_BOOL, &keep_all_lit, 0 },

   { "show_cells", TOGGLE_BOOL, &show_cells, "Display cells containing refs" },
   { "span_clip", TOGGLE_BOOL, &g2pt_span_clip, "enable span clipping in the renderer" },
   { "portal_model", TOGGLE_BOOL, &portal_model, "render models through portal tmappers" },
   { "full_obj_test", TOGGLE_BOOL, &portal_object_complete_test, "choose all/incomplete cell lists for object sort testing" },

   { "show_split", TOGGLE_BOOL, &show_split, "outline objects being split" },
   { "show_bbox", TOGGLE_BOOL, &show_bbox, "show bounding boxes around objects" },
   { "show_bbox_2d", TOGGLE_BOOL, &show_bbox_2d, "outline objects" },
   { "disable_topsort", TOGGLE_BOOL, &disable_topsort, "disable good objsort" },

   // Shading Table Fun
   { "show_shadtab", FUNC_VOID, show_shad_tab, "draw shading table" },
   { "pick_shade_dist", FUNC_INT, PickDistanceFunction, "pick which rule for distance" },

   // lgd3d hacks
   { "puntd3d", TOGGLE_BOOL, &lgd3d_punt_d3d, 0},
   { "zbuffer", TOGGLE_BOOL, &g_zbuffer, 0 },
   { "znup", FUNC_VOID, znup, 0 },
   { "zndown", FUNC_VOID, zndown, 0 },
   { "zfup", FUNC_VOID, zfup, 0 },
   { "zfdown", FUNC_VOID, zfdown, 0 },
   { "zlinear", FUNC_VOID, toggle_linear, 0 },
   { "agglight", TOGGLE_BOOL, &pt_aggregate_lightmaps, 0 },
   { "nullrast", FUNC_VOID, toggle_null_raster, 0},

 // the following flags should be desupported
//   { "asm_outer",     TOGGLE_BOOL, &asm_outer, 0 },
   { "palette_light", TOGGLE_BOOL, &palette_light, 0 },
   { "skip_clip",     TOGGLE_BOOL, &skip_clip, 0 },
   { "project_space", TOGGLE_BOOL, &project_space, 0 },
   { "linear_map",    TOGGLE_BOOL, &linear_map, "affine mapper for terrain" },
   { "surface_cache", TOGGLE_BOOL, &surface_cache, "cache lit surfaces" },

};

//////////////////////////////////////////////////


extern void brushPickColors(void);

////////////////////////////////////////////////////////////
// DATABASE EVENT HANDLERS
//



void editgeom_reset_database(void)
{




}

void editgeom_pal_change(void)
{
   brushPickColors();
   vm_pick_colors(); 
}

void editgeom_visual_message(DispatchData* msg)
{
   switch(msg->subtype)
   {
      case kPaletteChange:
         editgeom_pal_change(); 
         break;
   }
}

void editgeom_db_message(DispatchData* msg)
{
   ///   msgDatabaseData* data = (msgDatabaseData*)msg->data;
   switch(DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         editgeom_reset_database();
         break;
      case kDatabaseLoad:
         //gedit_update();
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kDBMap)
         {
            if (blistCount() == 0)
               scene_setup();
            undoClearUndoStack();
         }

         if (msg->subtype & kObjPartConcrete)
            ged_room_postload(); 
         break;
      case kDatabaseDefault:
         scene_setup();
         undoClearUndoStack();
         ged_room_postload(); 
         break;
   }
}

////////////////////////////////////////////////////////////
// STARTUP/SHUTDOWN
//

static int texturefilenum;

void editgeom_init(void)
{
   texturefilenum = ResOpenFile("texture.res");
   gedit_init();
   COMMANDS(parse_keys, HK_EDITOR);
   COMMANDS(all_keys, HK_ALL);
}

void editgeom_term(void)
{
   ResCloseFile(texturefilenum);
   gedit_term();
}

////////////////////////////////////////////////////////////
// LOOPMODE HANDLERS
//

void testgeom_startup(void)
{
   gedit_full_redraw();
}

void testgeom_shutdown(void)
{
}

bool testgeom_inner(void)
{  PROF
   mxs_vector *loc;
   mxs_angvec *ang;
   bool have_spot;

   have_spot=vm_spotlight_loc(&loc, &ang);

   reset_dynamic_lights();
   if (spotlight&&have_spot)
   {
      Position pos;
      MakePositionFromVectors(&pos, loc, ang);
      gr_push_canvas(grd_canvas);
      portal_add_spotlight(100.0, &pos, 1.0, LIGHT_DYNAMIC);
      gr_pop_canvas();
   }

   if (!have_spot)
    { loc=NULL; ang=NULL; }
   SFX_Frame(loc,ang); // remove me too, please

   vm_render_cameras();
   END_PROF;
   return TRUE;
}


////////////////////////////////////////////////////////////
// LOOP CLIENT
//

typedef void Context;

typedef struct _StateRecord
{
   Context* context;
} StateRecord;



////////////////////////////////////////
//
// LOOP/DISPATCH callback
//

static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
         testgeom_startup();
         editgeom_pal_change(); 
         break;

      case kMsgExitMode:
      case kMsgSuspendMode:
         testgeom_shutdown();
         break;

      case kMsgNormalFrame:
      case kMsgPauseFrame:
         testgeom_inner();
         break;

      case kMsgDatabase:
         editgeom_db_message(info.dispatch);
         break;

      case kMsgVisual:
         editgeom_visual_message(info.dispatch);
         break;

      case kMsgAppInit:
         editgeom_init();
         break;

      case kMsgAppTerm:
         editgeom_term();
         break;

      case kMsgEnd:
         Free(state);
         break;
   }
   return result;
}

extern sLoopClientDesc EditGeomBaseClientDesc;

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   sLoopClientDesc* desc = (data) ? &EditGeomClientDesc : &EditGeomBaseClientDesc;
   //   Assrt((void*)data != NULL);
   state = (StateRecord*) Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

#define BASE_MSGS (kMsgDatabase|kMsgVisual|kMsgEnd|kMsgAppInit|kMsgAppTerm)

sLoopClientDesc EditGeomClientDesc =
{
   &LOOPID_EditGeom,
   "EditGeom Client",
   kPriorityNormal,
   kMsgsMode|kMsgsFrame|BASE_MSGS,

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode},
      { kConstrainAfter, &LOOPID_ObjSys, (ulong)kMsgsAll},
      { kConstrainAfter, &LOOPID_BrushList, kMsgDatabase }, 
      { kConstrainAfter, &LOOPID_Wr, kMsgDatabase},

      {kNullConstraint}
   }
};

sLoopClientDesc EditGeomBaseClientDesc =
{
   &LOOPID_EditGeom,
   "EditGeom Client",
   kPriorityNormal,
   BASE_MSGS,
   kLCF_Callback,
   
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode},
      { kConstrainAfter, &LOOPID_ObjSys, (ulong)kMsgsAll},
      { kConstrainAfter, &LOOPID_BrushList, (ulong)kMsgsAll},
      { kConstrainAfter, &LOOPID_Wr, kMsgDatabase},

      {kNullConstraint}
   }
};

