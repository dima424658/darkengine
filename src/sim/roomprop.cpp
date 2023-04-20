/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomprop.cpp,v 1.10 1999/12/01 20:10:46 BFarquha Exp $
//
// Room-specific property implementations
//

#include <lg.h>
#include <lgsound.h>

#include <objtype.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <filevar.h>

#include <sdesbase.h>
#include <sdesc.h>

#include <roomprop.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

IAcousticsProperty  *gAcousticsProperty   = NULL;
IAmbientProperty    *gAmbientProperty     = NULL;
IIntProperty        *gRoomGravityProperty = NULL;

///////////////////////////////////////////////////////////////////////////////
//
// Acoustics Property
//

typedef cGenericProperty<IAcousticsProperty,&IID_IAcousticsProperty, sAcousticsProperty*> cAcousticsPropertyBase;

class cAcousticsProperty : public cAcousticsPropertyBase
{
   cClassDataOps< sAcousticsProperty> mOps;

public:
   cAcousticsProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cAcousticsPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cAcousticsProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cAcousticsPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE( sAcousticsProperty);
};


// Constraints
static sPropertyConstraint AcousticsProp_const[] =
{
   { kPropertyNullConstraint, NULL }
};

// Description
static sPropertyDesc AcousticsProp_desc =
{
   PROP_ACOUSTICS_NAME,    // Name
   kPropertyConcrete,      // Flags
   AcousticsProp_const,    // Constraints
   2,                      // Version
   0,                      // Last ok version (0 means none)
   { "Room", "Acoustics" },
};

void SetupAcousticsProperty(void);

///////////////////
// we also want a mission and campaign variable for us

sFileVarDesc gMissionAcoustics =
{
   kMissionVar,
   "MissionEAX",
   "Mission Default EAX Value",
   FILEVAR_TYPE(sAcousticsProperty),
   { 1, 0 },
   { 1, 0 },
};

sFileVarDesc gGameSysAcoustics =
{
   kGameSysVar,
   "GameSysEAX",
   "GameSys Default EAX Value",
   FILEVAR_TYPE(sAcousticsProperty),
   { 1, 0 },
   { 1, 0 },
};

class cGameAcoustics : public cFileVar<sAcousticsProperty,&gGameSysAcoustics>
{
public:
   void Reset()
   {
      SecretEAXReverbConstant=kREVERB_Generic;
   }
};

class cMissAcoustics : public cFileVar<sAcousticsProperty,&gMissionAcoustics>
{
public:
   void Reset()
   {
      SecretEAXReverbConstant=kREVERB_Generic;
   }
};

static cGameAcoustics gGameAcoustics;
static cMissAcoustics gMissAcoustics;

int GetGameSysSecretEAXVar(void)
{
   return gGameAcoustics.SecretEAXReverbConstant;
}

int GetMissionSecretEAXVar(void)
{
   return gMissAcoustics.SecretEAXReverbConstant;
}

///////////
// actual setup

IAcousticsProperty *CreateAcousticsProperty(const sPropertyDesc *desc, ePropertyImpl impl)
{
   SetupAcousticsProperty();
   return new cAcousticsProperty(desc, impl);
}

void AcousticsPropInit(void)
{
   gAcousticsProperty = CreateAcousticsProperty(&AcousticsProp_desc, kPropertyImplLlist);
   // Install listeners here
}

void AcousticsPropTerm(void)
{
   SafeRelease(gAcousticsProperty);
   gAcousticsProperty = NULL;
}

// Structure Descriptor
#ifdef EDITOR
// must match hateful lgsound.h definitions
static const char *EAXCodeNames[]=
{
 "Generic",
 "PaddedCell",
 "Room",
 "Bathroom",
 "LivingRoom",
 "StoneRoom",
 "Auditorium",
 "ConcertHall",
 "Cave",
 "Arena",
 "Hangar",
 "CarpetedHallway",
 "Hallway",
 "StoneCorridor",
 "Alley",
 "Forest",
 "City",
 "Mountains",
 "Quarry",
 "Plain",
 "ParkingLot",
 "SewerPipe",
 "UnderWater",
 "Drugged",
 "Dizzy",
 "Psychotic",
};

const int g_nRoomTypes = 22;

const char *g_RoomTypes[g_nRoomTypes] =
{
   "Generic",
   "Small Dead",
   "Small Normal",
   "Type 3",
   "Type 4",
   "Large Normal",
   "Type 6",
   "Type 7",
   "Large Live",
   "Caverns",
   "Type 10",
   "Dead Hallway",
   "Normal Hallway",
   "Live Hallway",
   "Tunnels",
   "Outside",
   "Type 16",
   "Type 17",
   "Type 18",
   "Large Dead",
   "Small Live",
   "Sewers",
};

#endif

// also want to add a filevar in here somehow..
static sFieldDesc AcousticsFields[] =
{
#ifdef EDITOR
   { "EAX Reverb Code",  kFieldTypeEnum, FieldLocation(sAcousticsProperty, SecretEAXReverbConstant), FullFieldNames(EAXCodeNames) },
#else
   { "EAX Reverb Code",  kFieldTypeInt,  FieldLocation(sAcousticsProperty, SecretEAXReverbConstant) },
#endif
   { "Dampening Factor", kFieldTypeInt,  FieldLocation(sAcousticsProperty, Dampening) },
   { "Height Override",  kFieldTypeInt,  FieldLocation(sAcousticsProperty, Height)    },
};

static sStructDesc AcousticsDesc = StructDescBuild(sAcousticsProperty, kStructFlagNone, AcousticsFields);

static void SetupAcousticsProperty(void)
{
   StructDescRegister(&AcousticsDesc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Ambient Sound Property
//

typedef cGenericProperty<IAmbientProperty,&IID_IAmbientProperty, sAmbientProperty*> cAmbientPropertyBase;

class cAmbientProperty : public cAmbientPropertyBase
{
   cClassDataOps< sAmbientProperty> mOps;

public:
   cAmbientProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cAmbientPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cAmbientProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cAmbientPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE( sAmbientProperty);
};


// Constraints
static sPropertyConstraint AmbientProp_const[] =
{
   { kPropertyNullConstraint, NULL }
};

// Description
static sPropertyDesc AmbientProp_desc =
{
   PROP_AMBIENT_NAME,    // Name
   kPropertyConcrete,    // Flags
   AmbientProp_const,    // Constraints
   1,                    // Version
   0,                    // Last ok version (0 means none)
   { "Room", "Ambient" },
};

void SetupAmbientProperty(void);

IAmbientProperty *CreateAmbientProperty(const sPropertyDesc *desc, ePropertyImpl impl)
{
   SetupAmbientProperty();
   return new cAmbientProperty(desc, impl);
}

void AmbientPropInit(void)
{
   gAmbientProperty = CreateAmbientProperty(&AmbientProp_desc, kPropertyImplLlist);

   // Install listeners here
}

void AmbientPropTerm(void)
{
   SafeRelease(gAmbientProperty);
   gAmbientProperty = NULL;
}

// Structure Descriptor
static sFieldDesc AmbientFields[] =
{
   { "Schema Name",  kFieldTypeString, FieldLocation(sAmbientProperty, Name)    },
   { "Volume",       kFieldTypeInt,    FieldLocation(sAmbientProperty, Volume)  },
};

static sStructDesc AmbientDesc = StructDescBuild(sAmbientProperty, kStructFlagNone, AmbientFields);

static void SetupAmbientProperty(void)
{
   StructDescRegister(&AmbientDesc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Room Gravity Property
//

// Constraints
static sPropertyConstraint RoomGravityProp_const[] =
{
   { kPropertyNullConstraint, NULL }
};

// Description
static sPropertyDesc RoomGravityProp_desc =
{
   PROP_ROOMGRAVITY_NAME, // Name
   kPropertyConcrete,     // Flags
   RoomGravityProp_const, // Constraints
   1,                     // Version
   0,                     // Last ok version (0 means none)
   { "Room", "Gravity %" },
};

void RoomGravPropInit(void)
{
   gRoomGravityProperty = CreateIntProperty(&RoomGravityProp_desc, kPropertyImplDense);
}

void RoomGravPropTerm(void)
{
   SafeRelease(gRoomGravityProperty);
   gRoomGravityProperty = NULL;
}

float GetRoomGravity(ObjID objID)
{
   int val;

   if ((gRoomGravityProperty == NULL) || !gRoomGravityProperty->Get(objID, &val) || (val == 0))
      return 1.0;
   else
      return (float)(val) / 100;
}




//////////////////////////////


IFloatProperty *g_pLoudRoomProperty = 0;

static sPropertyDesc _g_LoudRoomPropertyDesc =
{
   "LoudRoom", 0,
   NULL, 0, 0, // constraints, versions
   { "Room", "Loud Room" },
};


void LoudRoomInit()
{
   g_pLoudRoomProperty = CreateFloatProperty(&_g_LoudRoomPropertyDesc, kPropertyImplSparse);
}

void LoudRoomTerm()
{
   SafeRelease(g_pLoudRoomProperty);
}


