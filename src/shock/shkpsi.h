/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsi.h,v 1.8 2000/01/31 09:58:46 adurant Exp $
#pragma once

#ifndef __SHKPSI_H
#define __SHKPSI_H

// interface to psionics system

#include <simtime.h>
#include <shkpsity.h>
#include <objtype.h>
#include <dmgmodel.h>

class cPsionics
{
public:
   cPsionics(ObjID objID, float maxPower);

   void Init(ObjID objID, float maxPower);
   void Select(ePsiType type);
   BOOL Activate(void);
   void Deactivate(void);
   void Frame(int dt);
   BOOL IsActive(void) const;
   ePsiType GetType(void);
   float GetPower(void);
   float GetMaxPower(void);
   void SetMaxPower(float maxPower);
   void SetPower(float power);
   BOOL IsCrashing(void) const;
   eDamageResult CrashImpactHandler();

private:
   ePsiType m_type;     // type of selected power
   ePsiState m_state;   
   tSimTime m_activateTime;
   BOOL m_crashing;     // we're out of control, bad things are happening
   tSimTime m_endCrashTime; // when do we end the crash, if above is TRUE
   float m_power;
   float m_maxPower;
   ObjID m_objID;       // user's ID
   tSimTime m_lastTime; // used by healing power

   void Overload(void);
};

#define min(a,b)  (((a) < (b)) ? (a) : (b))

inline ePsiType cPsionics::GetType(void) {return m_type;}
inline float cPsionics::GetPower(void) {return m_power;}
inline float cPsionics::GetMaxPower(void) {return m_maxPower;}
inline void cPsionics::SetMaxPower(float maxPower) {m_maxPower = maxPower; m_power = min(m_power, m_maxPower);}
inline void cPsionics::SetPower(float power) {m_power = min(power, m_maxPower);}
inline BOOL cPsionics::IsCrashing(void) const {return m_crashing;}

#endif