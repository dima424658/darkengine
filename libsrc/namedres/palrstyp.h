/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/palrstyp.h 1.2 1998/07/30 14:02:44 JUSTIN Exp $
//
// Palette Resource Type.
//


#ifndef _PALRSTYP_H
#pragma once
#define _PALRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_PALETTE "Palette"

//
// Use this method to get the palette type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakePaletteResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
