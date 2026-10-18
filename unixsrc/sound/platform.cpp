#pragma pack(push, 8)
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#pragma pack(pop)

#include <lgsndi.h>
#include <sndplatform.h>

namespace {
struct Timer { std::atomic<bool> active{true}; std::thread thread; };
std::mutex g_mutex;
std::unordered_map<uint32, Timer *> g_timers;
std::atomic<uint32> g_nextTimer{1};
}

uint32 SndTimeMs(void)
{
   using namespace std::chrono;
   return static_cast<uint32>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

uint32 SndStartPeriodicTimer(uint32 period, SndTimerCallback callback, void *user)
{
   Timer *timer = new Timer;
   uint32 id = g_nextTimer++;
   timer->thread = std::thread([timer, period, callback, user] {
      while (timer->active) {
         std::this_thread::sleep_for(std::chrono::milliseconds(std::max<uint32>(period, 1)));
         if (timer->active) callback(user);
      }
   });
   std::lock_guard<std::mutex> lock(g_mutex); g_timers[id] = timer; return id;
}

void SndEndPeriodicTimer(uint32 id)
{
   Timer *timer = nullptr;
   { std::lock_guard<std::mutex> lock(g_mutex); auto it = g_timers.find(id);
     if (it == g_timers.end()) return; timer = it->second; g_timers.erase(it); }
   timer->active = false;
   if (timer->thread.joinable() && timer->thread.get_id() != std::this_thread::get_id()) timer->thread.join();
   else if (timer->thread.joinable()) timer->thread.detach();
   delete timer;
}

void SndStopPeriodicTimer(uint32 id, uint32) { SndEndPeriodicTimer(id); }

void SndPrioritizeStreamingThread(void) {}
void cSndMixer::InitPlatformState(void) { mpDisplayDevice = nullptr; mpWinDisplayDevice = nullptr; }
void cSndMixer::ReleasePlatformState(void) { mpDisplayDevice = nullptr; mpWinDisplayDevice = nullptr; }
int cSndMixer::BlockDisplay(void) { return 0; }
void cSndMixer::ReleaseDisplay(int) {}
