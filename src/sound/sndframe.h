/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndframe.h,v 1.2 2000/02/26 12:43:39 adurant Exp $

// Utilities for keeping track of which objects made noise heard by
// the player in each frame.  

#ifndef SNDFRAME_H
#define SNDFRAME_H

#include <objtype.h>

void SNDFrameInit();
void SNDFrameTerm();
void SNDFrameNextFrame();
void SNDFrameAddObj( ObjID objID );
BOOL SNDFramePlayedObj( ObjID objID );

#endif
