/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxvlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:22:55 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <mxlin.h>
#include <genlin.h>

void (*modex_uvline_func[])() =
{ 0 };

void init_modex_uvline_func(void)
{
	modex_uvline_func[FILL_NORM] = modex_norm_uvline;
	modex_uvline_func[FILL_XOR] = gen_uvline;
	modex_uvline_func[FILL_BLEND] = gen_uvline;
	modex_uvline_func[FILL_CLUT] = modex_clut_uvline;
	modex_uvline_func[FILL_SOLID] = modex_solid_uvline;
}
