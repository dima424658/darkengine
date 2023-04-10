/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/framewrk/hchkthrd.cpp,v $
// $Author: CCAROLLO $
// $Date: 1998/02/13 09:43:10 $
// $Revision: 1.2 $
//

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <errno.h>

#include <lg.h>
#include <config.h>
#include <breakkey.h>
#include <hchkthrd.h>

#include <mprintf.h>

#include <dbmem.h>

static int  g_ToggleKey;
static int  g_ToggleStatus = TRUE;
static BOOL g_fHeapCheckActive = FALSE;
static BOOL g_fHeapCheckThread = TRUE;
static int  g_HeepSleep=0;

#define VISIBLE_CHECKS
#ifdef VISIBLE_CHECKS
static int *g_MonoScreen = (int *)(0xb0000+((80-3)*2));
#define VisibleCheck(x) *g_MonoScreen=0x43##x##43##x
#else
#define VisibleCheck(x)
#endif

static unsigned __stdcall HeapCheckPoll(void *)
{
   g_fHeapCheckActive = TRUE;
   while (g_fHeapCheckActive)
   {
      if (GetAsyncKeyState(g_ToggleKey)&1)
         g_ToggleStatus = !g_ToggleStatus;
      if (g_ToggleStatus)
      {
         VisibleCheck(03);
         _heapchk();
         VisibleCheck(fa);
      }
      else
         VisibleCheck(02);
      Sleep(g_HeepSleep);
   }
   return 0;
}

void LGAPI HeapCheckActivate(int toggleKey)
{
   unsigned ignored;
   g_ToggleKey = toggleKey;
   config_get_int("heapsleep",&g_HeepSleep);
   if (!g_fHeapCheckActive)
      _beginthreadex(NULL, 0, HeapCheckPoll, NULL, 0, &ignored);
}

void LGAPI HeapCheckEnd()
{
   g_fHeapCheckActive = FALSE;
}

#endif
