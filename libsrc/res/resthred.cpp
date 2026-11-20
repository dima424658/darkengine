#include <mutex>
///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resthred.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:15:23 $
// $Revision: 1.4 $
//

#include <lg.h>

#include <resthred.h>

static std::recursive_mutex g_ResThreadLock;
#ifdef RES_THREAD_TRACE
static int g_iLock;
#endif

void _ResThreadLock()
{
   g_ResThreadLock.lock();

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
   g_ResThreadLock.unlock();
}
