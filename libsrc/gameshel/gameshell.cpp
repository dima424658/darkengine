#include <chrono>
#include <cstdlib>

#include <aggmemb.h>
#include <appagg.h>
#include <appapi.h>
#include <dispapi.h>
#include <gshelapi.h>

#include <display_window.h>
#include <runtime.h>
#include "sdl_events.h"

bool DarkPumpEvents(void)
{
   return DarkPumpSDLEvents();
}

class cSDLGameShell : public IGameShell
{
   DECLARE_SIMPLE_AGGREGATION(cSDLGameShell);

public:
   cSDLGameShell(IUnknown *outer, int flags)
      : m_flags(flags), m_epoch(clock::now())
   {
      INIT_AGGREGATION_1(outer, IID_IGameShell, this, kPriorityLibrary, nullptr);
   }

   STDMETHOD_(void, BeginFrame)() override
   {
      DarkRenderBeginApplicationFrame();
      PumpEvents(kPumpAll);
      if (m_flags & kLockFrame)
         DisplayLock();
   }

   STDMETHOD_(void, EndFrame)() override
   {
      if (m_flags & kLockFrame)
         DisplayUnlock();
      if (m_flags & kFlipOnEndFrame)
      {
         auto *display = AppGetObj(IDisplayDevice);
         if (display)
         {
            display->PageFlip();
            display->Release();
         }
      }
      else if (m_flags & kFlushOnEndFrame)
         DisplayFlush();
      DarkRenderEndApplicationFrame();
   }

   STDMETHOD(BringToFront)() override { return S_OK; }
   STDMETHOD(Obsolete1)() override { return S_OK; }

   STDMETHOD_(void, PumpEvents)(int) override
   {
      if (DarkPumpEvents() && (m_flags & kExitOnQuitEvent))
      {
         auto *application = AppGetObj(IApplication);
         if (application)
         {
            application->Quit();
            application->Release();
         }
      }
   }

   STDMETHOD_(int, DisplayLock)() override
   {
      auto *display = AppGetObj(IDisplayDevice);
      const int result = display ? display->Lock() : 0;
      SafeRelease(display);
      return result;
   }

   STDMETHOD_(int, DisplayUnlock)() override
   {
      auto *display = AppGetObj(IDisplayDevice);
      const int result = display ? display->Unlock() : 0;
      SafeRelease(display);
      return result;
   }

   STDMETHOD_(void, DisplayFlush)() override
   {
      auto *display = AppGetObj(IDisplayDevice);
      if (display)
         display->Flush();
      SafeRelease(display);
   }

   STDMETHOD_(unsigned long, GetTimerTick)() override
   {
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
         clock::now() - m_epoch).count();
      return m_offset + static_cast<unsigned long>(elapsed);
   }

   STDMETHOD_(void, SetTimer)(unsigned long value) override
   {
      m_offset = value;
      m_epoch = clock::now();
   }

   STDMETHOD_(BOOL, ShowHelp)(const char *,uint,ulong) override { return FALSE; }
   STDMETHOD_(BOOL, SetOSCursor)(sGrBitmap *,const Point *) override { return FALSE; }
   STDMETHOD_(BOOL, ShowOSCursor)(BOOL) override { return FALSE; }
   STDMETHOD_(void, SetCursorCallback)(tGSCursorCallbackFunc callback) override { m_cursor=callback; }

   STDMETHOD_(BOOL, Exec)(const char *command) override
   {
      return command && std::system(command) == 0;
   }

   STDMETHOD_(BOOL, SetFlags)(int flags) override
   {
      m_flags = flags;
      SDLDisplaySetNativeCursorVisible((flags & kShowNativeCursor) != 0);
      return TRUE;
   }

   STDMETHOD_(BOOL, GetFlags)(int *flags) override
   {
      if (!flags)
         return FALSE;
      *flags = m_flags;
      return TRUE;
   }

private:
   using clock = std::chrono::steady_clock;

   int m_flags;
   unsigned long m_offset{};
   clock::time_point m_epoch;
   tGSCursorCallbackFunc m_cursor{};
};
static_assert(alignof(cSDLGameShell) >=
   alignof(std::chrono::steady_clock::time_point),
   "SDL game shell must retain native clock alignment");
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cSDLGameShell);

extern "C" tResult LGAPI _GameShellCreate(REFIID, IGameShell **, IUnknown *outer, int flags, int)
{
   return new cSDLGameShell(outer, flags) ? S_OK : E_FAIL;
}
