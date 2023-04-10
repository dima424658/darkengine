/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/lnktrait.h,v 1.1 1998/06/02 12:29:47 mahk Exp $
#pragma once  
#ifndef __LNKTRAIT_H
#define __LNKTRAIT_H

#include <comtools.h>
#include <linktype.h>

////////////////////////////////////////////////////////////
// LINK EDIT TRAITS
//
// Basically, adding links to the "property" editor
//

F_DECLARE_INTERFACE(IEditTrait); 

// can be LINKID_WILDCARD 
EXTERN IEditTrait* CreateLinkEditTrait(RelationID id, BOOL hidden); 


#endif // __LNKTRAIT_H
