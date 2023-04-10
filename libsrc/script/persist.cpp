/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/persist.cpp,v 1.7 1999/11/03 12:08:36 Justin Exp $
//
// 
//

#include <string.h>
#include <lg.h>
#include <comtools.h>

#include <scrptapi.h>
#include <scrptmsg.h>
#include <persist.h>

#include <dbmem.h>

#if defined(__WATCOMC__)
#pragma initialize library
#else
#pragma init_seg (lib)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sPersistent
//

void *         sPersistent::gm_pContextIO;
tPersistIOFunc sPersistent::gm_pfnIO;
BOOL           sPersistent::gm_fReading;

///////////////////////////////////////


BOOL sPersistent::Persistent(string & s)
{
   BOOL fSuccess;
   int len;

   if (gm_fReading)
   {
      cStr tempStr;
      fSuccess = (*gm_pfnIO)(gm_pContextIO, &len, sizeof(int)) == sizeof(int);
      if (len)
      {
         fSuccess = fSuccess
            && (*gm_pfnIO)(gm_pContextIO, tempStr.GetBuffer(len), len) == len;
         tempStr.ReleaseBuffer(len);
      }
      s = tempStr;
   }
   else
   {
      len = strlen(s);
      fSuccess = (*gm_pfnIO)(gm_pContextIO, &len, sizeof(int)) == sizeof(int);
      if (len)
      {
         fSuccess = fSuccess
            && (*gm_pfnIO)(gm_pContextIO, 
                           (void *)s.operator const char *(), len) == len;
      }
   }

   return fSuccess;
}

///////////////////////////////////////

BOOL sPersistent::Persistent(const char * & psz) // be aware that here, const is a lie
{
   BOOL fSuccess;
   int len;

   if (gm_fReading)
   {
      fSuccess = (*gm_pfnIO)(gm_pContextIO, &len, sizeof(int)) == sizeof(int);
      psz = (char *) malloc(len + 1);
      if (len && fSuccess)
      {
         fSuccess = (*gm_pfnIO)(gm_pContextIO, (void *)psz, len) == len;
      }
      ((char *) psz)[len] = 0;
   }
   else
   {
      if (psz)
         len = strlen(psz);
      else
         len = 0;
      fSuccess = (*gm_pfnIO)(gm_pContextIO, &len, sizeof(int)) == sizeof(int);
      if (len)
      {
         fSuccess = fSuccess
            && (*gm_pfnIO)(gm_pContextIO, (void *)psz, len) == len;
      }
   }

   return fSuccess;
}

///////////////////////////////////////

BOOL sPersistent::Persistent(cMultiParm & parm)
{
   AssertMsg(sizeof(eMultiParmType) == sizeof(int), "Enum eMultiParmType must be sizeof(int)");

   if (gm_fReading)
   {
      ClearParm(&parm);
   }
   
   BOOL fSuccess;

   fSuccess
      = (*gm_pfnIO)(gm_pContextIO, &(parm.type), sizeof(int)) == sizeof(int);

   if (fSuccess)
   {
      switch (parm.type)
      {
         case kMT_Undef:
            fSuccess = TRUE;
            break;
         case kMT_Int:
            fSuccess = Persistent(parm.i);
            break;
         case kMT_Obj:
            fSuccess = Persistent(parm.o);
            break;
         case kMT_Float:
            fSuccess = Persistent(parm.f);
            break;
         case kMT_String:
            fSuccess = Persistent(*((const char **)&parm.psz));
            break;
         case kMT_Vector:
            if (gm_fReading)
               parm.pVec = new vector; 
            fSuccess = Persistent(*(vector*)parm.pVec);
            break;
         default:
            fSuccess = FALSE;
      }
   }

   return fSuccess;
}


///////////////////////////////////////////////////////////////////////////////
//
// utility functions
//

// When we are reading in a collection of objects of different types,
// we need to know the type of each object before we can create it.
// So this function, which is outside of the classes we're loading and
// saving, figures out what type we're dealing with.  The name of our
// type is saved in the relevent stream.

// This works by saving the name of each object's class before the
// object.  For a bunch of objects of the same type it's more
// efficient to call p->ReadWrite and new manually, or allocate into
// some other structure, rather than going through this routine.

sPersistent * PersistReadWrite(sPersistent *p, tPersistIOFunc pfnIO,
                               void * pContextIO, BOOL fReading)
{
   char aTypeName[kPersistNameMax + 1];
   if (!fReading)
      strcpy(aTypeName, p->GetName());

   pfnIO(pContextIO, aTypeName, kPersistNameMax + 1);

   // We need a factory.  When we're writing this is just validation.
   sPersistReg *pReg = PersistLookupReg(aTypeName); 
   AssertMsg1(pReg, "PersistReadWriteNamed: type %s not registered",
             aTypeName);

   if (!pReg)
      return p;

   if (fReading)
      p = pReg->m_pfnFactory();

   p->ReadWrite(pfnIO, pContextIO, fReading);

   return p;
}

sPersistReg *g_pPersistFactoryList = 0;

extern sPersistReg* PersistLookupReg(const char* pszName)
{

   // We need a factory.  When we're writing this is just validation.
   sPersistReg *pReg = g_pPersistFactoryList;
   while (pReg)
   {
      if (strcmp(pReg->m_pszName, pszName) == 0)
         break;
      pReg = pReg->m_pNext;
   }

   return pReg; 
}



///////////////////////////////////////////////////////////////////////////////
