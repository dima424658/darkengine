// $Header: x:/prj/tech/libsrc/ui/RCS/uires.h 1.2 1998/06/18 13:31:33 JAEMZ Exp $

#ifndef __UIRES_H
#define __UIRES_H
#pragma once

#include <cursors.h>
#include <res.h>
#include <2dres.h>

// -------
// DEFINES
// -------

// -------
// GLOBALS
// -------


// This is a global buffer used as temporary memory by the cursor extraction
// routine.  The client can fill in an address and size of memory
// that will be safe to use by these routines.  If this data is not filled in,
// data loading routines will use Malloc to get temporary buffer space.  They
// will Free their buffers immediately.
extern struct _uirestempbuffer
{
   char* mem;
   uint size;
} uiResTempBuffer;


// ----------
// PROTOTYPES
// ----------

// Loads in a cursor from a ref into an image resource.
// The hotspot for the cursor is filled in from the image's anchor point data
// The arguments are as follows:
// Cursor* c: a pointer to a cursor struct to be filled in with cursor data
// grs_bitmap* bmp: a pointer to a bitmap to be filled in with the cursor's bitmap
// Ref rid: The ref of the bitmap to be loaded. 
// bool alloc: if alloc is true, the bmp->bits will be set to a Malloc'd buffer
//             for the bitmap's bits. 
//             if alloc is false, then bmp->bits must already point to a memory
//             buffer big enough to hold the bitmaps bits. 

EXTERN errtype uiLoadRefBitmapCursor(Cursor* c, grs_bitmap* bmp, Ref rid, bool alloc);

#endif // __UIRES_H





