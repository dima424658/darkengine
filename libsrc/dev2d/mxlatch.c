/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/mxlatch.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/10/21 09:33:36 $
 *
 * Mode X latch globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <types.h>
volatile uchar modex_wlatch = 0;
volatile uchar modex_rlatch = 0;
volatile int wlatch_semaphore = 0;
volatile int rlatch_semaphore = 0;

uchar modex_lmask[] = {  15, 14, 12, 8 };
uchar modex_rmask[] = {  1,  3,  7, 15 };
