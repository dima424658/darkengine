/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdesc.h,v 1.14 2000/01/31 09:50:59 adurant Exp $
#pragma once

#ifndef __MOTDESC_H
#define __MOTDESC_H

#include <motion.h>
#include <motdbase.h>

EXTERN int MotDescNumMotions();
EXTERN BOOL MotDescNeckFixed(int index);
EXTERN char *MotDescGetName(int index);
EXTERN int MotDescNameGetNum(char *name);
EXTERN int MotDescBlendLength(int index);

#endif
