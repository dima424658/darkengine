///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/stktrace.cpp $
// $Author: TOML $
// $Date: 1997/10/16 13:20:37 $
// $Revision: 1.8 $
//

#include <types.h>

#if defined(_WIN32) && defined(_MSC_VER)

#include <tchar.h>
#include <windows.h>
#include <stktrace.h>
typedef DWORD MOFFSET;
typedef struct tagSTACKTRACEENTRY
    {
    WORD    dwSize;
    WORD    wSS;
    MOFFSET wBP;
    WORD    wCS;
    MOFFSET wIP;
    HMODULE hModule;
    WORD    wSegment;
    WORD    wFlags;
    } STACKTRACEENTRY;

#if 0
#define MAX_MODULE_NAME MAX_PATH

typedef struct tagMODULEENTRY
    {
    DWORD   dwSize;
    char    szModule[MAX_MODULE_NAME + 1];
    HMODULE hModule;
    WORD wcUsage;
    char szExePath[MAX_PATH + 1];
    WORD wNext;
    } MODULEENTRY;
#endif

static BOOL StackTraceCSIPFirst(STACKTRACEENTRY * lpStackTrace,
                                 WORD wSS, WORD wCS, MOFFSET wIP, MOFFSET wBP)
    {
    lpStackTrace->wSS = wSS;
    lpStackTrace->wCS = wCS;
    lpStackTrace->wIP = wIP;
    lpStackTrace->wBP = wBP;
    lpStackTrace->hModule = 0;
    lpStackTrace->wSegment = 0;
    lpStackTrace->wFlags = 0;
    return TRUE;
    }

static BOOL StackTraceNext(STACKTRACEENTRY * lpStackTrace)
    {
    MOFFSET *pBP = (MOFFSET *) lpStackTrace->wBP;
    if (!pBP || IsBadReadPtr(pBP, 4))
        return FALSE;

    lpStackTrace->wBP = pBP[0];
    lpStackTrace->wIP = pBP[1];
    lpStackTrace->hModule = 0;
    lpStackTrace->wSegment = 0;
    lpStackTrace->wFlags = 0;

    if (lpStackTrace->wIP == 0 || lpStackTrace->wIP == 0xffffffff)
      return FALSE;

    return TRUE;
    }


// Get the CS, SS, IP, and BP
static WORD     g_wCS;
static WORD     g_wSS;
static MOFFSET  g_wIP;
static MOFFSET  g_wBP;
static MOFFSET  g_wBX;
static MOFFSET  g_wAX;

//
// GetTraceParams()
//
#ifndef __WATCOMC__
    inline void GetTraceParams(void)
    {
    	__asm
    	{
        mov g_wAX, eax          ; Save AX
        mov g_wBX, ebx          ; Save BX
        mov g_wCS, cs
        mov g_wSS, ss

        mov ebx, ebp
        mov eax, ss:[ebx]     ; Get BP for caller
        mov g_wBP, eax

        mov eax, ss:[ebx+4]   ; Get return address for caller
        mov g_wIP, eax

        mov ebx, g_wBX         ; Restore BX
        mov eax, g_wBX         ; Restore AX
    	}
    }
#else
    void GetTraceParams(void);
    #pragma aux GetTraceParams =\
        "mov g_wAX, eax" \
        "mov g_wBX, ebx" \
        "mov g_wCS, cs" \
        "mov g_wSS, ss" \
        "mov ebx, ebp" \
        "mov eax, ss:[ebx]" \
        "mov g_wBP, eax" \
        "mov eax, ss:[ebx+4]" \
        "mov g_wIP, eax" \
        "mov ebx, g_wBX" \
        "mov eax, g_wAX" \
        modify [eax ebx];
#endif

static int __stdcall DoFillStackArray(int Skip, int MaxFrames, void **p)
    {
    static BOOL fTracing;

    if (fTracing)
        return 0;

    fTracing = TRUE;

    STACKTRACEENTRY ste;
    ste.dwSize = sizeof(STACKTRACEENTRY);

    if (!StackTraceCSIPFirst(&ste, g_wSS, g_wCS, g_wIP, g_wBP))
        {
        fTracing = FALSE;
        return 0;
        }

    for (int j = 0; j < Skip; j++)
        {
        if (!StackTraceNext(&ste))
            {
            fTracing = FALSE;
            return 0;
            }
        }

    for (int i = 0; i < MaxFrames; i++)
        p[i] = 0;

    int i;
    for (i = 0; i < MaxFrames; i++)
        {
        *p++ = (void *)(ste.wIP);
        if (!StackTraceNext(&ste))
            break;
        }

    fTracing = FALSE;
    return i;
    }

int LGAPI FillStackArray(int Skip, int MaxFrames, void **p)		// phs, 7/1/96
    {
    STACKTRACEENTRY ste;
    ste.dwSize = sizeof(STACKTRACEENTRY);

    GetTraceParams();

    return DoFillStackArray(Skip, MaxFrames, p);
    }

int LGAPI FillThreadStackArray(HANDLE hThread, int Skip, int MaxFrames, void **p)	// phs, 7/1/96
    {
    CONTEXT threadContext;
    threadContext.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(hThread, &threadContext))
        {
        g_wCS = (WORD) threadContext.SegCs;
        g_wSS = (WORD) threadContext.SegSs;
        g_wIP = threadContext.Eip;
        g_wBP = threadContext.Ebp;
        return DoFillStackArray(Skip, MaxFrames, p);
        }
    return 0;
    }

#else

#pragma off (unreferenced)
int FillStackArray(int Skip, int MaxFrames, void **p)
    {
    return 0;
    }
#pragma on (unreferenced)

#endif
