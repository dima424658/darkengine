#pragma once

#ifdef _WIN32
#include "../winsrc/lg/thrdtool.h"
#else

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <types.h>

#ifndef INFINITE
#define INFINITE 0xffffffffu
#endif

class cThreadLock
{
public:
   void Lock() { m_mutex.lock(); }
   void Unlock() { m_mutex.unlock(); }
   BOOL AssertOwnedByCurrentThread() { return TRUE; }
   void SetTrace(BOOL) {}

private:
   std::recursive_mutex m_mutex;
};

class cAutoLock
{
public:
   explicit cAutoLock(cThreadLock &lock) : m_lock(lock) { m_lock.Lock(); }
   ~cAutoLock() { m_lock.Unlock(); }

private:
   cThreadLock &m_lock;
};

class cThreadEvent
{
public:
   explicit cThreadEvent(BOOL manualReset = FALSE)
      : m_manualReset(manualReset), m_signaled(false) {}

   BOOL Set()
   {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_signaled = true;
      m_manualReset ? m_condition.notify_all() : m_condition.notify_one();
      return TRUE;
   }

   BOOL Reset()
   {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_signaled = false;
      return TRUE;
   }

   BOOL Pulse() { BOOL result = Set(); Reset(); return result; }
   BOOL Check() { return Wait(0); }

   BOOL Wait(uint32 timeout = INFINITE)
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      bool ready;
      if (timeout == INFINITE) {
         m_condition.wait(lock, [this] { return m_signaled; });
         ready = true;
      } else {
         ready = m_condition.wait_for(lock, std::chrono::milliseconds(timeout),
                                      [this] { return m_signaled; });
      }
      if (ready && !m_manualReset)
         m_signaled = false;
      return ready ? TRUE : FALSE;
   }

   BOOL operator!() const { return FALSE; }

private:
   BOOL m_manualReset;
   bool m_signaled;
   std::mutex m_mutex;
   std::condition_variable m_condition;
};

#endif
