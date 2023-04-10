/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbasemsg.h,v 1.9 2000/01/29 13:20:33 adurant Exp $
#pragma once

#ifndef __DBASEMSG_H
#define __DBASEMSG_H
#include <looptype.h>
#include <comtools.h>
#include <dbtype.h>
#include <objnotif.h>

////////////////////////////////////////////////////////////
// DATABASE DISPATCH MESSAGES
//

#define kMsgDatabase    kMsgLoad  
F_DECLARE_INTERFACE(ITagFile);

//
// Message types
//

typedef enum msgDatabaseKind
{
   kDatabaseReset,        // reset to empty but valid database
   kDatabaseLoad,         // load the database from a file
   kDatabaseSave,         // save the database to a file
   kDatabaseDefault,      // build the "default" database
   kDatabasePostLoad,     // Post-load cleanup
   kDatabaseNumKinds
}  msgDatabaseKind;

//
// Database partitions (mission, map gamesys) 
//


enum edbPartition // database partitions
{
   kDBMission  = 1 << (0 + DB_PARTITION_SHF),
   kDBMap      = 1 << (1 + DB_PARTITION_SHF),
   kDBGameSys  = 1 << (2 + DB_PARTITION_SHF),

};

//
// File type partitions, which are unions of object sys partitions and 
// db partitions
//

enum edbFiletype_ 
{

   kFiletypeMIS = kDBMission  | kObjPartConcrete|kObjPartMission, 
   kFiletypeMAP = kDBMap      | kObjPartConcrete|kObjPartTerrain,
   kFiletypeGAM = kDBGameSys  | kObjPartAbstract,
   kFiletypeAll = kFiletypeMIS|kFiletypeGAM|kFiletypeMAP|kObjParts|kObjConcreteSubparts|kObjAbstractSubparts,
};



//
// Satellite data types
//

typedef union msgDatabaseData
{
   void* raw;            // raw data;
   void* reset;          // no reset data
   ITagFile* load; // file to load from
   ITagFile* save; // file to save to 
} msgDatabaseData;


#endif // __DBASEMSG_H






