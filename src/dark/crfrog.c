/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crfrog.c,v 1.2 1998/10/23 01:49:10 CCAROLLO Exp $

#include <creatur_.h>
#include <crfrog.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header


static sCrPhysModOffset g_aFrogPhysModOffsets[] = \
{
   { 8, 8, 0.0, 0.3},
   { 18, 9, 0.8, 0.4},
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
static int g_aFrogJointMap[] = 
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
   20,
};               

// array of mappings from jointIDs -> parent jointIDs
static int g_aFrogJointParents[] =
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
   BUTT,     //    TAIL
};               
                 
static torso g_aDefTorsos[]= {\
   {BUTT,-1,3,{LHIP,RHIP,ABDOMEN}},
   {BUTT,-1,1,{TAIL}},
   {ABDOMEN,0,3,{LSHLDR,RSHLDR,NECK}},
};

static limb g_aDefLimbs[]= {\
   {0,1,3,{LHIP,LKNEE,LANKLE,LTOE}},
   {0,1,3,{RHIP,RKNEE,RANKLE,RTOE}},
   {1,0,3,{LSHLDR,LELBOW,LWRIST,LFINGER}},
   {1,0,3,{RSHLDR,RELBOW,RWRIST,RFINGER}},
   {1,0,1,{NECK,HEAD}},
};

static int g_FootJoints[2]={ LANKLE, RANKLE };

const sCreatureDesc sCrFrogDesc = \
{
     kDATYPE_Burrick, // actor type
     sizeof(g_aDefTorsos)/sizeof(g_aDefTorsos[0]),    // num torsos
     sizeof(g_aDefLimbs)/sizeof(g_aDefLimbs[0]),      // num limbs
     21,        // num joints
     "expfrog",  // default lengths filename
     sizeof(g_aFrogPhysModOffsets)/sizeof(g_aFrogPhysModOffsets[0]),  // number of phys models
     1.5, // default phys radius
     g_aFrogPhysModOffsets,
     g_aFrogJointParents,   // joint parent mapping
     g_aFrogJointMap,
     2,         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kDCrWeap_NumWeapons, // number of weapons
     NULL,
     FALSE,    // is player limb
     FALSE,    // uses head tracking
};
