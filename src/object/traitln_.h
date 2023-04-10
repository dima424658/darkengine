/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitln_.h,v 1.4 2000/01/29 13:25:08 adurant Exp $
#pragma once
#ifndef __TRAITLN__H
#define __TRAITLN__H
#include <traittyp.h>
#include <linktype.h>
#include <dlist.h>

////////////////////////////////////////////////////////////
// TRAIT MANAGER INTERNAL LINK STUFF
//

//------------------------------------------------------------
// RELATION IDS
//
EXTERN IRelation* gpMetaPropRel; 

//------------------------------------------------------------
// FAST RELATION ACCESS
//

struct sMetaPropLink
{
   LinkID id; 
   ObjID dest; 
   tMetaPropertyPriority pri; 
}; 


typedef cSimpleDList<sMetaPropLink> MetaPropLinks;

//
// Get a prioritized list of metaproperty links (null if none)
//

EXTERN MetaPropLinks* GetObjectMetaPropLinks(ObjID obj);

//------------------------------------------------------------
// RELATION INITIALIZATION
//

EXTERN void InitTraitRelations(void); 
EXTERN void TermTraitRelations(void); 


#endif // __TRAITLN__H



