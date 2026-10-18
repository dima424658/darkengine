#include <dev2d.h>
#include <gensf.h>
#include <fl8sf.h>
#include <swarn.h>

#define MAKE_INDEX(bmtType, fillType, bmfType) ((bmfType) + BMF_TYPES * ((fillType) + FILL_TYPES * (bmtType)))

void (*flat8_scale_func[BMT_TYPES * FILL_TYPES * BMF_TYPES])() =
{ 0 };

void init_flat8_scale_func(void)
{
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_TRANS)] = trans_8to8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_OPAQUE)] = opaque_8to8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_TRANS)] = trans_clut_8to8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_OPAQUE)] = opaque_clut_8to8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_TRANS)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_OPAQUE)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_TRANS)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_OPAQUE)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_TRANS)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_OPAQUE)] = gen_flat8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_TRANS)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_OPAQUE)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_TRANS)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_OPAQUE)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_TRANS)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_OPAQUE)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_TRANS)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_OPAQUE)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_TRANS)] = gen_rsd8_scale;
	flat8_scale_func[MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_OPAQUE)] = gen_rsd8_scale;
}
