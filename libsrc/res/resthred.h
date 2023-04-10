///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/resthred.h $
// $Author: TOML $
// $Date: 1996/10/21 14:15:24 $
// $Revision: 1.4 $
//

#ifndef __RESTHRED_H
#define __RESTHRED_H

#ifdef _WIN32
EXTERN void _ResThreadLock();
EXTERN void _ResThreadUnlock();
#ifndef RES_THREAD_TRACE

#define ResThreadLock()     _ResThreadLock()
#define ResThreadUnlock()   _ResThreadUnlock()

#else

#pragma message ("Tracing thread locks");

#define ResThreadLock() \
    { \
    LogMsg2("Locking at %s, %d\n", __FILE__, __LINE__); \
    _ResThreadLock(); \
    }

#define ResThreadUnlock() \
    { \
    LogMsg2("Unlocking at %s, %d\n", __FILE__, __LINE__); \
    _ResThreadUnlock(); \
    }

#endif
#else
#define ResThreadLock()
#define ResThreadUnlock()
#endif

#if defined(__cplusplus)

class cAutoResThreadLock
{
 public:
   cAutoResThreadLock()
   {
      ResThreadLock();
   }

   ~cAutoResThreadLock()
   {
      ResThreadUnlock();
   }
};
#endif

#endif /* !__RESTHRED_H */
