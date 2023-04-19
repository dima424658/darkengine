/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/utilcomp.h 1.2 1996/09/19 17:58:11 xemu Exp $

#ifndef __UTILCOMP_H
#define __UTILCOMP_H

#include <lzw.h>

#define UTIL2D_COMPRESS_STATE_SIZE     LZW_PARTIAL_STATE_SIZE
#define UTIL2D_COMPRESS_BUFFER_SIZE    LZW_BUFF_SIZE

typedef struct {
   void *lzw_state;
   uchar *lzw_buffer;
   int init_state;
} UtilCompressState;

typedef void (*UtilCompressCallback)(void);

#endif
