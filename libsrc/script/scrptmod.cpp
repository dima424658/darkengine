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

#include <windows.h>
#include <scrptman.h>
#include <allocapi.h>
#include <filespec.h>

///////////////////////////////////////////////////////////////////////////////

BOOL cScriptMan::LoadModule(const cFileSpec & fsModule, sScrModuleInfo * pInfo)
{
   LGALLOC_AUTO_CREDIT();

   if ((pInfo->hModule = (HANDLE)LoadLibrary(fsModule.GetName())) != 0)
   {
      tScriptModuleInitFunc pfnInit
         = (tScriptModuleInitFunc)GetProcAddress((HINSTANCE)pInfo->hModule,
                                                 "_ScriptModuleInit@20");
      if (pfnInit && (*pfnInit)(fsModule.GetFileName(), this, m_pfnPrint,
                                g_pMalloc, &pInfo->pModule))
         return TRUE;
      FreeLibrary((HINSTANCE)pInfo->hModule);
      pInfo->hModule = NULL;
   }

   pInfo->pModule = NULL;
   return FALSE;
}

///////////////////////////////////////

void cScriptMan::FreeModule(sScrModuleInfo * pInfo)
{
   SafeRelease(pInfo->pModule);
   if (pInfo->hModule)
   {
      FreeLibrary((HINSTANCE)pInfo->hModule);
      pInfo->hModule = NULL;
   }
}

///////////////////////////////////////////////////////////////////////////////
