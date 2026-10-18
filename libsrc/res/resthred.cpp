///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resthred.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:15:23 $
// $Revision: 1.4 $
//

#include <lg.h>
#include <platform_services.h>
#include <resthred.h>

class cResourceThreadLock
{
public:
   cResourceThreadLock() { Verify(PlatformMutexInit(&m_Mutex)); }
   ~cResourceThreadLock() { PlatformMutexTerm(&m_Mutex); }
   void Lock() { PlatformMutexLock(&m_Mutex); }
   void Unlock() { PlatformMutexUnlock(&m_Mutex); }

private:
   sPlatformMutex m_Mutex;
};

static cResourceThreadLock g_ResThreadLock;
#ifdef RES_THREAD_TRACE
static int g_iLock;
#endif

void _ResThreadLock()
{
   g_ResThreadLock.Lock();

#ifdef RES_THREAD_TRACE
   g_iLock++;
   LogMsg1("   %d", g_iLock);
#endif
}

void _ResThreadUnlock()
{
#ifdef RES_THREAD_TRACE
   g_iLock--;
   LogMsg1("   %d", g_iLock);
#endif
   g_ResThreadLock.Unlock();
}
