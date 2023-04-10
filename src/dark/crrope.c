/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crrope.c,v 1.3 1998/09/20 15:56:56 CCAROLLO Exp $
// XXX this could really a DLL.  just contains humanoid-specific data

#include <creatur_.h>
#include <crhumand.h>
#include <drkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

// Joints
#define NODE_0    0
#define NODE_1    1
#define NODE_2    2
#define NODE_3    3
#define NODE_4    4
#define NODE_5    5
#define NODE_6    6
#define NODE_7    7
#define NODE_8    8

// mapping from joint id to exposed joint enum
static int g_aRopeJointMap[] = 
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
};               

// array of mappings from jointIDs -> parent jointIDs
static int g_aRopeJointParents[] =
{
   NODE_0,   //    NODE_0
   NODE_0,   //    NODE_1
   NODE_1,   //    NODE_2
   NODE_2,   //    NODE_3
   NODE_3,   //    NODE_4
   NODE_4,   //    NODE_5
   NODE_5,   //    NODE_6
   NODE_6,   //    NODE_7
   NODE_7,   //    NODE_8
};
                 
static torso g_aDefTorsos[1] = 
{
   {NODE_0, -1, 1, {NODE_0}},
};

static limb g_aDefLimbs[1] = 
{
   { 0, 1, 8, {NODE_0, NODE_1, NODE_2, NODE_3, NODE_4, NODE_5, NODE_6, NODE_7, NODE_8}},
};

const sCreatureDesc sCrRopeDesc = \
{
     kDATYPE_Rope, // actor type
     1,         // num torsos
     1,         // num limbs
     9,        // num joints
     "rope",  // default lengths filename
     0,                    // number of phys models
     0, // default phys radius
     NULL, // array of phys model offsets
     g_aRopeJointParents,   // joint parent mapping
     g_aRopeJointMap,
     0,         // num feet
     NULL, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     0, // number of weapons
     NULL, // weapon physics model offsets
     FALSE,    // is player limb
};


