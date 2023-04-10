/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitnul.cpp,v 1.2 1997/10/14 11:20:47 TOML Exp $
#include <traitnul.h>
#include <objtype.h>
#include <objquery.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// NULL TRAIT METHODS
//

static sTraitDesc nulldesc = { "NULL" }; 

cNullTrait::cNullTrait() 
   : cBaseTrait(nulldesc)
{
}

////////////////////////////////////////

cNullTrait::~cNullTrait()
{
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cNullTrait::PossessedBy(ObjID )
{
   return FALSE;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cNullTrait::IntrinsicTo(ObjID )
{
   return FALSE;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cNullTrait::GetDonor(ObjID )
{
   return OBJ_NULL;
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cNullTrait::GetAllDonors(ObjID )
{
   return CreateEmptyObjectQuery(); 
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cNullTrait::GetAllHeirs(ObjID, eObjConcreteness )
{
   return CreateEmptyObjectQuery(); 
}


////////////////////////////////////////////////////////////

EXTERN ITrait* CreateEmptyTrait(void)
{
   return new cNullTrait;
}
