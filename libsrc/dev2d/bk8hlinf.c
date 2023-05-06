/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8hlinf.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:19:01 $
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

void (*bank8_uhline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	bank8_norm_uhline,
	[FILL_XOR] =	bank8_xor_uhline,
	[FILL_BLEND] =	bank8_tluc_uhline,
	[FILL_CLUT] =	bank8_clut_uhline,
	[FILL_SOLID] =	bank8_solid_uhline,
};