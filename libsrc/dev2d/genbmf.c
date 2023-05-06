#include <bitmap.h>
#include <genbm.h>
#include <grnull.h>

void (*gen_bitmap_func[BMT_TYPES])() =
{
	[BMT_MONO] =	gen_mono_bitmap,
	[BMT_FLAT8] =	gen_flat8_bitmap,
	[BMT_BANK8] =	gen_flat8_bitmap,
	[BMT_MODEX] =	gen_modex_bitmap,
	[BMT_TLUC8] =	gen_flat8_bitmap,
	[BMT_FLAT16] =	gen_flat16_bitmap,
	[BMT_RSD8] =	gen_rsd8_bitmap,
};