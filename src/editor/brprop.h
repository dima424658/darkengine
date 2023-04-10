/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brprop.h,v 1.5 2000/01/29 13:11:16 adurant Exp $
#pragma once

#ifndef BRPROP_H
#define BRPROP_H

#include <property.h>
#include <propdef.h>
#include <editbr.h>
#include <propface.h>

////////////////////////////////////////////////////////////
// BRUSH PROPERTY INTERFACE
//


#undef INTERFACE
#define INTERFACE IBrushProperty
DECLARE_PROPERTY_INTERFACE(IBrushProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(editBrush*); 
}; 


////////////////////////////////////////////////////////////
// THE BRUSH PROPERTY
//

#define PROP_BRUSH_NAME "Brush" 
#define PROP_HASBRUSH_NAME "HasBrush"

EXTERN IBoolProperty* HasBrushPropInit(void); 
EXTERN IBrushProperty* BrushPropInit(void);


#endif // BRPROP_H
