/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scalprop.cpp,v 1.13 1999/06/17 23:31:35 Justin Exp $

#include <propert_.h>
#include <propface.h>
#include <scalprop.h>
#include <objscale.h>
#include <osysbase.h>
#include <objpos.h>
#include <dataops_.h>
#include <prophash.h>
#include <trait.h>
#include <objquery.h>

#include <sdesbase.h>
#include <sdesc.h>

// Must be last header 
#include <dbmem.h>


static IVectorProperty* scaleprop = NULL;

////////////////////////////////////////////////////////////
// SCALE PROPERTY CREATION 
//
// @TBD: for now, we're making this property local-only, since we're only
// using it dynamically from tweq and it really shouldn't be networked
// there. Once we have more elegant solutions for tweq, this might become
// networked again.
//

#define SCALEPROP_IMPL kPropertyImplDense

static sPropertyDesc scaleprop_desc  = 
{
   PROP_SCALE_NAME,
   kPropertyNoInherit|kPropertyInstantiate, // Flags
   NULL, // Constraints
   0, 0, // Version
   { "Shape", "Scale" }, 
   kPropertyChangeLocally,
};

//
// Scale data ops
//

struct cConstructedVector : public mxs_vector
{
   cConstructedVector(float xx, float yy, float zz) 
   { x = xx; y = yy; z = zz; }; 
};

static cConstructedVector def_scale(1.0,1.0,1.0);

class cScaleOps : public cClassDataOps<mxs_vector>
{
public:
   STDMETHOD_(sDatum,New)()
   {
      return new mxs_vector(def_scale); 
   }
}; 

//
// Scale store
// 

class cScaleStore: public cHashPropertyStore<cScaleOps>
{
}; 

//
// Scale property
//

typedef mxs_vector Vector; 


typedef cSpecificProperty<IVectorProperty,&IID_IVectorProperty,mxs_vector*,cScaleStore> cBaseScaleProp; 

class cScaleProp: public cBaseScaleProp 
{
public: 
   cScaleProp(const sPropertyDesc* desc) : cBaseScaleProp(desc) {}; 

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue value)
   {
      if (msg & kListenPropLoad) 
         return; 

      if (msg & (kListenPropModify|kListenPropUnset))
      {
         // really, we only want to do this on Concrete, right??
         if (OBJ_IS_CONCRETE(obj))
            ObjForceReref(obj); 
         else // descend the hierarchy, re-ref all concretes
         {
            IObjectQuery* query = mpDonors->GetAllHeirs(obj,kObjectConcrete);
            for (; !query->Done(); query->Next())
               ObjForceReref(query->Object());
            SafeRelease(query); 
         }
      }

      cBaseScaleProp::OnListenMsg(msg, obj, value);
   }

   STANDARD_DESCRIBE_TYPE(Vector); 
}; 


//---------------------------------------------------------------------------------
// Initialize the "Scale" property.
//---------------------------------------------------------------------------------

void ScalePropInit(void)
{
   scaleprop = new cScaleProp(&scaleprop_desc); 
}

//---------------------------------------------------------------------------------
// Get and Set functions for the "Scale" property.
//---------------------------------------------------------------------------------

BOOL ObjGetScale(ObjID obj, mxs_vector *scale)
{
   Assert_(scaleprop);
   mxs_vector* s;
   BOOL retval = scaleprop->Get(obj,&s);  
   if (retval)
      *scale = *s;   
   return retval;
}

BOOL ObjSetScale(ObjID obj, mxs_vector *scale)
{
   Assert_(scaleprop);
   scaleprop->Set(obj,scale);
   return scaleprop != NULL;
}

void ObjResetScale(ObjID obj)
{
   if (scaleprop)
      scaleprop->Delete(obj);
}
