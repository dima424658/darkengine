/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roombase.h,v 1.2 2000/01/31 10:00:33 adurant Exp $
//
// Room System base structures
//
#pragma once

#ifndef __ROOMBASE_H
#define __ROOMBASE_H

#include <wrtype.h>
#include <matrixs.h>

#include <roomsys.h>


typedef int ObjID;


////////////////////////
//
// Structure declarations
//
struct tOBB
{
   Position   pos;
   mxs_vector scale;
};

struct tPlane
{
   mxs_vector normal;
   mxs_real   d;
};

struct tEdge
{
   int from;
   int to;
};


////////////////////////
//
// Callbacks
//
typedef BOOL (*tWatchCallback) (ObjID objID);
typedef void (*tPortalCallback) (ObjID objID, cRoom *roomFrom, cRoom *roomTo);   


////////////////////////
//
// Constants
//

const int kAllObjWatchHandle = 0;

const tEdge OBBEdge[12] = { {0, 1}, {1, 2}, {2, 3}, {3, 0},
                            {4, 5}, {5, 6}, {6, 7}, {7, 4},
                            {0, 4}, {1, 5}, {2, 6}, {3, 7} };


#endif






