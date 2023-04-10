/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcrara.cpp,v 1.5 1999/08/24 14:21:48 ccarollo Exp $

#include <creatur_.h>
#include <shkcrara.h>
#include <shkcret.h> // to get actor type definition

#include <dbmem.h> // must be last included header

// Joints
#define base            0
#define lmand           1
#define lmelbow         2
#define rmand           3
#define rmelbow         4
#define r1shldr         5
#define r1elbow         6
#define r1wrist         7
#define r2shldr         8
#define r2elbow         9
#define r2wrist         10
#define r3shldr         11
#define r3elbow         12
#define r3wrist         13
#define r4shldr         14
#define r4elbow         15
#define r4wrist         16
#define l1shldr         17
#define l1elbow         18
#define l1wrist         19
#define l2shldr         20
#define l2elbow         21
#define l2wrist         22
#define l3shldr         23
#define l3elbow         24
#define l3wrist         25
#define l4shldr         26
#define l4elbow         27
#define l4wrist         28
#define r1finger        29
#define r2finger        30
#define r3finger        31
#define r4finger        32
#define l1finger        33
#define l2finger        34
#define l3finger        35
#define l4finger        36
#define ltip            37
#define rtip            38
#define sac             39

// mapping from joint id to exposed joint enum
static int g_aSpidJointMap[] = 
{
   -1,
    0, // head
   -1,
   -1,
   0,
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

#define ARACHNID_LEG_PHYS_RADIUS 0.3

static sCrPhysModOffset g_SwordPhysOffsets[] = \
{
   {l1wrist, l1finger, 0.0, ARACHNID_LEG_PHYS_RADIUS},
   {l1wrist, l1finger, 0.8, ARACHNID_LEG_PHYS_RADIUS},
   {r1wrist, r1finger, 0.0, ARACHNID_LEG_PHYS_RADIUS},
   {r1wrist, r1finger, 0.8, ARACHNID_LEG_PHYS_RADIUS},
};

static sCrPhysModOffsetTable g_WeaponPhysTable = \
{
   sizeof(g_SwordPhysOffsets)/sizeof(g_SwordPhysOffsets[0]), g_SwordPhysOffsets
};

static sCrPhysModOffset g_aPhysModOffsets[] = \
{
   { base, base, 0.0, 0.8},
   { base, sac, 2.0, 1 },
};


// array of mappings from jointIDs -> parent jointIDs
static int g_aSpidJointParents[] =
{
   base,    //  base   
   base,    //  lmand  
   lmand,   //  lmelbow
   base,    //  rmand  
   rmand,   //  rmelbow
   base,    //  r1shldr
   r1shldr, //  r1elbow
   r1elbow, //  r1wrist
   base,    //  r2shldr
   r2shldr, //  r2elbow
   r2elbow, //  r2wrist
   base,    //  r3shldr
   r3shldr, //  r3elbow
   r3elbow, //  r3wrist
   base,    //  r4shldr
   r4shldr, //  r4elbow
   r4elbow, //  r4wrist
   base,    //  l1shldr
   l1shldr, //  l1elbow
   l1elbow, //  l1wrist
   base,    //  l2shldr
   l2shldr, //  l2elbow
   l2elbow, //  l2wrist
   base,    //  l3shldr
   l3shldr, //  l3elbow
   l3elbow, //  l3wrist
   base,    //  l4shldr
   l4shldr, //  l4elbow
   l4elbow, //  l4wrist
   r1wrist, // r1finger
   r2wrist, // r2finger
   r3wrist, // r3finger
   r4wrist, // r4finger
   l1wrist, // l1finger
   l2wrist, // l2finger
   l3wrist, // l3finger
   l4wrist, // l4finger
   lmelbow, // ltip    
   rmelbow, // rtip    
   base,    //  sac
};
        
// @TODO: finish filling these out for real
         
static torso g_aDefTorsos[]= {\
   {base,-1,3,{lmand,rmand,r1shldr}},
   {base,-1,3,{r2shldr,r3shldr,r4shldr}},
   {base,-1,3,{l1shldr,l1shldr,l3shldr}},
   {base,-1,2,{l4shldr, sac}},
};

static limb g_aDefLimbs[]= {\
   {0,1,2,{rmand,rmelbow,rtip}},   
   {0,1,2,{lmand,lmelbow,ltip}},   
   {0,1,3,{r1shldr,r1elbow,r1wrist,r1finger}},   
   {0,1,3,{r2shldr,r2elbow,r2wrist,r2finger}},   
   {0,1,3,{r3shldr,r3elbow,r3wrist,r3finger}},   
   {0,1,3,{r4shldr,r4elbow,r4wrist,r4finger}},   
   {0,1,3,{l1shldr,l1elbow,l1wrist,l1finger}},   
   {0,1,3,{l2shldr,l2elbow,l2wrist,l2finger}},   
   {0,1,3,{l3shldr,l3elbow,l3wrist,l3finger}},   
   {0,1,3,{l4shldr,l4elbow,l4wrist,l4finger}},   
};

static int g_FootJoints[]={ r1finger, l1finger };

const sCreatureDesc sCrArachnidDesc = \
{
     kShATYPE_Arachnid, // actor type
     4,        // num torsos
     10,       // num limbs
     40,        // num joints
     "aracboss",  // default lengths filename
     sizeof(g_aPhysModOffsets)/sizeof(g_aPhysModOffsets[0]),  // number of phys models
     0.8, // default phys radius
     g_aPhysModOffsets, // array of phys model offsets
     g_aSpidJointParents,   // joint parent mapping
     g_aSpidJointMap,
     sizeof(g_FootJoints)/sizeof(g_FootJoints[0]),         // num feet
     g_FootJoints, // foot joints
     g_aDefTorsos,      // default torso values (if lengths file not found)
     g_aDefLimbs,      // default limb values (if lengths file not found)
     kShCrWeap_NumWeapons, // number of weapons
     &g_WeaponPhysTable, // weapon physics model offsets
     FALSE,    // always ballistic
     FALSE,    // uses head tracking
     TRUE,     // special physics
};
