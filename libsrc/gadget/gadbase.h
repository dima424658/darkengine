/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadbase.h 1.4 1998/08/04 14:09:36 JAEMZ Exp $

#ifndef __GADBASE_H
#define __GADBASE_H
#pragma once

#include <cursors.h>
#include <slab.h>
#include <region.h>
#include <res.h>
#include <drawelem.h>
#include <2d.h>
#include <lgadover.h>
#include <guistyle.h>

#include <datasrc.h>


//////////////////////////////////////////////////////////////
// BASE-LEVEL GADGET API/INFRASTRUCTURE
//////////////////////////////////////////////////////////////

//------------------------------------------------------------
// PALTYPE SUPPORT
// This is sort of outdated, and is most useful when drawing an 8-bit 
// ui onto a 16-bit screen.  Newer apps should consider using styles instead.
//
// Basically, there is a cache of 16 "paltypes," which are referred to by the 
// integers 0..15.  Each paltype has a palette, and potentially a style.

#define MAX_PALTYPES 16

// The id of the resource that holds all the palettes for all the paltypes.  Each 
// paltype is a different ref in the compound resource.  
EXTERN Id lgad_pal_id;

// For each paltype, which color is black
EXTERN short lgad_pal_blacks[MAX_PALTYPES]; 

// for each paltype, which color is good for drawing text
EXTERN short lgad_pal_text[MAX_PALTYPES];

// A complete guiStyle for each paltype
EXTERN guiStyle* lgad_styles[MAX_PALTYPES];

//------------------------------------------------------------
// 8-to-16 bit palette support
// 

// are we using 8-to-16 palettes
EXTERN bool lgad_use_pal16;

//------------------------------------------------------------
// Installable callbacks
//

// clipboard support
EXTERN char *(*lgad_paste_func)(void);
// overlay support
EXTERN bool (*lgad_overlay_check)(Rect *r);
EXTERN short (*lgad_overlay_add)(short x,short y, grs_bitmap *bm,OverlayStatus (*update_func)(),void *update_arg,ushort *pal);
// recorder support
EXTERN void (*lgad_recorder_func)(void);
// cosmic strings
EXTERN char *(*lgad_string_get)(int strref, char *buf, int bufsize);
// memory allocation/deallocation
EXTERN char *(*lgad_malloc)(long size);
EXTERN void (*lgad_free)(char *ptr);

//------------------------------------------------------------
// Other globals/defaults
//

EXTERN void LGadSetDefaultFont(IDataSource *pFont);
EXTERN IDataSource *LGadGetDefaultFont(void);

// Xemu, what do these do? 
EXTERN Id lgad_btype_ids[MAX_BTYPES]; 
EXTERN Id lgad_internal_id;

//------------------------------------------------------------
// PROTOTYPES
// 

// 
// Initializaiton/shutdown
// 

EXTERN int LGadInit(void);
EXTERN int LGadTerm(void);

//
// Frame by frame
//

EXTERN int LGadFrame(void);
EXTERN void LGadSimplePoll(void);



// Other useful stuff!

// This works in conjunction with 
// LGadPopCursor which is how the ref count is freed.
// So if you push, don't forget to pop.
// We might want to consider having the shutdown routine
// do an auto-pop without the other stuff on exit
EXTERN int LGadPushCursor(IDataSource *pCurs, Point anchor,int paltype);
EXTERN int LGadPopCursor();

// The user is in charge of all the memory management here,
// this just puts it together for them.
EXTERN int LGadSetCursor(grs_bitmap *bm,Point anchor, int paltype, Cursor *c);
EXTERN int LGadFlush();

#endif // __GADBASE_H



