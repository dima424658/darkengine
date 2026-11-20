#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

#include <sndplatform.h>

namespace {
struct Timers
{
   std::mutex mutex;
   std::unordered_map<uint32, std::thread> threads;
   bool stopping = false;

   ~Timers()
   {
      std::unordered_map<uint32, std::thread> pending;
      {
         std::lock_guard<std::mutex> lock(mutex);
         stopping = true;
         pending.swap(threads);
      }
      for (auto &entry : pending)
      {
         if (entry.second.get_id() == std::this_thread::get_id())
            entry.second.detach();
         else
            entry.second.join();
      }
   }
};

Timers g_timers;
std::atomic<uint32> g_nextTimer{1};
}

uint32 SndTimeMs(void)
{
   using namespace std::chrono;
   return static_cast<uint32>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

uint32 SndStartPeriodicTimer(uint32 period, SndTimerCallback callback, void *user)
{
   const uint32 id = g_nextTimer++;
   std::lock_guard<std::mutex> lock(g_timers.mutex);
   if (g_timers.stopping)
      return 0;
   auto [it, inserted] = g_timers.threads.try_emplace(id);
   if (!inserted)
      return 0;

   try
   {
      it->second = std::thread([id, period, callback, user] {
         for (;;)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::max<uint32>(period, 1)));
            {
               std::lock_guard<std::mutex> lock(g_timers.mutex);
               if (g_timers.threads.find(id) == g_timers.threads.end())
                  return;
            }
            callback(user);
         }
      });
   }
   catch (...)
   {
      g_timers.threads.erase(it);
      throw;
   }
   return id;
}

void SndEndPeriodicTimer(uint32 id)
{
   std::thread timer;
   {
      std::lock_guard<std::mutex> lock(g_timers.mutex);
      auto it = g_timers.threads.find(id);
      if (it == g_timers.threads.end())
         return;

      timer = std::move(it->second);
      g_timers.threads.erase(it);
   }

   if (timer.joinable())
   {
      if (timer.get_id() == std::this_thread::get_id())
         timer.detach();
      else
         timer.join();
   }
}

void SndStopPeriodicTimer(uint32 id, uint32) { SndEndPeriodicTimer(id); }

void SndPrioritizeStreamingThread(void) {}
