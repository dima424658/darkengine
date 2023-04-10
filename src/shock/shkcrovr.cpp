/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkcrovr.cpp,v 1.5 1999/02/17 13:34:22 JON Exp $
//

#include <creatur_.h>
#include <shkcrovr.h>
#include <shkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

// Joints
#define BASE            0
#define F1SNOUT         1
#define F2SNOUT         2
#define F3SNOUT         3
#define F4SNOUT         4
#define B1STEM          5
#define B2STAGE         6
#define B3STAGE         7
#define B4STAGE         8
#define B5STAGE         9
#define B6STAGE         10
#define R1SHLDR         11
#define R1ELBOW         12
#define R1WRIST         13
#define R1FINGER        14
#define R1APP           15
#define R1TIP           16
#define R2SHLDR         17
#define R2ELBOW         18
#define R2WRIST         19
#define R2FINGER        20
#define R2APP           21
#define R2TIP           22
#define L1SHLDR         23
#define L1ELBOW         24
#define L1WRIST         25
#define L1FINGER        26
#define L1APP           27
#define L1TIP           28
#define L2SHLDR         29
#define L2ELBOW         30
#define L2WRIST         31
#define L2FINGER        32
#define L2APP           33
#define L2TIP           34
#define SAC             35
#define LSAC            36

// mapping from joint id to exposed joint enum
static int g_aOverlordJointMap[] = 
{
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
   -1,
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

// NOTE: since this creature is ballistic, it should only have
// one physics model, and that model should be centered at the
// BASE joint.  Otherwise, there will be physics rotation problems
// (just ask Chris).      - Kate 1/99
static sCrPhysModOffset g_aPhysModOffsets[] = \
{
   { BASE, BASE, 0.0, 4.0},
};


// array of mappings from jointIDs -> parent jointIDs
static int g_aOverlordJointParents[] =
{
   BASE,       // BASE
   BASE,       // F1SNOUT
   F1SNOUT,    // F2SNOUT
   F2SNOUT,    // F3SNOUT
   F3SNOUT,    // F4SNOUT
   BASE,       // B1STEM
   B1STEM,     // B2STAGE
   B2STAGE,    // B3STAGE
   B3STAGE,    // B4STAGE
   B4STAGE,    // B5STAGE
   B5STAGE,    // B6STAGE
   BASE,       // R1SHLDR
   R1SHLDR,    // R1ELBOW
   R1ELBOW,    // R1WRIST
   R1WRIST,    // R1FINGER
   R1FINGER,   // R1APP
   R1APP,      // R1TIP
   BASE,       // R2SHLDR
   R2SHLDR,    // R2ELBOW
   R2ELBOW,    // R2WRIST
   R2WRIST,    // R2FINGER
   R2FINGER,   // R2APP
   R2APP,      // R2TIP
   BASE,       // L1SHLDR
   L1SHLDR,    // L1ELBOW
   L1ELBOW,    // L1WRIST
   L1WRIST,    // L1FINGER
   L1FINGER,   // L1APP
   L1APP,      // L1TIP
   BASE,       // L2SHLDR
   L2SHLDR,    // L2ELBOW
   L2ELBOW,    // L2WRIST
   L2WRIST,    // L2FINGER
   L2FINGER,   // L2APP
   L2APP,      // L2TIP
};
        
static torso g_aOverlordDefTorsos[]= {\
   {BASE,-1,3,{F1SNOUT, R1SHLDR, L1SHLDR,}},
   {BASE,-1,3,{B1STEM, R2SHLDR, L2SHLDR,}},
   {BASE,-1,2,{SAC, LSAC,}},
};

static limb g_aOverlordDefLimbs[]= {\
   {0,1,3,{F1SNOUT,F2SNOUT,F3SNOUT,F4SNOUT,}},
   {1,1,5,{B1STEM,B2STAGE,B3STAGE,B4STAGE,B5STAGE,B6STAGE,}},
   {0,1,5,{R1SHLDR,R1ELBOW,R1WRIST,R1FINGER,R1APP,R1TIP,}},
   {1,1,5,{R2SHLDR,R2ELBOW,R2WRIST,R2FINGER,R2APP,R2TIP,}},
   {0,1,5,{L1SHLDR,L1ELBOW,L1WRIST,L1FINGER,L1APP,L1TIP,}},
   {1,1,5,{L2SHLDR,L2ELBOW,L2WRIST,L2FINGER,L2APP,L2TIP,}},
};

const sCreatureDesc sCrOverlordDesc = \
{
     kShATYPE_Overlord, // actor type
     3,        // num torsos
     6,       // num limbs
     37,        // num joints
     "Overlord",  // default lengths filename
     sizeof(g_aPhysModOffsets)/sizeof(g_aPhysModOffsets[0]),  // number of phys models
     1.0, // default phys radius
     g_aPhysModOffsets, // array of phys model offsets
     g_aOverlordJointParents,   // joint parent mapping
     g_aOverlordJointMap,
     0,         // num feet
     NULL, // foot joints
     g_aOverlordDefTorsos,      // default torso values (if lengths file not found)
     g_aOverlordDefLimbs,      // default limb values (if lengths file not found)
     0, // number of weapons
     NULL, // weapon physics model offsets
     TRUE,    // always ballistic
     FALSE,    // uses head tracking
     TRUE,     // special physics
};
