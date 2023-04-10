/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/ownprop.cpp,v 1.6 1998/10/05 17:26:53 mahk Exp $
#include <propert_.h>
#include <propface.h>
#include <objowner.h>
#include <ownprop.h>


// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// OWNER PROP API 
//


////////////////////////////////////////////////////////////
// OWNER PROPERTY CREATION 
//

#define OWNERPROP_IMPL kPropertyImplDense

static PropertyID ownerprop_id = PROPID_NULL;

static sPropertyDesc ownerprop_desc  = 
{
   PROP_OWNER_NAME,
   kPropertyTransient
   |kPropertyInstantiate
   |kPropertyNoInherit
   |kPropertyNoClone
   |kPropertyNoEdit,
};

void OwnerPropInit(void)
{
   IIntProperty* prop;
   prop = CreateIntProperty(&ownerprop_desc,OWNERPROP_IMPL);
   ownerprop_id = prop->GetID();
}

#define get_owner_prop(pp)  (GetPropertyInterface(ownerprop_id,IIntProperty,pp))

BOOL ObjGetOwner(ObjID obj, int *owner)
{
   IIntProperty* prop;
   if (get_owner_prop(&prop))
   {
      return prop->GetSimple(obj,owner);
   }
   return FALSE;
}

BOOL ObjSetOwner(ObjID obj, int owner)
{
   IIntProperty* prop;
   if (get_owner_prop(&prop))
   {
      prop->Set(obj,owner);
      return TRUE;
   }
   return FALSE;
}

