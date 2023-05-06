/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16hlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:17:14 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */
#include <fill.h>
#include <grnull.h>
#include <f16lin.h>

void (*flat16_uhline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat16_norm_uhline,
	[FILL_XOR] =	flat16_xor_uhline,
	[FILL_BLEND] =	flat16_tluc_uhline,
	[FILL_CLUT] =	flat16_clut_uhline,
	[FILL_SOLID] =	flat16_solid_uhline,
};