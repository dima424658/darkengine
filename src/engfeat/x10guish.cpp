/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $
#include <x10guish.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// HAS REFS PROPERTIES
//

static sPropertyDesc x10PropDesc = 
{
   PROP_X10GUISH_NAME,
   0,  //kPropertyNoInherit|kPropertyInstantiate,
   NULL, 0, 0, 
   { "SFX", "Particle Type"    },  // This is the particle type hack now
};

#define X10_IMPL kPropertyImplSparseHash

ILabelProperty* x10Prop = NULL;

BOOL ObjExtinguishable(ObjID obj,Label** ppModel)
{
   Assert_(x10Prop);
   if (ppModel != NULL)
      return x10Prop->Get(obj,ppModel);
   else
      return x10Prop->IsRelevant(obj);
}

void ObjSetExtinguishable(ObjID obj,Label* pModel)
{
   Assert_(x10Prop);
   x10Prop->Set(obj,pModel);
}


void ExtinguishablePropInit(void)
{
   x10Prop = CreateLabelProperty(&x10PropDesc,X10_IMPL);
}

