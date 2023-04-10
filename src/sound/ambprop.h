/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambprop.h,v 1.4 2000/01/29 13:41:41 adurant Exp $
// ambient sound wackiness for Dark
#pragma once

#ifndef __AMBPROP_H
#define __AMBPROP_H

#include <property.h>
#include <ambbase.h>

#undef INTERFACE
#define INTERFACE IAmbientSoundProperty
DECLARE_PROPERTY_INTERFACE(IAmbientSoundProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(AmbientSound*); 
}; 


#define PROP_AMBIENTSOUND_NAME "AmbientHacked"

EXTERN BOOL AmbSoundPropInit(void);

#endif  // __AMBPROP_H
