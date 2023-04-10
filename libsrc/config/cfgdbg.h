#ifndef __CFGDBG_H
#define __CFGDBG_H
/*
 * $Source: r:/prj/lib/src/config/rcs/cfgdbg.h $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1995/04/10 04:54:48 $
 *
 */
#include <stdarg.h>
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus



// WACKY CONFIG DEBUG SPEW LIBRARY


// this is braindamaged, but it is rex-compiant.
bool CfgSpewTest(char* var);
void CfgDoSpew(char* msg, ...);

// is this system on at all?  defaults to TRUE
extern bool config_spew_on;

#ifdef SPEW_ON

#define ConfigSpew(var, msg) do { if (config_spew_on && CfgSpewTest(var)) CfgDoSpew msg; } while(0)
   

#else // !SPEW_ON

#define ConfigSpew(var, msg) 

#endif 

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __CFGDBG_H


