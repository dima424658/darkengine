#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

void (*gen_uscale_func[BMT_TYPES])() =
{
	[BMT_FLAT8] = gen_flat8_uscale,
	[BMT_RSD8] = gen_rsd8_uscale,
};