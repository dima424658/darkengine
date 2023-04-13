/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/object/objedit/RCS/linkedit.h 1.1 1999/04/07 14:39:39 TOML Exp $
#pragma once  
#ifndef __LINKEDIT_H
#define __LINKEDIT_H

#include <objtype.h>
#include <linktype.h>

typedef struct sLinkEditorDesc sLinkEditorDesc;

// 
// Edit the links that match a pattern.
//

EXTERN void EditLinks(const sLinkEditorDesc* desc, ObjID src, ObjID dest, RelationID flavor); 

#endif // __LINKEDIT_H