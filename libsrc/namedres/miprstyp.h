/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/RCS/miprstyp.h 1.1 1999/07/26 11:04:39 JAEMZ Exp $
//
// Mipmap Resource Type.
// A mipmap is a conceptual single texture map, but
// processed so as to exist as a pyramid of images
// filtered and downsampled a factor of two each step of the way


#ifndef _MIPRSTYP_H
#pragma once
#define _MIPRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_MIP "Mip"

//
// Use this method to get the image type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeMipResourceType();

#endif

//////////////////////////////////////////////////////////////////////////








