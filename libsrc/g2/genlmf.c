#include <dev2d.h>
#include <gentf.h>
#include <tftype.h>

tmap_setup_func* gen_ulmap_setup_func[BMT_TYPES] =
{
	[BMT_FLAT8] =	gen_flat8_ulmap_setup,
	[BMT_FLAT16] =	gen_flat16_ulmap_setup,
	[BMT_RSD8] =	gen_rsd8_ulmap_setup,
};