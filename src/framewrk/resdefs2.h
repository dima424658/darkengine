/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/resdefs2.h,v 1.4 2000/01/31 09:48:36 adurant Exp $
#pragma once

// Defines for our usage of the resource system.
// =============================================


// RESid range limits to be obeyed by resfile scripts.
// ===================================================

// These resource id bases and limits are used directly
// by Voyager source code.  Together with those defined in resdefs.h, 
// bases and limits for each of the resources are defined. The separation
// of resdefs.h and resdefs2.h was done to avoid massive recompiles caused by
// adding new resources to resdefs.h

#define IDBASE_MOTION_PROPORTIONS 1401 
#define IDLIMIT_MOTION_PROPORTIONS 1499
#define IDBASE_MOTION      1500
#define IDLIMIT_MOTION     2500
