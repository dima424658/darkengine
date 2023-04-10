/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crsweel.c,v 1.3 1998/09/20 15:57:00 CCAROLLO Exp $

#include <creatur_.h>
#include <crsweel.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header


// Joints
#define BASE 0
#define BACK 1
#define SHOULDER 2
#define NECK 3
#define HEAD 4
#define TAIL 5
#define TIP  6

// mapping from joint id to exposed joint enum
static int g_aSweelJointMap[] = 
{
   -1,
   4,
   3,
   1,
   0,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
};               

static sCrPhysModOffset g_JawsPhysOffsets[] = \
{
   { HEAD, HEAD, 0.0, 0.5},
};

static sCrPhysModOffsetTable g_WeaponPhysTable = \
{
   sizeof(g_JawsPhysOffsets)/sizeof(g_JawsPhysOffsets[0]), g_JawsPhysOffsets
};

#define SWEEL_PHYS_RADIUS 0.3

static sCrPhysModOffset g_aPhysModOffsets[] = \
{
   { BASE, BASE, 0.0, SWEEL_PHYS_RADIUS},
   { SHOULDER, SHOULDER, 0.0, SWEEL_PHYS_RADIUS},
};


// array of mappings from jointIDs -> parent jointIDs
static int g_aSweelJointParents[] =
{
   BASE,      // BASE 
   BASE,      // BACK 
   BACK,      // SHOULDER
   SHOULDER,  // NECK 
   NECK,      // HEAD 
   BASE,      // TAIL 
   TAIL,      // TIP
};
                 
static torso g_aDefTorsos[]= {\
   {BASE,-1,2,{BACK,TAIL}},
};

static limb g_aDefLimbs[]= {\
   {0,1,3,{BACK,SHOULDER,NECK,HEAD}},
   {0,1,1,{TAIL,TIP}},
};

static int g_FootJoints[1]={ BASE };

const sCreatureDesc sCrSweelDesc = \
{
     kDATYPE_Sweel, // actor type
     sizeof(g_aDefTorsos)/sizeof(g_aDefTorsos[0]),    // num torsos
     sizeof(g_aDefLimbs)/sizeof(g_aDefLimbs[0]),      // num limbs
     7,        // num joints
     "sweel",  // default lengths filename
     sizeof(g_aPhysModOffsets)/sizeof(g_aPhysModOffsets[0]),  // number of phys models
     SWEEL_PHYS_RADIUS, // default phys radius
     g_aPhysModOffsets, // array of phys model offsets
     g_aSweelJointParents,   // joint parent mapping
     g_aSweelJointMap,
     1,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kDCrWeap_NumWeapons, // number of weapons
     &g_WeaponPhysTable, // weapon physics model offsets
     TRUE,    // is ballistic by default
};
