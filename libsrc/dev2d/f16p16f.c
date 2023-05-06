/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16p16f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:17:35 $
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

void (*flat16_upix16_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat16_norm_upix16,
	[FILL_XOR] =	flat16_xor_upix16,
	[FILL_BLEND] =	flat16_tluc_upix16,
	[FILL_CLUT] =	flat16_clut_upix16,
	[FILL_SOLID] =	flat16_solid_upix16,
};