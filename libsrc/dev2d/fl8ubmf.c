#include <bitmap.h>
#include <fill.h>
#include <fl8bm.h>
#include <genbm.h>
#include <grnull.h>

#define MAKE_INDEX(bmtType, fillType, bmfType) ((bmfType & BMF_TYPES) + BMF_TYPES * ((fillType & FILL_TYPES) + FILL_TYPES * (bmtType & BMT_TYPES)))

void (*flat8_ubitmap_func[BMT_TYPES * FILL_TYPES * BMF_TYPES])() =
{
	[MAKE_INDEX(BMT_MONO ,FILL_NORM ,BMF_TRANS)] =		flat8_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_NORM ,BMF_OPAQUE)] =		flat8_mono_opaque_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_CLUT ,BMF_TRANS)] =		flat8_mono_trans_clut_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_CLUT ,BMF_OPAQUE)] =		flat8_mono_opaque_clut_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_SOLID ,BMF_TRANS)] =		flat8_mono_trans_solid_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_SOLID ,BMF_OPAQUE)] =	gen_opaque_solid_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_BLEND ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_BLEND ,BMF_OPAQUE)] =	gen_mono_opaque_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_XOR ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO ,FILL_XOR ,BMF_OPAQUE)] =		gen_mono_opaque_ubitmap,

	[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_TRANS)] =		flat8_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_OPAQUE)] =	flat8_flat8_opaque_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_TRANS)] =		flat8_flat8_trans_clut_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_OPAQUE)] =	flat8_flat8_opaque_clut_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_TRANS)] =	flat8_flat8_trans_solid_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_OPAQUE)] =	gen_opaque_solid_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_TRANS)] =	gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_OPAQUE)] =	gen_flat8_opaque_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_TRANS)] =		gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_OPAQUE)] =		gen_flat8_opaque_ubitmap,

	[MAKE_INDEX(BMT_RSD8 ,FILL_NORM ,BMF_TRANS)] =		flat8_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_NORM ,BMF_OPAQUE)] =		flat8_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_CLUT ,BMF_TRANS)] =		gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_CLUT ,BMF_OPAQUE)] =		gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_SOLID ,BMF_TRANS)] =		gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_SOLID ,BMF_OPAQUE)] =	gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_BLEND ,BMF_TRANS)] =		gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_BLEND ,BMF_OPAQUE)] =	gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_XOR ,BMF_TRANS)] =		gen_rsd8_ubitmap,
	[MAKE_INDEX(BMT_RSD8 ,FILL_XOR ,BMF_OPAQUE)] =		gen_rsd8_ubitmap,

	[MAKE_INDEX(BMT_BANK8 ,FILL_NORM ,BMF_TRANS)] =		flat8_bank8_trans_ubitmap,
	[MAKE_INDEX(BMT_BANK8 ,FILL_NORM ,BMF_OPAQUE)] =	flat8_bank8_opaque_ubitmap,
	[MAKE_INDEX(BMT_MODEX ,FILL_NORM ,BMF_TRANS)] =		flat8_modex_trans_ubitmap,
	[MAKE_INDEX(BMT_MODEX ,FILL_NORM ,BMF_OPAQUE)] =	flat8_modex_opaque_ubitmap,
	
	/*
	[MAKE_INDEX(BMT_TLUC8, FILL_NORM, BMF_TRANS)] =		flat8_tluc8_trans_ubitmap,
	[MAKE_INDEX(BMT_TLUC8, FILL_NORM, BMF_OPAQUE)] =	flat8_tluc8_opaque_ubitmap,
	[MAKE_INDEX(BMT_TLUC8, FILL_CLUT, BMF_TRANS)] =		flat8_tluc8_trans_clut_ubitmap,
	[MAKE_INDEX(BMT_TLUC8, FILL_CLUT, BMF_OPAQUE)] =	flat8_tluc8_opaque_clut_ubitmap,
	[MAKE_INDEX(BMT_TLUC8, FILL_SOLID, BMF_TRANS)] =	flat8_tluc8_trans_solid_ubitmap,
	[MAKE_INDEX(BMT_TLUC8, FILL_SOLID, BMF_OPAQUE)] =	flat8_tluc8_opaque_solid_ubitmap,
	*/
};