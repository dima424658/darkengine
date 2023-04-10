/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprdev.cpp,v 1.9 2000/02/19 12:45:11 toml Exp $
//
// Device ability properties
//

#include <aiprdev.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <prophash.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

IAIDeviceProperty *     g_pAIDeviceProperty;

////////////////////////////////////////

const sAIDeviceParams g_AIDefaultDeviceParams =
{
   0,    // activate joint num
   0,    // inactive posn
   2,    // active posn
   0.1,  // activate speed
   1,    // rotational joint
   0.1,  // facing epsilon
   0,    // raise / rotate on activate
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Device", Complex
//

static sFieldDesc DevicePropertyFields[] = 
{
   { "Activate Joint", kFieldTypeInt, FieldLocation(sAIDeviceParams, m_jointActivate),  kFieldFlagNone },
   { "Inactive Posn", kFieldTypeFloat, FieldLocation(sAIDeviceParams, m_inactivePos),  kFieldFlagNone },
   { "Active Posn", kFieldTypeFloat, FieldLocation(sAIDeviceParams, m_activePos),  kFieldFlagNone },
   { "Activate Speed", kFieldTypeFloat, FieldLocation(sAIDeviceParams, m_activateSpeed),  kFieldFlagNone },
   { "Rotational Joint", kFieldTypeInt, FieldLocation(sAIDeviceParams, m_jointRotate),  kFieldFlagNone },
   { "Facing Epsilon", kFieldTypeFloat, FieldLocation(sAIDeviceParams, m_facingEpsilon),  kFieldFlagNone },
   { "Rotate to Activate", kFieldTypeBool, FieldLocation(sAIDeviceParams, m_activateRotate),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AIDevicePropertyDesc = 
{
   PROP_AI_Device,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Device: parameters" },
   kPropertyChangeLocally,  // net_flags
};

////////////////////////////////////////

class cAIDeviceParams : public sAIDeviceParams
{
public:
   cAIDeviceParams() { *this = *(cAIDeviceParams *)&g_AIDefaultDeviceParams; }
};

static sStructDesc DeviceDesc = StructDescBuild(cAIDeviceParams, kStructFlagNone, DevicePropertyFields);

////////////////////////////////////////

class cDeviceOps : public cClassDataOps<cAIDeviceParams>
{
public:
   cDeviceOps() : cClassDataOps<cAIDeviceParams>(kNoFlags) {};
};

typedef cHashPropertyStore<cDeviceOps> cDevicePropertyStore;

typedef cSpecificProperty<IAIDeviceProperty, &IID_IAIDeviceProperty, sAIDeviceParams *, cDevicePropertyStore> cBaseDeviceProp;

class cAIDeviceProp : public cBaseDeviceProp
{
public:
   cAIDeviceProp(const sPropertyDesc* desc) : cBaseDeviceProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIDeviceParams);
};

///////////////////////////////////////////////////////////////////////////////

IAITurretProperty *     g_pAITurretProperty;

////////////////////////////////////////

const sAITurretParams g_AIDefaultTurretParams =
{
   0.1,  // fire epsilon
   0,    // fire pause
   10,   // pitch epsilon
   100,  // max range
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Turret", Complex
//

static sFieldDesc TurretPropertyFields[] = 
{
   { "Fire Epsilon", kFieldTypeFloat, FieldLocation(sAITurretParams, m_fireEpsilon),  kFieldFlagNone },
   { "Fire Pause", kFieldTypeInt, FieldLocation(sAITurretParams, m_firePause),  kFieldFlagNone },
   { "Pitch Epsilon", kFieldTypeFloat, FieldLocation(sAITurretParams, m_pitchEpsilon),  kFieldFlagNone },
   { "Max Range", kFieldTypeFloat, FieldLocation(sAITurretParams, m_maxRange),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AITurretPropertyDesc = 
{
   PROP_AI_Turret,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Turret: parameters" },
   kPropertyChangeLocally,  // net_flags
};

////////////////////////////////////////

class cAITurretParams : public sAITurretParams
{
public:
   cAITurretParams() { *this = *(cAITurretParams *)&g_AIDefaultTurretParams; }
};

static sStructDesc TurretDesc = StructDescBuild(cAITurretParams, kStructFlagNone, TurretPropertyFields);

////////////////////////////////////////

class cTurretOps : public cClassDataOps<cAITurretParams>
{
public:
   cTurretOps() : cClassDataOps<cAITurretParams>(kNoFlags) {};
};

typedef cHashPropertyStore<cTurretOps> cTurretPropertyStore;

typedef cSpecificProperty<IAITurretProperty, &IID_IAITurretProperty, sAITurretParams *, cTurretPropertyStore> cBaseTurretProp;

class cAITurretProp : public cBaseTurretProp
{
public:
   cAITurretProp(const sPropertyDesc* desc) : cBaseTurretProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAITurretParams);
};

///////////////////////////////////////////////////////////////////////////////

IAICameraProperty *     g_pAICameraProperty;

////////////////////////////////////////

const sAICameraParams g_AIDefaultCameraParams =
{
   -180, // scan angle 1
   180,  // scan angle 2
   0.05,  // scan rate (deg/msec)
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Camera", Complex
//

static sFieldDesc CameraPropertyFields[] = 
{
   { "Scan Angle 1 (deg)", kFieldTypeFloat, FieldLocation(sAICameraParams, m_scanAngle1),  kFieldFlagNone },
   { "Scan Angle 2", kFieldTypeFloat, FieldLocation(sAICameraParams, m_scanAngle2),  kFieldFlagNone },
   { "Scan Speed (deg/msec)", kFieldTypeFloat, FieldLocation(sAICameraParams, m_scanSpeed),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AICameraPropertyDesc = 
{
   PROP_AI_Camera,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Camera: parameters" },
};

////////////////////////////////////////

class cAICameraParams : public sAICameraParams
{
public:
   cAICameraParams() { *this = *(cAICameraParams *)&g_AIDefaultCameraParams; }
};

static sStructDesc CameraDesc = StructDescBuild(cAICameraParams, kStructFlagNone, CameraPropertyFields);

////////////////////////////////////////

class cCameraOps : public cClassDataOps<cAICameraParams>
{
public:
   cCameraOps() : cClassDataOps<cAICameraParams>(kNoFlags) {};
};

typedef cHashPropertyStore<cCameraOps> cCameraPropertyStore;

typedef cSpecificProperty<IAICameraProperty, &IID_IAICameraProperty, sAICameraParams *, cCameraPropertyStore> cBaseCameraProp;

class cAICameraProp : public cBaseCameraProp
{
public:
   cAICameraProp(const sPropertyDesc* desc) : cBaseCameraProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAICameraParams);
};

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitDeviceProp(void)
{
   g_pAIDeviceProperty = new cAIDeviceProp(&g_AIDevicePropertyDesc);
   StructDescRegister(&DeviceDesc);
}

///////////////////////////////////////

void AITermDeviceProp(void)
{
   SafeRelease(g_pAIDeviceProperty);
}

///////////////////////////////////////

void AIInitCameraProp(void)
{
   g_pAICameraProperty = new cAICameraProp(&g_AICameraPropertyDesc);
   StructDescRegister(&CameraDesc);
}

///////////////////////////////////////

void AITermCameraProp(void)
{
   SafeRelease(g_pAICameraProperty);
}

///////////////////////////////////////

void AIInitTurretProp(void)
{
   g_pAITurretProperty = new cAITurretProp(&g_AITurretPropertyDesc);
   StructDescRegister(&TurretDesc);
}

///////////////////////////////////////

void AITermTurretProp(void)
{
   SafeRelease(g_pAITurretProperty);
}

///////////////////////////////////////////////////////////////////////////////


