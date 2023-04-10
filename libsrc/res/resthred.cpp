///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resthred.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:15:23 $
// $Revision: 1.4 $
//

#ifdef _WIN32
#include <windows.h>
#include <lg.h>
#include <thrdtool.h>
#include <resthred.h>

cThreadLock g_ResThreadLock;
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
#endif
