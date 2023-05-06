/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxhlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:21:54 $
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

void (*modex_uhline_func[])() =
{
	[FILL_NORM] =	modex_norm_uhline,
	[FILL_XOR] =	gen_uhline,
	[FILL_BLEND] =	gen_uhline,
	[FILL_CLUT] =	modex_clut_uhline,
	[FILL_SOLID] =	modex_solid_uhline,
};