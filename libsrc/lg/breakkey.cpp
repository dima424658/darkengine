///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/breakkey.cpp $
// $Author: TOML $
// $Date: 1996/10/10 10:07:42 $
// $Revision: 1.5 $
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>
#include <process.h>
#include <breakkey.h>

static BOOL g_fBreakPollActive;
static int g_BreakKey;
static int g_EndKey;

static unsigned __stdcall BreakPoll(void *)
{
   g_fBreakPollActive = TRUE;
   while (g_fBreakPollActive)
   {
      if (GetAsyncKeyState(g_BreakKey))
         DebugBreak();

      if (GetAsyncKeyState(g_EndKey))
         g_fBreakPollActive = FALSE;

      Sleep(0);
   }
   return 0;
}

void LGAPI BreakKeyActivate(int breakKey, int endKey)
{
   unsigned ignored;
   g_BreakKey = breakKey;
   g_EndKey = endKey;
   if (!g_fBreakPollActive)
        _beginthreadex(NULL,
                       0,
                       BreakPoll,
                       NULL,
                       0,
                       &ignored);
}

void LGAPI BreakKeyEnd()
{
   g_fBreakPollActive = FALSE;
}

#endif
