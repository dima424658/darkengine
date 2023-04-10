///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memcore.h $
// $Author: TOML $
// $Date: 1998/06/10 13:58:16 $
// $Revision: 1.5 $
//

#ifndef __MEMCORE_H
#define __MEMCORE_H

#include <allocapi.h>
#include <fastmutx.h>

#ifndef SHIP
EXTERN IDebugMalloc * g_pMalloc;
#else
EXTERN IMalloc *      g_pMalloc;
#endif

class cHeapDebug;
extern cHeapDebug *   g_pHeapDebug;
extern BOOL           g_bAllocDumpLeaks;

class cMemAllocTimer;
extern cMemAllocTimer * g_pMemAllocTimer;

#if OS_MUTEX
extern void AllocThreadLock(void);
extern void AllocThreadUnlock(void);
#else

extern volatile cFastMutex g_AllocMutex;

#define AllocThreadLock()     g_AllocMutex.Lock()
#define AllocThreadUnlock()   g_AllocMutex.Unlock()

#endif

#endif /* !__MEMCORE_H */
