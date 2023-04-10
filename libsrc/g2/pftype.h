/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/pftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 09:57:12 $
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __PFTYPE_H
#define __PFTYPE_H

#include <plytyp.h>
typedef void (g2up_func)(int c, int n, grs_vertex **vpl);
typedef g2up_func *g2up_func_p;
typedef g2up_func_p (g2up_expose_func)(int c, int n, grs_vertex **vpl);

typedef int (g2p_func)(int c, int n, grs_vertex **vpl);
typedef g2p_func *g2p_func_p;
typedef g2p_func_p (g2p_expose_func)(int c, int n, grs_vertex **vpl);

#endif

