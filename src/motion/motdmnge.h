/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdmnge.h,v 1.3 2000/01/31 09:51:00 adurant Exp $
#pragma once

#ifndef __MOTDMNGE_H
#define __MOTDMNGE_H

EXTERN void MotDmngeInit(int num_motions);
EXTERN void MotDmngeClose();
EXTERN void MotDmngeLock(int mot_num);
EXTERN void MotDmngeUnlock(int mot_num);


#endif
