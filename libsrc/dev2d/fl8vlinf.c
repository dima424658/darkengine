/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8vlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:15:55 $
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

void (*flat8_uvline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat8_norm_uvline,
	[FILL_XOR] =	flat8_xor_uvline,
	[FILL_BLEND] =	flat8_tluc_uvline,
	[FILL_CLUT] =	flat8_clut_uvline,
	[FILL_SOLID] =	flat8_solid_uvline,
};