/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptmm.h,v 1.13 1998/07/30 20:57:30 TOML Exp $
//
//
//

#ifndef __SCRPTMM_H
#define __SCRPTMM_H

#include <multparm.h>

#ifndef SCRIPT
#error ("scrptmm.h only valid for use in scripts, not in app");
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Script DLL main module magic (please ignore)
//

#ifdef SCRIPT_MAIN_MODULE

// @TBD (toml 12-05-97): Rework this after real DLL/Library issues
// have been solved

#include <allocovr.h>
#include <scrptmem.h>

cScriptModuleAlloc g_Malloc;
IAllocator * g_pMalloc;
tScriptPrintFunc g_pPrint;
EXTERN IMalloc * g_pAppAlloc = NULL;


EXTERN int LGAPI HeapInit()
{
   Assert_(g_pMalloc);
   return !!g_pMalloc;
}

EXTERN void LGAPI HeapTerm()
{
}

extern "C"
{

void *(*f_malloc)(size_t size) = malloc;
void *(*f_realloc)(void *p, size_t size) = realloc;
void (*f_free)(void *p) = free;
size_t (*f_msize)(void *p) = _msize;
void *(*f_malloc_db)(size_t size, const char *, int) = malloc_db;
void *(*f_realloc_db)(void *p, size_t size, const char *, int) = realloc_db;
void (*f_free_db)(void *p, const char *, int) = free_db;

void *MallocSpew(size_t size, const char *file, int line)
{
   void *p;
   p = (*f_malloc_db)(size, file, line);

   return(p);
}

}
#include <pool.h>
//
// Create a pool of fixed sized objects
//
EXTERN HPOOL LGAPI PoolCreate(size_t s)
{
   Assert_(g_pMalloc);
   return (HPOOL)s;
}

//
// Allocate an item from the pool
//
EXTERN void * LGAPI PoolAlloc(HPOOL hPool)
{
   Assert_(g_pMalloc);
   return malloc((size_t)hPool);
}

//
// Free an item from the pool
//
EXTERN void LGAPI PoolFree(HPOOL pPool, void *pData)
{
   Assert_(g_pMalloc);
   free(pData);
}

//
// Destroy the pool
//
EXTERN void LGAPI PoolDestroy(HPOOL pPool)
{

}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Main module data and entry point
//

#ifdef SCRIPT_MAIN_MODULE

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

#pragma comment(linker, "/entry:_ScriptDllMainCRTStartup")

EXTERN BOOL __stdcall _CRT_INIT(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        void *  lpreserved
        );

static HANDLE g_hDll;
static void * g_pStartupReserved;

BOOL __stdcall _ScriptDllMainCRTStartup(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        void *  lpreserved
        )
{
   BOOL retcode;

   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hDll             = hDllHandle;
         g_pStartupReserved = lpreserved;
         retcode = TRUE;
         break;

      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
         retcode = _CRT_INIT(hDllHandle, dwReason, lpreserved);
         if (dwReason == DLL_PROCESS_DETACH) {

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   The extra memory tracking layer has been removed for EEE since it
   caused a heap trash which has been difficult to track, associated
   with dropping any script module.  We die right here with an invalid
   block on the free list.

   (Mat 5/24/98)

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef NO_ATTACH_ALLOC
            g_Malloc.Remove();         // restores g_pMalloc
#endif 
            SafeRelease(g_pMalloc);
         }
         break;
   }

   return retcode;
}

cMultiParm::uScratch cMultiParm::gm_scratch;

void cMultiParm::VecToTempString() const
{
   sprintf(gm_scratch.szBuf, "%f, %f, %f", pVec->x, pVec->y, pVec->z);
}

#include <hshsttem.h>
template cStrHashSet<cScript::sScrMsgMapEntry *>;

#include <initguid.h>
#include <scrguid.h>
#include <allcguid.h>

cScriptModule g_ScriptModule;
IScriptMan * g_pScriptMan;

EXTERN
__LGEXPORT
BOOL LGAPI ScriptModuleInit(const char * pszName,
                            IScriptMan * pScriptMan,
                            tScriptPrintFunc pPrint,
                            IUnknown *   pMalloc,
                            IScriptModule ** ppModule)
{
   pMalloc->QueryInterface(IID_IDebugMalloc, (void **) &g_pMalloc);
   if (!g_pMalloc)
      pMalloc->QueryInterface(IID_IMalloc, (void **) &g_pMalloc); // ! (toml 06-14-98)
   
   g_pScriptMan = pScriptMan;
   g_pAppAlloc = g_pMalloc;
#ifndef NO_ATTACH_ALLOC
   g_Malloc.cScriptModuleAlloc::cScriptModuleAlloc();   // force        
   g_Malloc.Attach((IMalloc **)&g_pMalloc);
#endif 

   _CRT_INIT(g_hDll, DLL_PROCESS_ATTACH, g_pStartupReserved);

   g_ScriptModule.SetName(pszName);
   g_pPrint = pPrint;

   // We don't AddRef as the module is initialized with a reference.
   *ppModule = &g_ScriptModule;

   return TRUE;
}

#endif

///////////////////////////////////////////////////////////////////////////////
#endif /* !__SCRPTMM_H */
