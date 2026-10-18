#include <dev2d.h>
#include <fl8lf.h>
#include <lftype.h>

g2ul_func* flat8_uline_func[FILL_TYPES] =
{ 0 };

void init_flat8_uline_func(void)
{
	flat8_uline_func[FILL_NORM] = flat8_uline_norm;
	flat8_uline_func[FILL_CLUT] = flat8_uline_clut;
	flat8_uline_func[FILL_SOLID] = flat8_uline_solid;
	flat8_uline_func[FILL_BLEND] = flat8_uline_blend;
	flat8_uline_func[FILL_XOR] = flat8_uline_xor;
}
