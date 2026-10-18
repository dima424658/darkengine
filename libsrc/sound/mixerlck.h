///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sound/RCS/mixerlck.h $
// $Author: dc $
// $Date: 1997/11/10 10:28:05 $
// $Revision: 1.1 $
//
// Tools for thread synchronization and management.
//
//

#ifndef __MIXERLCK_H
#define __MIXERLCK_H

#include <mutex>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMixerAutoLock
//
// a threadMutex autolock for the mixer
// really could just be cMutexAutoLock, but hey
//

class cMixerAutoLock
{
public:
    cMixerAutoLock(std::recursive_mutex & mutex);
    ~cMixerAutoLock();

private:
    std::recursive_mutex & m_mutex;

    // make copy constructor and assignment operator inaccessible
    cMixerAutoLock(const cMixerAutoLock &);
    cMixerAutoLock &operator=(const cMixerAutoLock &);
};

//////////////////////////////////////
//
// CLASS: cMixerAutoLock
//

inline
cMixerAutoLock::cMixerAutoLock(std::recursive_mutex & mutex)
  : m_mutex(mutex)
{
   m_mutex.lock();
}

///////////////////////////////////////

inline
cMixerAutoLock::~cMixerAutoLock()
{
    m_mutex.unlock();
}

#endif /* !__MIXERLCK_H */
