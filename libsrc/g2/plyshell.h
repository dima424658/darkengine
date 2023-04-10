// $Header: x:/prj/tech/libsrc/g2/RCS/plyshell.h 1.3 1997/05/01 15:38:08 KEVIN Exp $

#ifndef __PLYSHELL_H
#define __PLYSHELL_H

#include <g2spoint.h>
#include <plyrast.h>
#include <plyparam.h>

extern void g2_poly_shell(g2s_poly_params *p, int num_scanlines);
extern void g2_trifan(g2s_poly_params *p, int n, g2s_point **vpl);

#endif
