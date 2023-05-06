// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16vlinf.tbl 1.1 1998/04/02 11:47:32 KEVIN Exp $

#include <fill.h>
#include <grnull.h>
#include <f16lin.h>

void (*flat16_uvline_func[FILL_TYPES])() =
{
	[FILL_NORM] =	flat16_norm_uvline,
	[FILL_XOR] =	flat16_xor_uvline,
	[FILL_BLEND] =	flat16_tluc_uvline,
	[FILL_CLUT] =	flat16_clut_uvline,
	[FILL_SOLID] =	flat16_solid_uvline,
};