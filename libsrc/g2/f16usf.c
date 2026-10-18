#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

#define MAKE_INDEX(bmtType, fillType, bmfType) ((bmfType) + BMF_TYPES * ((fillType) + FILL_TYPES * (bmtType)))

void (*flat16_uscale_func[BMT_TYPES * FILL_TYPES * BMF_TYPES])() =
{ 0 };

void init_flat16_uscale_func(void)
{
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_TRANS)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_OPAQUE)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_TRANS)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_OPAQUE)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_TRANS)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_OPAQUE)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_TRANS)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_OPAQUE)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_TRANS)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_OPAQUE)] = gen_flat8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_TRANS)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_OPAQUE)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_TRANS)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_OPAQUE)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_TRANS)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_OPAQUE)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_TRANS)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_OPAQUE)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_TRANS)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_OPAQUE)] = gen_rsd8_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_NORM, BMF_TRANS)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_NORM, BMF_OPAQUE)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_CLUT, BMF_TRANS)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_CLUT, BMF_OPAQUE)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_SOLID, BMF_TRANS)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_SOLID, BMF_OPAQUE)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_BLEND, BMF_TRANS)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_BLEND, BMF_OPAQUE)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_XOR, BMF_TRANS)] = gen_flat16_uscale;
	flat16_uscale_func[MAKE_INDEX(BMT_FLAT16, FILL_XOR, BMF_OPAQUE)] = gen_flat16_uscale;
}
