/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/isdesced.cpp,v 1.2 1998/10/05 17:26:21 mahk Exp $
#include <isdesced.h>
#ifdef EDITOR
#include <darkdlgs/darkdlgs.h>
#endif

// must be last header
#include <dbmem.h>

IStructEditor* CreateStructEditor(const sStructEditorDesc* eddesc, const sStructDesc* sdesc, void* editme)
{
#ifdef EDITOR
   return ConstructStructEditor(eddesc,sdesc,editme);
#else
   return NULL;
#endif
}



