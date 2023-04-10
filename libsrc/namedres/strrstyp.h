/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/strrstyp.h 1.2 1998/07/30 14:03:52 JUSTIN Exp $
//
// String Resource Type.
//


#ifndef _STRRSTYP_H
#pragma once
#define _STRRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_STRING "String"

//
// Use this method to get the string type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeStringResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
