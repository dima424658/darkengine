/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/smodinfo.cpp,v 1.2 1998/10/03 10:27:15 TOML Exp $
//
//
//

#include <lg.h>
#include <smodinfo.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrModuleInfoTable
//

void cScrModuleInfoTable::Insert(const sScrModuleInfo & info)
{
   Append(info);
}

///////////////////////////////////////

BOOL cScrModuleInfoTable::Remove(const char * pszName, sScrModuleInfo * pInfoOut)
{
   index_t i = LSearch((void *)pszName, SearchFunc);
   if (i != BAD_INDEX)
   {
      *pInfoOut = ((*this)[i]);
      FastDeleteItem(i);
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////

int cScrModuleInfoTable::SearchFunc(const void * pKey, const sScrModuleInfo * pItem)
{
   return stricmp((const char *) pKey, ((sScrModuleInfo *)pItem)->pModule->GetName());
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrClassDescTable
//

tHashSetKey cScrClassDescTable::GetKey(tHashSetNode p) const
{
   return (tHashSetKey)(((sScrClassDesc *)p)->pszClass);
}

///////////////////////////////////////////////////////////////////////////////
