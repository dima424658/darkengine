/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/RCS/fonrstyp.h 1.1 1998/08/19 20:03:42 JUSTIN Exp $
//
// Font Resource Type.
//

#ifndef _FNTRSTYP_H
#pragma once
#define _FNTRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_FONT "Font"

//
// Use this method to get the font type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeFontResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
