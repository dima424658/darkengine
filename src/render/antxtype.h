/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/antxtype.h,v 1.3 2000/01/29 13:38:32 adurant Exp $
#pragma once

#ifndef __ANTXTYPE_H
#define __ANTXTYPE_H

#include <hashpp.h>
#include <hshpptem.h>

typedef cHashTableFunctions<int> IntHashFunctions;
typedef cHashTable<int, int, IntHashFunctions> cTexturePatchTable;

extern cTexturePatchTable gTexturePatchTable;
extern cTexturePatchTable gTextureInverseTable;

#endif