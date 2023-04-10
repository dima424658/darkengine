/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/pch.h,v 1.2 2000/02/21 12:14:42 toml Exp $
//

#ifndef INCLUDED_PCH_H
#define INCLUDED_PCH_H

#pragma once

#if defined(__cplusplus) && !defined(NO_PCH)

#include <types.h>

#pragma warning (disable:4786)
#pragma warning (disable:4200)
#pragma warning (disable:4042)
#pragma warning (disable:4390)
#pragma warning (disable:4005)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MEMALL_H

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <refsys.h>

#include <dev2d.h>
#include <resapi.h>
#include <storeapi.h>

#include <dynarray.h>
#include <hashset.h>
#include <hashpp.h>

#include <aggmemb.h>
#include <matrix.h>
#include <mprintf.h>
#include <matrixs.h>
#include <loopapi.h>
#include <gadget.h>

#include <dataops.h>
#include <objpos.h>
#include <wrtype.h>
#include <iobjsys.h>
#include <objdef.h>
#include <config.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <property.h>
#include <propface.h>

#include <autolink.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <linkman.h>
#include <traitman.h>
#include <objpos.h>
#include <propbase.h>
#include <iobjsys.h>
#include <objdef.h>
#include <playrobj.h>

#include <aiutils.h>
#include <aidebug.h>

#define __NZPRUTIL_H

#include <mds.h>

#undef MEMALL_H
#include <memall.h>
#include <dataops_.h>
#include <aiprops.h>

#endif /*__cplusplus */
#endif /* !INCLUDED_PCH_H */
