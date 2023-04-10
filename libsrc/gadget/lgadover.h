/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/lgadover.h,v 1.1 1996/05/21 13:38:38 xemu Exp $

#ifndef __LGADOVER_H
#define __LGADOVER_H

typedef enum {OverlayUpdate,OverlayDelete,OverlayNoChange} OverlayStatus;

typedef OverlayStatus (*OverlayUpdateFunc)(void *arg);

#endif