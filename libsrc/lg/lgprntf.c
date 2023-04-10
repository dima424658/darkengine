/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/lgprntf.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/10/10 11:59:56 $
 *
 * no-floats baby printf.
 */

#include <io.h>
#include <stdarg.h>
#include <dbg.h>
#include <lgsprntf.h>
#include <memall.h>
#include <tmpalloc.h>

/* arbitrary maximum size of string buffer. */
#define PRINT_BUF_SIZE 2048

/* these baby-printf routines allocate a temporary buffer via the temp
   mem routines, then call lg_sprintf() which does all the real work,
   and use write() to output the string to stdout.  see comments of
   lg_sprintf() for a comprehensive list of differences between it and
   the c-library version. */

/* stripped-down vprintf which supports fixed-point numbers instead of
   floats.  fmt is the format string, and arg is the list of the rest
   of the arguments.  returns the number of characters written or <0 if
   there was an error.  this can only print a string of up to
   PRINT_BUF_SIZE. */
int lg_vprintf(char *fmt,va_list arg)
{
   char *s;                     /* buffer for string */
   int n;                       /* number of characters in string */
   int r;                       /* number of characters printed */

   /* allocate a temporary buffer, and sprintf() the string there. */
   if ((s=TempMalloc(PRINT_BUF_SIZE))==NULL) {
      Warning(("lg_vprintf: can't allocate %d byte buffer for string"));
      return 0;
   }
   if ((n=lg_vsprintf(s,fmt,arg))>=PRINT_BUF_SIZE)
      Warning(("lg_vprintf: string buffer overflow"));
   /* copy string to stdout, free buffer. */

   r=printf(s);
   TempFree(s);
   return r;      /* return #of chars or -1 if error */
}

/* stripped-down printf.  arguments are format string (fmt), and values
   to printf.  returns number of characters written. */
int lg_printf(char *fmt,...)
{
   va_list arg;                 /* argument list */
   int r;                       /* nubmer of characters printed */

   /* set up arg list and use lg_printf(). */
   va_start(arg,fmt);
   r=lg_vprintf(fmt,arg);
   va_end(arg);

   return r;
}
