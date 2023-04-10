/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcobpr.cpp,v 1.3 1999/11/19 14:58:38 adurant Exp $

#include <shkcobpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <proplist.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <nzprutil.h>
 
// must be last header
#include <dbmem.h>

//////////////////////////////////////
// Property stuff
//

typedef cSpecificProperty<ICameraObjProperty, &IID_ICameraObjProperty, cCameraObj*, cListPropertyStore<cNoZeroDataOps <cCameraObj> > > cCameraObjPropertyBase;

// property implementation class
class cCameraObjProperty: public cCameraObjPropertyBase
{
public:
   cCameraObjProperty(const sPropertyDesc* desc)
      : cCameraObjPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cCameraObj); 
};

ICameraObjProperty *g_CameraObjProperty;

sCameraObj g_defaultCameraObj = {{0, 0, 0}, {0, 0, 0}, 0, 0, 0};

static sFieldDesc CameraObjFields[] = 
{
   {"Offset", kFieldTypeVector, FieldLocation(cCameraObj, m_offset), kFieldFlagNone},
   {"Bank", kFieldTypeInt, FieldLocation(cCameraObj, m_angle.tx), kFieldFlagHex},
   {"Pitch", kFieldTypeInt, FieldLocation(cCameraObj, m_angle.ty), kFieldFlagHex},
   {"Heading", kFieldTypeInt, FieldLocation(cCameraObj, m_angle.tz), kFieldFlagHex},
   {"Lock Bank?", kFieldTypeBool, FieldLocation(cCameraObj, m_lockang[0]), kFieldFlagNone},
   {"Lock Pitch?", kFieldTypeBool, FieldLocation(cCameraObj, m_lockang[1]), kFieldFlagNone},
   {"Lock Heading?", kFieldTypeBool, FieldLocation(cCameraObj, m_lockang[2]), kFieldFlagNone},
   {"Draw?", kFieldTypeBool, FieldLocation(cCameraObj, m_draw), kFieldFlagNone},
};

static sStructDesc CameraObjStructDesc = StructDescBuild(cCameraObj, kStructFlagNone, CameraObjFields);

ICameraObjProperty *CreateCameraObjProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&CameraObjStructDesc);
   return new cCameraObjProperty(desc);
}

static sPropertyDesc CameraObjDesc = 
{
   PROP_CAMERA_OBJ_DESC, 
   kPropertyNoInherit|kPropertyInstantiate,
   NULL, 
   1,    // version
   0,
   {"Renderer", "Camera Attach"},
};

void CameraObjPropertyInit()
{
   g_CameraObjProperty = CreateCameraObjProperty(&CameraObjDesc, kPropertyImplDense);
}

void CameraObjPropertyTerm()
{
   SafeRelease(g_CameraObjProperty);
}

////////////////////////////////////

cCameraObj::cCameraObj()
{
   m_offset = g_defaultCameraObj.m_offset;
   m_angle = g_defaultCameraObj.m_angle;

   int i;
   for (i=0; i< 3; i++)
      m_lockang[i] = g_defaultCameraObj.m_lockang[i];

   m_draw = TRUE;
}
