/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brprop.cpp,v 1.8 1998/10/02 10:44:35 JUSTIN Exp $

#include <propert_.h>
#include <dataops_.h>
#include <brprop.h>
#include <prophash.h>
#include <editbr_.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// BRUSH PROPERTY INTERFACE
//

class cBrushStore: public cHashPropertyStore<cSimpleDataOps>
{
}; 

typedef cSpecificProperty<IBrushProperty,&IID_IBrushProperty,editBrush*,cBrushStore> cBaseBrushProp; 

class cBrushProperty : public cBaseBrushProp
{
public:
   cBrushProperty(const sPropertyDesc* desc)
      : cBaseBrushProp(desc)
   {
   };

   STANDARD_DESCRIBE_TYPE(editBrush); 
   
};

////////////////////////////////////////////////////////////
// THE UNIQUE BRUSH PROPERTY
//

static sPropertyDesc PropDesc = 
{
   PROP_BRUSH_NAME,
   kPropertyTransient|kPropertyNoInherit
   |kPropertyNoClone|kPropertyNoEdit,
   NULL, // constraints
   0, 0, // version
   {
      "Editor",
      "Brush Name",  // named, but not seen in the editor
   },
   kPropertyChangeLocally, // net_flags
}; 

IBrushProperty* BrushPropInit(void)
{
   IBrushProperty* prop = new cBrushProperty(&PropDesc);

   return prop;
}

//////////////////////////////////////////////////////////////
// THE "HAS BRUSH" PROPERTY
//
// If FALSE, the object can't have a brush.  
// 

static sPropertyDesc HasBrushDesc = 
{
   PROP_HASBRUSH_NAME,
   0, // flags
   NULL, // constraints
   0, 0, // version
   {
      "Editor",
      "Has Brush", 
   },
   0, // net_flags
}; 

IBoolProperty* HasBrushPropInit(void)
{
   return CreateBoolProperty(&HasBrushDesc,kPropertyImplSparseHash); 
}

