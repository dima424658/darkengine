/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCOBPR_H
#define __DPCCOBPR_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // __MATRIXS_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

#pragma pack(4)

struct sCameraObj
{
   mxs_vector m_offset;
   mxs_angvec m_angle;
   BOOL m_lockang[3];
   BOOL m_draw;
};

class cCameraObj: public sCameraObj
{
 public:
   cCameraObj();
};

#undef INTERFACE
#define INTERFACE ICameraObjProperty

DECLARE_PROPERTY_INTERFACE(ICameraObjProperty)
{
   DECLARE_UNKNOWN_PURE();                      // IUnknown methods 
   DECLARE_PROPERTY_PURE();                     // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(cCameraObj*);     // Type-specific accessors, by reference
};

#define PROP_CAMERA_OBJ_DESC "CameraObj"

EXTERN ICameraObjProperty *g_CameraObjProperty;

EXTERN void CameraObjPropertyInit();
EXTERN void CameraObjPropertyTerm();

#pragma pack()

#endif // __DPCCOBPR_H