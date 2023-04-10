/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkpsibs.h,v 1.13 2000/01/31 09:58:47 adurant Exp $
//
// Psionics system - basic types
//
#pragma once

#ifndef __SHKPSIBS_H
#define __SHKPSIBS_H

#include <simtime.h>
#include <objtype.h>

// argh, scripts
enum ePsiPowers;
typedef int ePsiPowerType;

enum ePsiPowerState_ {kPsiInactive, kPsiActive,};

// Player current psi state
struct sPsiState
{
   ePsiPowers m_currentPower;
   int m_points;
   int m_maxPoints;
};

enum ePsiPowerType_ {kPsiTypeShot=0, kPsiTypeShield=1, kPsiTypeOneShot=2, kPsiTypeSustained = 3, kPsiTypeCursor = 4, kPsiTypeMax};

#define kPsiDataNum 4

// Data for each psi power
struct sPsiPower
{
   ePsiPowers m_power;
   ePsiPowerType m_type;
   int m_startCost;   // cost for activating
   float m_data[kPsiDataNum];      // interpreted per power
};

// Data for psi shields
struct sPsiShield
{
   tSimTime m_baseTime;
   tSimTime m_addTime;  // time we add per point of int over base
   int m_baseInt;
};

#endif