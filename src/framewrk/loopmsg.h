/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/loopmsg.h,v 1.3 2000/01/29 13:21:07 adurant Exp $
#pragma once

#ifndef __LOOPMSG_H
#define __LOOPMSG_H

#include <looptype.h>

// This is a union of the possible message types

typedef union _LoopMsg
{
   tLoopMessageData        raw;           // the raw handle
   sLoopFrameInfo*         frame;         // frame message data
   sLoopTransition*        mode;          // mode message data
   struct DispatchData*    dispatch;      // dispatch data
   int                     minormode;     // minor mode change
} LoopMsg;


#endif // __LOOPMSG_H
