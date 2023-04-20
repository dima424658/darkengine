/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/trapprop.cpp,v 1.1 1999/12/02 15:28:19 BFarquha Exp $
#include <propert_.h>
#include <propface.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <tfint.h>
#include <trapprop.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// TrapFlags PROP API
//


////////////////////////////////////////////////////////////
// TrapFlags PROPERTY CREATION
//


static IIntProperty *pPropTrapFlags = 0;

static sPropertyDesc trapFlagsDesc =
{
   PROP_TF_NAME, 0,
   NULL, 0, 0,
   { "Script", "Trap Control Flags" },
};


static const char *trapflag_names[] =
{ "Once", "Invert", "NoOn", "NoOff"};

static sFieldDesc trapFlagsFields[] =
{
   { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagUnsigned, 0, 4, 4, trapflag_names},
};

static sStructDesc trapFlagsStructDesc =
{
   PROP_TF_NAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(trapFlagsFields)/sizeof(trapFlagsFields[0]),
   trapFlagsFields,
};

static sPropertyTypeDesc trapFlagsTypeDesc = {PROP_TF_NAME, sizeof(int)};


void TrapFlagsPropInit(void)
{
   StructDescRegister(&trapFlagsStructDesc);
   pPropTrapFlags = CreateIntegralProperty(&trapFlagsDesc, &trapFlagsTypeDesc, kPropertyImplHash);
}

BOOL ObjGetTrapFlags(ObjID obj, int *tf)
{
   if (!pPropTrapFlags)
      return FALSE;
   return pPropTrapFlags->Get(obj, tf);
}

BOOL ObjSetHitPoints(ObjID obj, int tf)
{
   if (!pPropTrapFlags)
      return FALSE;
   return pPropTrapFlags->Set(obj, tf);
}

