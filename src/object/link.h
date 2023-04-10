/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/link.h,v 1.2 2000/01/29 13:23:12 adurant Exp $
#pragma once

#ifndef __LINK_H
#define __LINK_H
#include <linktype.h>

//
// Convenience functions that go through the linkman 
//

EXTERN LinkID  LinkCreate(ObjID source, ObjID dest, RelationID id);
EXTERN LinkID  LinkCreateFull(ObjID source, ObjID desc, RelationID id, void* data);
EXTERN HRESULT LinkDestroy(LinkID id); 
EXTERN BOOL    LinkGet(LinkID id, sLink* out); 
EXTERN HRESULT LinkSetData(LinkID id, void* data);
EXTERN void*   LinkGetData(LinkID id);
EXTERN ILinkQuery* LinkQuery(ObjID source, ObjID dest, RelationID relation);
EXTERN HRESULT LinkDestroyMany(ObjID source, ObjID dest, RelationID relation);
EXTERN BOOL    LinkAnyLinks(ObjID source, ObjID dest, RelationID relation);
EXTERN int     LinkCount(ObjID source, ObjID dest, RelationID relation);
EXTERN BOOL    LinkGetSingleton(ObjID source, ObjID dest, RelationID rel, LinkID* id, sLink* link);



#endif // __LINK_H
