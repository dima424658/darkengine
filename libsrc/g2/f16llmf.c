#include <dev2d.h>
#include <gentf.h>
#include <f16tf.h>

#define MAKE_INDEX(bmtType, fillType, bmfType) ((bmfType) + BMF_TYPES * ((fillType) + FILL_TYPES * (bmtType)))

tmap_setup_func *flat16_lit_ulmap_setup_func[BMT_TYPES * FILL_TYPES * BMF_TYPES] =
{ 0 };

void init_flat16_lit_ulmap_setup_func(void)
{
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_TRANS)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_OPAQUE)] = opaque_lit_8to16_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_TRANS)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_OPAQUE)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_TRANS)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_OPAQUE)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_TRANS)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_OPAQUE)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_TRANS)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_OPAQUE)] = gen_flat8_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_NORM ,BMF_TRANS)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_NORM ,BMF_OPAQUE)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_CLUT ,BMF_TRANS)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_CLUT ,BMF_OPAQUE)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_SOLID ,BMF_TRANS)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_SOLID ,BMF_OPAQUE)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_BLEND ,BMF_TRANS)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_BLEND ,BMF_OPAQUE)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_XOR ,BMF_TRANS)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_FLAT16 ,FILL_XOR ,BMF_OPAQUE)] = gen_flat16_lit_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_NORM ,BMF_TRANS)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_NORM ,BMF_OPAQUE)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_CLUT ,BMF_TRANS)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_CLUT ,BMF_OPAQUE)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_SOLID ,BMF_TRANS)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_SOLID ,BMF_OPAQUE)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_BLEND ,BMF_TRANS)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_BLEND ,BMF_OPAQUE)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_XOR ,BMF_TRANS)] = gen_rsd8_ulmap_setup;
	flat16_lit_ulmap_setup_func[MAKE_INDEX(BMT_RSD8 ,FILL_XOR ,BMF_OPAQUE)] = gen_rsd8_ulmap_setup;
}
