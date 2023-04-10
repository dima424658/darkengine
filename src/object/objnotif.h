/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objnotif.h,v 1.6 2000/01/29 13:23:56 adurant Exp $
#pragma once
#ifndef __OBJNOTIF_H
#define __OBJNOTIF_H
#include <objtype.h>
#include <comtools.h>


//
// ObjNotify Messages
//

enum eObjNotifyMsg_
{  // Listen messages                             // Message                        Data type
   kObjNotifyCreate,                              // Object has been created        ObjID
   kObjNotifyDelete,                              // Object has been deleted        ObjID
   kObjNotifyLoadObj,                             // Object has been loaded         ObjID
   kObjNotifyBeginCreate,                         // Start creating an object       ObjID
                                                
   // Messages seen by properties, relations    
   kObjNotifyDatabase  = 0x80000000,            
   kObjNotifyReset     = kObjNotifyDatabase|0,    // Database is being reset        msgDatabaseData
   kObjNotifyLoad      = kObjNotifyDatabase|1,    // Database is being loaded       msgDatabaseData
   kObjNotifySave      = kObjNotifyDatabase|2,    // Database is being saved        msgDatabaseData
   kObjNotifyDefault   = kObjNotifyDatabase|3,    // Build the default database     msgDatabaseData
   kObjNotifyPostLoad  = kObjNotifyDatabase|4,    // Post-load setup                msgDatabaseData
};

//
// Partition 
// 
//
// The object database is divided into "partitions."  Partitions save and load as a unit. 
// A database message will contain a bitmask of which partitions are to be saved. 
//
// There are two *major* partitions, the "abstract" partition and the "concrete" partition. 
// These are divided into sub-partitions.  There are currently no abstract sub-partitions. 
// 
// Here are the rules:
//    1) a bitmask with the major partition bit set but *no* sub-partition bits set means *all subpartitions*
//       *not* none.  
//    2) a link saves in the *higher* subpartition of its two enpart objects. 
//    3) It's not meaningful to have a subpartition bit without its major partition bit also.  
//

enum eObjPartition__ 
{
   kObjPartShf        =  8,
   kObjPartBase       = 1 << kObjPartShf, 

   kObjPartConcrete   = kObjPartBase << 0, 
   kObjPartAbstract   = kObjPartBase << 1, 

   // the following are sub-partitions of the concrete partition, 
   // their bits should be or'ed with kObjPartConcrete
   kObjPartBriefcase  = kObjPartBase << 2,  // special subpartition for inter-database transport

   kObjPartConc1      = kObjPartBase << 3,  // Primary concrete subpartition 
   kObjPartConc2      = kObjPartBase << 4,  // Secondary concrete subpartition 

   // special constants used by "mission based" games like dark 
   kObjPartTerrain    = kObjPartConc1,      // "terrain" objects
   kObjPartMission    = kObjPartConc2,      // "mission" objects

   kObjParts = kObjPartConcrete|kObjPartAbstract,
   kObjConcreteSubparts = kObjPartBriefcase|kObjPartConc1|kObjPartConc2,
   kObjAbstractSubparts = 0, 
};

#define NOTIFY_MSG(m) (m & 0x800000FF)
#define NOTIFY_PARTITION(x) ((x) & 0x0000FF00)

//------------------------------------------------------------
// Notify message union
//

F_DECLARE_INTERFACE(ITagFile); 
typedef union uObjDatabase
{
   ITagFile* load;
   ITagFile* save; 
   void* reset;
   void* postload;
   void* builddefault; 
} uObjDatabase;

typedef union uObjNotifyData
{
   ObjNotifyData raw;
   ObjID obj;
   uObjDatabase db;
} uObjNotifyData;

#endif // __OBJNOTIF_H







