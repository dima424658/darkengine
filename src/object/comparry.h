/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/comparry.h,v 1.3 2000/01/29 13:22:56 adurant Exp $
#pragma once

#ifndef COMPARRY_H
#define COMPARRY_H

#include <objtype.h>

////////////////////////////////////////////////////////////
// COMPACT ARRAY PROPERTIES
//
// This is a convenient back-door interface for properties that 
// use compact arrays.
//

typedef struct sCompactArrayProp sCompactArrayProp;
typedef struct sCompactArrayDesc sCompactArrayDesc;


//
// Map an ObjID to an index into the array < 0 means not there. 
//
EXTERN int ObjID2IDX(sCompactArrayProp* array, ObjID id);

//
// Returns an index one greater than the largest used index
//
EXTERN int CompactArrayMaxIdx(sCompactArrayProp* array);
#endif // COMPARRY_H
