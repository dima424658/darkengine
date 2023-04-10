/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/RCS/mdlrstyp.h 1.1 1998/10/30 22:15:06 JUSTIN Exp $
//
// Model Resource Type. This should work for both MD Models and Mesh models.
//

#ifndef _MDLRSTYP_H
#pragma once
#define _MDLRSTYP_H

#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_MODEL "Model"

//
// Use this method to get the model type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeModelResourceType();

#endif

//////////////////////////////////////////////////////////////////////////
