/*
 * $Source: x:/prj/tech/libsrc/config/RCS/cfg.c $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1995/11/27 03:14:35 $
 *
 */

#include <config.h>
#include <cfg.h>

#pragma off(unreferenced)
uint real_simple_readfunc(char* var)
{
   return 0xFFFFFFFE;
}
#pragma on(unreferenced)

errtype config_load(const char* fn)
{
   return config_read_file(fn,real_simple_readfunc);
}

#define ASSERT_OK(x) do { errtype goof = x; if (goof != OK) return goof; } while(0)
errtype config_startup(const char* fn)
{
   ASSERT_OK(config_init());
   return config_load(fn);
}
