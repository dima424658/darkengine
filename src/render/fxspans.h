/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   fxspans.h

   structure file for fxspan.c--simple span buffer for special
   effects

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _FXSPANS_H_
#define _FXSPANS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fxspant.h>

// This is a package for a span list.  The spans have no relation to
// screen space.  Note the limitation of one span per row.
struct sFXSpanSet {
   sFXSpan *span_list;
   ushort num_spans;
   ushort first_span_used;
   ushort num_spans_used;
   ushort bounding_width;
};

// But really, we only deal with spans stored in arrays.
struct sFXSpan {
   short left, length;
};

// Some effects require temporary copies of a part of the screen.  
// We allocate the size of the bounding box of the associated
// sFXSpanSet.
struct sFXSpanSnarf {
   uchar *bits;
   ushort height, width;
   int size;
   int row;     // This could be a short if we needed another field.
};

#ifdef __cplusplus
};
#endif

#endif // ~_FXSPANS_H_
