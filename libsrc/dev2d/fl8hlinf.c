/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8hlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:12:03 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <fl8lin.h>

void (*flat8_uhline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat8_norm_uhline,
	[FILL_XOR] =	flat8_xor_uhline,
	[FILL_BLEND] =	flat8_tluc_uhline,
	[FILL_CLUT] =	flat8_clut_uhline,
	[FILL_SOLID] =	flat8_solid_uhline,
};