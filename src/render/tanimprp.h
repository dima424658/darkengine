/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/tanimprp.h,v 1.1 1998/06/16 17:40:22 mahk Exp $
#pragma once  
#ifndef __TANIMPROP_H
#define __TANIMPROP_H

#include <comtools.h>
#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// TEXTURE ANIM PROP 
// 

//
// data types
//


struct sAnimTexProp
{
   int rate;  // rate in ms/frame 
   ulong flags; 
};

typedef struct sAnimTexProp sAnimTexProp; 

enum eAnimTexFlags
{
   kAnimTexWrap     = 1 << 0,
   kAnimTexRanding  = 1 << 1, 
   kAnimTexReverse  = 1 << 2,
   kAnimTexPortal   = 1 << 3,
}; 

//
// Property Interface
//

#undef INTERFACE
#define INTERFACE IAnimTexProperty 
DECLARE_PROPERTY_INTERFACE(IAnimTexProperty)
{
   DECLARE_UNKNOWN_PURE();  // IUnknown methods
   DECLARE_PROPERTY_PURE();      // IProperty methods 
   DECLARE_PROPERTY_ACCESSORS(sAnimTexProp*);  // Accessors for type Foo, (to set/get by reference, use Foo* instead)
}; 

#undef INTERFACE 


// Property name
#define PROP_ANIM_TEX_NAME "AnimTex" 

// Property factory 
EXTERN IAnimTexProperty* CreateAnimTexProperty(void); 


#endif // __TANIMPROP_H
