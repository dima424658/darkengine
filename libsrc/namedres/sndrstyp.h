/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/sndrstyp.h 1.3 1998/07/30 14:03:20 JUSTIN Exp $
//
// Sound Resource Type.
//

#ifndef _SNDRSTYP_H
#pragma once
#define _SNDRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_SOUND "Sound"

//
// Use this method to get the sound type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeSoundResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
