/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/goof.c,v 1.3 2000/02/19 12:14:17 toml Exp $

#include <fix.h>
#include <platform_services.h>

fix fixtime(void)
{
   return PlatformMilliseconds() * 65;
}
