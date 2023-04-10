/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/crbowarm.c,v 1.7 1998/10/23 01:49:02 CCAROLLO Exp $
// XXX this could really a DLL.  just contains humanoid-specific data

#include <creatur_.h>
#include <crbowarm.h>
#include <drkcret.h>

#include <dbmem.h> // must be last included header

// Joints
#define BUTT      0
#define SHLDR     1
#define ELBOW     2
#define WRIST     3
#define TOPMID    4
#define TOP       5
#define BOTMID    6
#define BOTTOM    7

// mapping from joint id to exposed joint enum
static int g_aArmJointMap[] = 
{
   -1,
   -1,
   -1,
   -1,
   0,
   -1,
   1,
   -1,
   2,
   -1,
   3,
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
static int g_aArmJointParents[] =
{
   BUTT,    //   BUTT   
   BUTT,    //   SHLDR  
   SHLDR,   //   ELBOW  
   ELBOW,   //   WRIST  
   WRIST,   //   TOPMID
   TOPMID,   //   TOP
   WRIST,    //   BOTMID
   BOTMID,   //   BOTTOM
};                                

static torso g_aDefTorsos[4]= {\
   {BUTT,-1,1,{SHLDR}},
   {SHLDR,0,1,{ELBOW}},
   {ELBOW,1,1,{WRIST}},
   {WRIST,2,2,{TOPMID,BOTMID}},
};

static limb g_aDefLimbs[2]= {\
   {3,0,1,{TOPMID,TOP}},
   {3,0,1,{BOTMID,BOTTOM}},
};

const sCreatureDesc sCrBowArmDesc = \
{
     kDATYPE_PlayerBowLimb, // actor type
     4,         // num torsos
     2,         // num limbs
     8,        // num joints
     "bowaim8",  // default lengths filename
     0,                    // number of phys models
     0,
     NULL,
     g_aArmJointParents,   // joint parent mapping
     g_aArmJointMap,
     0,         // num feet
     NULL, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     0,              // number of weapons
     NULL,     // weapon phys models
     FALSE,    // is always ballistic
     FALSE,    // uses head tracking
};













