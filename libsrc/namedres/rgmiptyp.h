/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*=========================================================

  Created:  11/1/99 11:26:54 AM

  File:  RGMipTyp.h

  Description:  


=========================================================*/

// $Header: x:/prj/tech/libsrc/namedres/RCS/rgmiptyp.h 1.1 1970/01/01 00:00:00 Zarko Exp $

#pragma once 

#ifndef _RGMIPTYP_H
#define _RGMIPTYP_H


//_____INCLUDES_AND_DEFINITIONS___________________________


#include <comtools.h>

F_DECLARE_INTERFACE(IResType);

#define RESTYPE_RG_MIP "RGMip"



//______EXPORTED_DATA_____________________________________

//
// Use this method to get the image type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IResType *MakeRGMipResourceType();



#endif //_RGMIPTYP_H