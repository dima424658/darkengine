/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/status.h,v 1.7 2000/01/29 13:13:13 adurant Exp $
#pragma once

#ifndef __STATUS_H
#define __STATUS_H

#define SF_FILESTATE   0
#define SF_FILENAME    1
#define SF_GAMESYSNAME 2
#define SF_CUR_ITEM    3
#define SF_STATUS      4
#define SF_UNUSED1     5
#define SF_UNUSED2     6
#define SF_TIME        7

#define SF_NUM_FIELDS (SF_TIME+1)

EXTERN void StatusField(int x, const char *s);
EXTERN void Status(const char *s);

EXTERN void StatusEnable(void);
EXTERN void StatusDisable(void);
EXTERN void StatusUpdate(void);

EXTERN void StatusDrawStringAll(void);

#endif

