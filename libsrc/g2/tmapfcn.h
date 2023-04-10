/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/tmapfcn.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 09:56:35 $
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __TMAPFCN_H
#define __TMAPFCN_H
#include <plytyp.h>
#include <tmapf.h>
extern void g2_calc_deltas(grs_vertex **vpl, int flags);
extern void lmap_shell(grs_vertex **vpl);
extern void lit_lmap_shell(grs_vertex **vpl);
extern void spoly_shell(grs_vertex **vpl);
extern void do_lmap(int n, grs_vertex **vpl, void (*shell)());
#endif
