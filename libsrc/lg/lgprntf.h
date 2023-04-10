/*
 * $Source: r:/prj/lib/src/lg/rcs/lgprntf.h $
 * $Revision: 1.1 $
 * $Author: kaboom $
 * $Date: 1994/08/04 22:33:13 $
 *
 * prototypes for no-floats baby printf.
 */

#ifndef __LGPRNTF_H
#define __LGPRNTF_H
#include <stdarg.h>

extern int lg_vprintf(char *fmt,va_list arg);
extern int lg_printf(char *fmt,...);
#endif /* !__PRINTF_H */
