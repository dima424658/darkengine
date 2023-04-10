/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/disploop.h,v 1.4 2000/01/29 13:38:40 adurant Exp $
#pragma once
#ifndef __DISPLOOP_H
#define __DISPLOOP_H


//
// @INSTRUCTIONS: 1) Check out camguids.c and grab a GUID (instructions are in that file)
//                2) Name the GUID and put it's definition below.
//                3) Change MY_GUID in DISPLOOP.c to point to your new GUID.
//

// Loop client GUID
DEFINE_LG_GUID(LOOPID_Display, 0x4b);


// Loop Client Factory Descriptor
EXTERN struct sLoopFactoryDesc DisplayLoopFactory;

// Loop client context data
typedef struct sDisplayLoopContext sDisplayLoopContext;

enum eDisplayLoopFlags
{
   kDispLoopProhibitD3D = 1 << 0, 
   kDispLoopPreserveDDraw = 1 << 1, 
}; 

struct sDisplayLoopContext
{
   int kind;  // kind to change display to (eDisplayDeviceKind)
   ulong flags; 
};

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __DISPLOOP_H
