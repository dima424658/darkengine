/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcrarm.cpp,v 1.1 1999/04/28 13:45:04 JON Exp $

#include <shkcrarm.h>

#include <crplyarm.h>
#include <shkcret.h> // for actor type
#include <mp.h>

#include <dbmem.h> // must be last included header

#define WRENCH_PHYS_RADIUS 0.15

static sCrPhysModOffset g_WrenchPhysOffsets[4] =  \
{
   { 3, 4, 0.22, WRENCH_PHYS_RADIUS},
   { 3, 4, 0.47,  WRENCH_PHYS_RADIUS},
   { 3, 4, 0.74, WRENCH_PHYS_RADIUS},
   { 3, 4, 1.0,    WRENCH_PHYS_RADIUS},
};

static sCrPhysModOffsetTable g_WeaponPhysTable = \
{
   4, g_WrenchPhysOffsets
};


// Joints
#define BUTT      0
#define SHLDR     1
#define ELBOW     2
#define WRIST     3
#define FINGER    4

// mapping from joint id to exposed joint enum
static int g_aArmJointMap[] = 
{
   -1,
   -1,
   -1,
   -1,
   0,
   -1,
   1,
   -1,
   2,
   -1,
   3,
   -1,
   4,
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

// array of mappings from jointIDs -> parent jointIDs
static int g_aArmJointParents[] =
{
   BUTT,    //   BUTT   
   BUTT,    //   SHLDR  
   SHLDR,   //   ELBOW  
   ELBOW,   //   WRIST  
   WRIST,   //   FINGER 
};                                

static torso g_DefTorso={BUTT,-1,1,{SHLDR}};
static limb  g_DefLimb={0,0,3,{SHLDR,ELBOW,WRIST,FINGER}};

const sCreatureDesc sCrShockWrenchDesc = \
{
     kShCRTYPE_PlayerLimb, // actor type
     1,         // num torsos
     1,         // num limbs
     5,        // num joints
     "wrench_h",  // default lengths filename
     0,                    // number of phys models
     0,
     NULL,
     g_aArmJointParents,   // joint parent mapping
     g_aArmJointMap,
     0,         // num feet
     NULL, // foot joints
     &g_DefTorso,      // default torso values (if lengths file not found)
     &g_DefLimb,      // default limb values (if lengths file not found)
     kShCrWeap_NumWeapons, // number of weapons - this seems bogus?
     &g_WeaponPhysTable, // weapon physics model offsets
     FALSE,    // is always ballistic
     FALSE,    // uses head tracking
};

