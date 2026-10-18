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
{ 0 };

void init_flat16_uhline_func(void)
{
	flat16_uhline_func[FILL_NORM] = flat16_norm_uhline;
	flat16_uhline_func[FILL_XOR] = flat16_xor_uhline;
	flat16_uhline_func[FILL_BLEND] = flat16_tluc_uhline;
	flat16_uhline_func[FILL_CLUT] = flat16_clut_uhline;
	flat16_uhline_func[FILL_SOLID] = flat16_solid_uhline;
}
