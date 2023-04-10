/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/scrndump.h,v 1.2 2000/01/31 09:48:41 adurant Exp $
#pragma once

#ifndef __SCRNDUMP_H
#define __SCRNDUMP_H

////////////////////////////////////////////////////////////
// Screen Dump API
//


//
// Cause a screen dump to happen at the end of the frame
//
EXTERN void SetScreenDump(const char* filename); 

//
// It's the end of the frame, let's do a screen dump 
//
EXTERN void DumpScreenMaybe(void); 

#endif // __SCRNDUMP_H
