/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netloop.h,v 1.6 2000/01/29 13:21:25 adurant Exp $
#pragma once
#ifndef __NETLOOP_H
#define __NETLOOP_H

//
//

//
//
DEFINE_LG_GUID(LOOPID_Network, 0x1a7);
DEFINE_LG_GUID(LOOPID_ObjNet, 0x268);
DEFINE_LG_GUID(LOOPID_NetworkReset, 0x298);
DEFINE_LG_GUID(LOOPID_NetworkSim, 0x2ae);
DEFINE_LG_GUID(LOOPID_Ghost, 0x2c8);

//
//
EXTERN struct sLoopClientDesc NetworkLoopClientDesc;
EXTERN struct sLoopClientDesc NetworkResetLoopClientDesc;
EXTERN struct sLoopClientDesc NetworkSimLoopClientDesc;
EXTERN struct sLoopClientDesc ObjNetLoopClientDesc;
EXTERN struct sLoopClientDesc GhostLoopClientDesc;

//
// Context for loopmode go 
// 

typedef struct _NetLoopContext
{
   int dummy;
} NetLoopContext;
typedef struct _NetResetLoopContext
{
   int dummy;
} NetResetLoopContext;
typedef struct _NetSimLoopContext
{
   int dummy;
} NetSimLoopContext;
typedef struct _ObjNetLoopContext
{
   int dummy;
} ObjNetLoopContext;
typedef struct _GhostLoopContext
{
   int dummy;
} GhostLoopContext;


//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __NETLOOP_H
