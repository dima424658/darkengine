/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propdb.h,v 1.3 2000/01/29 13:24:21 adurant Exp $
#pragma once

#ifndef PROPDB_H
#define PROPDB_H

#include <propsprs.h>

////////////////////////////////////////////////////////////
// SHARED PROPERTY DATABASES
// 
// This is the repository for global databases that service more than one property 
// implementation.
//
////////////////////////////////////////////////////////////

struct sPropertyDatabases
{
   cObjPropDatumHashTable mSparseHash; 
   // More databases go here

   static sPropertyDatabases* DB;   // the current databases.  
};




#endif // PROPDB_H
