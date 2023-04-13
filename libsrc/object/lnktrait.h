/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/object/RCS/lnktrait.h 1.2 1999/01/27 16:00:41 mahk Exp $
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