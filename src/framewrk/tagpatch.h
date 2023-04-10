/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagpatch.h,v 1.1 1999/01/11 11:38:59 mahk Exp $
#pragma once  
#ifndef __TAGPATCH_H
#define __TAGPATCH_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// TAG PATCH FILE
//

//
// Basically, this creates an aggregate tag file out of a "base" file
// and a "patch" file.  The set of tags in the aggregate file is the
// union of the tags in the base and the tags in the patch, with the
// data in the patch tags overriding the data in the base tags.
//

F_DECLARE_INTERFACE(ITagFile); 

ITagFile* CreatePatchedTagFile(ITagFile* base, ITagFile* patch); 



#endif // __TAGPATCH_H
