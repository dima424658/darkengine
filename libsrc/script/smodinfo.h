/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/smodinfo.h,v 1.2 1998/10/03 10:27:22 TOML Exp $
//
//
//

#ifndef __SMODINFO_H
#define __SMODINFO_H

#include <dynarray.h>
#include <hashset.h>
#include <scrptapi.h>

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sScrModuleInfo
//

struct sScrModuleInfo
{
   IScriptModule * pModule;
   HANDLE          hModule;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrModuleInfoTable
//

class cScrModuleInfoTable : public cDynArray<sScrModuleInfo>
{
public:
   void Insert(const sScrModuleInfo &);
   BOOL Remove(const char *, sScrModuleInfo *);

private:
   static int SearchFunc(const void *pKey, const sScrModuleInfo *);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrClassDescTable
//

class cScrClassDescTable : public cStrIHashSet<sScrClassDesc *>
{
public:
   cScrClassDescTable()   {}

   virtual tHashSetKey GetKey(tHashSetNode p) const;

   const sScrClassDesc * Search(const char * p)
   {
      return cStrIHashSet<sScrClassDesc *>::Search(p);
   }

   void Insert(const sScrClassDesc * p)
   {
      cStrIHashSet<sScrClassDesc *>::Insert((sScrClassDesc *)p);
   }

   void Remove(const sScrClassDesc * p)
   {
      cStrIHashSet<sScrClassDesc *>::Remove((sScrClassDesc *)p);
   }

};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SMODINFO_H */
