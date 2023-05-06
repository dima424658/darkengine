/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxp8f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:22:20 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <mxpix.h>

void (*modex_upix8_func[])() =
{
	[FILL_NORM] =	modex_norm_upix8,
	[FILL_XOR] =	modex_xor_upix8,
	[FILL_BLEND] =	modex_tluc_upix8,
	[FILL_CLUT] =	modex_clut_upix8,
	[FILL_SOLID] =	modex_solid_upix8,
};