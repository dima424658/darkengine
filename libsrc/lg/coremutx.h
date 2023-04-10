///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/coremutx.h $
// $Author: TOML $
// $Date: 1997/09/28 11:56:07 $
// $Revision: 1.3 $
//
// The core mutex for central tech libraries.  This mutex is reserved strictly
// for the use by selected tech libraries.
//

#ifndef __COREMUTX_H
#define __COREMUTX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
extern void CoreMutexInit(void);
extern void CoreMutexTerm(void);
extern void CoreThreadLock(void);
extern void CoreThreadUnlock(void);
#define CORE_LOCK() cCoreLock __coreLock;

#if defined(__cplusplus)
class cCoreLock
{
 public:
   cCoreLock()
   {
      CoreThreadLock();
   }

   ~cCoreLock()
   {
      CoreThreadUnlock();
   }
};
#endif

#else

#define CoreThreadLock()
#define CoreThreadUnlock()
#define CORE_LOCK()

#endif

#ifdef __cplusplus
}
#endif

#endif /* !__COREMUTX_H */
