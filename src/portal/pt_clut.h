/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/pt_clut.h,v 1.3 2000/01/29 13:37:37 adurant Exp $
#pragma once

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
