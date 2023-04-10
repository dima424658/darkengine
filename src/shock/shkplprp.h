/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkplprp.h,v 1.13 2000/01/31 09:58:35 adurant Exp $
#pragma once

#ifndef __SHKPLPRP_H
#define __SHKPLPRP_H

#include <propface.h>
#include <shkpdcst.h>

// Stats description property
F_DECLARE_INTERFACE(IStatsDescProperty);

#undef INTERFACE
#define INTERFACE IStatsDescProperty

DECLARE_PROPERTY_INTERFACE(IStatsDescProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sStatsDesc*);  // Type-specific accessors, by reference
};

// Traits description property
F_DECLARE_INTERFACE(ITraitsDescProperty);

#undef INTERFACE
#define INTERFACE ITraitsDescProperty

DECLARE_PROPERTY_INTERFACE(ITraitsDescProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sTraitsDesc*);  // Type-specific accessors, by reference
};

// WeaponSkills description property
F_DECLARE_INTERFACE(IWeaponSkillsProperty);

#undef INTERFACE
#define INTERFACE IWeaponSkillsProperty

DECLARE_PROPERTY_INTERFACE(IWeaponSkillsProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sWeaponSkills*);  // Type-specific accessors, by reference
};

// TechSkills description property
F_DECLARE_INTERFACE(ITechSkillsProperty);

#undef INTERFACE
#define INTERFACE ITechSkillsProperty

DECLARE_PROPERTY_INTERFACE(ITechSkillsProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sTechSkills*);  // Type-specific accessors, by reference
};

// LogData description property
F_DECLARE_INTERFACE(ILogDataProperty);

#undef INTERFACE
#define INTERFACE ILogDataProperty

DECLARE_PROPERTY_INTERFACE(ILogDataProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sLogData*);  // Type-specific accessors, by reference
};

EXTERN IIntProperty *g_PlayerExpProperty;
EXTERN IStringProperty *g_PlayerNameProperty;
EXTERN IIntProperty *g_PlayerPoolProperty;
EXTERN IIntProperty *g_PlayerLevelProperty;
EXTERN ITraitsDescProperty *g_TraitsProperty;

EXTERN IStatsDescProperty *g_BaseStatsProperty;
EXTERN IStatsDescProperty *g_PsiStatsProperty;
EXTERN IStatsDescProperty *g_DrugStatsProperty;
EXTERN IStatsDescProperty *g_ImplantStatsProperty;

EXTERN IWeaponSkillsProperty *g_BaseWeaponProperty;
EXTERN ITechSkillsProperty *g_BaseTechProperty;

EXTERN IStatsDescProperty *g_ReqStatsProperty;
EXTERN ITechSkillsProperty *g_ReqTechProperty;
EXTERN IIntProperty *g_ImplantProperty;
EXTERN IIntProperty *g_PsiLevelProperty;
EXTERN IIntProperty *g_PsiPowerProperty;
EXTERN IIntProperty *g_PsiPower2Property;

EXTERN ILogDataProperty *g_LogProperties[NUM_PDA_LEVELS];

EXTERN IIntProperty *g_ServiceProperty;

EXTERN IIntProperty *g_ResearchReportProperty;
EXTERN IIntProperty *g_HelpTextProperty;

EXTERN IIntProperty *g_CGYearProperty;
EXTERN IIntProperty *g_CGRoomProperty;

EXTERN IIntProperty *g_MapLocProperty;

#endif