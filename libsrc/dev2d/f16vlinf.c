// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16vlinf.tbl 1.1 1998/04/02 11:47:32 KEVIN Exp $

#include <fill.h>
#include <grnull.h>
#include <f16lin.h>

void (*flat16_uvline_func[FILL_TYPES])() =
{ 0 };

void init_flat16_uvline_func(void)
{
	flat16_uvline_func[FILL_NORM] = flat16_norm_uvline;
	flat16_uvline_func[FILL_XOR] = flat16_xor_uvline;
	flat16_uvline_func[FILL_BLEND] = flat16_tluc_uvline;
	flat16_uvline_func[FILL_CLUT] = flat16_clut_uvline;
	flat16_uvline_func[FILL_SOLID] = flat16_solid_uvline;
}
