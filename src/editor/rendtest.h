/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/rendtest.h,v 1.2 2000/01/29 13:13:07 adurant Exp $
#pragma once

#ifndef __RENDTEST_H
#define __RENDTEST_H

#ifndef SHIP
// the rendtest command itself
EXTERN void do_rendtest_cmd(int cnt);
EXTERN void do_rendtest_rerun_cmd(char *fname);
#endif  // SHIP

#endif  // __RENDTEST_H
