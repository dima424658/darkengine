/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/goof.c,v 1.3 2000/02/19 12:14:17 toml Exp $

#include <fix.h>

#ifdef WIN32

  #include <windows.h>

  fix fixtime(void)
  {
     return GetTickCount() * 65 / 1;   // this isn't very accurate, duh
  }

#else

  #include <timer.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
 
  fix fixtime(void)
  {
     return tm_get_fix();
  }

#endif
