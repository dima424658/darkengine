/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <creatur_.h>
#include <dpccrhum.h>
#include <dpccret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

#define HUMANOID_PHYS_RADIUS 1.0

static sCrPhysModOffset g_SwordPhysOffsets[] = \
{
   { 15, 17, 0.25, 0.4},
   { 15, 17, 0.5,  0.4},
   { 15, 17, 0.75, 0.4},
   { 17, 17, 0,    0.4},
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

// The physics model for avatar. This deliberately tries to ape the player
// physics model from physapi. These numbers are wildly hacked, and are
// in large part based upon the Deep Cover avatar; this system should be made
// smarter in the future.
// Numbers played with: radius of 1.6 seems to best match the actual
// player radius of 1.2 in looks, but still gets stuck in things.
// Making the butt number based on neck and abdomen (2.15) makes his butt
// stick out too far.
static sCrPhysModOffset g_aAvaPhysModOffsets[] = \
{
   { 18, 8, 1.0, 1.0}, // The butt
   { 18, 9, 0.3, 1.2}, // The head
   { 0, 1, 0.5, 0.0}, // The feet (the toes)
   { 4, 5, 0.5, 0.0}, // The knees
   { 6, 7, 0.5, 0.0}, // The hips (placeholder for the shins)
      // { 18, 8, 6, 0.0}, // The stick up his butt (to keep him elevated)
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

const sCreatureDesc sCrDPCHumanDesc = \
{
     kDPCATYPE_Humanoid,         // actor type
     2,                          // num torsos
     5,                          // num limbs
     20,                         // num joints
     "mguard",                   // default lengths filename
     2,                          // number of phys models
     HUMANOID_PHYS_RADIUS,       // default phys radius
     g_aHumPhysModOffsets,       // array of phys model offsets
     g_aHumJointParents,         // joint parent mapping
     g_aHumJointMap,             
     2,                          // num feet
     g_FootJoints,               // foot joints
     g_aDefTorsos,               // default torso values (if lengths file not found)
     g_aDefLimbs,                // default limb values (if lengths file not found)
     kDPCCrWeap_NumWeapons,      // number of weapons - this seems bogus?
     &g_WeaponPhysTable,         // weapon physics model offsets
     FALSE,                      // is player limb
     TRUE,                       // uses head tracking
     TRUE,                       // special physics
};

// The avatar is much like an ordinary humanoid, but has a more sophisticated
// physics model, to roughly match the player's model.
const sCreatureDesc sCrDPCAvatarDesc = \
{
     kDPCATYPE_Humanoid,          // actor type
     2,                          // num torsos
     5,                          // num limbs
     20,                         // num joints
     "mguard",                   // default lengths filename
     5,                          // number of phys models
     HUMANOID_PHYS_RADIUS,       // default phys radius
     g_aAvaPhysModOffsets,       // array of phys model offsets
     g_aHumJointParents,         // joint parent mapping
     g_aHumJointMap,             
     2,                          // num feet
     g_FootJoints,               // foot joints
     g_aDefTorsos,               // default torso values (if lengths file not found)
     g_aDefLimbs,                // default limb values (if lengths file not found)
     kDPCCrWeap_NumWeapons,      // number of weapons - this seems bogus?
     &g_WeaponPhysTable,         // weapon physics model offsets
     FALSE,                      // is player limb
     TRUE,                       // uses head tracking
     TRUE,                       // special physics
};
