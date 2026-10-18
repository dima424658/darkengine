///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/coremutx.cpp $
// $Author: TOML $
// $Date: 1997/07/15 21:29:19 $
// $Revision: 1.2 $
//

#include <coremutx.h>
#include <mutex>

namespace
{
    std::recursive_mutex &CoreMutex()
    {
        static std::recursive_mutex mutex;
        return mutex;
    }
}

///////////////////////////////////////////////////////////////////////////////

extern void CoreMutexInit(void)
{
    (void)CoreMutex();
}

///////////////////////////////////////

extern void CoreMutexTerm(void)
{
}

///////////////////////////////////////

extern void CoreThreadLock(void)
{
    CoreMutex().lock();
}

///////////////////////////////////////

extern void CoreThreadUnlock(void)
{
    CoreMutex().unlock();
}

///////////////////////////////////////////////////////////////////////////////
