/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/persist.h,v 1.2 1999/06/29 19:10:57 mahk Exp $
//
// sPersistent is a weird little utility struct for loading and
// saving the basic types used by scripts, including vectors and
// cMultiParms.
//
// A struct or class which wants to be persistent should descend
// from sPersistent.  For the sake of one of our macros, it will
// also want a default constructor.

#ifndef __PERSIST_H
#define __PERSIST_H

#pragma once


#define kPersistNameMax 31


typedef int (*tPersistIOFunc)(void * pContext, void * p, size_t size);


struct sPersistent
{
   virtual ~sPersistent() { };

   BOOL ReadWrite(tPersistIOFunc pfnIO, void * pContextIO, BOOL fReading);
   BOOL Reading() { return gm_fReading; }; 

   BOOL Persistent(string &);
   BOOL Persistent(const char *&);
   BOOL Persistent(void *, size_t);
   BOOL Persistent(const char *, size_t);
   BOOL Persistent(real &);
   BOOL Persistent(integer &);
   BOOL Persistent(vector &);
   BOOL Persistent(boolean &);
   BOOL Persistent(ulong &);
   BOOL Persistent(cMultiParm &);

   virtual BOOL Persistence() { return FALSE; }
   virtual const char *GetName() { return "no factory"; }

protected:
   static tPersistIOFunc gm_pfnIO;
   static void *        gm_pContextIO;
   static BOOL          gm_fReading;
};

///////////////////////////////////////

inline BOOL sPersistent::ReadWrite(tPersistIOFunc pfnIO,
                                   void * pContextIO, BOOL fReading)
{
   gm_pContextIO = pContextIO;
   gm_pfnIO      = pfnIO;
   gm_fReading   = fReading;

   return Persistence();
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(void * p, size_t size)
{
   return (*gm_pfnIO)(gm_pContextIO, p, size) == size;
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(const char * p, size_t size)
{
   return Persistent((void *)p, size);
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(real & r)
{
   return (*gm_pfnIO)(gm_pContextIO, &r, sizeof(real)) == sizeof(real);
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(integer & i)
{
   return (*gm_pfnIO)(gm_pContextIO, &i, sizeof(integer)) == sizeof(integer);
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(vector & v)
{
   return (*gm_pfnIO)(gm_pContextIO, &v, sizeof(vector)) == sizeof(vector);
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(boolean & b)
{
   return (*gm_pfnIO)(gm_pContextIO, &b, sizeof(boolean)) == sizeof(boolean);
}

///////////////////////////////////////

inline BOOL sPersistent::Persistent(ulong & i)
{
   return (*gm_pfnIO)(gm_pContextIO, &i, sizeof(ulong)) == sizeof(ulong);
}


///////////////////////////////////////////////////////////////////////////////


typedef sPersistent * (*tPersistFactory)();

struct sPersistReg;
extern sPersistReg *g_pPersistFactoryList;

struct sPersistReg
{
   sPersistReg(tPersistFactory pfnFactory, const char *pszName)
      : m_pfnFactory(pfnFactory),
        m_pszName(pszName)
   {
      m_pNext = g_pPersistFactoryList;
      g_pPersistFactoryList = this;
   }

   const tPersistFactory m_pfnFactory;
   const char *m_pszName;

   sPersistReg *m_pNext;
};

extern sPersistent * PersistReadWrite(sPersistent *p, tPersistIOFunc pfnIO,
                                      void * pContextIO, BOOL fReading);

extern sPersistReg* PersistLookupReg(const char* pszName); 

///////////////////////////////////////////////////////////////////////////////


#define PERSIST_DECLARE() \
   virtual const char * GetName()

#define PERSIST_DECLARE_PERSISTENT() \
   PERSIST_DECLARE(); \
   BOOL Persistence()

///////////////////////////////////////

#define PERSIST_IMPLEMENT(STR) \
   \
   sPersistent * g_PersistFactory##STR () \
   { \
      sPersistent * p = new STR; \
      return p; \
   } \
   \
   sPersistReg g_PersistReg##STR(g_PersistFactory##STR, #STR); \
   \
   const char * STR::GetName() \
   { \
      return #STR; \
   }

#define PERSIST_IMPLEMENT_PERSISTENT(STR) \
   PERSIST_IMPLEMENT(STR); \
   BOOL STR::Persistence()

#define PersistentEnum(e) \
   Assert_(sizeof(e) == sizeof(int)); \
   Persistent(*((int *)(&(e))))

///////////////////////////////////////////////////////////////////////////////

#endif // ~__PERSIST_H
