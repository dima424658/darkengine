#include <bitmap.h>
#include <genbm.h>
#include <grnull.h>

gdbm_func *gen_bitmap_func[BMT_TYPES] =
{ 0 };

void init_gen_bitmap_func(void)
{
	gen_bitmap_func[BMT_MONO] = gen_mono_bitmap;
	gen_bitmap_func[BMT_FLAT8] = gen_flat8_bitmap;
	gen_bitmap_func[BMT_BANK8] = gen_flat8_bitmap;
	gen_bitmap_func[BMT_MODEX] = gen_modex_bitmap;
	gen_bitmap_func[BMT_TLUC8] = gen_flat8_bitmap;
	gen_bitmap_func[BMT_FLAT16] = gen_flat16_bitmap;
	gen_bitmap_func[BMT_RSD8] = gen_rsd8_bitmap;
}
