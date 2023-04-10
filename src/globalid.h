/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/globalid.h,v 1.3 2000/01/29 12:41:20 adurant Exp $
#pragma once

#ifndef GLOBALID_H
#define GLOBALID_H
#include <objtype.h>

//------------------------------------------------------------
// GLOBAL ID TYPE
//

// the method for passing around objects on the network
typedef struct GlobalID
{
   ulong creator;         // the appPID of machine which created (connectionID)
   ObjID local_objid;     // objID on that machine (or other unique metric)
} GlobalID;


#define GlobalIDGetCreator(gid) (gid->creator)

EXTERN BOOL ObjGetGlobalID(ObjID obj, GlobalID* fillmein);
EXTERN BOOL ObjSetGlobalID(ObjID obj, GlobalID* fromhere);

EXTERN BOOL ObjIsLocal(ObjID obj);
EXTERN void ObjMakeLocal(ObjID obj);
EXTERN void ObjMakeGlobal(ObjID obj);


EXTERN ObjID GlobalID2Local(GlobalID* gid);
EXTERN BOOL  GlobalIDCreate(int creator, ObjID objid, GlobalID* gid); 

#define GlobalIDEqual(g1,g2) \
((g1)->creator == (g2)->creator && (g1)->local_objid == (g2)->local_objid)


#endif // GLOBALID_H
