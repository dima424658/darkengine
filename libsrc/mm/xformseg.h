// $Header:
// this module contains the various segment transforming routines

#ifndef __XFORMSEG_H
#define __XFORMSEG_H

#include <mms.h>

void mm_transform_seg_prim_layout(mms_segment *s);
void mm_transform_seg_any(mms_segment *s, int segment_index);

void mm_o2w_seg_any(mms_segment *s);

#endif
