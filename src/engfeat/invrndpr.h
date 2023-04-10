/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/invrndpr.h,v 1.2 1998/07/01 13:00:26 mahk Exp $
#pragma once  
#ifndef __INVRNDPR_H
#define __INVRNDPR_H

#include <label.h>
#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// INVENTORY RENDERING PROPERTY 
//

enum eInvRenderType
{
   kInvRenderDefault,  // use the normal object model
   kInvRenderModel,    // use the model resource specified
   kInvRenderBitmap,   // use a bitmap 
   kInvRenderTypeMax, 

   kInvRenderTypePad = 0xFFFFFFFF,
};


typedef struct sInvRenderType 
{
   int type; 
   Label resource; 
} sInvRenderType; 

////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IInvRenderProperty
DECLARE_PROPERTY_INTERFACE(IInvRenderProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(struct sInvRenderType*); 
}; 

#define PROP_INVREND_NAME "InvRendType" 

EXTERN IInvRenderProperty* CreateInvRenderProp(); 



#endif // __INVRNDPR_H


