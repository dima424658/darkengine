/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendloop.h,v 1.4 2000/01/31 09:53:09 adurant Exp $
#pragma once
#ifndef __RENDLOOP_H
#define __RENDLOOP_H


// Loop client GUID
DEFINE_LG_GUID(LOOPID_Render, 0x45);

// Base mode client 
DEFINE_LG_GUID(LOOPID_RenderBase, 0x147);

// Loop Client Factory Descriptor
EXTERN struct sLoopClientDesc RenderLoopClientDesc;
EXTERN struct sLoopClientDesc RenderBaseLoopClientDesc;

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __RENDLOOP_H
