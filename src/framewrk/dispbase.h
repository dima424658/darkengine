/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dispbase.h,v 1.2 2000/01/29 13:20:41 adurant Exp $
#pragma once

#ifndef DISPBASE_H
#define DISPBASE_H

#include <looptype.h>
#include <dispatch.h>

//
// The data passed into a dispatch message
//

struct DispatchData
{
   uint subtype; // dispatch message subtypes
   void* data;
};

//
// Some dispatch message types.  Others are in foomsg.h
//

enum esDispatchKind
{
   kMsgAppInit    =  kMsgUserReserved1, // one-time app init.
   kMsgAppTerm    =  kMsgUserReserved2, // one-time app term.
};

//
// More message groups
//

enum DispatchGroups
{
   kMsgsAppOuter = (kMsgAppInit | kMsgAppTerm),
   kMsgsFrameMid = (kMsgNormalFrame|kMsgPauseFrame),
   kMsgsFrameEdge = (kMsgBeginFrame|kMsgEndFrame),
};

#endif // DISPBASE_H
