///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/coremutx.cpp $
// $Author: TOML $
// $Date: 1997/07/15 21:29:19 $
// $Revision: 1.2 $
//

#ifdef _WIN32
#include <windows.h>
#include <coremutx.h>

#ifdef __WATCOMC__
#pragma initialize 1
#else
#pragma init_seg (compiler)
#endif

///////////////////////////////////////////////////////////////////////////////

static int       g_MutexInitDepth;
CRITICAL_SECTION g_CoreMutex;

///////////////////////////////////////////////////////////////////////////////

class cCoreMutexInit
{
public:
    cCoreMutexInit()
    {
        CoreMutexInit();
    }

    ~cCoreMutexInit()
    {
        CoreMutexTerm();
    }
};

///////////////////////////////////////

static cCoreMutexInit g_CoreMutexInit;

///////////////////////////////////////////////////////////////////////////////

extern void CoreMutexInit(void)
{
    g_MutexInitDepth++;
    if (g_MutexInitDepth == 1)
        InitializeCriticalSection(&g_CoreMutex);
}

///////////////////////////////////////

extern void CoreMutexTerm(void)
{
    g_MutexInitDepth--;
    if (g_MutexInitDepth == 0)
        DeleteCriticalSection(&g_CoreMutex);
}

///////////////////////////////////////

extern void CoreThreadLock(void)
{
    EnterCriticalSection(&g_CoreMutex);
}

///////////////////////////////////////

extern void CoreThreadUnlock(void)
{
    LeaveCriticalSection(&g_CoreMutex);
}

///////////////////////////////////////////////////////////////////////////////

#endif
