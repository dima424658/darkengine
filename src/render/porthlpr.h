/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   porthlpr.h

   This is the extern declarations for porthlpr.c.  It's all little
   accessory material to help deal with Portal.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _PORTHLPT_H_
#define _PORTHLPR_H_

// This is a master toggle to switch between portalizing a level
// for a quick look around and running all of the slow analysis
// which improves the level's framerate and appearance.  It does
// this by overriding other toggles; see porthlpr.c for the list.
// @TODO: hook this up to something!
extern bool portal_postprocess;

EXTERN void show_one_cell(int cell_id);
EXTERN void teleport_to_cell(int cell_id);

// This turns a selected object into a blocker by making a multibrush
// which includes the object and a blocking brush.
// @TODO: hook this up to something, too!
extern void block_object(void);

// temp cheap shit
extern void PortalBlockDoor(void);
extern void PortalUnblockDoor(void);

#endif // _PORTHLPR_H_

