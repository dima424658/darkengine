/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcinvpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

IContainDimsProperty *g_ContainDimsProperty;
IInvDimsProperty     *g_InvDimsProperty;

/////////////////////////////////////////////////////////////
// Object Dimensions in inventory
/////////////////////////////////////////////////////////////

// the all new improved property system...
// data ops
class cInvDimsDataOps: public cClassDataOps<sInvDims>
{
};

// storage class
class cInvDimsStore: public cHashPropertyStore<cInvDimsDataOps>
{
};

// property implementation class
class cInvDimsProp: public cSpecificProperty<IInvDimsProperty, &IID_IInvDimsProperty, sInvDims*, cInvDimsStore>
{
   typedef cSpecificProperty<IInvDimsProperty, &IID_IInvDimsProperty, sInvDims*, cInvDimsStore> cParent; 

public:
   cInvDimsProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sInvDims); 

};

static sFieldDesc InvDimsFields[] = 
{
   {"Width", kFieldTypeInt, FieldLocation(sInvDims, m_width),},
   {"Height", kFieldTypeInt, FieldLocation(sInvDims, m_height),},
};

static sStructDesc InvDimsStructDesc = 
   StructDescBuild(sInvDims, kStructFlagNone, InvDimsFields);

IInvDimsProperty *CreateInvDimsProperty(sPropertyDesc *desc)
{
   StructDescRegister(&InvDimsStructDesc);
   return new cInvDimsProp(desc);
}

static sPropertyDesc InvDimsDesc = 
{
   PROP_INVDIMS, 0,
   NULL, 0, 0,
   {"Obj", "InvDims"},
};


BOOL InvDimsGet(ObjID objID, sInvDims **ppInvDims)
{
   return g_InvDimsProperty->Get(objID, ppInvDims);
}

int InvDimsGetWidth(ObjID objID)
{
   sInvDims *pInvDims;

   if (g_InvDimsProperty->Get(objID, &pInvDims))
      return pInvDims->m_width;
   return 1;
}

int InvDimsGetHeight(ObjID objID)
{
   sInvDims *pInvDims;

   if (g_InvDimsProperty->Get(objID, &pInvDims))
      return pInvDims->m_height;
   return 1;
}

Point InvDimsGetSize(ObjID objID)
{
   sInvDims *pInvDims;
   Point retval = {0,0};

   if (g_InvDimsProperty->Get(objID, &pInvDims))
   {
      retval.x = pInvDims->m_width;
      retval.y = pInvDims->m_height;
   }
   return retval;
}

/////////////////////////////////////////////////////////////
// Container Dimensions (capacity)
/////////////////////////////////////////////////////////////

// the all new improved property system...
// data ops
class cContainDimsDataOps: public cClassDataOps<sContainDims>
{
};

// storage class
class cContainDimsStore: public cHashPropertyStore<cContainDimsDataOps>
{
};

// property implementation class
class cContainDimsProp: public cSpecificProperty<IContainDimsProperty, &IID_IContainDimsProperty, sContainDims*, cContainDimsStore>
{
   typedef cSpecificProperty<IContainDimsProperty, &IID_IContainDimsProperty, sContainDims*, cContainDimsStore> cParent; 

public:
   cContainDimsProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sContainDims); 

};

BOOL ContainDimsGet(ObjID objID, sContainDims **ppContainDims)
{
   return g_ContainDimsProperty->Get(objID, ppContainDims);
}

static sFieldDesc ContainDimsFields[] = 
{
   {"Width", kFieldTypeInt, FieldLocation(sContainDims, m_width),},
   {"Height", kFieldTypeInt, FieldLocation(sContainDims, m_height),},
};

static sStructDesc ContainDimsStructDesc = 
   StructDescBuild(sContainDims, kStructFlagNone, ContainDimsFields);

static sPropertyDesc ContainDimsDesc = 
{
   PROP_ContainDims, 0,
   NULL, 0, 0,
   {"Obj", "ContainDims"},
};

IContainDimsProperty *CreateContainDimsProperty(sPropertyDesc *desc)
{
   StructDescRegister(&ContainDimsStructDesc);
   return new cContainDimsProp(desc);
}

int ContainDimsGetWidth(ObjID objID)
{
   sContainDims *pContainDims;

   if (g_ContainDimsProperty->Get(objID, &pContainDims))
      return pContainDims->m_width;
   return 1;
}

int ContainDimsGetHeight(ObjID objID)
{
   sContainDims *pContainDims;

   if (g_ContainDimsProperty->Get(objID, &pContainDims))
      return pContainDims->m_height;
   return 1;
}

Point ContainDimsGetSize(ObjID objID)
{
   sContainDims *pContainDims;
   Point retval = {0,0};

   if (g_ContainDimsProperty->Get(objID, &pContainDims))
   {
      retval.x = pContainDims->m_width;
      retval.y = pContainDims->m_height;
   }
   return retval;
}

void InvDimsPropertyInit()
{
   g_InvDimsProperty = CreateInvDimsProperty(&InvDimsDesc);
   g_ContainDimsProperty = CreateContainDimsProperty(&ContainDimsDesc);
}

