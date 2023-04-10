/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <creatur_.h>
#include <dpccrdog.h>
#include <dpccret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

// Joints
#define LTOE           0  // 0
#define RTOE           1  // 1
#define LFOOT          2  // 2
#define RFOOT          3  // 3
#define LANKLE         4  // 4
#define RANKLE         5  // 5

#define LKNEE          6  // 6
#define RKNEE          7  // 7
#define LHIP           8  // 8
#define RHIP           9  // 9
#define BUTT           10 // 10
#define ABDOMEN        11 // 11

#define CHEST          12 // 30
#define NECK           13 // 12
#define HEAD           14 // 13

#define JAW            15 // 15
#define LCLAVICLE      16 // 16
#define RCLAVICLE      17 // 17
#define LSHLDR         18 // 18
#define RSHLDR         19 // 19
#define LELBOW         20 // 20
#define RELBOW         21 // 21
#define LWRIST         22 // 22
#define RWRIST         23 // 23
#define LFINGER        24 // 24
#define RFINGER        25 // 25

#define LEFTEAR        26 // --
#define RIGHTEAR       27 // --

#define TAIL           28 // 26
#define MIDTAIL        29 // 27
#define ENDTAIL        30 // 28
#define TIPTAIL        31 // 29

#define LEFTFRONTCLAW  32 // --
#define RIGHTFRONTCLAW 33 // --
#define LEFTTOECLAW    34 // --
#define RIGHTTOECLAW   35 // --

#define NOSE           36 // 14
#define LEFTEARTIP     37 // --
#define RIGHTEARTIP    38 // --
#define TOOTH          39 // -- Think of "jaw tip."

#define kNumDogJoints  40

#define kNumDogPhysModels 2
static sCrPhysModOffset g_aDogPhysModOffsets[kNumDogPhysModels] =
{
  // J1 -- J2 -- Fraction of J1 & J2 -- Radius
   { BUTT,  BUTT,     0,                    2.5},
   { NECK,  HEAD,     0.5,                  2.5},
};

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
static int g_aDogJointParents[kNumDogJoints] =
{
   LFOOT,     // LTOE
   RFOOT,     // RTOE
   LANKLE,    // LFOOT
   RANKLE,    // RFOOT
   LKNEE,     // LANKLE
   RKNEE,     // RANKLE
   LHIP,      // LKNEE
   RHIP,      // RKNEE
   BUTT,      // LHIP
   BUTT,      // RHIP
   BUTT,      // BUTT
   BUTT,      // ABDOMEN
   ABDOMEN,   // CHEST
   CHEST,     // NECK
   NECK,      // HEAD
   HEAD,      // JAW
   NECK,      // LCLAVICLE
   NECK,      // RCLAVICLE
   LCLAVICLE, // LSHLDR
   RCLAVICLE, // RSHLDR
   LSHLDR,    // LELBOW
   RSHLDR,    // RELBOW
   LELBOW,    // LWRIST
   RELBOW,    // RWRIST
   LWRIST,    // LFINGER
   RWRIST,    // RFINGER
   HEAD,      // LEFTEAR
   HEAD,      // RIGHTEAR
   BUTT,      // TAIL
   TAIL,      // MIDTAIL
   MIDTAIL,   // ENDTAIL
   ENDTAIL,   // TIPTAIL
   LFINGER,   // LEFTFRONTCLAW
   RFINGER,   // RIGHTFRONTCLAW
   LTOE,      // LEFTTOECLAW
   RTOE,      // RIGHTTOECLAW
   HEAD,      // NOSE               // @TODO:  Bodisafa 1/10/2000
                                    // Is this right in the MJO file?
   LEFTEAR,   // LEFTEARTIP
   RIGHTEAR,  // RIGHTEARTIP
   JAW,       // TOOTH
};               


#define TORSO_NONE   -1
#define TORSO_BUTT    0
#define TORSO_ABDOMEN 1
#define TORSO_CHEST   2
#define TORSO_NECK    3
#define TORSO_HEAD    4

#define kNumDogTorsos 5
static torso g_aDefTorsos[kNumDogTorsos] = 
{
   // Joint Parent,        #points, {points}
   {BUTT,    TORSO_NONE,   4,       {LHIP,RHIP,ABDOMEN,TAIL}},       // TORSO_BUTT   
   {ABDOMEN, TORSO_BUTT,   1,       {CHEST}},                        // TORSO_ABDOMEN
   {CHEST,   TORSO_ABDOMEN,3,       {NECK, LCLAVICLE, RCLAVICLE}},   // TORSO_CHEST  
   {NECK,    TORSO_CHEST,  1,       {HEAD}},                         // TORSO_NECK   
   {HEAD,    TORSO_NECK,   4,       {JAW, LEFTEAR, RIGHTEAR, NOSE}}, // TORSO_HEAD   
};

#define kNumDogLimbs 9
#define DIR_ARMS 0
#define DIR_LEGS 1

static limb g_aDefLimbs[kNumDogLimbs] = 
{
   // torso_id,  bend dir,       #points, {points}
   {TORSO_BUTT,  DIR_LEGS,       6,       {LHIP,LKNEE,LANKLE,LFOOT,LTOE,LEFTTOECLAW}},
   {TORSO_BUTT,  DIR_LEGS,       6,       {RHIP,RKNEE,RANKLE,RFOOT,RTOE, RIGHTTOECLAW}},
   {TORSO_BUTT,  DIR_LEGS,       4,       {TAIL,MIDTAIL,ENDTAIL,TIPTAIL}},
   {TORSO_CHEST, DIR_ARMS,       6,       {LCLAVICLE,LSHLDR,LELBOW,LWRIST,LFINGER, LEFTFRONTCLAW}},
   {TORSO_CHEST, DIR_ARMS,       6,       {RCLAVICLE,RSHLDR,RELBOW,RWRIST,RFINGER, RIGHTFRONTCLAW}},
   {TORSO_HEAD,  DIR_ARMS,       1,       {NOSE}},
   {TORSO_HEAD,  DIR_ARMS,       2,       {JAW,TOOTH}},
   {TORSO_HEAD,  DIR_ARMS,       2,       {LEFTEAR, LEFTEARTIP}},
   {TORSO_HEAD,  DIR_ARMS,       2,       {RIGHTEAR, RIGHTEARTIP}},
};

#define kNumDogFeet 4
static int g_FootJoints[kNumDogFeet] = { LFOOT, RFOOT, LFINGER, RFINGER };

const sCreatureDesc sCrDogDesc =
{
     kDPCATYPE_Dog,           // actor type
     kNumDogTorsos,           // num torsos
     kNumDogLimbs,          // num limbs
     kNumDogJoints,           // num joints
//     "jdog3",                 // default lengths filename
     "shepherd",                 // default lengths filename
     kNumDogPhysModels,       // number of phys models
     1.5,                     // default phys radius
     g_aDogPhysModOffsets,    // array of phys model offsets
     g_aDogJointParents,      // joint parent mapping
     g_aDogJointMap,          
     kNumDogFeet,             // num feet
     g_FootJoints,            // foot joints
     g_aDefTorsos,            // default torso values (if lengths file not found)
     g_aDefLimbs,             // default limb values (if lengths file not found)
     kDPCCrWeap_NumWeapons,   // number of weapons
     NULL,                    // weapon physics model offsets
     FALSE,                   // is player limb
     FALSE,                   // uses head tracking
};