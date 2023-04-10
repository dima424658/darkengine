/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crcutty.c,v 1.1 1998/10/24 19:48:36 CCAROLLO Exp $
// XXX this could really a DLL.  just contains humanoid-specific data

#include <creatur_.h>
#include <crcutty.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

#define HUMANOID_PHYS_RADIUS 1.0

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


static sCrPhysModOffset g_aHumPhysModOffsets[] = \
{
   { 8, 8, 0.0, 1.0},
   { 18, 9, 0.8, 1.2},
};

// Joints
#define LTOE      0
#define RTOE      1
#define LANKLE    2
#define RANKLE    3
#define LKNEE     4
#define RKNEE     5
#define LHIP      6
#define RHIP      7
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
#define LSHLDRIN 20
#define RSHLDRIN 21
#define LWEAP    22
#define RWEAP    23

// mapping from joint id to exposed joint enum
static int g_aHumJointMap[] = 
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
   6,
   7,
   4,
   5,
   2,
   3,
   0,
   1,
   -1,
};               

// array of mappings from jointIDs -> parent jointIDs
static int g_aHumJointParents[] =
{
   LANKLE,   //    LTOE    
   RANKLE,   //    RTOE    
   LKNEE,    //    LANKLE  
   RKNEE,    //    RANKLE  
   LHIP,     //    LKNEE   
   RHIP,     //    RKNEE
   BUTT,     //    LHIP   
   BUTT,     //    RHIP   
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
                 
static torso g_aDefTorsos[2]= {\
   {BUTT,-1,3,{LHIP,RHIP,ABDOMEN}},
   {ABDOMEN,0,3,{LSHLDR,RSHLDR,NECK}},
};

static limb g_aDefLimbs[5]= {\
   {0,1,3,{LHIP,LKNEE,LANKLE,LTOE}},
   {0,1,3,{RHIP,RKNEE,RANKLE,RTOE}},
   {1,0,3,{LSHLDR,LELBOW,LWRIST,LFINGER}},
   {1,0,3,{RSHLDR,RELBOW,RWRIST,RFINGER}},
   {1,0,1,{NECK,HEAD}},
};

static int g_FootJoints[2]={ LANKLE, RANKLE };

const sCreatureDesc sCrCuttyDesc = \
{
     kDATYPE_Humanoid, // actor type
     2,         // num torsos
     5,         // num limbs
     20,        // num joints
     "mguard",  // default lengths filename
     2,                    // number of phys models
     HUMANOID_PHYS_RADIUS, // default phys radius
     g_aHumPhysModOffsets, // array of phys model offsets
     g_aHumJointParents,   // joint parent mapping
     g_aHumJointMap,
     2,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kDCrWeap_NumWeapons, // number of weapons
     &g_WeaponPhysTable, // weapon physics model offsets
     TRUE,    // always ballistic
     TRUE,    // uses head tracking
};
