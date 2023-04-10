/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/linkedit.h,v 1.2 1997/11/06 13:46:14 mahk Exp $
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



