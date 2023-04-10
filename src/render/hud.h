/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/hud.h,v 1.2 2000/01/29 13:39:00 adurant Exp $
// stupid hud system
#pragma once

bool hudAddString(char *str, int color, int duration);
void hudDraw(void *fnt);
