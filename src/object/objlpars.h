/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/objlpars.h,v 1.2 2000/01/29 13:23:54 adurant Exp $
//
//
//
#pragma once

#ifndef __OBJSET_H
#define __OBJSET_H

#include <delmlist.h>
#include <dynarray.h>
#include <objtype.h>

F_DECLARE_INTERFACE(IObjectSystem);
F_DECLARE_INTERFACE(ITraitManager);

class cOLPTable;

///////////////////////////////////////////////////////////////////////////////

enum eObjListParseFlags
{
   kOLP_None,
   
   // Remove any duplicates
   kOLP_RemoveDups = 0x01
   
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cObjListParser
//
// Turns a comma delimited list of objects into an array of ObjIDs. 
//

class cObjListParser : private cDelimList
{
public:   
   cObjListParser();
   virtual ~cObjListParser();

   void Parse(const char *, cDynArray<ObjID> *, unsigned flags = kOLP_RemoveDups);
   
private:
   void PostParse(unsigned flags);
   
   void AddDescendants(ObjID ancestor);
   void RemoveDuplicates();
   void AddExclusions(const char *);
   void RemoveExcluded();
   
   void DestroyAll();
   BOOL Append(const char *);
   
   ObjID ObjFromString(const char *);
   
   cDynArray<ObjID> * m_pResult;
   IObjectSystem *    m_pObjectSystem;
   ITraitManager *    m_pTraitManager;
   cOLPTable *        m_pExclusions;
};

///////////////////////////////////////

inline cObjListParser::cObjListParser()
 : m_pObjectSystem(NULL),
   m_pResult(NULL),
   m_pTraitManager(NULL),
   m_pExclusions(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__OBJSET_H */
