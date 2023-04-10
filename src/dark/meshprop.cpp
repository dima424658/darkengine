/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/meshprop.cpp,v 1.1 1999/11/02 16:15:27 adurant Exp $
//
// core weapon (hand-to-hand-combat) routines
//

#include <lg.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>

#include <relation.h>
#include <linkbase.h>

#include <meshprop.h>

#include <sdesbase.h>
#include <sdestool.h>

#include <matrixc.h>

#ifdef DBG_ON
#include <mprintf.h>
#include <objedit.h>
#include <config.h>
#endif

// Must be last header 
#include <dbmem.h>


//////////////////////////////
// Mesh attachment property 1, for the belt.

#define BELT_LINK_IMPL kPropertyImplDense

typedef cGenericProperty<IBeltLinkProperty,&IID_IBeltLinkProperty,sBeltLink*> cBeltLinkPropertyBase;

class cBeltLinkProperty : public cBeltLinkPropertyBase
{
  cClassDataOps<sBeltLink> mOps;

public:
  cBeltLinkProperty(const sPropertyDesc* desc, IPropertyStore* store)
    : cBeltLinkPropertyBase(desc,store)
      {
	SetOps(&mOps);
      }
  
  cBeltLinkProperty(const sPropertyDesc* desc, ePropertyImpl impl)
    : cBeltLinkPropertyBase(desc,CreateGenericPropertyStore(impl))
      {
	SetOps(&mOps);
      }

  STANDARD_DESCRIBE_TYPE(sBeltLink);

};

static sPropertyDesc BeltLinkPropDesc =
{
  PROP_BELT_LINK_NAME,
  0,
  NULL,
  0,
  0,
  { "Dark GameSys","BeltLinkLocation"}
};

static IBeltLinkProperty* gBeltLinkProp = NULL;

void SetupBeltLinkProperty(void);

void BeltLinkInit(void)
{
  SetupBeltLinkProperty();
  gBeltLinkProp = new cBeltLinkProperty(&BeltLinkPropDesc, BELT_LINK_IMPL);
}

void BeltLinkTerm(void)
{
  SafeRelease(gBeltLinkProp);
}

static sFieldDesc BeltLinkFields[] =
{
  {"Mesh Joint",kFieldTypeInt, FieldLocation(sBeltLink, meshjoint) },
  {"X Rotation",kFieldTypeInt, FieldLocation(sBeltLink, xrotation) },
  {"Y Rotation",kFieldTypeInt, FieldLocation(sBeltLink, yrotation) },
  {"Z Rotation",kFieldTypeInt, FieldLocation(sBeltLink, zrotation) },
  {"Offset",    kFieldTypeVector,FieldLocation(sBeltLink,vecoffset)},
};

static sStructDesc BeltLinkDesc = StructDescBuild(sBeltLink,kStructFlagNone, BeltLinkFields);

static void SetupBeltLinkProperty(void)
{
  AutoAppIPtr_(StructDescTools,pTools);
  pTools->Register(&BeltLinkDesc);
}

sMeshAttachInstance GetBeltLinkAttachment(ObjID o)
{
  sMeshAttachInstance beltlinkData={0,0,{0,0,0},0,{0,0,0}};
  sBeltLink* prop;
  if (gBeltLinkProp->Get(o,&prop))
    {
      mx_mk_angvec(&beltlinkData.m_Angles,
		   prop->xrotation,
		   prop->yrotation,
		   prop->zrotation);
      beltlinkData.m_Offset = prop->vecoffset;
      //must set object at other end.
      beltlinkData.m_iJoint=prop->meshjoint;
      return beltlinkData;
    }
  mx_mk_angvec(&beltlinkData.m_Angles,0,16384,9000);
  beltlinkData.m_Offset = cMxsVector(0.24,0.75,0.0);
  //must set object at other end.
  beltlinkData.m_iJoint=8;
  return beltlinkData;
  
}


//////////////////////////////
// Mesh attachment property 2, for the alternate location.

#define ALT_LINK_IMPL kPropertyImplDense

typedef cGenericProperty<IAltLinkProperty,&IID_IAltLinkProperty,sAltLink*> cAltLinkPropertyBase;

class cAltLinkProperty : public cAltLinkPropertyBase
{
  cClassDataOps<sAltLink> mOps;

public:
  cAltLinkProperty(const sPropertyDesc* desc, IPropertyStore* store)
    : cAltLinkPropertyBase(desc,store)
      {
	SetOps(&mOps);
      }
  
  cAltLinkProperty(const sPropertyDesc* desc, ePropertyImpl impl)
    : cAltLinkPropertyBase(desc,CreateGenericPropertyStore(impl))
      {
	SetOps(&mOps);
      }

  STANDARD_DESCRIBE_TYPE(sAltLink);

};

static sPropertyDesc AltLinkPropDesc =
{
  PROP_ALT_LINK_NAME,
  0,
  NULL,
  0,
  0,
  { "Dark GameSys","AltLinkLocation"}
};

static IAltLinkProperty* gAltLinkProp = NULL;

void SetupAltLinkProperty(void);

void AltLinkInit(void)
{
  SetupAltLinkProperty();
  gAltLinkProp = new cAltLinkProperty(&AltLinkPropDesc, ALT_LINK_IMPL);
}

void AltLinkTerm(void)
{
  SafeRelease(gAltLinkProp);
}

static sFieldDesc AltLinkFields[] =
{
  {"Mesh Joint",kFieldTypeInt, FieldLocation(sAltLink, meshjoint) },
  {"X Rotation",kFieldTypeInt, FieldLocation(sAltLink, xrotation) },
  {"Y Rotation",kFieldTypeInt, FieldLocation(sAltLink, yrotation) },
  {"Z Rotation",kFieldTypeInt, FieldLocation(sAltLink, zrotation) },
  {"Offset",    kFieldTypeVector,FieldLocation(sAltLink,vecoffset)},
};

static sStructDesc AltLinkDesc = StructDescBuild(sAltLink,kStructFlagNone, AltLinkFields);

static void SetupAltLinkProperty(void)
{
  AutoAppIPtr_(StructDescTools,pTools);
  pTools->Register(&AltLinkDesc);
}

sMeshAttachInstance GetAltLinkAttachment(ObjID o)
{
  sMeshAttachInstance AltLinkData={0,0,{0,0,0},0,{0,0,0}};
  sAltLink* prop;
  if (gAltLinkProp->Get(o,&prop))
    {
      mx_mk_angvec(&AltLinkData.m_Angles,
		   prop->xrotation,
		   prop->yrotation,
		   prop->zrotation);
      AltLinkData.m_Offset = prop->vecoffset;
      //must set object at other end.
      AltLinkData.m_iJoint=prop->meshjoint;
      return AltLinkData;
    }
  mx_mk_angvec(&AltLinkData.m_Angles,0,16384,9000);
  AltLinkData.m_Offset = cMxsVector(0.24,0.75,0.0);
  //must set object at other end.
  AltLinkData.m_iJoint=8;
  return AltLinkData;
  
}



