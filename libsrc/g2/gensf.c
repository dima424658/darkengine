#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

void (*gen_scale_func[BMT_TYPES])() =
{ 0 };

void init_gen_scale_func(void)
{
	gen_scale_func[BMT_FLAT8] = gen_flat8_scale;
	gen_scale_func[BMT_RSD8] = gen_rsd8_scale;
}
