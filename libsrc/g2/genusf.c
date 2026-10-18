#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

void (*gen_uscale_func[BMT_TYPES])() =
{ 0 };

void init_gen_uscale_func(void)
{
	gen_uscale_func[BMT_FLAT8] = gen_flat8_uscale;
	gen_uscale_func[BMT_RSD8] = gen_rsd8_uscale;
}
