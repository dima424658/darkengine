/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/init.cpp,v 1.107 2000/02/25 22:58:18 mwhite Exp $

#include <float.h>

#include <comtools.h>
#include <recapi.h>
#include <gshelapi.h>
#include <dispapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>
#include <config.h>
#include <cfg.h>
#include <mprintf.h>
#include <kbcook.h>
#include <stdlib.h>
#include <res.h>
#include <resapi.h>
#include <inpapi.h>
#include <inpinit.h>
#include <tminit.h>
#include <timer.h>
#include <resagg.h>
#include <string.h>
#include <cfgtool.h>
#ifdef _WIN32
#include <breakkey.h>
#endif
#include <iobjsys.h>
#include <scrptapi.h>
#include <scrnmode.h>
#include <mode.h>

#include <appname.h>
#include <contexts.h>
#include <init.h>
#include <2dapp.h>
#include <resapp.h>
#include <gameinfo.h>
#include <loopapp.h>
#include <uiapp.h>
#include <r3app.h>
#include <portapp.h>
#include <dispatch.h>
#include <dispbase.h>
#include <propman.h>
#include <pickgame.h>
#include <traitman.h>
#include <linkman.h>
#include <trcache.h>
#include <propag8n.h>
#include <stimuli.h>
#include <stimsens.h>
#include <reaction.h>
#include <stimul8r.h>
#include <stimsrc.h>
#include <simtime.h>
#include <gametool.h>
#include <aiapi.h>
#include <movieapi.h>
#include <contain.h>
#include <gen_bind.h>
#include <keysys.h>
#include <simman.h>
#include <questapi.h>
#include <gamestr.h>
#include <vocore.h>
#include <campaign.h>
#include <netman.h>
#include <iobjnet.h>
#include <movie.h>
#include <arqapi.h>
#include <diskfree.h>
#include <platform_services.h>
#include <dyntex.h>
#include <scrptne_.h>
#include <random.h>

#include <md.h>

// must be last header
#include <dbmem.h>

// huh?
#ifdef __WATCOMC__
#pragma warning 555 9
#endif

//------------------------------------------------------------
// CONFIG INITIALIZATION
//

#define CONFIG_FILE "cam.cfg"
#define GAME_CFG_VAR "game"
#define INCLUDE_PREFIX "include_"
void AppShutdownConfig(void);
static void AppSaveConfig(void);

static void RegisterDisplayMode(int width, int height)
{
   int mode;

   if (width < 400 || height < 300 || width > 8191 || height > 8191)
      return;
   mode = gr_register_mode(width, height, 16);
   if (mode >= 0)
      grd_mode_info[mode].flags |= GRM_CAN_WINDOW;
}

static void RegisterDisplayModes(void)
{
   sPlatformDisplayMode modes[512];
   int count;
   int i;

   // Register the union of modes exposed by every connected monitor before
   // screen-mode configuration is read.  D3D11 presents these as internal
   // render sizes, so no exclusive legacy display-mode switch is required.
   count = PlatformGetDisplayModes(kPlatformAllDisplays, modes,
                                   sizeof(modes) / sizeof(modes[0]),
                                   NULL, NULL);
   for (i = 0; i < count; ++i)
      if (modes[i].bit_depth >= 24)
         RegisterDisplayMode(modes[i].width, modes[i].height);
   RegisterDisplayMode(640, 480);
   RegisterDisplayMode(800, 600);
}

#ifdef THIEF2_GAME
static const char kUserSettingsStore[] = "Thief2";

static void LoadUserVideoSettings(void)
{
   uint32 version;
   uint32 width;
   uint32 height;
   uint32 depth;
   uint32 fullscreen;
   uint32 fit;
   uint32 gamma_milli;

   if (PlatformReadUserUInt(kUserSettingsStore, "VideoSettingsVersion",
                            &version) &&
       (version == 1 || version == 2))
   {
      if (PlatformReadUserUInt(kUserSettingsStore, "GammaMilli",
                               &gamma_milli) &&
          gamma_milli >= 100 && gamma_milli <= 4000)
      {
         float gamma = gamma_milli / 1000.0f;
         config_set_float_from_var("gamma", gamma);
      }

      if (PlatformReadUserUInt(kUserSettingsStore, "ScreenWidth", &width) &&
          PlatformReadUserUInt(kUserSettingsStore, "ScreenHeight", &height) &&
          width > 0 && height > 0)
      {
         int dimensions[2] = { (int)width, (int)height };
         config_set_value("game_screen_size", CONFIG_INT_TYPE,
                          dimensions, 2);
      }

      if (PlatformReadUserUInt(kUserSettingsStore, "ScreenDepth", &depth) &&
          depth > 0)
         config_set_int("game_screen_depth", (int)depth);
      if (PlatformReadUserUInt(kUserSettingsStore, "Fullscreen", &fullscreen))
         config_set_int("game_full_screen", fullscreen != 0);
      if (version >= 2 &&
          PlatformReadUserUInt(kUserSettingsStore, "Fit", &fit))
         config_set_int("game_screen_fit", fit != 0);
   }
}
#else
static void LoadUserVideoSettings(void)
{
}
#endif

void LGAPI CoreEngineSaveVideoSettings(void)
{
#ifdef THIEF2_GAME
   uint32 version = 2;
   uint32 width;
   uint32 height;
   uint32 depth;
   uint32 fullscreen;
   uint32 fit;
   uint32 gamma_milli;
   int dimensions[2];
   int count = 2;
   int value;
   float gamma = 1.0f;

   if (config_get_float("gamma", &gamma))
   {
      gamma_milli = (uint32)(gamma * 1000.0f + 0.5f);
      PlatformWriteUserUInt(kUserSettingsStore, "GammaMilli", gamma_milli);
   }

   if (config_get_value("game_screen_size", CONFIG_INT_TYPE,
                        dimensions, &count) && count == 2)
   {
      width = dimensions[0];
      height = dimensions[1];
      PlatformWriteUserUInt(kUserSettingsStore, "ScreenWidth", width);
      PlatformWriteUserUInt(kUserSettingsStore, "ScreenHeight", height);
   }

   if (config_get_int("game_screen_depth", &value))
   {
      depth = value;
      PlatformWriteUserUInt(kUserSettingsStore, "ScreenDepth", depth);
   }
   if (config_get_int("game_full_screen", &value))
   {
      fullscreen = value != 0;
      PlatformWriteUserUInt(kUserSettingsStore, "Fullscreen", fullscreen);
   }
   if (config_get_int("game_screen_fit", &value))
   {
      fit = value != 0;
      PlatformWriteUserUInt(kUserSettingsStore, "Fit", fit);
   }

   PlatformWriteUserUInt(kUserSettingsStore, "VideoSettingsVersion", version);
#endif
}

//----------------------------------------

static uint app_read_cfg(char* )
{
   return CONFIG_DFT_HI_PRI;
}


//------------------------------------------------------------
// MONOCHROME
//


static void init_monochrome()
{
   static BOOL initialized = FALSE;

   if (initialized)
      return;

   initialized = TRUE;

   char monofname[256];
   monofname[0] = '\0';

   mono_win_init(FALSE);

   mono_clear();
   mono_set_flags(MONO_FLG_WRAPCLEAR,NULL);
   config_get_raw("monolog",monofname,256);
   if (strcmp(monofname,"")==0)
      strcpy(monofname,"monolog.txt");
   if (config_is_defined("monolog"))
      mono_logon(monofname, MONO_LOG_NEW, MONO_LOG_ALLWIN);
   mono_to_debugger = !config_is_defined("nomonodebug");
}

//------------------------------------------------------------
// GAME SELECTION
//

static void pick_game(void)
{
   char buf[80];
   if (config_get_raw(GAME_CFG_VAR,buf,sizeof(buf)))
      AppSelectGame(buf);  // select the game we named
   else
      AppSelectGame(NULL); // select the default game

#ifndef SHIP
   // for programmers only, load prog.cfg so we can find game.cfg and other
   // stuff 
   config_load("prog.cfg");
#endif 

   //
   // Include "game.cfg" 
   //
   char game[256]; 
   
   if (config_get_raw("game", game, sizeof(game)) == FALSE)
       CriticalMsg("\"game\" entry not found");

   // punt trailing space
   char* s; 
   for (s = game + strlen(game) - 1; s >= game && isspace(*s); s--)
      *s = '\0'; 
   s++; 
   strcat(game,".cfg");
   
   char path[256]; 
   Verify(find_file_in_config_path(path,game,"include_path")); 
   config_load(path); 
}



//------------------------------------------------------------
// CUSTOM PURE-VIRTUAL TRAP

#if defined(_MSC_VER)
EXTERN int __cdecl _purecall(void)
{
   CriticalMsg("Pure-virtual function call!");
   exit(1);
   return 0;
}
#endif

//------------------------------------------------------------
// DEBUG/SPEW SUPPORT
//

#ifdef DBG_ON
int KeyGetch ()
{
   ushort code;

   kb_flush_bios();

   while (TRUE)
   {
      if (kb_get_cooked (&code) && (code & KB_FLAG_DOWN))
      {
         return code ^ KB_FLAG_DOWN;
      }
   }
}
#endif

#ifndef SHIP 
#define ScriptPrintString mprintf
#else 
#define ScriptPrintString NULL
#endif

//------------------------------------------------------------
// MINIMUM DISK SPACE
//

#define MIN_STARTUP_DISK_MB 35

//------------------------------------------------------------
// CORE ENGINE AGGREGATE OBJECT CREATION
//

tResult LGAPI CoreEngineCreateObjects(int argc, const char *argv[])
{
   char buffer[40];

   g_argv = argv;
   g_argc = argc;

   //
   // config sys initializes NOW so that it can be used to create objects
   //
   config_init();
   config_read_file(CONFIG_FILE,app_read_cfg);

   pick_game();

   // must come after pick_game
   process_config_includes(INCLUDE_PREFIX);
#ifdef EDITOR
   process_config_includes("editor_" INCLUDE_PREFIX); 

   // Some retail/NewDark installs ship the editor menu definitions as the
   // optional sample file instead of including them from DromEd.cfg. Prefer
   // a user-provided menus.cfg, then make the shipped sample the fallback.
   if (!config_is_defined("menu_edit"))
   {
      config_load("menus.cfg");
      if (!config_is_defined("menu_edit"))
         config_load("menus-sample.cfg");
   }
#endif 

   // Keep runner-specific video preferences outside the retail data folder.
   // Command-line values are parsed afterward and therefore still win.
   LoadUserVideoSettings();

   config_parse_commandline(g_argc,g_argv,NULL);

   AtExit(AppShutdownConfig);

   // now that we have config, lets go make sure we can run

   if (!config_is_defined("skip_starting_checks"))
   {
      if (!CheckForDiskspaceAndMessage(NULL,MIN_STARTUP_DISK_MB))
         Exit(1,NULL); // CheckFor does a message box

   }

   // umm, ship only or something?
   init_monochrome();

   //
   //  Next let's do fault
   //

#ifdef FAULT
   if (!config_is_defined("nofault"))
      ex_startup(EXM_DIVIDE_ERR);
#endif

#if defined(_MSC_VER) && !defined(SHIP)
   int cw = _controlfp( 0, 0 );
   
   if (config_is_defined("fp_fault_over"))
      cw &= ~EM_OVERFLOW;
   if (config_is_defined("fp_fault_under"))
      cw &= ~EM_UNDERFLOW;
   if (config_is_defined("fp_fault_divzero"))
      cw &= ~EM_ZERODIVIDE;
   if (config_is_defined("fp_fault_denorm"))
      cw &= ~EM_DENORMAL;
      
   _controlfp( cw, MCW_EM );
#endif

   //
   // From this point on, all we're doing is adding COM objects to the
   // app aggregate.  They initialize in constrained priority order.
   //


   // Get display device from config
   eDisplayDeviceKind dispkind = kDispDebug;
   config_get_int("display",&dispkind);

   // Get other options from config
   int opt = kGameShellDefault & ~(kLockFrame | kFlushOnEndFrame);
#ifdef EDITOR
   // The editor is a desktop application.  Use the native pointer instead of
   // the legacy framebuffer cursor, which is not reliably presented while an
   // otherwise-idle D3D11 viewport is on screen.
   opt |= kShowNativeCursor;
#endif
   if (config_is_defined("multithread"))
      opt |= kMultithreadedShell;

   // Use the convenience macro to create all the basic game components
   CoreGameLibrariesCreate(APPNAME, argc, argv, dispkind, opt);
   ScriptManCreate(GetSimTime, ScriptPrintString);
   Gr2dCreate();
   ResSysCreate();
   LoopManagerCreate();
   LoopAppCreate();
   uiSysCreate();
   r3SysCreate();
   PortalSysCreate();
   InputManagerCreate();
   InputCreate();
   ResCreate();
   Res2Create();
   PropertyManagerCreate();
   LinkManagerCreate();
   TraitManagerCreate();
   ObjectSystemCreate();
   DonorCacheCreate();
   PropagationCreate();
   StimuliCreate();
   StimSensorsCreate();
   ReactionsCreate();
   StimulatorCreate();
   StimSourcesCreate();
   GameToolsCreate(); 
   AIManagerCreate();
   MoviePlayer1Create(); 
   SimManagerCreate(); 
   QuestDataCreate();
   GameStringsCreate(); 
   VoiceOverCreate(); 
   CampaignCreate(); 
   InputBinderCreate (&g_pInputBinder);
   AsyncReadQueueCreate();
   DynTextureCreate();

   // @TODO: move these out to an "engine features" app object creation function,
   // that is called by the systems that use engfeat? 
   ContainSysCreate();
   KeySysCreate();
#ifdef NEW_NETWORK_ENABLED
   NetManagerCreate();
   ObjectNetworkingCreate();
   ScriptNetworkingCreate();
#endif

   ResSharedCacheCreate();


   // start recording or playing back
   if (config_get_raw("record",buffer,38)) {
      RecorderCreate(kRecRecord, buffer);
   } else if (config_get_raw("playback",buffer,38)) {
      RecorderCreate(kRecPlayback, buffer);
   }

   // set the API switch 
   md_use_lgd3d();

   return NOERROR;
}


//------------------------------------------------------------
// CORE INIT FUNCTION
//
#ifdef THIEF2_GAME
static void SetSupportedScreenSize(const char *size_var, const char *depth_var,
                                   int native_width, int native_height)
{
   int dimensions[2] = { native_width, native_height };
   int count = 2;
   int depth = 16;

   config_get_value(size_var, CONFIG_INT_TYPE, dimensions, &count);
   config_get_int(depth_var, &depth);
   if (gr_find_closest_registered_mode(dimensions[0], dimensions[1], depth,
                                       &dimensions[0], &dimensions[1]) < 0)
   {
      dimensions[0] = 640;
      dimensions[1] = 480;
   }
   config_set_value(size_var, CONFIG_INT_TYPE, dimensions, 2);
}
#endif

tResult LGAPI CoreEngineAppInit()
{
   pGameShell = AppGetObj(IGameShell);

   init_monochrome();

#ifdef WE_CARED_ABOUT_DBG
   DbgInit();
   DbgInstallGetch (KeyGetch);

   if (config_is_defined ("dbg"))
   {
      DbgMonoConfig ();
   }
#endif

#ifdef _WIN32
   if (config_is_defined("breakkey"))
      BreakKeyActivate(VK_F12,VK_F11);
#endif

   tm_init();

   // D3D11 presents these as render-canvas sizes instead of requesting
   // obsolete exclusive desktop modes.  Both flavors of the shared project,
   // including DromEd, need the same compatibility mode list.
   RegisterDisplayModes();

#ifdef THIEF2_GAME
   // The modern lgd3d implementation uses D3D11. Prefer it for new runner
   // configurations while retaining an explicit user override.
   if (!config_is_defined("game_hardware"))
      config_set_int("game_hardware", 1);

   // The D3D11 scene is 32-bit, while the retained 2D UI/movie canvas is
   // 16-bit. Normalize NewDark configuration values to that canvas format.
   {
      int depth;
      if (!config_get_int("screen_depth", &depth) || depth > 16)
         config_set_int("screen_depth", 16);
      if (!config_get_int("game_screen_depth", &depth) || depth > 16)
         config_set_int("game_screen_depth", 16);
   }

   // Respect an explicit user setting. Otherwise start at the native size of
   // the display under the pointer, which is the best available "current"
   // display before the game creates its window.
   {
      int native_width;
      int native_height;

      PlatformGetDisplayModes(kPlatformCurrentDisplay, NULL, 0,
                              &native_width, &native_height);

      SetSupportedScreenSize("screen_size", "screen_depth",
                             native_width, native_height);
      SetSupportedScreenSize("game_screen_size", "game_screen_depth",
                             native_width, native_height);
   }
#endif

   // set default screen mode
   sScrnMode mode = { 0 }; 
   ScrnModeGetConfig(&mode,"");
   ScrnModeSetDefault(&mode); 

   DispatchInit();
   DispatchMsgAllClients(kMsgAppInit,NULL,kDispatchForward);


   //input binding stuff. load all contexts from "default.bnd" and "<game>.bnd"
   g_pInputBinder->Init (NULL, NULL);
   InitIBVars ();

   // initialize the random number lib
   // note fullwise: we retardedly could be calling this twice,
   // since the net lib wants to init it as well
   // if we didn't suck we would give this a better seed
   RandInit(tm_get_millisec());

   return NOERROR;
}

//------------------------------------------------------------
// CORE ENGINE APP EXIT
//

tResult LGAPI CoreEngineAppExit()
{
   MovieOnExit();
   g_pInputBinder->Term ();
   DispatchMsgAllClients(kMsgAppTerm,NULL,kDispatchReverse);
   DispatchShutdown();
   SafeRelease(pGameShell);
   return NOERROR;
}


//------------------------------------------------------------
// Config Sys Shutdown
//

static config_write_spec ConfigWritableTable[] =
{
   { NULL, }
};

static bool write_func(char* filename, char* var)
{
   if (config_write_to_same_file(filename,var))
      return TRUE;
   else
   {
      char buf[2]; // don't really need the whole filename
      // did the var come from a file
      config_get_origin(var,buf,sizeof(buf));
      if (buf[0] == '\0')
         return TRUE;
   }
   return config_default_writable(filename,var);
}

static bool g_ConfigSaved = FALSE;

static void AppSaveConfig(void)
{
   if (g_ConfigSaved)
      return;

   config_set_writable_table(ConfigWritableTable);
   config_write_file(CONFIG_FILE,write_func);
   g_ConfigSaved = TRUE;
}

void AppShutdownConfig(void)
{
   static bool shutdown_complete = FALSE;

   if (shutdown_complete)
      return;
   shutdown_complete = TRUE;

   AppSaveConfig();
   config_shutdown();
}

//------------------------------------------------------------
// INITIALIZATION GLOBALS
//

const char ** g_argv;
int g_argc;

sLoopInstantiator* gPrimordialMode = NULL;




/*
Local Variables:
typedefs:("config_write_spec" "uint" "ushort")
End:
*/
