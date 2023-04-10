/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitprp.cpp,v 1.18 2000/02/24 23:41:09 mahk Exp $
#include <propface.h>

#include <propbase.h>
#include <traitprp.h>
#include <traitpr_.h>
#include <propface.h>
#include <osysbase.h>
#include <proparry.h>
#include <propbase.h>

#include <linktype.h>
#include <linkbase.h>
#include <proplink.h>
#include <linkqdb.h>
#include <nameprop.h>
#include <proparry.h>

#include <relation.h>
#include <traitln_.h>

// Must be last header
#include <dbmem.h>

static cGenericArrayPropertyStore* gDonorTypeStore = NULL; 


int ObjDonorType(ObjID obj)
{
   return (int)(*gDonorTypeStore)[obj].value; 
}



//------------------------------------------------------------
// DONOR TYPE PROP


static sPropertyDesc donordesc = 
{
   PROP_DONOR_TYPE_NAME,
   kPropertyNoInherit|kPropertyNoClone|kPropertyNoEdit,
   NULL, 0, 0, 
   { "Object System", "Donor Type" }, 
}; 

//------------------------------------------------------------
// SYMNAME PROP
//

static sPropertyDesc symnamedesc = 
{
   PROP_SYMNAME_NAME,
   kPropertyNoInherit|kPropertyNoClone|kPropertyNoEdit,
   NULL, 0, 0,
   { "Object System", "Symbolic Name" }, 
};

#define SYMNAME_IMPL kPropertyImplHash

void TraitPropertiesInit(void)
{
   IProperty* prop; 
   gDonorTypeStore = new cGenericArrayPropertyStore(); 
   prop = CreateIntPropertyFromStore(&donordesc,gDonorTypeStore);
   SafeRelease(prop);   

   prop = CreateInvStringProperty(&symnamedesc,SYMNAME_IMPL);
   SafeRelease(prop);
}

void TraitPropertiesTerm(void)
{
   SafeRelease(gDonorTypeStore); 
}

