//		LG.H		Looking Glass Over-Arching Master Control Header File
//
/*
* $Header: x:/prj/tech/libsrc/lg/RCS/lg.h 1.4 1996/04/30 18:23:20 TOML Exp $
* $Log: lg.h $
 * Revision 1.4  1996/04/30  18:23:20  TOML
 * Added support for additional log, debug message and assertions
 * 
 * Revision 1.3  1993/08/06  11:00:11  rex
 * Removed libdbg.h
 *
 * Revision 1.2  1993/03/19  18:20:22  rex
 * Added RCS header
 *
*/

#define LG_HOME

#include <types.h>
#include <lglog.h>
#include <lglocmsg.h>
#include <lgassert.h>

#ifdef LG_HOME

#include <memall.h>
#include <dbg.h>

#else

#include <lgalt.h>

#endif

