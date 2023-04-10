/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptimp.h,v 1.3 1997/12/19 19:08:58 TOML Exp $
//
// Everything involved with script implementation, both scripts themselves
// and tools for buliding programmer script services
//

#ifndef __SCRPTIMP_H
#define __SCRPTIMP_H

#ifndef SCRIPT
#error ("scrptimp.h only valid for use in scripts, not in app");
#endif

#include <types.h>

#include <lg.h>
#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <string.h>
#include <dynarray.h>
#include <hashset.h>
#include <matrixs.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <dbmem.h>
#include <scrptbas.h>
#include <scrptmm.h>

#endif /* !__SCRPTIMP_H */
