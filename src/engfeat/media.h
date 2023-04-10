/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/media.h,v 1.4 2000/01/29 13:19:47 adurant Exp $
  // used internally for undecided regions
#pragma once
#define NO_MEDIUM               -1

#define MEDIA_SOLID             0
#define MEDIA_AIR               1
#define MEDIA_WATER             2

// These are used for doors.  They let us make cell boundaries in mid-air,
// mid-water, etc.
#define MEDIA_SOLID_PERSIST     3
#define MEDIA_AIR_PERSIST       4
#define MEDIA_WATER_PERSIST     5

// This would really mess with the media_op arrays.
//#define SPECIAL_MEDIUM(x)  ((x) >= 64)
