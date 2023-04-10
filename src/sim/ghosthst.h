/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghosthst.h,v 1.1 1999/08/05 18:08:08 Justin Exp $
// histogram/analysis for ghost bandwidth/etc

#ifndef __GHOSTHST_H
#pragma once
#define __GHOSTHST_H

#ifdef PLAYTEST
// add a sample
EXTERN void _ghost_histo_add(ObjID ghost, int reason, int time);

// clear/show the data
EXTERN void _ghost_histo_clear(void);
EXTERN void _ghost_histo_show(char *cmd);

// init/term
EXTERN void _ghost_histo_init(int num_reasons, int short_time);
EXTERN void _ghost_histo_term(void);

#else  // PLAYTEST
#define _ghost_histo_add(ghost,reason,time)
#define _ghost_histo_clear()
#define _ghost_histo_show(cmd)
#define _ghost_histo_init(num_reasons,short_time)
#define _ghost_histo_term()
#endif // PLAYTEST

#endif  // __GHOSTHST_H
