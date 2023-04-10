/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/edtraist.h,v 1.3 1997/12/03 16:33:00 mahk Exp $
#pragma once  
#ifndef __EDTRAIST_H
#define __EDTRAIST_H

//
// Editable trait descriptor 
//


// Descriptor flags
enum eEditTraitDescFlags
{
   kTraitInvisible = 1 << 0,  // doesn't show up unless explicitly requested
};

// Generic trait "kind", has cosmetic editor effects
enum eEditTraitKind_
{
   kRelationCustom, // a custom hand-written trait
   kPropertyTrait,  // a property
   kRelationTrait,  // a relation or link-like thing
}; 

typedef ulong eEditTraitKind; 

// Descriptor
struct sEditTraitDesc
{
   eEditTraitKind kind; 
   struct sEditTraitUIStrings
   {
      char name[16];             // symbolic name
      char friendly_name[32];    // name for use in UI, defaults to above
      char category[32];         // Organizational category name, defaults to none
   } strings; 
   ulong flags; 
}; 

//
// Editable trait capabilities
//

enum eTraitCapFlags
{
   kTraitCanAdd      = 1 << 0, // "Add" method is supported
   kTraitCanRemove   = 1 << 1, // "Remove" method is supported
   kTraitCanEdit     = 1 << 2, // "Edit" method is supported 
   kTraitCanParse    = 1 << 3, // "Parse" method is supported 
   kTraitCanUnparse  = 1 << 4, // "Unparse" method is supported 

   kTraitCanTextEdit = (kTraitCanParse|kTraitCanUnparse),
   kTraitFullyCapable = 0xFFFFFFFF
}; 

struct sEditTraitCaps
{
   ulong flags;  // see above 
}; 

#endif // __EDTRAIST_H

