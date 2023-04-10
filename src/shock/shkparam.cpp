/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkparam.cpp,v 1.23 1999/06/17 23:54:04 XEMU Exp $
//

#include <shkparam.h>

#include <filevar.h>

#include <sdesc.h>
#include <sdesbase.h>

// must be last headers!!!!
#include <dbmem.h>

//////////////////////////////////////////
// Game constants, exposed as filevars
//////////////////////////////////////////

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gStatCostDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "STATCOST",          // Tag file tag
   "Stats Cost",     // friendly name
   FILEVAR_TYPE(sStatCost),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sStatCost,&gStatCostDesc> gStatCost; 

// sdescs
#ifdef EDITOR
static sFieldDesc StatCostFields[] =
{
   { "STR 1->2",    kFieldTypeInt,  FieldLocation(sStatCost, costs[0][0]),  kFieldFlagNone },
   { "STR 2->3",    kFieldTypeInt,  FieldLocation(sStatCost, costs[0][1]),  kFieldFlagNone },
   { "STR 3->4",    kFieldTypeInt,  FieldLocation(sStatCost, costs[0][2]),  kFieldFlagNone },
   { "STR 4->5",    kFieldTypeInt,  FieldLocation(sStatCost, costs[0][3]),  kFieldFlagNone },
   { "STR 5->6",    kFieldTypeInt,  FieldLocation(sStatCost, costs[0][4]),  kFieldFlagNone },

   { "END 1->2",    kFieldTypeInt,  FieldLocation(sStatCost, costs[1][0]),  kFieldFlagNone },
   { "END 2->3",    kFieldTypeInt,  FieldLocation(sStatCost, costs[1][1]),  kFieldFlagNone },
   { "END 3->4",    kFieldTypeInt,  FieldLocation(sStatCost, costs[1][2]),  kFieldFlagNone },
   { "END 4->5",    kFieldTypeInt,  FieldLocation(sStatCost, costs[1][3]),  kFieldFlagNone },
   { "END 5->6",    kFieldTypeInt,  FieldLocation(sStatCost, costs[1][4]),  kFieldFlagNone },

   { "PSI 1->2",    kFieldTypeInt,  FieldLocation(sStatCost, costs[2][0]),  kFieldFlagNone },
   { "PSI 2->3",    kFieldTypeInt,  FieldLocation(sStatCost, costs[2][1]),  kFieldFlagNone },
   { "PSI 3->4",    kFieldTypeInt,  FieldLocation(sStatCost, costs[2][2]),  kFieldFlagNone },
   { "PSI 4->5",    kFieldTypeInt,  FieldLocation(sStatCost, costs[2][3]),  kFieldFlagNone },
   { "PSI 5->6",    kFieldTypeInt,  FieldLocation(sStatCost, costs[2][4]),  kFieldFlagNone },

   { "AGI 1->2",    kFieldTypeInt,  FieldLocation(sStatCost, costs[3][0]),  kFieldFlagNone },
   { "AGI 2->3",    kFieldTypeInt,  FieldLocation(sStatCost, costs[3][1]),  kFieldFlagNone },
   { "AGI 3->4",    kFieldTypeInt,  FieldLocation(sStatCost, costs[3][2]),  kFieldFlagNone },
   { "AGI 4->5",    kFieldTypeInt,  FieldLocation(sStatCost, costs[3][3]),  kFieldFlagNone },
   { "AGI 5->6",    kFieldTypeInt,  FieldLocation(sStatCost, costs[3][4]),  kFieldFlagNone },

   { "CYB 1->2",    kFieldTypeInt,  FieldLocation(sStatCost, costs[4][0]),  kFieldFlagNone },
   { "CYB 2->3",    kFieldTypeInt,  FieldLocation(sStatCost, costs[4][1]),  kFieldFlagNone },
   { "CYB 3->4",    kFieldTypeInt,  FieldLocation(sStatCost, costs[4][2]),  kFieldFlagNone },
   { "CYB 4->5",    kFieldTypeInt,  FieldLocation(sStatCost, costs[4][3]),  kFieldFlagNone },
   { "CYB 5->6",    kFieldTypeInt,  FieldLocation(sStatCost, costs[4][4]),  kFieldFlagNone },
};

static sStructDesc StatCostDesc = StructDescBuild(sStatCost, kStructFlagNone, StatCostFields);
#endif

sStatCost* GetStatCosts(void)
{
   return &gStatCost;
}

///////////////////////////////

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gWeaponSkillCostDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "WSKILLCOST",          // Tag file tag
   "Weapon Skills Cost",     // friendly name
   FILEVAR_TYPE(sWeaponSkillCost),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sWeaponSkillCost,&gWeaponSkillCostDesc> gWeaponSkillCost; 

// sdescs
#ifdef EDITOR
static sFieldDesc WeaponSkillCostFields[] =
{
   { "Conv 0->1",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][0]),  kFieldFlagNone },
   { "Conv 1->2",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][1]),  kFieldFlagNone },
   { "Conv 2->3",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][2]),  kFieldFlagNone },
   { "Conv 3->4",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][3]),  kFieldFlagNone },
   { "Conv 4->5",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][4]),  kFieldFlagNone },
   { "Conv 5->6",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[0][5]),  kFieldFlagNone },

   { "Energy 0->1",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][0]),  kFieldFlagNone },
   { "Energy 1->2",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][1]),  kFieldFlagNone },
   { "Energy 2->3",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][2]),  kFieldFlagNone },
   { "Energy 3->4",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][3]),  kFieldFlagNone },
   { "Energy 4->5",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][4]),  kFieldFlagNone },
   { "Energy 5->6",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[1][5]),  kFieldFlagNone },

   { "Heavy 0->1",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][0]),  kFieldFlagNone },
   { "Heavy 1->2",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][1]),  kFieldFlagNone },
   { "Heavy 2->3",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][2]),  kFieldFlagNone },
   { "Heavy 3->4",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][3]),  kFieldFlagNone },
   { "Heavy 4->5",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][4]),  kFieldFlagNone },
   { "Heavy 5->6",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[2][5]),  kFieldFlagNone },

   { "Alien 0->1",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][0]),  kFieldFlagNone },
   { "Alien 1->2",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][1]),  kFieldFlagNone },
   { "Alien 2->3",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][2]),  kFieldFlagNone },
   { "Alien 3->4",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][3]),  kFieldFlagNone },
   { "Alien 4->5",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][4]),  kFieldFlagNone },
   { "Alien 5->6",    kFieldTypeInt,  FieldLocation(sWeaponSkillCost, costs[3][5]),  kFieldFlagNone },
};

static sStructDesc WeaponSkillCostDesc = StructDescBuild(sWeaponSkillCost, kStructFlagNone, WeaponSkillCostFields);
#endif

sWeaponSkillCost* GetWeaponSkillCosts(void)
{
   return &gWeaponSkillCost;
}

/////////////////////

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gTechSkillCostDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "WTECHCOST",          // Tag file tag
   "Tech Skills Cost",     // friendly name
   FILEVAR_TYPE(sTechSkillCost),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sTechSkillCost,&gTechSkillCostDesc> gTechSkillCost; 

// sdescs
#ifdef EDITOR
static sFieldDesc TechSkillCostFields[] =
{
   { "Hack 0->1",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][0]),  kFieldFlagNone },
   { "Hack 1->2",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][1]),  kFieldFlagNone },
   { "Hack 2->3",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][2]),  kFieldFlagNone },
   { "Hack 3->4",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][3]),  kFieldFlagNone },
   { "Hack 4->5",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][4]),  kFieldFlagNone },
   { "Hack 5->6",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[0][5]),  kFieldFlagNone },

   { "Repair 0->1",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][0]),  kFieldFlagNone },
   { "Repair 1->2",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][1]),  kFieldFlagNone },
   { "Repair 2->3",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][2]),  kFieldFlagNone },
   { "Repair 3->4",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][3]),  kFieldFlagNone },
   { "Repair 4->5",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][4]),  kFieldFlagNone },
   { "Repair 5->6",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[1][5]),  kFieldFlagNone },

   { "Modify 0->1",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][0]),  kFieldFlagNone },
   { "Modify 1->2",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][1]),  kFieldFlagNone },
   { "Modify 2->3",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][2]),  kFieldFlagNone },
   { "Modify 3->4",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][3]),  kFieldFlagNone },
   { "Modify 4->5",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][4]),  kFieldFlagNone },
   { "Modify 5->6",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[2][5]),  kFieldFlagNone },

   { "Maint 0->1",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][0]),  kFieldFlagNone },
   { "Maint 1->2",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][1]),  kFieldFlagNone },
   { "Maint 2->3",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][2]),  kFieldFlagNone },
   { "Maint 3->4",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][3]),  kFieldFlagNone },
   { "Maint 4->5",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][4]),  kFieldFlagNone },
   { "Maint 5->6",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[3][5]),  kFieldFlagNone },

   { "Rsrch 0->1",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][0]),  kFieldFlagNone },
   { "Rsrch 1->2",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][1]),  kFieldFlagNone },
   { "Rsrch 2->3",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][2]),  kFieldFlagNone },
   { "Rsrch 3->4",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][3]),  kFieldFlagNone },
   { "Rsrch 4->5",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][4]),  kFieldFlagNone },
   { "Rsrch 5->6",    kFieldTypeInt,  FieldLocation(sTechSkillCost, costs[4][5]),  kFieldFlagNone },
};

static sStructDesc TechSkillCostDesc = StructDescBuild(sTechSkillCost, kStructFlagNone, TechSkillCostFields);
#endif

sTechSkillCost* GetTechSkillCosts(void)
{
   return &gTechSkillCost;
}

////////////////////////////

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gPsiCostDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "PSICOST",          // Tag file tag
   "Psi Cost",     // friendly name
   FILEVAR_TYPE(sPsiCost),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sPsiCost,&gPsiCostDesc> gPsiCost; 

// sdescs
#ifdef EDITOR
static sFieldDesc PsiCostFields[] =
{
   { "Level 1",    kFieldTypeInt,  FieldLocation(sPsiCost, costs[0]),  kFieldFlagNone },
   { "Psi Screen", kFieldTypeInt, FieldLocation(sPsiCost, costs[1]), kFieldFlagNone },
   { "Still Hand", kFieldTypeInt, FieldLocation(sPsiCost, costs[2]), kFieldFlagNone },
   { "Pull", kFieldTypeInt, FieldLocation(sPsiCost, costs[3]), kFieldFlagNone },
   { "Quickness", kFieldTypeInt, FieldLocation(sPsiCost, costs[4]), kFieldFlagNone },
   { "Cyber", kFieldTypeInt, FieldLocation(sPsiCost, costs[5]), kFieldFlagNone },
   { "Cryokinesis", kFieldTypeInt, FieldLocation(sPsiCost, costs[6]), kFieldFlagNone },
   { "Codebreaker", kFieldTypeInt, FieldLocation(sPsiCost, costs[7]), kFieldFlagNone },

   { "Level 2", kFieldTypeInt, FieldLocation(sPsiCost, costs[8]), kFieldFlagNone },
   { "Stability Field", kFieldTypeInt, FieldLocation(sPsiCost, costs[9]), kFieldFlagNone },
   { "Berserk", kFieldTypeInt, FieldLocation(sPsiCost, costs[10]), kFieldFlagNone },
   { "Rad Shield", kFieldTypeInt, FieldLocation(sPsiCost, costs[11]), kFieldFlagNone },
   { "Healing", kFieldTypeInt, FieldLocation(sPsiCost, costs[12]), kFieldFlagNone },
   { "Might", kFieldTypeInt, FieldLocation(sPsiCost, costs[13]), kFieldFlagNone },
   { "Psi", kFieldTypeInt, FieldLocation(sPsiCost, costs[14]), kFieldFlagNone },
   { "Immolate", kFieldTypeInt, FieldLocation(sPsiCost, costs[15]), kFieldFlagNone },

   { "Level 3", kFieldTypeInt, FieldLocation(sPsiCost, costs[16]), kFieldFlagNone },
   { "Fabricate", kFieldTypeInt, FieldLocation(sPsiCost, costs[17]), kFieldFlagNone },
   { "Electro", kFieldTypeInt, FieldLocation(sPsiCost, costs[18]), kFieldFlagNone },
   { "Anti-Psi", kFieldTypeInt, FieldLocation(sPsiCost, costs[19]), kFieldFlagNone },
   { "Toxin Shield", kFieldTypeInt, FieldLocation(sPsiCost, costs[20]), kFieldFlagNone },
   { "Radar", kFieldTypeInt, FieldLocation(sPsiCost, costs[21]), kFieldFlagNone },
   { "Pyrokinesis", kFieldTypeInt, FieldLocation(sPsiCost, costs[22]), kFieldFlagNone },
   { "Terror", kFieldTypeInt, FieldLocation(sPsiCost, costs[23]), kFieldFlagNone },

   { "Level 4", kFieldTypeInt, FieldLocation(sPsiCost, costs[24]), kFieldFlagNone },
   { "Invisiblity", kFieldTypeInt, FieldLocation(sPsiCost, costs[25]), kFieldFlagNone },
   { "Seekersense", kFieldTypeInt, FieldLocation(sPsiCost, costs[26]), kFieldFlagNone },
   { "Electro-Dampen", kFieldTypeInt, FieldLocation(sPsiCost, costs[27]), kFieldFlagNone },
   { "Vitality", kFieldTypeInt, FieldLocation(sPsiCost, costs[28]), kFieldFlagNone },
   { "Alchemy", kFieldTypeInt, FieldLocation(sPsiCost, costs[29]), kFieldFlagNone },
   { "Cyber-Hack", kFieldTypeInt, FieldLocation(sPsiCost, costs[30]), kFieldFlagNone },
   { "Psi Sword", kFieldTypeInt, FieldLocation(sPsiCost, costs[31]), kFieldFlagNone },

   { "Level 5", kFieldTypeInt, FieldLocation(sPsiCost, costs[32]), kFieldFlagNone },
   { "Major Heal", kFieldTypeInt, FieldLocation(sPsiCost, costs[33]), kFieldFlagNone },
   { "Soma Drain", kFieldTypeInt, FieldLocation(sPsiCost, costs[34]), kFieldFlagNone },
   { "Teleport", kFieldTypeInt, FieldLocation(sPsiCost, costs[35]), kFieldFlagNone },
   { "Enrage", kFieldTypeInt, FieldLocation(sPsiCost, costs[36]), kFieldFlagNone },
   { "Force Wall", kFieldTypeInt, FieldLocation(sPsiCost, costs[37]), kFieldFlagNone },
   { "Psi Mines", kFieldTypeInt, FieldLocation(sPsiCost, costs[38]), kFieldFlagNone },
   { "Psi Shield", kFieldTypeInt, FieldLocation(sPsiCost, costs[39]), kFieldFlagNone },

};

static sStructDesc PsiCostDesc = StructDescBuild(sPsiCost, kStructFlagNone, PsiCostFields);
#endif

sPsiCost* GetPsiCosts(void)
{
   return &gPsiCost;
}

///////////////////////////////////////////
// Skill related parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gSkillParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "SKILLPARAM",          // Tag file tag
   "Skill Params",     // friendly name
   FILEVAR_TYPE(sSkillParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sSkillParams,&gSkillParamsDesc> gSkillParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc SkillParamsFields[] =
{
   { "Inaccuracy (outdated)",    kFieldTypeInt,  FieldLocation(sSkillParams, m_inaccuracy),  kFieldFlagHex},
   { "Break Modifier",    kFieldTypeFloat,  FieldLocation(sSkillParams, m_breakModifier),  kFieldFlagNone},
   { "Research Factor",    kFieldTypeFloat,  FieldLocation(sSkillParams, m_research),  kFieldFlagNone},
   { "Weapon Skill/Damage",    kFieldTypeFloat,  FieldLocation(sSkillParams, m_damageModifier),  kFieldFlagNone},
   { "Organ Damage Factor",    kFieldTypeFloat,  FieldLocation(sSkillParams, m_researchdmg),  kFieldFlagNone},
};

static sStructDesc SkillParamsDesc = StructDescBuild(sSkillParams, kStructFlagNone, SkillParamsFields);
#endif

sSkillParams* GetSkillParams(void)
{
   return &gSkillParams;
}

///////////////////////////////////////////
// Misc game parameters
///////////////////////////////////////////

sFileVarDesc gGameParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "GAMEPARAM",          // Tag file tag
   "Game Parameters",     // friendly name
   FILEVAR_TYPE(sGameParams),  // Type (for editing)
   { 1, 1},             // version
   { 1, 1},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sGameParams,&gGameParamsDesc> gGameParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc GameParamsFields[] =
{
   { "Throw Power",   kFieldTypeFloat,  FieldLocation(sGameParams, throwpower),  kFieldFlagNone },

   { "Bash 1",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[0]),  kFieldFlagNone },
   { "Bash 2",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[1]),  kFieldFlagNone },
   { "Bash 3",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[2]),  kFieldFlagNone },
   { "Bash 4",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[3]),  kFieldFlagNone },
   { "Bash 5",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[4]),  kFieldFlagNone },
   { "Bash 6",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[5]),  kFieldFlagNone },
   { "Bash 7",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[6]),  kFieldFlagNone },
   { "Bash 8",   kFieldTypeFloat,  FieldLocation(sGameParams, bash[7]),  kFieldFlagNone },

   { "Speed 1",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[0]),  kFieldFlagNone },
   { "Speed 2",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[1]),  kFieldFlagNone },
   { "Speed 3",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[2]),  kFieldFlagNone },
   { "Speed 4",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[3]),  kFieldFlagNone },
   { "Speed 5",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[4]),  kFieldFlagNone },
   { "Speed 6",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[5]),  kFieldFlagNone },
   { "Speed 7",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[6]),  kFieldFlagNone },
   { "Speed 8",   kFieldTypeFloat,  FieldLocation(sGameParams, speed[7]),  kFieldFlagNone },

   { "Overlay Dist",   kFieldTypeFloat,  FieldLocation(sGameParams, overlaydist),  kFieldFlagNone },
   { "Frob Dist",   kFieldTypeFloat,  FieldLocation(sGameParams, frobdist),  kFieldFlagNone },
};

static sStructDesc GameParamsDesc = StructDescBuild(sGameParams, kStructFlagNone, GameParamsFields);
#endif

sGameParams* GetGameParams(void)
{
   return &gGameParams;
}

///////////////////////////////////////////
// Gun animation related parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gGunAnimParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "GUNANIM",          // Tag file tag
   "Gun Animation",     // friendly name
   FILEVAR_TYPE(sGunAnimParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sGunAnimParams,&gGunAnimParamsDesc> gGunAnimParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc GunAnimParamsFields[] =
{
   { "Wobble Speed",    kFieldTypeFloat,  FieldLocation(sGunAnimParams, m_wobbleSpeed),  kFieldFlagNone},
   { "Swing Amplitude",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_swingAmplitude),  kFieldFlagHex},
   { "Swing Period",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_swingPeriod),  kFieldFlagNone},
   { "Swing Return Rate",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_swingReturn),  kFieldFlagHex},
   { "Bob Amplitude",    kFieldTypeFloat,  FieldLocation(sGunAnimParams, m_bobAmplitude),  kFieldFlagNone},
   { "Bob Rate",    kFieldTypeFloat,  FieldLocation(sGunAnimParams, m_bobRate),  kFieldFlagNone},
   { "Raised Pitch",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_raisedPitch),  kFieldFlagHex},
   { "Lowered Pitch",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_loweredPitch),  kFieldFlagHex},
   { "Raise Rate",    kFieldTypeInt,  FieldLocation(sGunAnimParams, m_raisePitchRate),  kFieldFlagHex},
};

static sStructDesc GunAnimParamsDesc = StructDescBuild(sGunAnimParams, kStructFlagNone, GunAnimParamsFields);
#endif

sGunAnimParams* GetGunAnimParams(void)
{
   return &gGunAnimParams;
}

///////////////////////////////////////////
// Melee/Strength parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gMeleeStrengthParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "MELEESTR",          // Tag file tag
   "Melee-Strength",     // friendly name
   FILEVAR_TYPE(sMeleeStrengthParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sMeleeStrengthParams,&gMeleeStrengthParamsDesc> gMeleeStrengthParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc meleeStrengthParamsFields[] =
{
   { "Melee Adj Strength 1",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[0]),  kFieldFlagNone},
   { "Melee Adj Strength 2",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[1]),  kFieldFlagNone},
   { "Melee Adj Strength 3",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[2]),  kFieldFlagNone},
   { "Melee Adj Strength 4",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[3]),  kFieldFlagNone},
   { "Melee Adj Strength 5",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[4]),  kFieldFlagNone},
   { "Melee Adj Strength 6",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[5]),  kFieldFlagNone},
   { "Melee Adj Strength 7",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[6]),  kFieldFlagNone},
   { "Melee Adj Strength 8",    kFieldTypeInt,  FieldLocation(sMeleeStrengthParams, m_meleeMods[7]),  kFieldFlagNone},
};

static sStructDesc meleeStrengthParamsDesc = StructDescBuild(sMeleeStrengthParams, kStructFlagNone, meleeStrengthParamsFields);
#endif

sMeleeStrengthParams* GetMeleeStrengthParams(void)
{
   return &gMeleeStrengthParams;
}

///////////////////////////////////////////
// Elevator levels
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gElevParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "Elev",          // Tag file tag
   "Elev Levels",     // friendly name
   FILEVAR_TYPE(sElevParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sElevParams,&gElevParamsDesc> gElevParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc ElevParamsFields[] =
{
   { "Deck 1", kFieldTypeString, FieldLocation(sElevParams, m_levels[0]), kFieldFlagNone},
   { "Deck 2", kFieldTypeString, FieldLocation(sElevParams, m_levels[1]), kFieldFlagNone},
   { "Deck 3", kFieldTypeString, FieldLocation(sElevParams, m_levels[2]), kFieldFlagNone},
   { "Deck 4", kFieldTypeString, FieldLocation(sElevParams, m_levels[3]), kFieldFlagNone},
   { "Deck 5", kFieldTypeString, FieldLocation(sElevParams, m_levels[4]), kFieldFlagNone},
};

static sStructDesc ElevParamsDesc = StructDescBuild(sElevParams, kStructFlagNone, ElevParamsFields);
#endif

sElevParams* GetElevParams(void)
{
   return &gElevParams;
}

///////////////////////////////////////////
// Hack, repair, modify constants
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gHRMParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "HRM",          // Tag file tag
   "Tech Params",     // friendly name
   FILEVAR_TYPE(sHRMParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sHRMParams,&gHRMParamsDesc> gHRMParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc HRMParamsFields[] =
{
   { "Critfail Bonus (Skill)", kFieldTypeInt, FieldLocation(sHRMParams, m_skillCritfailBonus), kFieldFlagNone},
   { "Critfail Bonus (Stat)", kFieldTypeInt, FieldLocation(sHRMParams, m_statCritfailBonus), kFieldFlagNone},
   { "Success Bonus (Skill)", kFieldTypeInt, FieldLocation(sHRMParams, m_skillSuccessBonus), kFieldFlagNone},
   { "Success Bonus (Stat)", kFieldTypeInt, FieldLocation(sHRMParams, m_statSuccessBonus), kFieldFlagNone},
   { "Break Chance, Cyber = 1", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[0]), kFieldFlagNone},
   { "Break Chance, Cyber = 2", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[1]), kFieldFlagNone},
   { "Break Chance, Cyber = 3", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[2]), kFieldFlagNone},
   { "Break Chance, Cyber = 4", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[3]), kFieldFlagNone},
   { "Break Chance, Cyber = 5", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[4]), kFieldFlagNone},
   { "Break Chance, Cyber = 6", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[5]), kFieldFlagNone},
   { "Break Chance, Cyber = 7", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[6]), kFieldFlagNone},
   { "Break Chance, Cyber = 8", kFieldTypeFloat, FieldLocation(sHRMParams, m_statBreak[7]), kFieldFlagNone},
}; 

static sStructDesc HRMParamsDesc = StructDescBuild(sHRMParams, kStructFlagNone, HRMParamsFields);
#endif

sHRMParams* GetHRMParams(void)
{
   return &gHRMParams;
}


///////////////////////////////////////////
// Stat-based numbers (gamesys)
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gStatParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "STATPARAM",          // Tag file tag
   "Stat Params",     // friendly name
   FILEVAR_TYPE(sStatParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sStatParams,&gStatParamsDesc> gStatParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc StatParamsFields[] =
{
   { "Starting HP", kFieldTypeInt, FieldLocation(sStatParams, m_hpbase), kFieldFlagNone},
   { "HP per END", kFieldTypeInt, FieldLocation(sStatParams, m_hpbonus), kFieldFlagNone},
   { "Starting PP", kFieldTypeInt, FieldLocation(sStatParams, m_ppbase), kFieldFlagNone},
   { "PP per PSI", kFieldTypeInt, FieldLocation(sStatParams, m_ppbonus), kFieldFlagNone},
   { "Min Camera Vis", kFieldTypeFloat, FieldLocation(sStatParams, m_camvismin), kFieldFlagNone},
   { "Max Camera Vis", kFieldTypeFloat, FieldLocation(sStatParams, m_camvismax), kFieldFlagNone},
   { "Hazard Damage, END 1", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[0]), kFieldFlagNone},
   { "Hazard Damage, END 2", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[1]), kFieldFlagNone},
   { "Hazard Damage, END 3", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[2]), kFieldFlagNone},
   { "Hazard Damage, END 4", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[3]), kFieldFlagNone},
   { "Hazard Damage, END 5", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[4]), kFieldFlagNone},
   { "Hazard Damage, END 6", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[5]), kFieldFlagNone},
   { "Hazard Damage, END 7", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[6]), kFieldFlagNone},
   { "Hazard Damage, END 8", kFieldTypeFloat, FieldLocation(sStatParams, m_hazard[7]), kFieldFlagNone},
}; 

static sStructDesc StatParamsDesc = StructDescBuild(sStatParams, kStructFlagNone, StatParamsFields);
#endif

sStatParams* GetStatParams(void)
{
   return &gStatParams;
}


///////////////////////////////////////////
// Stat-based numbers (gamesys)
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gTraitParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "TRAITPARAM",          // Tag file tag
   "Trait Params",     // friendly name
   FILEVAR_TYPE(sTraitParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sTraitParams,&gTraitParamsDesc> gTraitParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc TraitParamsFields[] =
{
   { "Tank HP bonus", kFieldTypeInt, FieldLocation(sTraitParams, m_hpbonus), kFieldFlagNone},
   { "SharpShooter Stim Mult", kFieldTypeFloat, FieldLocation(sTraitParams, m_sharpMult), kFieldFlagNone},
   { "LethalWeapon Stim Mult", kFieldTypeFloat, FieldLocation(sTraitParams, m_lethalMult), kFieldFlagNone},
}; 

static sStructDesc TraitParamsDesc = StructDescBuild(sTraitParams, kStructFlagNone, TraitParamsFields);
#endif

sTraitParams* GetTraitParams(void)
{
   return &gTraitParams;
}

//////////////////////////////////////
// Implants
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gImplantParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "IMPLPARAM",          // Tag file tag
   "Implant Params",     // friendly name
   FILEVAR_TYPE(sImplantParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sImplantParams,&gImplantParamsDesc> gImplantParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc ImplantParamsFields[] =
{
   { "WormBlend Vis Mult", kFieldTypeInt, FieldLocation(sImplantParams, m_blendMult), kFieldFlagNone},
}; 

static sStructDesc ImplantParamsDesc = StructDescBuild(sImplantParams, kStructFlagNone, ImplantParamsFields);
#endif

sImplantParams* GetImplantParams(void)
{
   return &gImplantParams;
}

///////////////////////////////////////////
// overload parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gOverloadParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "OLPARAM",          // Tag file tag
   "Overload Params",     // friendly name
   FILEVAR_TYPE(sOverloadParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sOverloadParams,&gOverloadParamsDesc> gOverloadParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc OverloadParamsFields[] =
{
   { "Burn factor, END = 1", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[0]), kFieldFlagNone},
   { "Burn factor, END = 2", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[1]), kFieldFlagNone},
   { "Burn factor, END = 3", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[2]), kFieldFlagNone},
   { "Burn factor, END = 4", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[3]), kFieldFlagNone},
   { "Burn factor, END = 5", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[4]), kFieldFlagNone},
   { "Burn factor, END = 6", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[5]), kFieldFlagNone},
   { "Burn factor, END = 7", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[6]), kFieldFlagNone},
   { "Burn factor, END = 8", kFieldTypeFloat, FieldLocation(sOverloadParams, m_burnfactor[7]), kFieldFlagNone},
   { "Burnout time, level 1", kFieldTypeFloat, FieldLocation(sOverloadParams, m_length[0]), kFieldFlagNone},
   { "Burnout time, level 2", kFieldTypeFloat, FieldLocation(sOverloadParams, m_length[1]), kFieldFlagNone},
   { "Burnout time, level 3", kFieldTypeFloat, FieldLocation(sOverloadParams, m_length[2]), kFieldFlagNone},
   { "Burnout time, level 4", kFieldTypeFloat, FieldLocation(sOverloadParams, m_length[3]), kFieldFlagNone},
   { "Burnout time, level 5", kFieldTypeFloat, FieldLocation(sOverloadParams, m_length[4]), kFieldFlagNone},
   { "Threshold, PSI = 1", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[0]), kFieldFlagNone},
   { "Threshold, PSI = 2", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[1]), kFieldFlagNone},
   { "Threshold, PSI = 3", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[2]), kFieldFlagNone},
   { "Threshold, PSI = 4", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[3]), kFieldFlagNone},
   { "Threshold, PSI = 5", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[4]), kFieldFlagNone},
   { "Threshold, PSI = 6", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[5]), kFieldFlagNone},
   { "Threshold, PSI = 7", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[6]), kFieldFlagNone},
   { "Threshold, PSI = 8", kFieldTypeFloat, FieldLocation(sOverloadParams, m_thresh[7]), kFieldFlagNone},
   { "Burnout dmg per level", kFieldTypeInt, FieldLocation(sOverloadParams, m_burndmg), kFieldFlagNone},
}; 

static sStructDesc OverloadParamsDesc = StructDescBuild(sOverloadParams, kStructFlagNone, OverloadParamsFields);
#endif

sOverloadParams* GetOverloadParams(void)
{
   return &gOverloadParams;
}


///////////////////////////////////////////
// difficulty parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gDiffParamsDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "DIFFPARAM",          // Tag file tag
   "Difficulty Params",     // friendly name
   FILEVAR_TYPE(sDiffParams),  // Type (for editing)
   { 1, 1},             // version
   { 1, 1},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sDiffParams,&gDiffParamsDesc> gDiffParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc DiffParamsFields[] =
{
   { "Upgrade Mult: Playtest", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[0]), kFieldFlagNone},
   { "Upgrade Mult: Easy", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[1]), kFieldFlagNone},
   { "Upgrade Mult: Normal", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[2]), kFieldFlagNone},
   { "Upgrade Mult: Hard", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[3]), kFieldFlagNone},
   { "Upgrade Mult: Imposs", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[4]), kFieldFlagNone},
   { "Upgrade Mult: Multi", kFieldTypeFloat, FieldLocation(sDiffParams, m_traincost[5]), kFieldFlagNone},

   { "Base HP: Playtest", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[0]), kFieldFlagNone},
   { "Base HP: Easy", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[1]), kFieldFlagNone},
   { "Base HP: Normal", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[2]), kFieldFlagNone},
   { "Base HP: Hard", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[3]), kFieldFlagNone},
   { "Base HP: Imposs", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[4]), kFieldFlagNone},
   { "Base HP: Multi", kFieldTypeInt, FieldLocation(sDiffParams, m_basehp[5]), kFieldFlagNone},

   { "HP per END: Playtest", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[0]), kFieldFlagNone},
   { "HP per END: Easy", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[1]), kFieldFlagNone},
   { "HP per END: Normal", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[2]), kFieldFlagNone},
   { "HP per END: Hard", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[3]), kFieldFlagNone},
   { "HP per END: Imposs", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[4]), kFieldFlagNone},
   { "HP per END: Multi", kFieldTypeInt, FieldLocation(sDiffParams, m_hpEND[5]), kFieldFlagNone},

   { "Base PP: Playtest", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[0]), kFieldFlagNone},
   { "Base PP: Easy", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[1]), kFieldFlagNone},
   { "Base PP: Normal", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[2]), kFieldFlagNone},
   { "Base PP: Hard", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[3]), kFieldFlagNone},
   { "Base PP: Imposs", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[4]), kFieldFlagNone},
   { "Base PP: Multi", kFieldTypeInt, FieldLocation(sDiffParams, m_basepp[5]), kFieldFlagNone},

   { "PP per PSI: Playtest", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[0]), kFieldFlagNone},
   { "PP per PSI: Easy", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[1]), kFieldFlagNone},
   { "PP per PSI: Normal", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[2]), kFieldFlagNone},
   { "PP per PSI: Hard", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[3]), kFieldFlagNone},
   { "PP per PSI: Imposs", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[4]), kFieldFlagNone},
   { "PP per PSI: Multi", kFieldTypeInt, FieldLocation(sDiffParams, m_ppPSI[5]), kFieldFlagNone},

   { "Loot Hose %: Playtest", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[0]), kFieldFlagNone},
   { "Loot Hose %: Easy", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[1]), kFieldFlagNone},
   { "Loot Hose %: Normal", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[2]), kFieldFlagNone},
   { "Loot Hose %: Hard", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[3]), kFieldFlagNone},
   { "Loot Hose %: Imposs", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[4]), kFieldFlagNone},
   { "Loot Hose %: Multi", kFieldTypeInt, FieldLocation(sDiffParams, m_loothose[5]), kFieldFlagNone},

   { "Rep Cost: Playtest", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[0]), kFieldFlagNone},
   { "Rep Cost: Easy", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[1]), kFieldFlagNone},
   { "Rep Cost: Normal", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[2]), kFieldFlagNone},
   { "Rep Cost: Hard", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[3]), kFieldFlagNone},
   { "Rep Cost: Imposs", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[4]), kFieldFlagNone},
   { "Rep Cost: Multi", kFieldTypeFloat, FieldLocation(sDiffParams, m_repcost[5]), kFieldFlagNone},
}; 

static sStructDesc DiffParamsDesc = StructDescBuild(sDiffParams, kStructFlagNone, DiffParamsFields);
#endif

sDiffParams* GetDiffParams(void)
{
   return &gDiffParams;
}

///////////////////////////////////////////
// Map Params (level)
//

sFileVarDesc gMapParamsDesc = 
{
   kMissionVar,         // Where do I get saved?
   "MAPPARAM",          // Tag file tag
   "Map Params",     // friendly name
   FILEVAR_TYPE(sMapParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sMapParams,&gMapParamsDesc> gMapParams; 

// sdescs
#ifdef EDITOR
static sFieldDesc MapParamsFields[] =
{
   { "Rotate Hack?", kFieldTypeBool, FieldLocation(sMapParams, m_rotatehack), kFieldFlagNone},
}; 

static sStructDesc MapParamsDesc = StructDescBuild(sMapParams, kStructFlagNone, MapParamsFields);
#endif

sMapParams* GetMapParams(void)
{
   return &gMapParams;
}

///////////////////////////////////////////
void ShockParamEditInit(void)
{
#ifdef EDITOR
   StructDescRegister(&StatCostDesc);
   StructDescRegister(&TechSkillCostDesc);
   StructDescRegister(&WeaponSkillCostDesc);
   StructDescRegister(&PsiCostDesc);
   StructDescRegister(&SkillParamsDesc);
   StructDescRegister(&GameParamsDesc);
   StructDescRegister(&GunAnimParamsDesc);
   StructDescRegister(&meleeStrengthParamsDesc);
   StructDescRegister(&ElevParamsDesc);
   StructDescRegister(&HRMParamsDesc);
   StructDescRegister(&StatParamsDesc);
   StructDescRegister(&TraitParamsDesc);
   StructDescRegister(&ImplantParamsDesc);
   StructDescRegister(&OverloadParamsDesc);
   StructDescRegister(&DiffParamsDesc);
   StructDescRegister(&MapParamsDesc);
#endif
}
