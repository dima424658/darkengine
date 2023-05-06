#include <dev2d.h>
#include <fl8lf.h>
#include <lftype.h>

g2ul_func* flat8_uline_func[FILL_TYPES] =
{
	[FILL_NORM] =	flat8_uline_norm,
	[FILL_CLUT] =	flat8_uline_clut,
	[FILL_SOLID] =	flat8_uline_solid,
	[FILL_BLEND] =	flat8_uline_blend,
	[FILL_XOR] =	flat8_uline_xor,
};