/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/doorblok.h,v 1.4 2000/01/29 13:11:26 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   doorblok.h

   This is the editor-side code which enforces door boundaries during
   portalization.  It does this by generating temporary terrain
   brushes matching the OBBs of the doors, using the media_op for
   blocking.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#ifndef _DOORBLOK_H_
#define _DOORBLOK_H_

EXTERN void DrBlkGenerateBrushes(void);
EXTERN void DrBlkDestroyBrushes(void);

EXTERN void DrBlkUnblockAll(void);
EXTERN void DrBlkReblockAll(void);

#endif // _DOORBLOK_H_
