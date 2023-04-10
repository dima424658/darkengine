/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCSNDPR_H
#define __DPCSNDPR_H

#ifndef __PROPDEF_H
#include <propdef.h>
#endif // !__PROPDEF_H

// Sound attached to an object

#define PROP_OBJ_SOUND_NAME "ObjSoundName" 

// Property stuff

F_DECLARE_INTERFACE(IObjSoundNameProperty);

#undef INTERFACE
#define INTERFACE IObjSoundNameProperty

DECLARE_PROPERTY_INTERFACE(IObjSoundNameProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(Label *);  // Type-specific accessors, by reference
};

//EXTERN IObjSoundNameProperty *g_ObjSoundNameProperty;

EXTERN void ObjSoundNamePropInit(void);

#endif // !__DPCSNDPR_H
