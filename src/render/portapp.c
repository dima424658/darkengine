/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/portapp.c,v 1.41 2000/02/19 12:35:33 toml Exp $

#include <aggmemb.h>
#include <appagg.h>
#include <r3d.h>
#include <config.h>

#include <2dapp.h>
#include <r3app.h>
#include <portapp.h>
#include <portal.h>
#include <simtime.h>
#include <timer.h>
#include <init.h>

#include <command.h>

#include <animlgt.h>
#include <litprop.h>
#include <medmotn.h>
#include <porthlpr.h>
#include <wrcell.h>
#include <wrbsp.h>
#include <rendptst.h>
#include <playtest.h>

#include <report.h>

#include <ctimer.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// Constants
//

static const GUID* my_guid = &IID_Portal;
static char* my_name = "Portal";
static int my_priority = kPriorityLibrary;
#define MY_CREATEFUNC PortalSysCreate


////////////////////////////////////////////////////////////
// commands
//

// woo woo
#ifdef PLAYTEST
static void _toggle_light_bright(char *str)
{
   CheatCheckToggle(&portal_render_from_texture,str,"light_bright");
}

static void wr_cell_display(int cell)
{
   char buffer[256];
   sprintf(buffer, "show_one_cell %d", cell);
   CommandExecute(buffer);
   sprintf(buffer, "cell_teleport %d", cell);
   CommandExecute(buffer);
}

extern bool use_old_raycast;
extern float game_zoom_factor;

Command portal_commands[] =
{
   { "light_bright", FUNC_STRING, _toggle_light_bright,    "render without lightmaps" },

   // random testing 'n weirdness                             
   { "old_raycast", TOGGLE_BOOL, &use_old_raycast },
   { "render_backward", TOGGLE_BOOL, &render_backward,            "shows view back to front" },
   { "mip_detail", VAR_FLOAT, &portal_detail_level,               "MIP level slider (0-1.0)" },
   { "clear_surface_cache", FUNC_VOID, &clear_surface_cache,      "force surface rebuilding" },

   { "fog_on", TOGGLE_BOOL, &portal_fog_on,                       "is fog being used?" },
   { "fog_dist", VAR_FLOAT, &portal_fog_dist,                     "dist at which fog is opaque" },
   { "fog_r", VAR_INT, &portal_fog_color[0],                      "fog red (0-255)" },
   { "fog_g", VAR_INT, &portal_fog_color[1],                      "fog green (0-255)" },
   { "fog_b", VAR_INT, &portal_fog_color[2],                      "fog blue (0-255)" },

#ifndef SHIP
   // visualization
   { "show_mip", TOGGLE_BOOL, &draw_solid_by_MIP_level,           "levels of detail in rendering" },
   { "show_cell", TOGGLE_BOOL, &draw_solid_by_cell,               "show cells in rendering database" },
   { "show_poly", TOGGLE_BOOL, &draw_solid_wireframe,             "show polygon outlines" },
   { "show_poly_flags", TOGGLE_BOOL, &draw_solid_by_poly_flags,   "show flag settings of polygons" },
   { "show_cell_flags", TOGGLE_BOOL, &draw_solid_by_cell_flags,   "show flag settings of cells" },
   { "show_poly_edges", TOGGLE_BOOL, &draw_wireframe_around_tmap, "show wireframes in world" },
   { "show_all_edges", TOGGLE_BOOL, &draw_wireframe_around_poly,  "show complete wireframes" },
   // stat gathering fun
   { "wr_stats", FUNC_VOID, wr_mono_stats, "dump all worldrep stats" },
   { "wr_check_cells", FUNC_VOID, wr_check_cells, "do assertion checking on worldrep" },
   { "cc", FUNC_INT, wr_cell_display, "show cell and teleport to it" },
#endif

   { "set_3d_zoom", VAR_FLOAT, &game_zoom_factor },

   // These should really be elsewhere.
   { "ctimer_init", FUNC_VOID, &CTimerInit,    "clear c-function profiling" },
   { "ctimer_dump", FUNC_VOID, &CTimerDump,    "show results of c-function profiling" },

   { "dump_bsp", FUNC_VOID, wrBspTreeDump,  "dump the worldrep bsp tree" },

#ifdef EDITOR
   { "show_one_cell", FUNC_INT, show_one_cell,    "wireframe on given cell" },
   { "cell_teleport", FUNC_INT, teleport_to_cell, "move editor cam to given cell" },
#endif
};
#endif PLAYTEST

////////////////////////////////////////////////////////////
// INIT FUNC
//

long our_time(void)
{
   return tm_get_millisec_unrecorded();
}

long our_frame_time(void)
{
   return GetSimFrameTime();
}

#ifdef REPORT
static void _wrsize_report(int WarnLevel, void *data, char *buffer)
{
   char *p=buffer;
   rsprintf(&p,"\n");
   wr_stats(&p);
}
#endif

#pragma off(unreferenced)
static STDMETHODIMP _InitFunc(IUnknown* goof)
{
   int pr_shades=11;
#ifdef PLAYTEST
   int cfg_arr[1], cnt;
   if (config_is_defined("shade_levels"))
   {
      cnt=1;
      config_get_value("shade_levels",CONFIG_INT_TYPE,(config_valptr)cfg_arr,&cnt);
      if (cnt==1) pr_shades=cfg_arr[0];
   }
#endif

   portal_get_time = our_time;
   portal_get_frame_time = our_frame_time;
   init_portal_renderer(pr_shades,0);
   portal_anim_light_callback = ObjAnimLightFrameUpdate;
   portal_anim_medium_callback = MedMoUpdateCellMotion;
   AnimLightInit();

   ReportRegisterGenCallback(_wrsize_report,kReportWorldDB,"WorldRep Size",NULL);

#ifdef PLAYTEST
   COMMANDS(portal_commands, HK_EDITOR | HK_GAME_MODE);
   RendPlaytestCommandsInit();
#endif

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
static STDMETHODIMP _ShutdownFunc(IUnknown* goof)
{
   AnimLightCleanup();

   ReportUnRegisterGenCallback(_wrsize_report,kReportWorldDB,NULL);

   return kNoError;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// CONSTRAINTS
//

static sRelativeConstraint _Constraints[] = 
{
   { kNullConstraint, }
}; 

////////////////////////////////////////////////////////////
// Everything below here is boiler plate code.
// Nothing needs to change, unless you want to add postconnect stuff
////////////////////////////////////////////////////////////

#pragma off(unreferenced)
static STDMETHODIMP NullFunc(IUnknown* goof)
{
   return kNoError;
}
#pragma on(unreferenced)

#pragma off(unreferenced)
static void STDMETHODCALLTYPE FinalReleaseFunc(IUnknown* goof)
{
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SysCreate()
//
// Called during AppCreateObjects, adds the uiSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject; 




void LGAPI MY_CREATEFUNC(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ; 
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect   
               _InitFunc,     // init
               _ShutdownFunc,    // end
               NullFunc);     // disconnect
   add_info.pID = my_guid;
   add_info.pszName = my_name; 
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = my_priority;
   add_info.pControlConstraints = _Constraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}

