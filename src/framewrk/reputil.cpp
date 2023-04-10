/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/reputil.cpp,v 1.2 2000/02/19 13:16:30 toml Exp $
// report system

#ifndef SHIP

#include <report.h>

#include <stddef.h>
#include <stdarg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////
// handy dandy buffer stuffer

int rsprintf(char ** buf, const char * format, ...)
{
   va_list arg_ptr;
   va_start(arg_ptr, format);
   int n = vsprintf(*buf, format, arg_ptr);
   va_end(arg_ptr);
   *buf += n;
   return n;
}

#endif


