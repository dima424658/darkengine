/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/esndprop.h,v 1.4 2000/01/31 10:03:04 adurant Exp $
#pragma once

#ifndef _ESNDPROP_H_
#define _ESNDPROP_H_

#include <property.h>
#include <label.h>
#include <esndglue.h>
#include <ctagset.h>

////////////////////////////////////////////////////////////
// GENERIC "TAG LIST" PROPERTY INTERFACE
//
// The tag list is exposed as a string to the designer, but secretly 
// maintains a cTagSet for speed. 
//

//
// The actual "tag list" structure used by the property.
//

#define kESndStringMax (256 - sizeof(cTagSet*))

struct sESndTagList
{
   // This is first so we can make the string longer if we like.
   cTagSet *m_pTagSet;
   char m_TagStrings[kESndStringMax];
};

//////////////////////////////////////////////
// We'll be using this on archetypes.


F_DECLARE_INTERFACE(IESndTagProperty);
#undef INTERFACE
#define INTERFACE IESndTagProperty

DECLARE_PROPERTY_INTERFACE(IESndTagProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sESndTagList*);
};
#undef INTERFACE 

////////////////////////////////////////////
// This is for tags that describe the "class" of object 

#define PROP_ESND_CLASS_NAME "Class Tags"

EXTERN BOOL ObjGetESndClass(ObjID obj, sESndTagList **ppList);
EXTERN BOOL ObjSetESndClass(ObjID obj, sESndTagList *pList);

////////////////////////////////////////////
// This is for material archetypes, terrain, and who knows what else.

#define PROP_ESND_MATERIAL_NAME "Material Tags"

EXTERN BOOL ObjGetESndMaterial(ObjID obj, sESndTagList **ppList);
EXTERN BOOL ObjSetESndMaterial(ObjID obj, sESndTagList *pList);

//////////////////////////////////////////////
// other

EXTERN void ESndPropsInit(void);
EXTERN void ESndPropsTerm(void); 


#endif // ~_ESNDPROP_H_

