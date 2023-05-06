/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8p8f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:12:44 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <fl8pix.h>

void (*flat8_upix8_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat8_norm_upix8,
	[FILL_XOR] =	flat8_xor_upix8,
	[FILL_BLEND] =	flat8_tluc_upix8,
	[FILL_CLUT] =	flat8_clut_upix8,
	[FILL_SOLID] =	flat8_solid_upix8,
};