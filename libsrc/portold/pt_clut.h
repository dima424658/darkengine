/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/pt_clut.h,v 1.2 1996/09/10 18:25:20 buzzard Exp $

#ifndef __PT_CLUT_H
#define __PT_CLUT_H

typedef struct st_ClutChain
{
   struct st_ClutChain *next;
   uchar clut_id, clut_id2;
   uchar pad0,pad1;
} ClutChain;

uchar *pt_get_clut(ClutChain *cc);

#endif
