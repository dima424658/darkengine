/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16p8f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:17:56 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <f16pix.h>

void (*flat16_upix8_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat16_norm_upix8,
	[FILL_XOR] =	flat16_xor_upix8,
	[FILL_BLEND] =	flat16_tluc_upix8,
	[FILL_CLUT] =	flat16_clut_upix8,
	[FILL_SOLID] =	flat16_solid_upix8,
};