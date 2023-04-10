/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/mediaop.h,v 1.3 2000/01/29 13:19:48 adurant Exp $
#pragma once

// operations on media

#ifndef __MEDIA_H
#define __MEDIA_H

#include <lg.h>

#define MAX_MEDIA  16
#define MAX_OP     16

typedef uchar MediaOp[MAX_MEDIA];

EXTERN MediaOp media_op[MAX_OP];

#endif
