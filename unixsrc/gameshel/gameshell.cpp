#pragma pack(push, 8)
#include <chrono>
#include <cstdlib>
#pragma pack(pop)

#include <aggmemb.h>
#include <appagg.h>
#include <appapi.h>
#include <dispapi.h>
#include <gshelapi.h>

#include "../render/runtime.h"

class cUnixGameShell : public IGameShell
{
   DECLARE_SIMPLE_AGGREGATION(cUnixGameShell);
public:
   cUnixGameShell(IUnknown *outer, int flags) : m_flags(flags), m_epoch(clock::now())
   { INIT_AGGREGATION_1(outer, IID_IGameShell, this, kPriorityLibrary, nullptr); }
   void BeginFrame() override
   {
      DarkRenderBeginApplicationFrame();
      PumpEvents(kPumpAll);
      if(m_flags&kLockFrame)DisplayLock();
   }
   void EndFrame() override
   {
      if(m_flags&kLockFrame)DisplayUnlock();
      if(m_flags&kFlipOnEndFrame){auto*d=AppGetObj(IDisplayDevice);if(d){d->PageFlip();d->Release();}}
      else if(m_flags&kFlushOnEndFrame)DisplayFlush();
      DarkRenderEndApplicationFrame();
   }
   HRESULT BringToFront() override { return S_OK; }
   HRESULT Obsolete1() override { return S_OK; }
   void PumpEvents(int) override
   {
      if(DarkRenderPumpEvents()&&(m_flags&kExitOnQuitEvent)){auto*app=AppGetObj(IApplication);if(app)app->Quit();}
   }
   int DisplayLock() override { auto*d=AppGetObj(IDisplayDevice);int n=d?d->Lock():0;SafeRelease(d);return n; }
   int DisplayUnlock() override { auto*d=AppGetObj(IDisplayDevice);int n=d?d->Unlock():0;SafeRelease(d);return n; }
   void DisplayFlush() override { auto*d=AppGetObj(IDisplayDevice);if(d)d->Flush();SafeRelease(d); }
   unsigned long GetTimerTick() override
   {
      return m_offset+static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now()-m_epoch).count());
   }
   void SetTimer(unsigned long value) override { m_offset=value;m_epoch=clock::now(); }
   BOOL ShowHelp(const char *,uint,ulong) override { return FALSE; }
   BOOL SetOSCursor(sGrBitmap *,const Point *) override { return FALSE; }
   BOOL ShowOSCursor(BOOL) override { return FALSE; }
   void SetCursorCallback(tGSCursorCallbackFunc callback) override { m_cursor=callback; }
   BOOL Exec(const char *command) override { return command&&std::system(command)==0; }
   BOOL SetFlags(int flags) override
   {
      m_flags=flags;return TRUE;
   }
   BOOL GetFlags(int *flags) override { if(!flags)return FALSE;*flags=m_flags;return TRUE; }
private:
   using clock=std::chrono::steady_clock;
   int m_flags; unsigned long m_offset{}; clock::time_point m_epoch; tGSCursorCallbackFunc m_cursor{};
};
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cUnixGameShell);

extern "C" tResult LGAPI _GameShellCreate(REFIID, IGameShell **, IUnknown *outer, int flags, int)
{ return new cUnixGameShell(outer,flags)?S_OK:E_FAIL; }
