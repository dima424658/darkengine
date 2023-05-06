#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

void (*gen_scale_func[BMT_TYPES])() =
{
	[BMT_FLAT8] =	gen_flat8_scale,
	[BMT_RSD8] =	gen_rsd8_scale,
};