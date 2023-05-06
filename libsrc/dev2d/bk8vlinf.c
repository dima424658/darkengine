/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8vlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:20:06 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <bk8lin.h>

void (*bank8_uvline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	bank8_norm_uvline,
	[FILL_XOR] =	bank8_xor_uvline,
	[FILL_BLEND] =	bank8_tluc_uvline,
	[FILL_CLUT] =	bank8_clut_uvline,
	[FILL_SOLID] =	bank8_solid_uvline,
};