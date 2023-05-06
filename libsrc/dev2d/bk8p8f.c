/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8p8f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:19:27 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#include <fill.h>
#include <grnull.h>
#include <bk8pix.h>

void (*bank8_upix8_func[FILL_TYPES])() =
{
	[FILL_NORM] =	bank8_norm_upix8,
	[FILL_XOR] =	bank8_xor_upix8,
	[FILL_BLEND] =	bank8_tluc_upix8,
	[FILL_CLUT] =	bank8_clut_upix8,
	[FILL_SOLID] =	bank8_solid_upix8,
};