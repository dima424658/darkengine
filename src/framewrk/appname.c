/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/appname.c,v 1.7 2000/02/19 12:29:36 toml Exp $

#include <lgsprntf.h>
#include <appname.h>
#include <version.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#ifdef EDITOR
#define NAME "DromEd"
#else

#ifdef SHOCK
#define NAME "System Shock 2"
#endif

#ifdef THIEF
#define NAME "Thief 2"
#endif

#ifdef DEEPC
#define NAME "Deep Cover"
#endif

#endif

const char* AppName(void)
{
   static char buf[128];
   lg_sprintf(buf,"%s %s",NAME,VersionString());
   return buf;
}
