/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkgame.cpp,v 1.106 2000/03/09 16:16:14 toml Exp $
// dark specific game features

#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <2d.h>
#include <mprintf.h>

#include <config.h>
#include <command.h>

#include <init.h>
#include <appname.h>
#include <drkgame.h>
#include <contentctx.h>
#include <metagame.h>
#include <drkdebrf.h>
#include <bugterm.h>

#include <rect.h>
#include <kbcook.h>
#include <keydefs.h>
#include <res.h>

#include <types.h>
#include <lresname.h>

#include <editor.h>
#include <camera.h>
#include <headmove.h>
#include <player.h>
#include <playrobj.h>
#include <editobj.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <wrtype.h>
#include <lnkquery.h>

#include <scrnman.h>
#include <scrnovls.h>
#include <partprop.h>
#include <objshape.h>
#include <objmodel.h>
#include <texmem.h>
#include <gen_bind.h>

#include <drkcret.h>
#include <drkinvui.h>
#include <drkloop.h>
#include <simman.h>
#include <simdef.h>
#include <dspchdef.h>
#include <contain.h>
#include <rendprop.h>
#include <physapi.h>
#include <mnamprop.h>
#include <mnumprop.h>
#include <darkai.h>
#include <drkbook.h>
#include <pgrpprop.h>
#include <drkuires.h>
#include <scrnloop.h>
#include <drkfixpr.h>

#include <drksound.h>

#include <drkbreth.h>

#include <event.h>
#include <gadbox.h>

#include <prjctile.h>

#include <guistyle.h>
#include <cmdterm.h>
#include <simtime.h>
#include <pick.h>
#include <objcast.h>
#include <port.h>
#include <wr.h>
#include <wrdbrend.h>

#include <drk_bind.h>
#include <drkreact.h>
#include <drkinv.h>
#include <drklinks.h>
#include <drkwbow.h>
#include <drkwbdy.h>
#include <drkloot.h>
#include <drkmiss.h>
#include <drkamap.h>
#include <drkldout.h>
#include <drkdiff.h>
#include <drkmislp.h>
#include <drkvo.h>
#include <drkmenu.h>
#include <drksave.h>
#include <drksavui.h>
#include <drkreprt.h>
#include <drkscrm.h> //enter/exit script
#include <drkstats.h>
#include <drkcolor.h>
#include <gamescrn.h>
#include <scrnmode.h>
#include <meshprop.h>

#include <gametool.h>

// for player creation
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>
#include <contain.h>
#include <drkplinv.h>

#include <picklock.h>
#include <drkpwups.h>

#include <drkcmbat.h>
#include <drkmsg.h>

#include <culpable.h>

#include <questapi.h>
#include <campaign.h>
#include <drkgoalt.h>

// Goofy alpha hack
#include <imgsrc.h>

// scriptman
#include <scrptbas.h>
#include <scrptapi.h>

// E3 HACKS
#include <propface.h>
#include <propbase.h>
#include <plycbllm.h>
#include <anim_txt.h>
#include "../../libsrc/script/squirrel/squirrelhost.h"

#ifdef NEW_NETWORK_ENABLED
#include <netman.h>
#endif

#include <drkpanl.h>

#include <allocapi.h>

//guns
#include <gunprop.h>
#include <projlink.h>
#include <projprop.h>
#include <gunflash.h>
#include <gunproj.h>

#include <dbmem.h> // must be last included header

#define E398_HACK

// this is the default memory allocation limit
#ifdef EDITOR
#define DEFAULT_MEMORY_CAP (56*1024*1024)
#else
#define DEFAULT_MEMORY_CAP (48*1024*1024)
#endif

static void DarkScriptChangeMode(boolean resuming, boolean suspending);

#ifdef THIEF2_GAME
static BOOL gConsoleRetinfoHUD = FALSE;
static BOOL gTextureAuditDone = FALSE;
static void console_format_retinfo(char* message, int size);
static void console_texture_audit(char* value);
#endif

////////////////////////////////////////////////////////////
// PER-FRAME GAME EXECUTION
//

//
// Sim frame update, called before rendering 
//
void dark_sim_update_frame(void)
{
   int ms=GetSimFrameTime();
   

   ectsAnimTxtTime=GetSimTime();
   ectsAnimTxtUpdateAll();

   headmoveCheck(PlayerCamera(),ms);
   playerHeadControl();
   drkCheckHeadFocus();
   PickFrameUpdate();
   BreathSimUpdateFrame(ms); 

   PlayerCbllmUpdate(ms);

}

static IImageSource* greek_letter = NULL; 

//
// post render frame updates
//
void dark_rend_update_frame(void)
{
   // NewDark Squirrel overlays draw into the same retained software canvas as
   // the stock HUD, before the presenter uploads it for this hardware frame.
   ScrnLockDrawCanvas();
   SquirrelRenderOverlays();
   ScrnUnlockDrawCanvas();

   // display the greek letter
#ifdef PLAYTEST
   if (greek_letter)
   {
      grs_bitmap* bm = (grs_bitmap*)greek_letter->Lock(); 
      int x = grd_canvas->bm.w - bm->w; 
      int y = 0 ;
      ScrnLockDrawCanvas();
      gr_bitmap(bm,x,y); 
      ScrnUnlockDrawCanvas();
      greek_letter->Unlock(); 
   }
#endif //   

#ifdef THIEF2_GAME
   if (DarkConsoleReticleEnabled())
   {
      int old_color = gr_get_fcolor();
      int x = grd_canvas->bm.w / 2;
      int y = grd_canvas->bm.h / 2;

      ScrnLockDrawCanvas();
      gr_set_fcolor(guiStyleGetColor(NULL, StyleColorFG));
      gr_hline(x - 5, y, x - 2);
      gr_hline(x + 2, y, x + 5);
      gr_vline(x, y - 5, y - 2);
      gr_vline(x, y + 2, y + 5);
      gr_set_fcolor(old_color);
      ScrnUnlockDrawCanvas();
   }
#endif

#ifdef THIEF2_GAME
   if (!gTextureAuditDone)
   {
      char auditPath[260] = "";
      if (wr_num_cells > 0 &&
          config_get_raw("texture_audit", auditPath, sizeof(auditPath)))
      {
         gTextureAuditDone = TRUE;
         console_texture_audit(auditPath);
      }
   }

   if (gConsoleRetinfoHUD)
   {
      char message[256];
      console_format_retinfo(message, sizeof(message));
      DarkMessageParams(message, 100, kDefaultMsgColor);
   }
#endif

   DarkMessageUpdateFrame();

#ifdef THIEF2_GAME
   // Gameplay clears and rebuilds its software overlay every hardware frame.
   // A focused terminal therefore has to participate in that frame pass;
   // event-only redraws disappear behind the following 3D frame and flicker
   // back only while the user is typing.
   cmdterm_redraw();
#endif
}



////////////////////////////////////////////////////////////
// GAME UI 
//

//
// In-game command terminal
//

#define NUM_BUG_TERM_LINES 10
#define CMD_Y_MARGIN 2

static void build_cmd_term(LGadRoot* root)
{
   Rect r = *LGadBoxRect(root);
   short w,h;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_string_size("X",&w,&h);
   guiStyleCleanupFont(NULL,StyleFontNormal);

   // Give the larger in-game message font enough room for useful scrollback.
   // A proportional height also remains usable across the supported modes.
   r.lr.y = (short)(r.ul.y + RectHeight(&r) / 3);
   CreateCommandTerminal(root,&r,
                         kCmdTermHideUnfocused | kCmdTermScrollable);

   r = *LGadBoxRect(root);
   r.lr.y = h * NUM_BUG_TERM_LINES + CMD_Y_MARGIN;
   CreateBugTerminal(root,&r,kCmdTermHideUnfocused);
}

//
// Custom game input handlers
//

// parse the key
static bool game_key_parse(int keycode)
{
   return FALSE;
}

static BOOL key_handler_func(uiEvent* ev, Region* r, void* data)
{
   uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;
   return game_key_parse(kev->code);
}


static int key_handler_id;

//
// Game mode initialization
//

void dark_start_gamemode(BOOL resuming)
{
   LGadRoot* root = LGadCurrentRoot();

   uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_KBD_COOKED,key_handler_func,NULL,&key_handler_id);
   build_cmd_term(root);

   PickSetFocus(fix_make(grd_canvas->bm.w >> 1, 0),
                fix_make(grd_canvas->bm.h >> 1, 0),
                head_focus_dist2_tol);
   PickSetCanvas();

   DarkScriptChangeMode(resuming,FALSE);
   InvUIEnterMode(resuming); 

#ifdef PLAYTEST
   if (!config_is_defined("herr_rockmusiker"))
      greek_letter = CreateResourceImageSource("intrface\\","greekltr.pcx"); 
#endif // PLAYTEST

   if (gScrnLoopSetModeFailed)
   {
      DarkMessage(FetchUIString("misc","set_mode_failed")); 
   }
}

////////////////////////////////////////////////////////////
// GAME MODE SETUP/CLEANUP
//

//
// Game mode termination
//

void dark_end_gamemode(BOOL suspending)
{
   LGadRoot* root = LGadCurrentRoot();
   uiRemoveRegionHandler(LGadBoxRegion(root),key_handler_id);

   DestroyCommandTerminal();
   DestroyBugTerminal();
   SafeRelease(greek_letter); 
   DarkScriptChangeMode(FALSE,suspending);
   InvUIExitMode(suspending); 
}

//////////////////////////////////////////////////////////////
// COMMANDS
//

int master_overlay=0;

static void drkgame_OverlayToggle(int which)
{
   ScreenOverlaysChange(which,kScreenOverlaysToggle);
}


#ifndef SHIP 
// @HACK: this is for fixing up players with an "old" player object
static void fixup_player(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   ObjID old = pObjSys->GetObjectNamed("OldPlayer"); 
   if (old == OBJ_NULL)
   {
      mprintf("No old player object"); 
      return; 
   }

   ObjID marker_arch = pObjSys->GetObjectNamed("Marker"); 

   if (marker_arch == OBJ_NULL) 
      marker_arch = ROOT_ARCHETYPE; 

   ObjID start = pObjSys->BeginCreate(marker_arch,kObjectConcrete); 

   // move to player position 
   ObjPos* pos = ObjPosGet(old); 
   ObjPosCopyUpdate(start,pos); 

   // set up the "PlayerFactory" link to the "garrett" archetype
   ObjID garrett = pObjSys->GetObjectNamed("Garrett"); 
   if (garrett == OBJ_NULL) garrett = ROOT_ARCHETYPE; 

   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );
   pRel->Add(start,garrett); 

   // Name the starting point
   pObjSys->NameObject(start,"Starting Point"); 
   
   // move the old player's contents over to the marker
   AutoAppIPtr(ContainSys);
   sContainIter* iter;
   for (iter = pContainSys->IterStart(old); !iter->finished; pContainSys->IterNext(iter))
      pContainSys->Add(start,iter->containee,iter->type,CTF_NONE); 
   pContainSys->IterEnd(iter); 

   pObjSys->EndCreate(start);
   mprintf("Player Starting Point Created"); 
}
#endif /* SHIP */

static void do_version()
{
   DarkMessage(AppName()); 
}

static void win_mission()
{
  AutoAppIPtr(QuestData);
  pQuestData->Set(MISSION_COMPLETE_VAR,1);
  UnwindToMissionLoop();
}

#ifdef THIEF2_GAME

struct sConsoleMission
{
   int number;
   const char* short_name;
   const char* title;
};

static const sConsoleMission gConsoleMissions[] =
{
   { 0,  "training",     "Training" },
   { 1,  "interference", "Running Interference" },
   { 2,  "shipping",     "Shipping... and Receiving" },
   { 4,  "framed",       "Framed" },
   { 5,  "ambush",       "Ambush!" },
   { 6,  "eavesdropping", "Eavesdropping" },
   { 7,  "bank",         "First City Bank and Trust" },
   { 8,  "blackmail",    "Blackmail" },
   { 9,  "courier",      "Trace the Courier" },
   { 10, "trail",        "Trail of Blood" },
   { 11, "party",        "Life of the Party" },
   { 12, "cargo",        "Precious Cargo" },
   { 13, "kidnap",       "Kidnap" },
   { 14, "casing",       "Casing the Joint" },
   { 15, "masks",        "Masks" },
   { 16, "soulforge",    "Sabotage at Soulforge" },
};

static BOOL gConsoleImmune = FALSE;
static BOOL gConsoleFlying = FALSE;
static BOOL gConsolePlayerPhysics = TRUE;
static BOOL gConsoleInvisible = FALSE;
static BOOL gConsoleReticle = FALSE;

static void console_message(const char* format, ...)
{
   char message[256];
   va_list args;

   va_start(args, format);
   _vsnprintf(message, sizeof(message) - 1, format, args);
   va_end(args);
   message[sizeof(message) - 1] = '\0';

   mprintf("%s\n", message);
}

static int console_on_off(const char* value, BOOL current)
{
   while (value && isspace((unsigned char)*value))
      ++value;

   if (!value || !*value)
      return current;
   if (!_stricmp(value, "on") || !_stricmp(value, "1") || !_stricmp(value, "true"))
      return TRUE;
   if (!_stricmp(value, "off") || !_stricmp(value, "0") || !_stricmp(value, "false"))
      return FALSE;
   return -1;
}

static const sConsoleMission* console_find_mission(const char* value)
{
   char normalized[64];
   int output = 0;
   int i;

   if (!value)
      return NULL;
   while (*value && isspace((unsigned char)*value))
      ++value;

   if (isdigit((unsigned char)*value))
   {
      int number = atoi(value);
      for (i = 0; i < (int)(sizeof(gConsoleMissions) / sizeof(gConsoleMissions[0])); ++i)
         if (gConsoleMissions[i].number == number)
            return &gConsoleMissions[i];
      return NULL;
   }

   while (*value && output < (int)sizeof(normalized) - 1)
   {
      if (isalnum((unsigned char)*value))
         normalized[output++] = (char)tolower((unsigned char)*value);
      ++value;
   }
   normalized[output] = '\0';

   for (i = 0; i < (int)(sizeof(gConsoleMissions) / sizeof(gConsoleMissions[0])); ++i)
      if (!_stricmp(normalized, gConsoleMissions[i].short_name))
         return &gConsoleMissions[i];
   return NULL;
}

static void console_open_mission(char* value)
{
   const sConsoleMission* mission = console_find_mission(value);
   if (!mission)
   {
      console_message("Unknown mission. Use list_missions for valid numbers and short names.");
      return;
   }

   AutoAppIPtr(Campaign);
   pCampaign->New();

   AutoAppIPtr(QuestData);
   pQuestData->Create(DIFF_QVAR, 0, kQuestDataCampaign);

   SetNextMission(mission->number);
   MissionLoopReset(kMissLoopStartLoop);
   UnwindToMissionLoop();
}

static void console_list_missions(void)
{
   FILE* log = fopen("thief-console.log", "a");
   int available = 0;
   int i;

   if (log)
      fprintf(log, "\nAvailable missions:\n");

   for (i = 0; i < (int)(sizeof(gConsoleMissions) / sizeof(gConsoleMissions[0])); ++i)
   {
      char filename[32];
      FILE* mission_file;
      _snprintf(filename, sizeof(filename), "miss%d.mis", gConsoleMissions[i].number);
      mission_file = fopen(filename, "rb");
      if (!mission_file)
         continue;
      fclose(mission_file);
      ++available;

      mprintf("%2d %-14s %s\n", gConsoleMissions[i].number,
              gConsoleMissions[i].short_name, gConsoleMissions[i].title);
      if (log)
         fprintf(log, "%2d %-14s %s\n", gConsoleMissions[i].number,
                 gConsoleMissions[i].short_name, gConsoleMissions[i].title);
   }

   if (log)
      fclose(log);
   console_message("Listed %d installed missions in thief-console.log.", available);
}

static void console_immunity(char* value)
{
   int state = console_on_off(value, gConsoleImmune);
   if (state < 0)
   {
      console_message("Usage: immunity [on/off]");
      return;
   }
   gConsoleImmune = state;
   console_message("Immunity %s.", state ? "on" : "off");
}

static void console_flying(char* value)
{
   int state = console_on_off(value, gConsoleFlying);
   BOOL freeFlight;
   if (state < 0)
   {
      console_message("Usage: flying [on/off]");
      return;
   }

   gConsoleFlying = state;
   freeFlight = state || !gConsolePlayerPhysics;
   if (PlayerObjectExists())
   {
      PhysSetGravity(PlayerObject(), freeFlight ? 0.0f : 1.0f);
      PhysSetBaseFriction(PlayerObject(), freeFlight ? 320.0f : 0.0f);
      if (!freeFlight)
         PhysStopAxisControlVelocity(PlayerObject(), 2);
   }
   console_message("Flying %s.", state ? "on" : "off");
}

static void console_player_physics(char* value)
{
   int state = console_on_off(value, gConsolePlayerPhysics);
   BOOL freeFlight;

   if (state < 0)
   {
      console_message("Usage: player_physics [on/off]");
      return;
   }

   gConsolePlayerPhysics = state;
   PhysSetPlayerPhysicsEnabled(state);
   freeFlight = gConsoleFlying || !state;
   if (PlayerObjectExists())
   {
      PhysSetGravity(PlayerObject(), freeFlight ? 0.0f : 1.0f);
      PhysSetBaseFriction(PlayerObject(), freeFlight ? 320.0f : 0.0f);
      if (!freeFlight)
         PhysStopAxisControlVelocity(PlayerObject(), 2);
   }
   console_message("Player physics %s%s", state ? "on." : "off: ",
                   state ? "" : "collision and gravity disabled.");
}

static void console_invisible(char* value)
{
   int state = console_on_off(value, gConsoleInvisible);
   if (state < 0)
   {
      console_message("Usage: invisible [on/off]");
      return;
   }

   gConsoleInvisible = state;
   if (PlayerObjectExists() && g_pIsInvisibleProperty)
      g_pIsInvisibleProperty->Set(PlayerObject(), state ? 0 : -1);
   console_message("Invisibility %s.", state ? "on" : "off");
}

static void console_reticle(char* value)
{
   int state = console_on_off(value, gConsoleReticle);
   if (state < 0)
   {
      console_message("Usage: reticle [on/off]");
      return;
   }
   gConsoleReticle = state;
   console_message("Reticle %s.", state ? "on" : "off");
}

static void console_format_retinfo(char* message, int size)
{
   const float rayDistance = 1000.0f;
   mxs_vector cameraPosition;
   mxs_angvec cameraFacing;
   mxs_matrix cameraMatrix;
   mxs_vector cameraForward;
   mxs_vector forward;
   mxs_vector endPosition;
   Location startLocation;
   Location endLocation;
   Location hitLocation;
   eObjCastResult hitType;
   ObjID target;
   char model[64] = "(none)";
   const char* name;

   CameraGetLocation(PlayerCamera(), &cameraPosition, &cameraFacing);
   mx_ang2mat(&cameraMatrix, &cameraFacing);
   mx_mk_vec(&cameraForward, 1.0f, 0.0f, 0.0f);
   mx_mat_mul_vec(&forward, &cameraMatrix, &cameraForward);
   mx_scale_add_vec(&endPosition, &cameraPosition, &forward, rayDistance);
   MakeLocationFromVector(&startLocation, &cameraPosition);
   MakeLocationFromVector(&endLocation, &endPosition);

   hitType = ObjRaycast(&startLocation, &endLocation, &hitLocation, FALSE);
   if (hitType == kObjCastTerrain)
   {
      int polygon = PortalRaycastFindPolygon();
      int texture = -1;
      if (PortalRaycastCell != CELL_INVALID && polygon != POLY_INVALID)
         texture = WR_CELL(PortalRaycastCell)->render_list[polygon].texture_id;
      _snprintf(message, size - 1,
                "Terrain: cell %d, polygon %d, texture %d",
                PortalRaycastCell, polygon, texture);
      message[size - 1] = '\0';
      return;
   }

   if (hitType != kObjCastMD && hitType != kObjCastMesh)
   {
      _snprintf(message, size - 1,
                "Reticle target: none within %.0f units", rayDistance);
      message[size - 1] = '\0';
      return;
   }

   target = g_ObjCastObjID;
   if (target == OBJ_NULL)
   {
      _snprintf(message, size - 1,
                "Reticle target: raycast returned no object");
      message[size - 1] = '\0';
      return;
   }

   AutoAppIPtr_(ObjectSystem, pObjectSystem);
   name = pObjectSystem->GetName(target);
   ObjGetModelName(target, model);
   _snprintf(message, size - 1, "%s object %d: %s, model %s",
             hitType == kObjCastMesh ? "Mesh" : "MD",
             target, name && *name ? name : "(unnamed)", model);
   message[size - 1] = '\0';
}

static void console_retinfo(char* value)
{
   while (value && isspace((unsigned char)*value))
      ++value;

   if (value && *value)
   {
      int state = console_on_off(value, gConsoleRetinfoHUD);
      if (state < 0)
      {
         console_message("Usage: retinfo [on/off]");
         return;
      }
      gConsoleRetinfoHUD = state;
      console_message("Reticle info HUD %s.", state ? "on" : "off");
      return;
   }

   char message[256];
   console_format_retinfo(message, sizeof(message));
   console_message("%s", message);
}

static const char* audit_texture_status(int texture, IRes** resource)
{
   *resource = NULL;
   if (texture == 0)
      return "default-or-sky";
   if (texture < 0 || texture >= TEXMEM_MAX)
      return "INVALID";
   *resource = texmemGetHandle(texture);
   if (*resource)
      return "ok";
   if (TEXMEM_IS_RESERVED(texture))
      return "reserved";
   return "MISSING";
}

static void console_texture_audit(char* value)
{
   char path[260] = "texture-audit.log";
   int* terrainCounts;
   int terrainSurfaces = 0;
   int terrainMissing = 0;
   int objectCount = 0;
   int objectMissing;
   int objectReplacementsMissing = 0;
   int cellIndex;
   FILE* log;

   while (value && isspace((unsigned char)*value))
      ++value;
   if (value && *value)
   {
      strncpy(path, value, sizeof(path));
      path[sizeof(path) - 1] = '\0';
   }

   log = fopen(path, "w");
   if (!log)
   {
      console_message("Could not create texture audit: %s", path);
      return;
   }

   terrainCounts = (int*)calloc(TEXMEM_MAX, sizeof(*terrainCounts));
   if (!terrainCounts)
   {
      fclose(log);
      console_message("Could not allocate texture audit inventory.");
      return;
   }

   fprintf(log, "TEXTURE_AUDIT version=1 cells=%d loaded_terrain_slots=%d\n",
           wr_num_cells, texmemGetMax());
   for (cellIndex = 0; cellIndex < wr_num_cells; ++cellIndex)
   {
      PortalCell* cell = WR_CELL(cellIndex);
      int polygon;
      if (!cell || !cell->render_list)
         continue;
      for (polygon = 0; polygon < cell->num_render_polys; ++polygon)
      {
         int texture = cell->render_list[polygon].texture_id;
         IRes* resource;
         const char* status = audit_texture_status(texture, &resource);
         ++terrainSurfaces;
         if (texture >= 0 && texture < TEXMEM_MAX)
            ++terrainCounts[texture];
         if (!strcmp(status, "MISSING") || !strcmp(status, "INVALID"))
         {
            ++terrainMissing;
            fprintf(log,
                    "TERRAIN_SURFACE cell=%d polygon=%d texture=%d status=%s\n",
                    cellIndex, polygon, texture, status);
         }
      }
   }

   for (int texture = 0; texture < TEXMEM_MAX; ++texture)
   {
      IRes* resource;
      char* canonical = NULL;
      grs_bitmap* bitmap = NULL;
      const char* status;
      if (!terrainCounts[texture])
         continue;
      status = audit_texture_status(texture, &resource);
      if (resource)
      {
         IRes_GetCanonPathName(resource, &canonical);
         bitmap = (grs_bitmap*)IRes_DataPeek(resource);
      }
      fprintf(log,
              "TERRAIN_TEXTURE id=%d surfaces=%d status=%s resource=\"%s\" "
              "width=%d height=%d\n",
              texture, terrainCounts[texture], status,
              canonical ? canonical : "", bitmap ? bitmap->w : 0,
              bitmap ? bitmap->h : 0);
      if (canonical)
         free(canonical);
   }
   free(terrainCounts);

   {
      AutoAppIPtr_(ObjectSystem, pObjectSystem);
      IObjectQuery* objects = pObjectSystem->Iter(kObjectConcrete);
      for (; !objects->Done(); objects->Next())
      {
         ObjID object = objects->Object();
         char model[64] = "";
         int handle = MIDX_INVALID;
         const char* name;
         mxs_vector* position;
         if (!ObjGetModelName(object, model) || !model[0])
            continue;
         ObjGetModelNumber(object, &handle);
         name = pObjectSystem->GetName(object);
         position = ObjPosGetLocVector(object);
         fprintf(log,
                 "OBJECT_INSTANCE object=%d name=\"%s\" model=\"%s\" "
                 "model_handle=%d status=%s position=%.3f,%.3f,%.3f\n",
                 object, name && *name ? name : "(unnamed)", model, handle,
                 handle == MIDX_INVALID ? "MISSING" : "ok",
                 position ? position->x : 0.0f,
                 position ? position->y : 0.0f,
                 position ? position->z : 0.0f);
         if (handle != MIDX_INVALID)
            objectReplacementsMissing +=
               objmodelAuditObjectTextures(log, object, handle);
         ++objectCount;
      }
      objects->Release();
   }

   objectMissing = objmodelAuditTextures(log);
   fprintf(log,
           "SUMMARY terrain_surfaces=%d terrain_missing=%d "
           "object_instances=%d object_textures_missing=%d "
           "object_replacements_missing=%d\n",
           terrainSurfaces, terrainMissing, objectCount, objectMissing,
           objectReplacementsMissing);
   fclose(log);
   console_message("Texture audit wrote %s: terrain missing %d, object textures missing %d, replacements missing %d.",
                   path, terrainMissing, objectMissing,
                   objectReplacementsMissing);
}

static void console_list_scripts(void)
{
   FILE* log = fopen("thief-console.log", "a");
   int count = 0;
   tScrIter iterator;
   const sScrClassDesc* script;

   AutoAppIPtr(ScriptMan);
   script = pScriptMan->GetFirstClass(&iterator);
   if (log)
      fprintf(log, "\nLoaded script classes:\n");

   while (script)
   {
      mprintf("%s (%s)\n", script->pszClass,
              script->pszModule ? script->pszModule : "unknown module");
      if (log)
         fprintf(log, "%s (%s)\n", script->pszClass,
                 script->pszModule ? script->pszModule : "unknown module");
      ++count;
      script = pScriptMan->GetNextClass(&iterator);
   }
   pScriptMan->EndClassIter(&iterator);

   if (log)
      fclose(log);
   console_message("Listed %d scripts in thief-console.log.", count);
}

static void console_debug_scripts(char* value)
{
   int state = console_on_off(value, ScriptDebugIsEnabled());
   if (state < 0)
   {
      console_message("Usage: debug_scripts [on/off]");
      return;
   }
   ScriptDebugSetEnabled(state);
   console_message("Script logging %s%s", state ? "on: " : "off.",
                   state ? "script-debug.log" : "");
}

static Command thief_console_commands[] =
{
   { "open_mission", FUNC_STRING, console_open_mission,  "open_mission [number/shortname]" },
   { "list_missions", FUNC_VOID,  console_list_missions, "list installed missions" },
   { "immunity",     FUNC_STRING, console_immunity,      "immunity [on/off]" },
   { "flying",       FUNC_STRING, console_flying,        "flying [on/off]" },
   { "player_physics", FUNC_STRING, console_player_physics, "player_physics [on/off]" },
   { "invisible",    FUNC_STRING, console_invisible,     "invisible [on/off]" },
   { "reticle",      FUNC_STRING, console_reticle,       "reticle [on/off]" },
   { "reticle_info", FUNC_STRING, console_retinfo,       "reticle_info [on/off], or report once" },
   { "audit_textures", FUNC_STRING, console_texture_audit, "audit_textures [output file]" },
   { "list_scripts", FUNC_VOID,   console_list_scripts,  "list loaded script classes" },
   { "debug_scripts", FUNC_STRING, console_debug_scripts, "debug_scripts [on/off]" },
   { "openmission",  FUNC_STRING, console_open_mission,  "compatibility alias for open_mission" },
   { "listmissions", FUNC_VOID,   console_list_missions, "compatibility alias for list_missions" },
   { "playerphysics", FUNC_STRING, console_player_physics, "compatibility alias for player_physics" },
   { "retinfo",      FUNC_STRING, console_retinfo,       "compatibility alias for reticle_info" },
   { "ret_info",     FUNC_STRING, console_retinfo,       "compatibility alias for reticle_info" },
   { "listscripts",  FUNC_VOID,   console_list_scripts,  "compatibility alias for list_scripts" },
   { "debugscripts", FUNC_STRING, console_debug_scripts, "compatibility alias for debug_scripts" },
};

#endif // THIEF2_GAME

BOOL DarkConsoleIsImmune(void)
{
#ifdef THIEF2_GAME
   return gConsoleImmune;
#else
   return FALSE;
#endif
}

BOOL DarkConsoleReticleEnabled(void)
{
#ifdef THIEF2_GAME
   return gConsoleReticle;
#else
   return FALSE;
#endif
}

static Command drk_ui_keys[] =
{
   { "toggle_overlay",  FUNC_INT, drkgame_OverlayToggle, "takes which to toggle" },
   { "dark_version",  FUNC_VOID, do_version, "Display version in game mode." },
   { "win_mission", FUNC_VOID, win_mission, "Win the mission", HK_GAME_MODE },
};

#ifndef SHIP
// none defined presently, and can't have empty static structure - so if 0 it out
static Command drk_debug_keys[] =
{
   { "fixup_player",  FUNC_VOID, fixup_player, "Make a player starting point based on OldPlayer" },

};
#define DebugKeys() COMMANDS(drk_debug_keys,HK_ALL)
#else  // SHIP
#define DebugKeys() 
#endif // SHIP

////////////////////////////////////////////////////////////
// SIM MESSAGE HANDLER
// 

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit: 
         DarkStatInitMission();
         break; 

      case kSimTerm:
         DarkStatFinishMission();
         break;
   }
}

static sDispatchListenerDesc sim_listen = 
{
   &LOOPID_DarkSim,     // my guid
   kSimInit|kSimTerm,   // interests
   sim_msg,
};

static void init_sim_msg()
{
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->Listen(&sim_listen); 
}

/////////////////////////////////////////////////
//game mode script message handlers
/////////////////////////////////////////////////

static void DarkScriptChangeMode(boolean resuming, boolean suspending)
{

   AutoAppIPtr_(ObjectSystem,ObjSys);    
   AutoAppIPtr(ScriptMan);
         
   // Send start to all concrete objects!  Yum!         
   cAutoIPtr<IObjectQuery> query ( ObjSys->Iter(kObjectConcrete));             
   for (; !query->Done(); query->Next())            
   {               
      sDarkGameModeScrMsg msg(query->Object(),resuming,suspending);                  
      pScriptMan->SendMessage(&msg);               
   }      
}


////////////////////////////////////////////////////////////
// CONTAINS LISTENER 
//

static BOOL contain_CB(eContainsEvent ev, ObjID outer, ObjID inner, eContainType , ContainCBData )
{
   switch(ev)
   {
      case kContainRemove:
         ObjSetHasRefs(inner,TRUE);
         if (ObjIsParticleGroup(inner))
            ObjParticleSetActive(inner, TRUE);
         break;
      case kContainAdd:
         ObjSetHasRefs(inner,FALSE); 
         PhysDeregisterModel(inner);
         ObjForceReref(inner);
         if (ObjIsParticleGroup(inner))
            ObjParticleSetActive(inner, FALSE);
         break; 

      case kContainCombine:
	 //if not autoequip, don't refresh view on pickup.
	 if (atof(g_pInputBinder->ProcessCmd("echo $auto_equip"))!=0.0)
	   InvUIRefreshObj(outer); 
         break; 
   }
   return TRUE; 
}

static void setup_contain_CB(void)
{
   AutoAppIPtr(ContainSys);
   pContainSys->Listen(OBJ_NULL,contain_CB,NULL); 
}

////////////////////////////////////////////////////////////
// PLAYER CREATION CALLBACK
//

static void player_create_CB(ePlayerEvent event, ObjID player)
{
   switch (event)
   {
      case kPlayerCreate:
      {
         char buf[80];
         // @TODO: get rid of this
         if (config_get_raw("player_model",buf,sizeof(buf)))
         {
            buf[sizeof(buf)-1] = '\0';
            ObjSetModelName(player,buf);
         }

         PlayerCbllmCreate(); // creates "lower brain" and body
         PhysCreateDefaultPlayer(player);
      }
      break;

      case kPlayerLoad:
         PlayerCbllmCreate(); // creates "lower brain" and body
         break; 

      case kPlayerDestroy:
         PlayerCbllmDestroy(); 
         break; 
   }
}


// Look through the playerfactory links on the level. Ideally, we find
// a link whose data is this player's playernum. If not, we use some other
// link as a default.
static ObjID player_factory_CB(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );
   
   ulong myPlayerNum = 0;
#ifdef NEW_NETWORK_ENABLED
   // If this is a multiplayer-capable game, then find our player number.
   // If not, then there's probably only a single PlayerFactory link anyway.
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame()) {
      myPlayerNum = pNetManager->MyPlayerNum();
   } else {
      myPlayerNum = 0;
   }
#endif
   
   // LinkID id = pRel->GetSingleLink(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD); 
   LinkID defaultFactory = LINKID_NULL;
   LinkID id = LINKID_NULL;
   ILinkQuery *pQuery = pRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   if (pQuery == NULL)
      return OBJ_NULL;

   // Run through the PlayerFactory links, and see if any of them work
   for ( ; (!pQuery->Done()) && (id == LINKID_NULL); pQuery->Next()) 
   {
      int *factoryPtr = (int *) pQuery->Data();
      if (factoryPtr == NULL) {
         // It's an old factory with no player num; use it as the
         // default
         defaultFactory = pQuery->ID();
      } else {
         int factoryNum = *factoryPtr;
         if (factoryNum == myPlayerNum) {
            // Got the right one
            id = pQuery->ID();
         } else if (defaultFactory == LINKID_NULL) {
            // We don't have any default yet, so try this one
            defaultFactory = pQuery->ID();
         }
      }
   }

   SafeRelease(pQuery);

   if (id == LINKID_NULL) {
      // We didn't find an appropriate one, so fall back on a default
      if (defaultFactory == LINKID_NULL) {
         // There aren't *any* factories on this level!
         return OBJ_NULL;
      } else {
         id = defaultFactory;
      }
   }

   sLink link; 
   pRel->Get(id,&link); 
   return link.source; 
}

static void setup_player_CB()
{
   PlayerCbllmInit();
   HookPlayerCreate(player_create_CB);
   HookPlayerFactory(player_factory_CB); 
}

////////////////////////////////////////////////////////////
// GAME SCREEN MODE CONSTRAINT CB
//

static void gamescreen_cb(sScrnMode* mode)
{
   if (!(mode->valid_fields & kScrnModeFlagsValid))
   {
      mode->flags = kScrnModeFullScreen|kScrnMode2dDriver|kScrnMode3dDriver; 
      mode->valid_fields |= kScrnModeFlagsValid;
   }

   mode->bitdepth = 16; // always 16 bit

   mode->valid_fields |= kScrnModeBitDepthValid; 

}

////////////////////////////////////////
// Init culpability
struct sCulpRelations
{
   const char* name; 
   ulong flags; 
}; 

static sCulpRelations culp_rels[] = 
{
   {"Contains", 0 }, 
   { "~Firer", kCulpTransitive},
   { "CurWeapon", 0 }, 
}; 

#define NUM_CULP_RELS (sizeof(culp_rels)/sizeof(culp_rels[0]))

// Set up culpability relations for dark
static void dark_init_culpability()
{
   // set up culpability listeners
   AutoAppIPtr_(LinkManager,pLinkMan); 
   for (int i = 0; i < NUM_CULP_RELS; i++)
   {
      sCulpRelations& rel = culp_rels[i]; 
      cAutoIPtr<IRelation> pRel = pLinkMan->GetRelationNamed(rel.name); 
      AddCulpabilityRelation(pRel,rel.flags); 
   }
}

// Init the Dark-specific game tools.
void DarkToolsInit(void)
{
   IGameTools* pGameTools = AppGetObj(IGameTools);
   pGameTools->SetIsToGameModeGUIDCallback(DarkIsToGameModeGUID);
   SafeRelease(pGameTools);
}

//////////////////////////////////////////////////////////////
// ONE-TIME APP INIT/TERM
//

void dark_init_game(void)
{
   COMMANDS(drk_ui_keys,HK_GAME_MODE);
#ifdef THIEF2_GAME
   COMMANDS(thief_console_commands,HK_GAME_MODE);
#endif
   DebugKeys();

   config_get_int("drkgame_overlay",&master_overlay);

   // @TODO: perhaps there should be a good runtime way of figuring 
   // out how to start in editor 

#ifdef EDITOR
   BOOL start_metagame = config_is_defined("start_metagame")
      || config_is_defined("play"); 
#else
   BOOL start_metagame = TRUE;
#endif 

   // set the memory allocation cap
   sAllocLimits allocLimits;
   AllocGetLimits(&allocLimits);
   int memoryCap = (int)((allocLimits.allocCap > DEFAULT_MEMORY_CAP) ? allocLimits.allocCap : DEFAULT_MEMORY_CAP);   // quick kids!  lets put on our memory caps!
   if (config_is_defined("memory_cap"))
      config_get_int( "memory_cap", &memoryCap );
   AllocSetAllocCap( (size_t)memoryCap );

   ConstrainGameScreenMode(gamescreen_cb); 

   // This needs to come very early
   // to get called before 16 bit images are loaded
   DarkColorInit();

   MetaGameInit();
   MissionLoopInit(); 

   if (start_metagame)
      gPrimordialMode = (sLoopInstantiator*)DescribeMissionLoopMode(); 
   
   init_sim_msg(); 
   setup_contain_CB(); 
   InitDarkReactions();
   DarkInitLinks();
   DarkInitProps();
   drkInvInit();
   DarkCreaturesInit();
   PickLockInit();
   BeltLinkInit();
   AltLinkInit();
   BowInit(); 
   BodyCarryInit();
   DrkPowerupInit();
   InvUIInit(); 
   setup_player_CB();
   DarkSoundInit(); 
   BreathSimInit(); 
   DarkCombatInit(); 
//new gun init code
   BaseGunDescPropertyInit();
   AIGunDescPropertyInit();
   GunStatePropertyInit();
   ProjectileLinksInit();
   ProjectilePropertyInit();
   GunFlashInit();
   GunProjectileInit();
//wow,thats a lot for just guns AMSD
   DarkLootInit();
   init_fixture_prop();
   DebriefInit(); 
   MissionDataInit(); 
   MapSourceInfoInit();
   DarkAutomapInit(); 
   LoadoutInit(); 
   DarkDifficultyInit(); 
   DarkMessageInit(); 
   DarkAIInit();
   DarkBookInit(); 
   DarkVoiceOverInit(); 
   DarkMenusInit(); 
   if (DarkContentShowPicker())
      gPrimordialMode = (sLoopInstantiator*)DescribeContentMenuMode();
   DarkSaveGameInit(); 
   DarkSaveInitUI();
   DarkReportInit();
   DarkStatInit();
   dark_init_culpability();
   SetGameIBVarsFunc (InitDarkIBVars);
   DarkToolsInit();
}

void dark_term_game(void)
{
   PickLockTerm();
   BeltLinkTerm();
   AltLinkTerm();
   BowTerm();
   BodyCarryTerm();
   DrkPowerupTerm();
   InvUITerm(); 
   DarkSoundTerm(); 
   BreathSimTerm(); 
   DarkCombatTerm();
//releasing the gun stuff, although they really should have their own
//functions... 
   SafeRelease(g_baseGunDescProperty);
   SafeRelease(g_aiGunDescProperty);
   SafeRelease(g_pGunStateProperty);
   ProjectileLinksTerm();
   SafeRelease(g_pProjectileProperty);
   GunFlashTerm();
   GunProjectileTerm();
// Guns released.
   DarkLootTerm(); 
   term_fixture_prop();
   DebriefTerm(); 
   MissionDataTerm(); 
   MapSourceInfoTerm();
   DarkAutomapTerm(); 
   LoadoutTerm(); 
   MetaGameTerm(); 
   DarkDifficultyTerm(); 
   DarkMessageTerm(); 
   DarkCreaturesTerm();
   MissionLoopTerm(); 
   DarkAITerm();
   DarkBookTerm(); 
   DarkVoiceOverTerm(); 
   DarkMenusTerm(); 
   DarkSaveGameTerm(); 
   DarkSaveTermUI();
   DarkReportTerm();
   DarkStatTerm();
}


