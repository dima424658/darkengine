#include <win32_platform.h>

#include <algorithm>
#include <mmsystem.h>

#include <appagg.h>
#include <dispapi.h>
#include <lgsndi.h>
#include <sndplatform.h>
#include <wdispapi.h>

namespace
{
const UINT kBestTimerResolutionMs = 10;
SndTimerCallback gTimerCallback;
void *gTimerUser;

void CALLBACK TimerThunk(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
   if (gTimerCallback != NULL)
      gTimerCallback(gTimerUser);
}
}

uint32 SndTimeMs(void)
{
   return timeGetTime();
}

uint32 SndStartPeriodicTimer(uint32 periodMs, SndTimerCallback callback, void *user)
{
   TIMECAPS caps = {};
   if (callback == NULL || timeGetDevCaps(&caps, sizeof(caps)) != TIMERR_NOERROR)
      return 0;

   const UINT resolution =
      std::min(std::max(caps.wPeriodMin, kBestTimerResolutionMs), caps.wPeriodMax);
   if (timeBeginPeriod(periodMs) != TIMERR_NOERROR)
      return 0;

   gTimerCallback = callback;
   gTimerUser = user;
   const uint32 timerId = timeSetEvent(periodMs, resolution, TimerThunk, 0,
                                       TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
   if (timerId == 0)
   {
      gTimerCallback = NULL;
      gTimerUser = NULL;
      timeEndPeriod(periodMs);
   }
   return timerId;
}

void SndStopPeriodicTimer(uint32 timerId, uint32 periodMs)
{
   if (timerId != 0)
      timeKillEvent(timerId);
   gTimerCallback = NULL;
   gTimerUser = NULL;
   timeEndPeriod(periodMs);
}

void SndPrioritizeStreamingThread(void)
{
   SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

void cSndMixer::InitPlatformState(void)
{
   if (mpDisplayDevice == NULL)
      mpDisplayDevice = AppGetObj(IDisplayDevice);
   if (mpWinDisplayDevice == NULL)
      mpWinDisplayDevice = AppGetObj(IWinDisplayDevice);
}

void cSndMixer::ReleasePlatformState(void)
{
   SafeRelease(mpDisplayDevice);
   SafeRelease(mpWinDisplayDevice);
}

int cSndMixer::BlockDisplay(void)
{
   if (mpDisplayDevice == NULL || mpWinDisplayDevice == NULL)
      return 0;
   mpWinDisplayDevice->WaitForMutex();
   return mpDisplayDevice->BreakLock();
}

void cSndMixer::ReleaseDisplay(int cookie)
{
   if (mpDisplayDevice != NULL && mpWinDisplayDevice != NULL)
   {
      mpDisplayDevice->RestoreLock(cookie);
      mpWinDisplayDevice->ReleaseMutex();
   }
}
