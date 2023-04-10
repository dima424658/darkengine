/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/objmedia.h,v 1.3 2000/01/29 13:32:36 adurant Exp $
//
// Object media header
//
#pragma once

#ifndef __OBJMEDIA_H
#define __OBJMEDIA_H

#include <objtype.h>

///////////////////////////////////////

enum eMediaState
{
   kMS_Invalid          = 0x0000,
   kMS_Air              = 0x0001,
   kMS_Liquid_Standing  = 0x0002,
   kMS_Liquid_Wading    = 0x0004,
   kMS_Liquid_Submerged = 0x0008,
};

////////////////////////////////////////

EXTERN void ObjMediaTrans(ObjID objID, int cellID, eMediaState old_medium, eMediaState new_medium);

EXTERN int ObjMediumToPortalMedium(eMediaState obj_medium);
EXTERN eMediaState PortalMediumToObjMedium(int portal_medium);

////////////////////////////////////////

#endif
