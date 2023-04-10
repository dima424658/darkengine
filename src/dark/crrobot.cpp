/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <creatur_.h>
#include <crrobot.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

static sCrPhysModOffset g_aRobotPhysModOffsets[] = \
{
   { 8, 8, 0.0, 1.95},
   { 9, 10, 0.7, 1.95},
};

// Joint IDs
#define LTOE      0
#define RTOE      1
#define LANKLE    2
#define RANKLE    3
#define LKNEE     4
#define RKNEE     5
#define LHIP      6
#define RHIP      7
#define BUTT      8
#define ABDOMEN   9
#define NECK      10
#define LSHLDR    11
#define RSHLDR    12
#define LELBOW    13
#define RELBOW    14
#define LWRIST    15
#define RWRIST    16
#define HEAD      17

// mapping from joint enum (crjoint.h) to our joint IDs
static int g_aRobotJointMap[] = 
{
   -1,
   17,
   10,
   9,
   8,
   11,
   12,
   13,
   14,
   15,
   16,
   -1,
   -1,
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
static int g_aRobotJointParents[] =
{
LANKLE,  // LTOE
RANKLE,  // RTOE     
LKNEE,   // LANKLE    
RKNEE,   // RANKLE    
LHIP,    // LKNEE   
RHIP,    // RKNEE 
BUTT,    // LHIP
BUTT,    // RHIP  
BUTT,    // BUTT  
BUTT,    // ABDOMEN
ABDOMEN, // NECK 
ABDOMEN,    // LSHLDR 
ABDOMEN,    // RSHLDR 
LSHLDR,  // LELBOW 
RSHLDR,  // RELBOW 
LELBOW,  // LWRIST 
RELBOW,  // RWRIST 
NECK,    // HEAD
};               
                 
static torso g_aDefTorsos[2]= {\
   {BUTT,-1,3,{LHIP,RHIP,ABDOMEN}},
   {ABDOMEN,0,3,{LSHLDR,RSHLDR,NECK}},
};

static limb g_aDefLimbs[5]= {\
   {0,1,3,{LHIP,LKNEE,LANKLE,LTOE}},
   {0,1,3,{RHIP,RKNEE,RANKLE,RTOE}},
   {1,0,2,{LSHLDR,LELBOW,LWRIST}},
   {1,0,2,{RSHLDR,RELBOW,RWRIST}},
   {1,0,1,{NECK, HEAD}},
};

static int g_FootJoints[2]={ LANKLE, RANKLE };

const sCreatureDesc sCrRobotDesc = \
{
     kDATYPE_Robot, // actor type
     2,         // num torsos
     5,         // num limbs
     18,        // num joints
     "robotcom", // default lengths filename
     2,                    // number of phys models
     1, // default phys radius
     g_aRobotPhysModOffsets, // array of phys model offsets
     g_aRobotJointParents,   // joint parent mapping
     g_aRobotJointMap,
     2,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     0, // number of weapons
     NULL, // weapon physics model offsets
     FALSE,    // is player limb
     FALSE,    // uses head tracking
     TRUE,     // special physics
};


