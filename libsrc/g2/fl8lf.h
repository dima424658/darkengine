/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8lf.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 09:54:16 $
 * 
 * This file is part of the g2 library.
 *
 */

#ifndef __FL8LF_H
#define __FL8LF_H
#include <plytyp.h>
extern void flat8_uline_norm(grs_vertex *, grs_vertex *);
extern void flat8_uline_clut(grs_vertex *, grs_vertex *);
extern void flat8_uline_solid(grs_vertex *, grs_vertex *);
extern void flat8_uline_xor(grs_vertex *, grs_vertex *);
extern void flat8_uline_blend(grs_vertex *, grs_vertex *);
#endif
