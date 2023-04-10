/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <creatur_.h>
#include <dpccrdhu.h>
#include <dpccret.h> // to get actor type definition

#include <dbmem.h> // must be last included header


#define kNumDeepHumanPhysModels 2

static sCrPhysModOffset g_aDeepHumanPhysModOffsets[kNumDeepHumanPhysModels] = \
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
#define LCLAV     10
#define RCLAV     11
#define LSHLDR    12
#define RSHLDR    13
#define LELBOW    14
#define RELBOW    15
#define LWRIST    16
#define RWRIST    17
#define LFINGER   18
#define RFINGER   19
#define ABDOMEN   20
#define CHEST     21
#define HEAD      22
#define LWEAP     23
#define RWEAP     24
#define CROWN     25

#define kNumDeepHumanJoints 26

// mapping from joint id to exposed joint enum
static int g_aDeepHumanJointMap[] = 
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
static int g_aDeepHumanJointParents[kNumDeepHumanJoints] =
{
   LANKLE,   // LTOE
   RANKLE,   // RTOE
   LKNEE,    // LANKLE
   RKNEE,    // RANKLE
   LHIP,     // LKNEE
   RHIP,     // RKNEE
   BUTT,     // LHIP
   BUTT,     // RHIP
   BUTT,     // BUTT
   CHEST,    // NECK
   CHEST,    // LCLAV
   CHEST,    // RCLAV
   LCLAV,    // LSHLDR
   RCLAV,    // RSHLDR
   LSHLDR,   // LELBOW
   RSHLDR,   // RELBOW
   LELBOW,   // LWRIST
   RELBOW,   // RWRIST
   LWRIST,   // LFINGER
   RWRIST,   // RFINGER
   BUTT,     // ABDOMEN
   ABDOMEN,  // CHEST
   NECK,     // HEAD
   LFINGER,  // LWEAP
   RFINGER,  // RWEAP
   HEAD,     // CROWN
};               
                 
#define kNumDeepHumanTorsos 3

static torso g_aDefTorsos[kNumDeepHumanTorsos] = 
{
   {BUTT,   -1,3,{LHIP,RHIP,ABDOMEN}},
   {ABDOMEN, 0,1,{CHEST}},
   {CHEST,   1,3,{NECK,LCLAV,RCLAV}},
};

#define kNumDeepHumanLimbs 5

static limb g_aDefLimbs[kNumDeepHumanLimbs] = 
{
   {0,1,4,{LHIP,LKNEE,LANKLE,LTOE}},
   {0,1,4,{RHIP,RKNEE,RANKLE,RTOE}},
   {2,0,6,{LCLAV,LSHLDR,LELBOW,LWRIST,LFINGER,LWEAP}},
   {2,0,6,{RCLAV,RSHLDR,RELBOW,RWRIST,RFINGER,RWEAP}},
   {2,0,3,{NECK,HEAD,CROWN}},
};

#define kNumDeepHumanFootJoints 2
static int g_FootJoints[kNumDeepHumanFootJoints] = { LANKLE, RANKLE };

const sCreatureDesc sCrDeepHumanDesc = \
{
     kDPCATYPE_DeepHuman,        // actor type
     kNumDeepHumanTorsos,        // num torsos
     kNumDeepHumanLimbs,         // num limbs
     kNumDeepHumanJoints,        // num joints
     "deepman",                  // default lengths filename
     kNumDeepHumanPhysModels,    // number of phys models
     1.5,                        // default phys radius
     g_aDeepHumanPhysModOffsets, // array of phys model offsets
     g_aDeepHumanJointParents,   // joint parent mapping
     g_aDeepHumanJointMap,
     kNumDeepHumanFootJoints,    // num feet
     g_FootJoints,               // foot joints
     g_aDefTorsos,               // default torso values (if lengths file not found)
     g_aDefLimbs,                // default limb values (if lengths file not found)
     kDPCCrWeap_NumWeapons,      // number of weapons
     NULL,                       // weapon physics model offsets
     FALSE,                      // is player limb
     FALSE,                      // uses head tracking
};

