/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkcrrum.cpp,v 1.4 1999/02/17 13:34:28 JON Exp $
//

#include <creatur_.h>
#include <shkcrrum.h>
#include <shkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

static sCrPhysModOffset g_ClawPhysOffsets[] = \
{
   { 14, 16, 0, 1},
   { 14, 16, 1, 1.25},
   { 15, 17, 0, 1},
   { 15, 17, 1, 1.25},
};

static sCrPhysModOffsetTable g_WeaponPhysTable = \
{
   sizeof(g_ClawPhysOffsets)/sizeof(g_ClawPhysOffsets[0]), g_ClawPhysOffsets
};


static sCrPhysModOffset g_aRumblerPhysModOffsets[] = \
{
   { 8, 8, 0.0, 1.5},
   { 18, 9, 0.5, 1.5},
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
#define TAIL     20 

// mapping from joint id to exposed joint enum
static int g_aRumblerJointMap[] = 
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
static int g_aRumblerJointParents[] =
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

const sCreatureDesc sCrRumblerDesc = \
{
     kShATYPE_Humanoid, // actor type
     2,         // num torsos
     5,         // num limbs
     20,        // num joints
     "mguard",  // default lengths filename
     2,                    // number of phys models
     1, // default phys radius
     g_aRumblerPhysModOffsets, // array of phys model offsets
     g_aRumblerJointParents,   // joint parent mapping
     g_aRumblerJointMap,
     2,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kShCrWeap_NumWeapons, // number of weapons - this seems bogus?
     &g_WeaponPhysTable, // weapon physics model offsets
     FALSE,    // is player limb
     TRUE,    // uses head tracking
     TRUE,     // special physics
};
