/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/imgrstyp.h 1.3 1998/07/30 14:02:07 JUSTIN Exp $
//
// Image Resource Type.
//


#ifndef _IMGRSTYP_H
#pragma once
#define _IMGRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_IMAGE "Image"

//
// Use this method to get the image type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeImageResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
