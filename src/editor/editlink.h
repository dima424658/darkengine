/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editlink.h,v 1.3 2000/01/29 13:11:39 adurant Exp $
#pragma once

#ifndef __EDITLINK_H
#define __EDITLINK_H

#include <edlnktyp.h>
#include <objtype.h>
#include <linktype.h>

EXTERN LinkEditor* LinkEditorCreate(ObjID objid_src, ObjID objid_dest, RelationID relid, LinkEditorDesc* editdesc);
EXTERN void LinkEditorDestroy(LinkEditor* ed);

EXTERN BOOL LinkEditorShowAllLinks(BOOL newval);

#endif // __EDITLINK_H
