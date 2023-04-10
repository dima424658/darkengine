/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitprp.h,v 1.3 2000/01/29 13:25:13 adurant Exp $
#pragma once

#ifndef __TRAITPRP_H
#define __TRAITPRP_H

////////////////////////////////////////////////////////////
//
// PROPERTIES USED IN TRAIT IMPLEMENTATION
//
//

//
// Donor types   @TODO: move this out if necessary
//
enum eTraitDonorTypes
{
   kDonorArchetype,
   kDonorMetaProp,
};

//------------------------------------------------------------
// PROPERTY NAMES
//

// what kind of abstract object am I
#define PROP_DONOR_TYPE_NAME "DonorType" 
// what is my archetype
#define PROP_ARCHETYPE_NAME  "Archetype" 
// Symbolic name used for naming archetypes/metaprops
#define PROP_SYMNAME_NAME  "SymName"

EXTERN void TraitPropertiesInit(void);
EXTERN void TraitPropertiesTerm(void);


#endif // __TRAITPRP_H
