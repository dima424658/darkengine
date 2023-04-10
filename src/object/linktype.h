/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linktype.h,v 1.6 2000/01/29 13:23:37 adurant Exp $
#pragma once

#ifndef __LINKTYPE_H
#define __LINKTYPE_H

#include <objtype.h>
#include <osystype.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// ATOMIC TYPES FOR LINK MANAGER
//

#ifndef CONSTFUNC
#ifdef __cplusplus
#define CONSTFUNC const
#else 
#define CONSTFUNC
#endif 
#endif 

//------------------------------------------------------------
// COM INTERFACES
//

F_DECLARE_INTERFACE(ILinkQuery);
F_DECLARE_INTERFACE(IRelation);
F_DECLARE_INTERFACE(ILinkManager);
F_DECLARE_INTERFACE(ILinkQueryDatabase);
F_DECLARE_INTERFACE(ILinkManagerInternal);
F_DECLARE_INTERFACE(ILinkStore);
F_DECLARE_INTERFACE(ILinkDataStore);

//------------------------------------------------------------
// OBJECTS
//

#define LINKOBJ_WILDCARD 0  // OBJ_NULL
#define LINKOBJ_MATCH(a,b)   ((a)==(b) || (a) == LINKOBJ_WILDCARD || (b) == LINKOBJ_WILDCARD)

//------------------------------------------------------------
// RELATION IDS
//

typedef short RelationID;

#define RELID_NULL 0
#define RELID_WILDCARD RELID_NULL
#define RELID_MATCH(a,b)   ((a)==(b) || (a) == RELID_WILDCARD || (b) == RELID_WILDCARD)


// invert a relation
#define RELID_INVERT(id)  ((RelationID)(-(id)))
#define RELID_IS_INVERTED(id)  ((id) < 0)

//
// String prefix to indicate the name of an "inverted" relation
//
#define RELNAME_INVERSE_PREFIX "~"


//------------------------------------------------------------
// RELATION DESCRIPTOR STRUCTURE
//

typedef struct sRelationDesc sRelationDesc;
typedef struct sRelationDataDesc sRelationDataDesc;

// Version number
typedef ulong tRelationVersion;

//------------------------------------------------------------
// RELATION LISTENERS/NOTIFICATION
//

typedef eObjNotifyMsg eRelationNotifyMsg;
typedef ObjNotifyData RelationNotifyData; 

typedef ulong eRelationListenMsg, RelationListenMsgSet;
typedef void* RelationListenerData;
typedef struct sRelationListenMsg sRelationListenMsg;

typedef void (LGAPI * RelationListenFunc)(sRelationListenMsg* msg, RelationListenerData data);

//------------------------------------------------------------
// LINK IDS
//

typedef long LinkID;
typedef int eLinkPartition; 

// null link
#define LINKID_NULL ((LinkID)0 )

//------------------------------------------------------------
// LINKS
//

typedef struct sLink sLink;
typedef struct sLinkAndData sLinkAndData;


//------------------------------------------------------------
// LINK MANAGER SPECIFIC TYPES
//

typedef struct sRelationIter sRelationIter; 

typedef int tQueryDate;
#define QUERY_DATE_NONE -1

typedef ulong RelationQueryCaseSet; // bitmask of query cases


#endif // __LINKTYPE_H



