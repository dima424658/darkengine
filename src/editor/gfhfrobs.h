/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gfhfrobs.h,v 1.2 2000/01/29 13:12:18 adurant Exp $
#pragma once

void Create_GFHFrobs(LGadRoot *root, Rect *top_r, Rect *bot_r, editBrush *br);
void Destroy_GFHFrobs(void);
void Update_GFHFrobs(GFHUpdateOp op, editBrush *br);
