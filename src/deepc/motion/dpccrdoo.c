/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <creatur_.h>
#include <dpccrdoo.h>
#include <dpccret.h> // to get actor type definition

#include <dbmem.h> // must be last included header


static sCrPhysModOffset g_aDogPhysModOffsets[] = \
{
   { 8, 8, 0.0, 1.0},
   { 18, 9, 0.8, 1.2},
};

// Joints
#define LTOE      0
#define RTOE      1
#define LFOOT     2
#define RFOOT     3
#define LANKLE    4
#define RANKLE    5
#define LKNEE     6
#define RKNEE     7
#define LHIP      8
#define RHIP      9
#define BUTT     10
#define ABDOMEN  11
#define NECK     12
#define HEAD     13
#define NOSE     14
#define JAW      15
#define LSHLDR   16
#define RSHLDR   17
#define LELBOW   18
#define RELBOW   19
#define LWRIST   20
#define RWRIST   21
#define LFINGER  22
#define RFINGER  23
#define TAIL     24

// mapping from joint id to exposed joint enum
static int g_aDogJointMap[] = 
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
static int g_aDogJointParents[] =
{
   LFOOT,    //    LTOE
   RFOOT,    //    RTOE
   LANKLE,   //    LFOOT
   RANKLE,   //    RFOOT
   LKNEE,    //    LANKLE
   RKNEE,    //    RANKLE
   LHIP,     //    LKNEE
   RHIP,     //    RKNEE
   BUTT,     //    LHIP
   BUTT,     //    RHIP
   BUTT,     //    BUTT
   BUTT,     //    ABDOMEN
   ABDOMEN,  //    NECK
   NECK,     //    HEAD
   HEAD,     //    NOSE
   HEAD,     //    JAW
   NECK,     //    LSHLDR
   NECK,     //    RSHLDR
   LSHLDR,   //    LELBOW
   RSHLDR,   //    RELBOW
   LELBOW,   //    LWRIST
   RELBOW,   //    RWRIST
   LWRIST,   //    LFINGER
   RWRIST,   //    RFINGER
   BUTT,     //    TAIL
};               
                 
static torso g_aDefTorsos[]= {\
   {BUTT,-1,4,{LHIP,RHIP,ABDOMEN,TAIL}},
   {ABDOMEN,0,1,{NECK}},
   {NECK,1,3,{HEAD,LSHLDR,RSHLDR}},
   {HEAD,2,2,{NOSE,JAW}},
};

static limb g_aDefLimbs[]= {\
   {0,1,4,{LHIP,LKNEE,LANKLE,LFOOT,LTOE}},
   {0,1,4,{RHIP,RKNEE,RANKLE,RFOOT,RTOE}},
   {0,1,0,{TAIL}},
   {2,0,3,{RSHLDR,RELBOW,RWRIST,RFINGER}},
   {2,0,3,{NECK,HEAD}},
   {3,0,0,{NOSE}},
   {3,0,0,{JAW}},
};

static int g_FootJoints[]={ LFOOT, RFOOT, LFINGER, RFINGER };

const sCreatureDesc sCrOldDogDesc = \
{
     kDPCATYPE_Dog, // actor type
     4,         // num torsos
     7,         // num limbs
     25,        // num joints
     "dog1",  // default lengths filename
     2,                    // number of phys models
     1.5,       // default phys radius
     g_aDogPhysModOffsets, // array of phys model offsets
     g_aDogJointParents,   // joint parent mapping
     g_aDogJointMap,
     4,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kDPCCrWeap_NumWeapons, // number of weapons
     NULL,   // weapon physics model offsets
     FALSE,    // is player limb
     FALSE,    // uses head tracking
};