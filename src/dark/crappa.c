/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crappa.c,v 1.7 1998/10/23 01:48:35 CCAROLLO Exp $

#include <creatur_.h>
#include <crappa.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

#define SWORD_PHYS_RADIUS 0.4

static sCrPhysModOffset g_SwordPhysOffsets[] = \
{
   { 15, 17, 0.25, SWORD_PHYS_RADIUS},
   { 15, 17, 0.5,  SWORD_PHYS_RADIUS},
   { 15, 17, 0.75, SWORD_PHYS_RADIUS},
   { 17, 17, 0,    SWORD_PHYS_RADIUS},
//   { 15, 17, 1.5, SWORD_PHYS_RADIUS},
};

static sCrPhysModOffsetTable g_WeaponPhysTable = \
{
   sizeof(g_SwordPhysOffsets)/sizeof(g_SwordPhysOffsets[0]), g_SwordPhysOffsets
};


static sCrPhysModOffset g_aPhysModOffsets[] = \
{
   { 8, 8, 0.0, 1.0},
   { 18, 9, 0.8, 1.2},
      //   { 0, 8, 0.1, 0.0}, // toe phys model
};

// Joints
#define TOE      0
#define BUTT      8
#define NECK      9
#define LSHLDR   10
#define RSHLDR   11
#define LELBOW   12
#define RELBOW   13
#define LWRIST   14
#define RWRIST   15
#define LFINGER  16
#define RFINGER  17
#define ABDOMEN  18
#define HEAD     19

// mapping from joint id to exposed joint enum
static int g_aJointMap[] = 
{
   -1,
   19,
   9,
   18,
   8,
   10,
   11,
   12,
   13,
   14,
   15,
   16,
   17,
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
static int g_aJointParents[] =
{
   BUTT,     //    TOE
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    DUMMY
   BUTT,     //    BUTT    
   ABDOMEN,  //    NECK    
   ABDOMEN,  //    LSHLDR  
   ABDOMEN,  //    RSHLDR  
   LSHLDR,   //    LELBOW  
   RSHLDR,   //    RELBOW  
   LELBOW,   //    LWRIST  
   RELBOW,   //    RWRIST  
   LWRIST,   //    LFINGER 
   RWRIST,   //    RFINGER 
   BUTT,     //    ABDOMEN 
   NECK,     //    HEAD
};               

static torso g_aDefTorsos[]= {\
   {BUTT,-1,2,{TOE,ABDOMEN}},
   {ABDOMEN,0,3,{LSHLDR,RSHLDR,NECK}},
};

static limb g_aDefLimbs[]= {\
   {1,0,3,{LSHLDR,LELBOW,LWRIST,LFINGER}},
   {1,0,3,{RSHLDR,RELBOW,RWRIST,RFINGER}},
   {1,0,1,{NECK,HEAD}},
};

static int g_FootJoints[1]={ TOE };

const sCreatureDesc sCrApparitionDesc = \
{
     kDATYPE_Apparition, // actor type
     sizeof(g_aDefTorsos)/sizeof(g_aDefTorsos[0]),    // num torsos
     sizeof(g_aDefLimbs)/sizeof(g_aDefLimbs[0]),      // num limbs
     20,        // num joints
     "expappa",  // default lengths filename
     sizeof(g_aPhysModOffsets)/sizeof(g_aPhysModOffsets[0]),  // number of phys models
     1.5, // default phys radius
     g_aPhysModOffsets, // array of phys model offsets
     g_aJointParents,   // joint parent mapping
     g_aJointMap,
     1,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kDCrWeap_NumWeapons, // number of weapons
     &g_WeaponPhysTable, // weapon physics model offsets
     TRUE,    // is ballistic by default
     TRUE,    // uses head tracking
};
