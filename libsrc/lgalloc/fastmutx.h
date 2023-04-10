///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/lgalloc/RCS/fastmutx.h 1.1 1998/06/10 13:58:14 TOML Exp $
//
//
//

#ifndef __FASTMUTX_H
#define __FASTMUTX_H

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
EXTERN unsigned long __declspec(dllimport) __stdcall GetCurrentThreadId(void);
EXTERN void __declspec(dllimport) __stdcall DebugBreak(void);
#else
#define GetCurrentThreadId() (0)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFastMutex
//
// An alternative to critical sections that are useful for cases when thread
// contention is rare, but a mutex is required. Instances must be declared 
// volatile.
//

class cFastMutex
{
public:
   cFastMutex()
    : m_OwnerID(0),
      m_Depth(0)
   {
   }
   
   void Lock() volatile
   {
      const ulong curID = GetCurrentThreadId();
      if (m_OwnerID != curID)
      {
         while (m_OwnerID)
            ;
         m_OwnerID = curID;
      }
      m_Depth++;
#ifdef DEBUG
      if (m_OwnerID != curID)
         DebugBreak();
#endif
   }
   
   void Unlock() volatile
   {
#ifdef DEBUG
      if (m_OwnerID != GetCurrentThreadId())
         DebugBreak();
#endif
      m_Depth--;
      if (!m_Depth)
         m_OwnerID = 0;
   }
   
private:
   volatile DWORD m_OwnerID;
   volatile int   m_Depth;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__FASTMUTX_H */
