/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptmod.cpp,v 1.4 2000/02/22 19:49:47 toml Exp $
//
// This normally wouldn't want to be in its own file, but for the dependence on
// windows.h
//

#include <win32_platform.h>
#include <scrptman.h>
#include <allocapi.h>
#include <filespec.h>
#include <mprintf.h>
#include <osm_emu/osm32mod.h>
#include <squirrel/squirrelhost.h>

#include <stdlib.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////

BOOL cScriptMan::LoadModule(const cFileSpec & fsModule, sScrModuleInfo * pInfo)
{
   LGALLOC_AUTO_CREDIT();

   pInfo->hModule = NULL;
   pInfo->pModule = NULL;

   const char *profile = getenv("DARK_GAME_PROFILE");
   if (getenv("DARK_OSM32_TRACE"))
   {
      fprintf(stderr, "Script module '%s' profile='%s'\n",
              fsModule.GetName(), profile ? profile : "");
      fflush(stderr);
   }
   const BOOL modernProvider =
      _stricmp(fsModule.GetFileName(), "squirrel.osm") == 0;
   if (modernProvider)
   {
      // Squirrel is an engine-owned, native provider.  It deliberately does
      // not load NewDark's PE32 squirrel.osm: that module selects retail
      // Thief 1/2 service vtables and is not ABI-safe in this shared host.
      if (CreateSquirrelScriptModule(fsModule.GetFileName(), this, m_pfnPrint,
                                     &pInfo->pModule))
         return TRUE;
      mprintf("Built-in Squirrel 3.2 provider failed to initialize\n");
      return FALSE;
   }

   const char *modulePath = fsModule.GetName();
   if (profile && _stricmp(profile, "thief2") != 0)
   {
      // The Gold/TDP modules expose an older C++ ABI which is unsafe to
      // consume as the Thief II IScriptModule interface, even in a 32-bit
      // host.  Run them through the same 32-bit service bridge used by x64.
      std::string error;
      if (LoadOsm32Module(modulePath, fsModule.GetFileName(), this,
                          &pInfo->pModule, &error))
         return TRUE;
      mprintf("Script module '%s' failed to load: %s\n",
              fsModule.GetFileName(),
              error.empty() ? "unknown OSM32 error" : error.c_str());
      if (getenv("DARK_OSM32_TRACE"))
      {
         fprintf(stderr, "Script module '%s' failed to load: %s\n",
                 fsModule.GetFileName(),
                 error.empty() ? "unknown OSM32 error" : error.c_str());
         fflush(stderr);
      }
      return FALSE;
   }

   if ((pInfo->hModule = LoadLibraryA(modulePath)) != 0)
   {
      tScriptModuleInitFunc pfnInit
         = (tScriptModuleInitFunc)GetProcAddress((HMODULE)pInfo->hModule,
                                                 "_ScriptModuleInit@20");
      if (pfnInit && (*pfnInit)(fsModule.GetFileName(), this, m_pfnPrint,
                                g_pMalloc, &pInfo->pModule))
         return TRUE;
      FreeLibrary((HMODULE)pInfo->hModule);
      pInfo->hModule = NULL;
   }

   std::string error;
   if (LoadOsm32Module(modulePath, fsModule.GetFileName(), this,
                       &pInfo->pModule, &error))
      return TRUE;

   mprintf("Script module '%s' failed to load: %s\n",
           fsModule.GetFileName(),
           error.empty() ? "native and OSM32 loaders rejected the module" :
                           error.c_str());
   if (getenv("DARK_OSM32_TRACE"))
   {
      fprintf(stderr, "Script module '%s' failed to load: %s\n",
              fsModule.GetFileName(),
              error.empty() ? "native and OSM32 loaders rejected the module" :
                              error.c_str());
      fflush(stderr);
   }

   return FALSE;
}

///////////////////////////////////////

void cScriptMan::FreeModule(sScrModuleInfo * pInfo)
{
   SafeRelease(pInfo->pModule);
   if (pInfo->hModule)
   {
      FreeLibrary((HMODULE)pInfo->hModule);
      pInfo->hModule = NULL;
   }
}

///////////////////////////////////////////////////////////////////////////////
