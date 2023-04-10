// lgsprntf.h -- no-floats baby sprintf

// Tim Stellmach
// LookingGlass Technologies 1993

/*
* $Header: x:/prj/tech/libsrc/lg/RCS/lgsprntf.h 1.5 1996/01/24 10:49:05 DAVET Exp $
* $Log: lgsprntf.h $
 * Revision 1.5  1996/01/24  10:49:05  DAVET
 * Added cplusplus stuff
 * 
 * Revision 1.4  1994/02/27  02:25:50  tjs
 * Changed format to const
 * 
 * Revision 1.3  1993/11/04  09:44:35  tjs
 * Added this very header.
 * 
 * 
 *
 *
 *
*/

#include <types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

int lg_sprintf(char *buf, const char *format, ...);
int lg_vsprintf(char *buf, const char *format, va_list arglist);
void lg_sprintf_install_stringfunc(char *(*func)(ulong strnum));

#ifdef __cplusplus
}
#endif  // cplusplus

