#include <bitmap.h>
#include <fill.h>
#include <bk8bm.h>
#include <genbm.h>
#include <grnull.h>

#define MAKE_INDEX(bmtType, fillType, bmfType) ((bmfType & BMF_TYPES) + BMF_TYPES * ((fillType & FILL_TYPES) + FILL_TYPES * (bmtType & BMT_TYPES)))

void (*bank8_ubitmap_func[BMF_TYPES * FILL_TYPES * BMT_TYPES])() =
{
	[MAKE_INDEX(BMT_MONO, FILL_NORM ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_NORM ,BMF_OPAQUE)] =		gen_mono_opaque_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_CLUT ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_CLUT ,BMF_OPAQUE)] =		gen_mono_opaque_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_SOLID ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_SOLID ,BMF_OPAQUE)] =	gen_opaque_solid_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_BLEND ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_BLEND ,BMF_OPAQUE)] =	gen_mono_opaque_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_XOR ,BMF_TRANS)] =		gen_mono_trans_ubitmap,
	[MAKE_INDEX(BMT_MONO, FILL_XOR ,BMF_OPAQUE)] =		gen_mono_opaque_ubitmap,

	[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_TRANS)] =		bank8_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_NORM ,BMF_OPAQUE)] =	bank8_flat8_opaque_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_TRANS)] =		gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_CLUT ,BMF_OPAQUE)] =	gen_flat8_opaque_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_TRANS)] =	gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_SOLID ,BMF_OPAQUE)] =	gen_opaque_solid_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_TRANS)] =	gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_BLEND ,BMF_OPAQUE)] =	gen_flat8_opaque_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_TRANS)] =		gen_flat8_trans_ubitmap,
	[MAKE_INDEX(BMT_FLAT8 ,FILL_XOR ,BMF_OPAQUE)] =		gen_flat8_opaque_ubitmap,

	/*
	[MAKE_INDEX(BMT_FLAT8, FILL_CLUT ,BMF_TRANS)] =		bank8_flat8_trans_clut_ubitmap,
	[MAKE_INDEX(BMT_FLAT8, FILL_CLUT ,BMF_OPAQUE)] =	bank8_flat8_opaque_clut_ubitmap,
	[MAKE_INDEX(BMT_FLAT8, FILL_SOLID ,BMF_TRANS)] =	bank8_flat8_trans_solid_ubitmap,

	[MAKE_INDEX(BMT_TLUC8 ,FILL_NORM ,BMF_TRANS)] =		bank8_tluc8_trans_ubitmap,
	[MAKE_INDEX(BMT_TLUC8 ,FILL_NORM ,BMF_OPAQUE)] =	bank8_tluc8_opaque_ubitmap,
	[MAKE_INDEX(BMT_TLUC8 ,FILL_CLUT ,BMF_TRANS)] =		bank8_tluc8_trans_clut_ubitmap,
	[MAKE_INDEX(BMT_TLUC8 ,FILL_CLUT ,BMF_OPAQUE)] =	bank8_tluc8_opaque_clut_ubitmap,
	[MAKE_INDEX(BMT_TLUC8 ,FILL_SOLID ,BMF_TRANS)] =	bank8_tluc8_trans_solid_ubitmap,
	[MAKE_INDEX(BMT_TLUC8 ,FILL_SOLID ,BMF_OPAQUE)] =	bank8_tluc8_opaque_solid_ubitmap,
	*/
};