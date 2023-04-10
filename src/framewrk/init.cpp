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
#include <fault.h>
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
#include <breakkey.h>
#include <hchkthrd.h>
#include <iobjsys.h>
#include <scrptapi.h>
#include <scrnmode.h>

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
#include <dxload.h>
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
#include <dxwrndlg.h>
#include <cdplayer.h>
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

EXTERN BOOL CheckForCD(void);

//------------------------------------------------------------
// CONFIG INITIALIZATION
//

#define CONFIG_FILE "cam.cfg"
#define GAME_CFG_VAR "game"
#define INCLUDE_PREFIX "include_"
void AppShutdownConfig(void);

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
   static initialized = FALSE;

   if (initialized)
      return;

   initialized = TRUE;

   char monofname[256];
   monofname[0] = '\0';

#ifdef WIN32
#ifndef SHIP
   {
      bool is_mono=!config_is_defined("mono_no_screen");
      char *buf;
      // for now, we know NT cant do Mono's
      if ((buf=getenv("OS"))!=NULL)
         if (stricmp(buf,"Windows_NT")==0)
            is_mono=FALSE;
      mono_win_init(is_mono);
   }
#else
   mono_win_init(FALSE);
#endif
#else
   mono_init();
#endif

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
   char game[128]; 
   config_get_raw("game",game,sizeof(game)); 

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
   process_config_includes("editor_"INCLUDE_PREFIX); 
#endif 

   config_parse_commandline(g_argc,g_argv,NULL);

   AtExit(AppShutdownConfig);

   // now that we have config, lets go make sure we can run

   if (!config_is_defined("skip_starting_checks"))
   {
      if (!CheckForDiskspaceAndMessage(NULL,MIN_STARTUP_DISK_MB))
         Exit(1,NULL); // CheckFor does a message box

      if (!LoadDirectX())
         Exit(1,NULL);  // message already printed in LoadDirectX itself
   }

   // umm, ship only or something?
   init_monochrome();

#ifndef DONT_CHECK_CD
   // Is the CD in the drive? - in ship, we always check this
#ifndef SHIP
   if (!config_is_defined("skip_starting_checks"))
#endif
      if (!CheckForCD())
         Exit(1,NULL);
#endif

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
   CDPlayerCreate();
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

   if (config_is_defined("breakkey"))
      BreakKeyActivate(VK_F12,VK_F11);

   if (config_is_defined("heapcheck"))
      HeapCheckActivate(VK_F9);

   tm_init();

   // set default screen mode
   sScrnMode mode = { 0 }; 
   ScrnModeGetConfig(&mode,"");
   ScrnModeSetDefault(&mode); 

   DispatchInit();
   DispatchMsgAllClients(kMsgAppInit,NULL,kDispatchForward);


   //input binding stuff. load all contexts from "default.bnd" and "<game>.bnd"
   g_pInputBinder->Init (NULL, NULL);
   InitIBVars ();

   //  pop dialog box about dxdrivers
   DxWarnDlg();

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

void AppShutdownConfig(void)
{
   config_set_writable_table(ConfigWritableTable);
   config_write_file(CONFIG_FILE,write_func);
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
