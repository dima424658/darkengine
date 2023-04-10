/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkbase.h,v 1.14 2000/01/29 13:23:15 adurant Exp $
#pragma once

#ifndef __LINKBASE_H
#define __LINKBASE_H
#include <linktype.h>
#include <objtype.h>

//
// Links
//

struct sLink 
{
   ObjID source; 
   ObjID dest;
   RelationID flavor;
};

//
// sLinkAndData
// A little preprocessor work to use derivation in C++ and 
// anonymous structures in C. 
//
// @ROBUSTIFY: anonymous structures are not part of the spec
//
#ifdef __cplusplus
#define SLINK_BASE : public sLink
#define SLINK_ANON  
#else
#define SLINK_BASE 
#define SLINK_ANON struct sLink;
#endif 


struct sLinkAndData  SLINK_BASE
{
   SLINK_ANON
   void* data;
};

//
// Link Partitions
//

enum eLinkPartition_
{
   kLinkGameSys,
   kLinkMission,
   kLinkMap,
   kNumLinkPartitions 
};

#define SuggestedLinkPartition(o1,o2) \
       (OBJ_IS_ABSTRACT(o1) && OBJ_IS_ABSTRACT(o2) ? kLinkGameSys : kLinkMission)

//
// Relations
//

enum eRelationFlags
{
   kRelationTransient         = 1 << 1,  // Transient (don't save to disk)
   kRelationDirectionless     = 1 << 2,  // My links are directionless (@NIY)
   kRelationNoEdit            = 1 << 3,  // Don't show this relation in the editor
   kRelationNetworkLocalOnly  = 1 << 4,  // Don't broadcast changes in a network game
   kRelationNetProxyChangeable = 1 << 5,  // Allow proxy machines to change this link
}; 

struct sRelationDesc 
{
   char name[32]; 
   ulong flags;
   tRelationVersion version;  // current version
   tRelationVersion oldest;   // oldest acceptable version (older ones will be purged)
                              // (oldest == 0 means oldest = version)
}; 

//
// Relation data
//

F_DECLARE_INTERFACE(IDataOps); 



enum eRelationDataFlags
{
   kRelationDataAutoCreate = 1 << 1,  // Create new blank data for each new link
};

struct sRelationDataDesc
{
   char type[32];  // type name string
   ulong size;     // data size 
   ulong flags;    // flags
   IDataOps* data_ops;  // data operations (optional)
};  

#define LINK_DATA_DESC(type) { #type, sizeof(type) }
#define LINK_DATA_DESC_FLAGS(type,flags) { #type, sizeof(type), flags}
#define LINK_NO_DATA { "None", 0 }

//
// Relation Listeners
//

enum eRelationListenMsg_
{
   kListenLinkModify    = 0x0001,   // Link Data has just been modified
   kListenLinkBirth     = 0x0002,   // A link is born
   kListenLinkDeath     = 0x0004,   // A link is about to die 
   kListenLinkPostMortem= 0x0008,   // A link is done dying. 
};

//
// Listener Message Structure
//

struct sRelationListenMsg
{
   eRelationListenMsg type;
   LinkID id;
   sLink link;
};

//
// Link Manager stuff
//

struct sRelationIter
{
   RelationID relid;
   IRelation* rel;
};

//
// QUERY CASES
//

enum eRelationQueryCase
{
   kRelationNoneKnown    = 0,
   kRelationSourceKnown  = 1,
   kRelationDestKnown    = 2,
   kRelationBothKnown    = kRelationSourceKnown|kRelationDestKnown,
   kRelationNumQueryCases = 4,
};

//
// And a bitmask of such cases
//

enum eRelationQueryCaseSet
{
   kQCaseSetNoneKnown    = 1 << kRelationNoneKnown,    
   kQCaseSetSourceKnown  = 1 << kRelationSourceKnown, 
   kQCaseSetDestKnown    = 1 << kRelationDestKnown,   
   kQCaseSetBothKnown    = 1 << kRelationBothKnown,   

   kQCaseSetAll = kQCaseSetNoneKnown|kQCaseSetSourceKnown|kQCaseSetDestKnown|kQCaseSetBothKnown

};

#define SingletonQCaseSet(x) (1 << (x))

//
// Bits for specifying that a query database should not be shared with other relations
//

#define PrivateQDatabase(x) ((x) << 4)

//
// descriptor for custom relation 
//
// Any of the fields can be set to NULL, indicating that a default should be 
// used. 
//

struct sCustomRelationDesc
{
   IUnknown* store;   // The link store to use  (ILinkStore)
   ILinkDataStore* datastore;  // the data store to use 
   ILinkQueryDatabase* DBs[kRelationNumQueryCases];  // the query database to use in each query case 
};

#endif // __LINKBASE_H



