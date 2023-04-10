/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPROP_H
#define __DPCPROP_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

// Object Name Property
// this is the property that goes on an obj to set it's description
#define PROP_OBJNAME_NAME "ObjName"
EXTERN IStringProperty* gPropObjName;
#define ObjHasObjName(obj) IProperty_IsRelevant(gPropObjName,obj)
EXTERN void ObjGetObjNameSubst(ObjID obj, char *text, int buflen);

// Object Short Name Property
// short version of the same
#define PROP_OBJSHORTNAME_NAME "ObjShort"
EXTERN IStringProperty* gPropObjShortName;
#define ObjHasObjShortName(obj) IProperty_IsRelevant(gPropObjShortName,obj)
EXTERN void ObjGetObjShortNameSubst(ObjID obj, char *text, int buflen);

// Object Look String Property
// Longer descriptive text
#define PROP_OBJLOOKSTRING_NAME "ObjLookS"
EXTERN IStringProperty* gPropObjLookString;
#define ObjHasObjLookString(obj) IProperty_IsRelevant(gPropObjLookString,obj)
EXTERN BOOL ObjGetObjLookString(ObjID obj, const char **ppName);

// Experience points property
// exp you get for killing a monster
#define PROP_EXP_NAME "ExP"
EXTERN  IIntProperty* gPropExp;
#define ObjHasExp(obj) IProperty_IsRelevant(gPropExp,obj)
EXTERN  int ObjGetExp(ObjID obj);
#define ObjSetExp(obj,exp) PROPERTY_SET(gPropExp,obj,exp)

// Object Icon Property
// this is the property that goes on an obj to set it's icon name
#define PROP_OBJICON_NAME "ObjIcon"
EXTERN ILabelProperty* gPropObjIcon;
#define ObjHasObjIcon(obj) IProperty_IsRelevant(gPropObjIcon,obj)
EXTERN BOOL ObjGetObjIcon(ObjID obj, Label **ppName);
EXTERN void ObjSetObjIcon(ObjID obj, Label *pName);

#define PROP_OBJBROKENICON_NAME "ObjBrokenIcon"
EXTERN ILabelProperty* gPropObjBrokenIcon;

// AI Frobbing property.
#define PROP_AIFROB "AIFrob"
EXTERN IStringProperty* gPropAIFrob;
#define ObjHasAIFrob(obj) IProperty_IsRelevant(gPropAIFrob, obj)

// hacking properties
// for difficulty level and current amount hacked
typedef struct sTechInfo
{
   int m_success;
   int m_critfail;
   float m_cost; // relative percentage
} sTechInfo;

// TechInfo description property
F_DECLARE_INTERFACE(ITechInfoProperty);

#undef INTERFACE
#define INTERFACE ITechInfoProperty

DECLARE_PROPERTY_INTERFACE(ITechInfoProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sTechInfo*);  // Type-specific accessors, by reference
};

#define PROP_HACKDIFF_NAME "HackDiff"
EXTERN  ITechInfoProperty* gPropHackDiff;
#define PROP_REPAIRDIFF_NAME "RepairDiff"
EXTERN  ITechInfoProperty* gPropRepairDiff;
#define PROP_MODIFYDIFF_NAME "ModifyDiff"
EXTERN  ITechInfoProperty* gPropModifyDiff;
#define PROP_MODIFY2DIFF_NAME "Modify2Diff"
EXTERN  ITechInfoProperty* gPropModify2Diff;

#define PROP_MODIFY1TEXT_NAME "Modify1"
EXTERN IStringProperty *gPropModify1Text;
#define PROP_MODIFY2TEXT_NAME "Modify2"
EXTERN IStringProperty *gPropModify2Text;
#define PROP_HACKTEXT_NAME "HackText"
EXTERN IStringProperty *gPropHackText;

#define PROP_USEMESSAGE_NAME "UseMsg"
EXTERN IStringProperty* gPropUseMessage;

#define PROP_LOCKEDMESSAGE_NAME "LockMsg"
EXTERN IStringProperty* gPropLockedMessage;

#define PROP_HUDTIME_NAME "HUDTime"
EXTERN IIntProperty* gPropHUDTime;

EXTERN IIntProperty* gPropStackIncrem;

#define PROP_HUDSELECT_NAME "HUDSelect"
EXTERN IBoolProperty* gPropAllowHUDSelect;

EXTERN IBoolProperty* gPropShowHP;

#define PROP_HACKTIME_NAME "HackTime"
EXTERN IIntProperty* gPropHackTime;

#define PROP_ALARM_NAME "Alarm"
EXTERN IBoolProperty* gPropAlarm;

#define PROP_BLOCKFROB_NAME "BlockFrob"
EXTERN IBoolProperty* gPropBlockFrob;

#define PROP_DELAYTIME_NAME "DelayTime"
EXTERN IFloatProperty *gPropDelayTime;

#define PROP_HACKVISIBILITY_NAME "HackVisi"
EXTERN IFloatProperty *gPropHackVisibility;

#define PROP_SHOVE_NAME "Shove"
EXTERN IVectorProperty *gPropShove;

#define PROP_HUDUSE_NAME "HUDUse"
EXTERN IStringProperty *gPropHUDUse;

#define PROP_AUTOPICKUP_NAME "AutoPickup"
EXTERN IBoolProperty *gPropAutoPickup;

#define PROP_IS_BINOCULARS_NAME "IsBinoculars"
EXTERN IBoolProperty *gPropIsBinoculars;

#define PROP_IS_PLAYERCAMERA_NAME "IsPlayerCamera"
EXTERN IBoolProperty *gPropIsPlayerCamera;

#define PROP_IS_CAMERATARGET_NAME "IsCameraTarget"
EXTERN IBoolProperty *gPropIsCameraTarget;

#define PROP_ENERGY_NAME "Energy"
EXTERN IFloatProperty *gPropEnergy;

#define PROP_DRAINRATE_NAME "DrainRate"
EXTERN IFloatProperty *gPropDrainRate;

#define PROP_DRAINAMT_NAME "DrainAmt"
EXTERN IFloatProperty *gPropDrainAmt;

#define PROP_CONSUMETYPE_NAME "ConsumeType"
EXTERN IStringProperty *gPropConsumeType;

#define PROP_METAPROPTYPE_NAME "MetapropType"
EXTERN IStringProperty *gPropMetapropType;

#define PROP_OBJLIST_NAME "ObjList"
EXTERN IStringProperty *gPropObjList;

#define PROP_SIGNALTYPE_NAME "SignalType"
EXTERN IStringProperty *gPropSignalType;

#define PROP_QBNAME_NAME "QBName"
EXTERN IStringProperty *gPropQBName;

#define PROP_QBVAL_NAME "QBVal"
EXTERN IIntProperty *gPropQBVal;

#define PROP_TRIPFLAGS_NAME "TripFlags"
EXTERN IIntProperty* gPropTripFlags;

#define PROP_PLOTCRITICAL_NAME "PlotCritical"
EXTERN IBoolProperty* gPlotCritical;

// Object state
typedef int eObjState;

#define PROP_OBJ_STATE_NAME "ObjState"
EXTERN IIntProperty* gPropObjState;
EXTERN eObjState ObjGetObjState(ObjID objID);
EXTERN void ObjSetObjState(ObjID objID, eObjState state);

// software properties
#define PROP_SOFTWARELEVEL_NAME "SoftLevel"
#define PROP_SOFTWARETYPE_NAME "SoftType"
EXTERN IIntProperty* gPropSoftwareLevel;
EXTERN IIntProperty* gPropSoftwareType;

// Loot property
#define MAX_LOOT_ITEMS  6
#define MAX_LOOT_PICKS  6
typedef struct sLootInfo
{
   int m_numpicks;
   char m_items[MAX_LOOT_ITEMS][64];
   int m_rarity[MAX_LOOT_ITEMS];
   float m_value[MAX_LOOT_ITEMS];
} sLootInfo;

// LootInfo description property
F_DECLARE_INTERFACE(ILootInfoProperty);

#undef INTERFACE
#define INTERFACE ILootInfoProperty

DECLARE_PROPERTY_INTERFACE(ILootInfoProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sLootInfo*);  // Type-specific accessors, by reference
};
#define PROP_LOOT_NAME  "LootInfo"
EXTERN ILootInfoProperty *gPropLoot;

#define PROP_GUARANTEEDLOOT_NAME "GuarLoot"
EXTERN ILabelProperty* gPropGuaranteedLoot;

#define PROP_REALLYGUARANTEEDLOOT_NAME "RGuarLoot"
EXTERN ILabelProperty* gPropReallyGuaranteedLoot;

EXTERN ILabelProperty* gPropMapObjIcon;
EXTERN IBoolProperty* gPropMapObjRotate;
EXTERN IStringProperty* gPropMapText;

EXTERN IIntProperty* gPropMiniGames;

#define PROP_TRANSLUCE_RATE "TransRate"

// hacking properties
// for difficulty level and current amount hacked
typedef struct sMapRef
{
   int m_x;
   int m_y;
   uint m_frame;
} sMapRef;

// MapRef description property
F_DECLARE_INTERFACE(IMapRefProperty);

#undef INTERFACE
#define INTERFACE IMapRefProperty

DECLARE_PROPERTY_INTERFACE(IMapRefProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sMapRef*);  // Type-specific accessors, by reference
};

EXTERN IMapRefProperty* gPropMapRef;

////////////////////////////////////////////

EXTERN void DPCPropertiesInit(void);
EXTERN void DPCPropertiesShutdown(void);
#endif 
