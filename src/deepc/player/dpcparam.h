/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Game parameters
//
#pragma once

#ifndef __DPCPARAM_H
#define __DPCPARAM_H

#ifndef __MXANGS_H
#include <mxangs.h>
#endif // !__MXANGS_H

#ifndef __SIMTIME_H
#include <simtime.h>
#endif // !__SIMTIME_H

#ifndef __LABEL_H
#include <label.h>
#endif // !__LABEL_H

//////////////////////////////////////////
// Stat costs
// This is stored as the cost to go FROM that level, plus 1 (since 0->1 is meaningless)

struct sStatCost
{
   int costs[5][5];
};

sStatCost* GetStatCosts(void);

///////////////////////////////
// Weapon skill costs
// This is stored as the cost to go FROM that level

struct sWeaponSkillCost
{
   int costs[4][6];
};

sWeaponSkillCost* GetWeaponSkillCosts(void);

///////////////////////////////
// Tech Skill costs
// This is stored as the cost to go FROM that level
//

struct sTechSkillCost
{
   int costs[5][6];
};

sTechSkillCost* GetTechSkillCosts(void);

///////////////////////////////////////////
// Skill related parameters
//

struct sSkillParams
{
   mxs_ang m_inaccuracy;   // shot deviation/weapon skill level
   float m_breakModifier;  // percent decrease (multiplicative) in break chance per weapon skill level
   float m_research; // research factor per (skill level - 1) squared
   float m_damageModifier; // damage *= 1+damageModifer*skill
   float m_researchdmg; // extra damage factor if organ researched
};

sSkillParams* GetSkillParams(void);

///////////////////////////////////////////

typedef struct sGameParams {
   float throwpower;
   float bash[8];
   float speed[8];
   float overlaydist;
   float frobdist;
} sGameParams;

sGameParams* GetGameParams(void);

///////////////////////////////////////////
// Gun animation related parameters
//

struct sGunAnimParams
{
   mxs_ang m_swingAmplitude;
   tSimTime m_swingPeriod;
   mxs_ang m_swingReturn;
   float m_bobRate;
   float m_bobAmplitude;
   mxs_ang m_raisePitchRate;
   mxs_ang m_raisedPitch;
   mxs_ang m_loweredPitch;
   float m_wobbleSpeed;
};

sGunAnimParams* GetGunAnimParams(void);

///////////////////////////////////////////

struct sMeleeStrengthParams
{
   int m_meleeMods[8];
};

sMeleeStrengthParams *GetMeleeStrengthParams(void);

///////////////////////////////////////////

struct sElevParams
{
   char m_levels[5][64];
};

sElevParams* GetElevParams(void);

///////////////////////////////////////////

struct sHRMParams
{
   int m_skillCritfailBonus;
   int m_skillSuccessBonus;

   int m_statCritfailBonus;
   int m_statSuccessBonus;

   float m_statBreak[8];
};

sHRMParams* GetHRMParams(void);

///////////////////////////////////////////
struct sStatParams
{
   int m_hpbase;
   int m_hpbonus;
   int m_ppbase;
   int m_ppbonus;
   float m_camvismin;
   float m_camvismax;
   float m_hazard[8];
};

sStatParams* GetStatParams(void);

///////////////////////////////////////////

struct sTraitParams
{
   int m_hpbonus;
   float m_sharpMult;   // sharpshooter
   float m_lethalMult;  // lethal weapon
};

sTraitParams* GetTraitParams(void);

///////////////////////////////////////////

struct sImplantParams
{
   float m_blendMult;   // WormBlend
};

sImplantParams* GetImplantParams(void);

///////////////////////////////////////////

struct sOverloadParams
{
   float m_burnfactor[8];
   float m_length[5];
   float m_thresh[8];
   int m_burndmg;
};

sOverloadParams* GetOverloadParams(void);

///////////////////////////////////////////

struct sDiffParams
{
   float m_traincost[6];

   int m_basehp[6];
   int m_hpEND[6];
   int m_basepp[6];
   int m_ppPSI[6];

   int m_loothose[6];
   float m_repcost[6]; 
};

sDiffParams* GetDiffParams(void);

///////////////////////////////////////////

struct sMapParams
{
   BOOL m_rotatehack;
};

sMapParams* GetMapParams(void);

///////////////////////////////////////////

void DPCParamEditInit(void);
void DPCParamEditTerm(void);

#endif // __DPCPARAM_H