/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/palmgr.h,v 1.7 2000/01/31 09:53:00 adurant Exp $
#pragma once

#ifndef __PALMGR_H
#define __PALMGR_H

EXTERN void palmgr_init(void);
EXTERN void palmgr_shutdown(void);

// recompute 16 bit palettes and lighting tables on screen mode change, for example
EXTERN void palmgr_recompute_all(void);

// change data for this slot and recompute lighting tables, etc as necessary.
// does _not_ affect ref count.
EXTERN void palmgr_set_pal_slot(int start, int n, uchar *pal, int slot);

// don't actually touch the 2d pal data
EXTERN void palmgr_set_pal_slot_passively(int start, int n, uchar *pal, int slot);

// Recompute one slot 
EXTERN void palmgr_update_pal_slot(int slot);


// either finds matching palette and increases ref count or 
// allocates new palette slot.
EXTERN int palmgr_alloc_pal(uchar *pal);

// Return ipal for given slot.  Creates if necessary.
EXTERN uchar *palmgr_get_ipal(int slot);
// just returns the palette in this slot
EXTERN uchar *palmgr_get_pal(int slot);

// just bump ref count for this slot...
EXTERN void palmgr_add_ref_slot(int slot);

// decrease ref count for this slot
EXTERN void palmgr_release_slot(int slot);

// Free all data associated with this slot, right now.
EXTERN void palmgr_free_slot(int slot);

// Free the ipal associated wth this slot, if it exists.
EXTERN void palmgr_free_ipal(int slot);

#ifdef PLAYTEST
// get the number of currently allocated palettes
EXTERN int palmgr_get_current_count(BOOL mono_print);
#endif

#endif
