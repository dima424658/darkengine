/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendptst.c,v 1.3 1999/08/05 17:31:49 Justin Exp $
// rendobj playtest stuff (some ifndef SHIP, some just PLAYTEST)

#ifdef PLAYTEST

#include <mprintf.h>
#include <config.h>

#include <dev2d.h>

#include <command.h>

#include <osysbase.h>
#include <objedit.h>
#include <matrixs.h>
#include <rendobj.h>
#include <objpos.h>
#include <guistyle.h>

#include <rendptst.h>

#include <dbmem.h>

// sadly, this is all in ship as well
// since for now that is how rendobj does it

#ifndef SHIP
// from rendobj.c
extern BOOL g_bDrawMeshVerts;
extern BOOL g_bDrawMeshJoints;
extern int  g_DbgMeshSegIndex;
extern BOOL g_bShowPhysModels;
extern BOOL g_bShowPhysBBox;
extern BOOL g_bShowCreatureJoints;

// do objs light
extern BOOL obj_lighting_on;

static void meshSegsReset()
{
   g_DbgMeshSegIndex=-1;
}

static void meshSegIncr()
{
   g_DbgMeshSegIndex++;
}
#endif

// for looking at names of rendered things
static void rend_name_toggle(int color)
{
   if (rendobj_name_color==0)
      rendobj_name_color=color;
   else
      rendobj_name_color=0;
}

#ifdef NEW_NETWORK_ENABLED
static void rend_net_name_color_set(const char *colorStr)
{  // for looking at names and network status
   int localColor=0x1b00000, hostedColor=0x100b000, proxyColor=0x10000b0;
   sscanf(colorStr, "%x, %x, %x", &localColor, &hostedColor, &proxyColor);
   rendobj_local_name_color = localColor;
   rendobj_hosted_name_color = hostedColor;
   rendobj_proxy_name_color = proxyColor;
}

static void _init_net_rend_colors(void)
{
   rendobj_local_name_color = 0x1b00000;
   rendobj_hosted_name_color = 0x100b000;
   rendobj_proxy_name_color = 0x10000b0;

   rendobj_net_name_coloring = config_is_defined("net_objnames");
   if ((rendobj_net_name_coloring==FALSE)||config_is_defined("net_name_allobjs"))
      rendobj_name_types=kRendObjNameAll;
   else
      rendobj_name_types=kRendObjNameAI|kRendObjNameActPhys;
}
#else
#define _init_net_rend_colors()
#endif

static void rend_name_list(void)
{
   int i;
   for (i=0; i<gMaxObjID; i++)
      if (rendobj_object_visible(i))
      {
         mxs_vector *pos=ObjPosGetLocVector((ObjID)i);
         mprintf("Saw %s at %d %d %d\n",ObjEditName(i),(int)pos->x,(int)pos->y,(int)pos->z);
      }
}

// actual command list
Command rend_all_mode_keys[] =
{
#ifndef SHIP
   { "show_phys_models",TOGGLE_BOOL,&g_bShowPhysModels},
   { "show_phys_bbox",TOGGLE_BOOL, &g_bShowPhysBBox},
   { "show_creature_joints",TOGGLE_BOOL, &g_bShowCreatureJoints},
   { "mesh_segs_display", TOGGLE_BOOL, &g_bDrawMeshVerts },
   { "mesh_joints_display", TOGGLE_BOOL, &g_bDrawMeshJoints },
   { "mesh_segs_reset", FUNC_VOID, meshSegsReset },
   { "mesh_seg_incr", FUNC_VOID, meshSegIncr },
   { "mesh_seg_set", VAR_INT, &g_DbgMeshSegIndex },
   { "lit_obj_toggle", TOGGLE_BOOL, &obj_lighting_on },
#endif
   { "rend_objname_color", VAR_INT, &rendobj_name_color, "3d obj name color" },
   { "rend_name_toggle", FUNC_INT, rend_name_toggle, "set color, toggle name color" },
   { "rend_name_types", VAR_INT, &rendobj_name_types, "set mask on things to draw" },
#ifdef NEW_NETWORK_ENABLED
   { "rend_net_name_toggle", TOGGLE_BOOL, &rendobj_net_name_coloring, "toggle name net color" },
   { "rend_net_color_set", FUNC_STRING, rend_net_name_color_set, "<local>, <hosted>, <proxy>" },
#endif
   { "rend_name_list", FUNC_VOID, rend_name_list, "list all rendered objs to mono" },
};

void RendPlaytestCommandsInit(void)
{
   COMMANDS(rend_all_mode_keys, HK_EDITOR | HK_GAME_MODE);
   _init_net_rend_colors();
}

#endif  // PLAYTEST
