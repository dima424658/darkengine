/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/isdesced.cpp,v 1.2 1998/10/05 17:26:21 mahk Exp $
#include <wtypes.h>
#include <isdesced.h>
#include <lgassert.h>

#include <dynfunc.h>

// must be last header
#include <dbmem.h>

static IStructEditor* LGAPI construct_woe(sStructEditorDesc* , sStructDesc* , void* )
{
   CriticalMsg("Could not load dialog!");
   return NULL;
}

DeclDynFunc_(IStructEditor*, LGAPI, ConstructStructEditor, (sStructEditorDesc*, sStructDesc*, void* ));
ImplDynFunc(ConstructStructEditor, "darkdlgs.dll", "_ConstructStructEditor@12", construct_woe);

#define Construct (DynFunc(ConstructStructEditor).GetProcAddress())

IStructEditor* CreateStructEditor(const sStructEditorDesc* eddesc, const sStructDesc* sdesc, void* editme)
{
   return Construct((sStructEditorDesc*)eddesc,(sStructDesc*)sdesc,editme);
}



