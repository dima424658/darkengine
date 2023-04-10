/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/vismsg.h,v 1.2 2000/01/29 12:41:53 adurant Exp $
#pragma once

#ifndef VISMSG_H
#define VISMSG_H

#include <looptype.h>


////////////////////////////////////////////////////////////
//
// VISUAL DISPATCH MESSAGES
//

#define kMsgVisual   kMsgApp1  // see the botton of dispatch.cc 

//
// Visual message subtypes
//

typedef enum msgVisualKind
{
   kPaletteChange,
   kNumVisualKinds,
} msgVisualKind;


#endif // VISMSG_H
