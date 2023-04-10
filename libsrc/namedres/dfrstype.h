/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/rcs/dfrstype.h 1.3 1998/07/30 14:01:17 JUSTIN Exp $
//
// Default Resource Type. This type is the "raw binary" type -- the Locked
// data will be exactly what was found on disk, with no processing, and all
// the default resource methods.
//


#ifndef _DFRSTYPE_H
#pragma once
#define _DFRSTYPE_H

#include <comtools.h>

#define RESTYPE_BINARY "RawBinary"

F_DECLARE_INTERFACE(IResType);

//
// Use this method to get the image type; register that with the Resource
// Manager to use that type from here on out.
//
// Note that, this being the default, it will be automatically registered
// with the Resource Manager under normal circumstances; applications
// generally need not do so.
//
EXTERN IResType *MakeDefaultResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
