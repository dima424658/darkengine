/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/comparrb.h,v 1.3 2000/01/29 13:22:55 adurant Exp $
#pragma once

#ifndef COMPARRB_H
#define COMPARRB_H

#include <proptype.h>


////////////////////////////////////////////////////////////
// COMPACT ARRAY BASE TYPES
//


struct sCompactArrayDesc
{
   void** elem_vec;           // vector of ptrs to data elements;
   ObjID* id_vec;             // vector of corresponding ObjIDs;
   int    vecsize;            // number of vector elements
   int    elemsize;           // size of a single data elem
   int    min_idx;            // minimum index
};


struct sCompactArrayProp 
{
   void* impl;  // cPImpCompactArray* 
   PropertyID id; 
};

#endif // COMPARRB_H


