/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/elitprop.cpp,v 1.1 1998/04/24 18:48:42 MAT Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   elitprop.cpp

   sExtraLightProp

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#include <objpos.h>
#include <matrix.h>
#include <osysbase.h>

#include <prophash.h>
#include <property.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <propface.h>
#include <proplist.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <simpwrap.h>

#include <elitprop.h>
#include <objnotif.h>

// Must be last header
#include <dbmem.h>


// property descriptor
static sPropertyDesc g_Desc = 
{
   PROP_EXTRA_LIGHT_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   0,   // constraints
   1,   // version
   0,   // last ok version (0 means no other ok versions)
   { "Renderer", "Extra Light" },
}; 

class cOps : public cClassDataOps<sExtraLightProp>
{
};

class cStore : public cHashPropertyStore<cOps>
{
}; 

class cExtraLightProperty : public cSpecificProperty<IExtraLightProperty,
                          &IID_IExtraLightProperty, sExtraLightProp*, cStore>
{
   typedef cSpecificProperty<IExtraLightProperty, &IID_IExtraLightProperty,
      sExtraLightProp*, cStore> cParent;

public:
   cExtraLightProperty(const sPropertyDesc* pDesc)
      : cParent(pDesc)
   {
   }

   // use the standard DescribeType method. 
   STANDARD_DESCRIBE_TYPE(sExtraLightProp); 
};


static sFieldDesc g_Fields[] =
{
   { "Amount (-1..1)",
     kFieldTypeFloat, FieldLocation(sExtraLightProp, m_fValue) },
   { "Additive?",
     kFieldTypeBool, FieldLocation(sExtraLightProp, m_bAdditive) },
};


static sStructDesc g_Struct
   = StructDescBuild(sExtraLightProp, kStructFlagNone, g_Fields);

static cExtraLightProperty* g_pExtraLightProperty = NULL;

////////////////////////////////////////////////////

// Let's make us a property...
extern "C" BOOL ExtraLightPropInit(void)
{
   StructDescRegister(&g_Struct);
   g_pExtraLightProperty = new cExtraLightProperty(&g_Desc);

   return TRUE;
}

// C access
extern "C" BOOL ObjExtraLightGet(ObjID obj, sExtraLightProp **ppLight)
{
   return g_pExtraLightProperty->Get(obj, ppLight); 
}

extern "C" BOOL ObjExtraLightSet(ObjID obj, sExtraLightProp *pLight)
{
   return g_pExtraLightProperty->Set(obj, pLight); 
}
