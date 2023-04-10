/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/skilltab.cpp,v 1.28 1998/02/23 14:07:51 kate Exp $
//
// XXX how to deal with alignment issues?  Guess I should pad structures
// (kNumLocoValues is currently derived from the size of a structure that really should
// get padded and isn't).
//
// XXX There's tons of junk pushed into cMSkill that isn't really motion-system
// related at all (player stuff).  This should get sorted out when this 
// starts using the object system.
//
// XXX There are also lots of player skill editing functions in here.
// The hope is to get this kind of editing done through dromed.
// The player cerebellum gets its skill information straight from here
// each time, which is slow since this is a naive implementation.  It may
// want to store off its own data like the motion system skill set does, but 
// again I want to hold off until I figure out what's going on for real. 
//                                                          KJ 12/97
//
// HOW TO ADD SKILLS OF AN EXISITING MOTION CATEGORY
//
// Go to the static array where that category's data is defined, and add a new 
// entry.
// Go to g_aSkills and add a line
//    cSkill( <skill name> , <skill category>, <offset of skill in category static array> ),

// HOW TO CREATE A NEW MOTION CATEGORY
//
// Add the motion category to the eMotionCategory enum in mclntapi.h
// Create a class for the new skill thats a subclass of cMSkill
// Create a factory method for your new class, and add it to the factory
// methods array, at the appropriate index for your category.
// Add a case to cSkillTable::GetSelectDescBasics to return info about
// skill selection for your category.
// Make sure your class sets m_KeyDesc fields if you want it to have skill
// selection criteria (otherwise can only have one skill for that category). 
//  - locomotion is a good example of this.


#include <mskilset.h>
//#include <msklhack.h>
#include <motdesc.h>
#include <mskltree.h>
#include <skilltab.h>
#include <mseltype.h>
#include <fix.h>
#include <skildtab.h>
#include <command.h>
#include <config.h>

// must be last header
#include <dbmem.h>


EXTERN cManeuverFactory *g_pTeleportManeuverFactory; // from mvrtport.cpp
EXTERN cManeuverFactory *g_pSingleManeuverFactory; // from mvrsngle.cpp
EXTERN cManeuverFactory *g_pGlideManeuverFactory; // from mvrglide.cpp
EXTERN cManeuverFactory *g_pGroundLocoManeuverFactory; // from mvrgloco.cpp
EXTERN cManeuverFactory *g_pPlayerStandardManeuverFactory; // from mvrplyr.cpp
EXTERN cManeuverFactory *g_pFlexBowManeuverFactory; // from mvrflbow.cpp
EXTERN cManeuverFactory *g_pCombatManeuverFactory; // from mvrcmbat.cpp

EXTERN cManeuverFactory *g_aManeuverFactory[] = {\
   g_pTeleportManeuverFactory,
   g_pSingleManeuverFactory,
   g_pGlideManeuverFactory,
   g_pGroundLocoManeuverFactory,
   g_pPlayerStandardManeuverFactory,
   g_pFlexBowManeuverFactory,
   g_pCombatManeuverFactory,
};   


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// DEFAULT MOTION DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

class cMDefaultSkill : public cMSkill
{
public:
   cMDefaultSkill(const char *name, const int controllerID) \
      { 
         m_Name=(char *)name; m_ControllerID=controllerID; m_Category=kMCat_Default; 
         m_KeyDesc.nValues=0; m_KeyDesc.pValues=NULL;
      }
};


class cMSkillFactory
{
public:
   // default implementation
   virtual cMSkill *Create(const char *name, const int offset) \
      {
         if(g_aDefaultDesc[offset].controllerID==kControllerInvalid)
            return NULL;
         return new cMDefaultSkill(name, g_aDefaultDesc[offset].controllerID);
      }
   virtual void BeginEdit(const char *name, const int offset) {}
   virtual void EndEdit(const char *name, const int offset) {}
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// SINGLEPLAY DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

class cMSinglePlaySkill : public cMSkill
{
public:
   cMSinglePlaySkill(const char *name, const int controllerID) \
      { 
         m_Name=(char *)name; m_ControllerID=controllerID; m_Category=kMCat_SinglePlay; 
         m_KeyDesc.nValues=0; m_KeyDesc.pValues=NULL;
      }
};


class cMSinglePlaySkillFactory : public cMSkillFactory
{
public:
   // default implementation
   virtual cMSkill *Create(const char *name, const int offset) \
      {
         if(g_aDefaultDesc[offset].controllerID==kControllerInvalid)
            return NULL;
         return new cMSinglePlaySkill(name, g_aSinglePlayDesc[offset].controllerID);
      }
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// COMBAT DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

class cMCombatSkill : public cMSkill
{
public:
   cMCombatSkill(const char *name, const int controllerID) \
      { 
         m_Name=(char *)name; m_ControllerID=controllerID; m_Category=kMCat_Combat; 
         m_KeyDesc.nValues=0; m_KeyDesc.pValues=NULL;
      }
};

sLocoFullDesc *g_pCombatEditDesc=NULL;

class cMCombatSkillFactory : public cMSkillFactory
{
public:
   // default implementation
   virtual cMSkill *Create(const char *name, const int offset) \
      {
         if(g_aDefaultDesc[offset].controllerID==kControllerInvalid)
            return NULL;
         return new cMCombatSkill(name, g_aCombatDesc[offset].controllerID);
      }
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// LOCOMOTION DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

const int kNumLocoValues=sizeof(sMLocoSelectDesc)/sizeof(uchar);

class cMLocomoteSkill : public cMSkill
{
public:
   cMLocomoteSkill(char *name, int controllerID, sMLocoSelectDesc *selectDesc, sMGaitSkillData *skillData) \
      {
         m_Name=name; m_ControllerID=controllerID; m_Category=kMCat_Locomote; 
         m_SelectDesc=*selectDesc; m_SkillData=*skillData;
         m_KeyDesc.nValues=kNumLocoValues; m_KeyDesc.pValues=m_SelectDesc.pValues;
      }


   ~cMLocomoteSkill() {}

   virtual sMGaitSkillData *GetGaitSkillData() { return &m_SkillData; }

private:
   sMGaitSkillData m_SkillData; // data to be used by maneuver for carring out gait
   // data to be used in gait selection
   sMLocoSelectDesc m_SelectDesc;

};

sLocoFullDesc *g_pLocoEditDesc=NULL;

class cMLocoSkillFactory : public cMSkillFactory
{
public:
   virtual cMSkill *Create(const char *name, const int offset) \
      {   sLocoFullDesc *pLDesc=&g_aLocoDesc[offset]; \
         if(g_aDefaultDesc[offset].controllerID==kControllerInvalid)
            return NULL;
          return new cMLocomoteSkill((char *)name,pLDesc->controllerID,&pLDesc->selDesc,&pLDesc->skillData);
      }


   virtual void BeginEdit(const char *name, const int offset) \
      { g_pLocoEditDesc=&g_aLocoDesc[offset];}

   virtual void EndEdit(const char *name, const int offset) { g_pLocoEditDesc=NULL; }

};


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// PLAYER SKILLS DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

const int kNumPlayerSkillValues=0; // don't go through skill selection tree that AIs do

#ifndef SHIP
EXTERN BOOL g_UseGlobalOffset;
EXTERN mxs_vector g_ArmRelPos;
EXTERN mxs_angvec g_ArmRelAng;
#endif

class cMPlayerSkill : public cMSkill
{
public:
   cMPlayerSkill(char *name, sPlayerSkillFullDesc *pDesc)
      {
         if(!pDesc)
            return;
         m_Name=name; m_Category=kMCat_Player; 
         m_ControllerID=pDesc->controllerID; 
         m_Connect=pDesc->connect; m_ModelName=pDesc->modelName; m_PlayerSkillData=pDesc->skillData;
         m_TimeToMax=pDesc->timeToMax; m_Shoot=pDesc->shoot;
         m_Transition=pDesc->trans;
         m_SchemaName=pDesc->schemaName;
         m_MouseSpeed=pDesc->mouseSpeed;
         m_MouseZone=pDesc->mouseZone;
         m_SlewSpeedScale=pDesc->slewSpeedScale;
         m_RotateSpeedScale=pDesc->rotateSpeedScale;
         m_KeyDesc.nValues=0; m_KeyDesc.pValues=NULL;
         m_IsIdleMotion=pDesc->IsIdleMotion;
      }
   ~cMPlayerSkill() {}

   virtual sSkillConnectDesc *GetSkillConnectData() { return &m_Connect; }
   virtual char *GetModelName() { return m_ModelName; }
   virtual char *GetSchemaName() { return m_SchemaName; }
   virtual ePlayerTransition GetTransitionType() { return m_Transition; }

   // NOTE: this modifies local data if g_UseGlobalOffset is set, and changes
   // remain even if g_UseGlobalOffset gets unset.
   virtual sMPlayerSkillData *GetPlayerSkillData() { 
#ifndef SHIP
      if(g_UseGlobalOffset)
      {
         mx_copy_vec(&m_PlayerSkillData.startPosOff,&g_ArmRelPos);
         m_PlayerSkillData.startAngOff=g_ArmRelAng;
      }
#endif
      return &m_PlayerSkillData; 
   }

   virtual BOOL Shoot() { return m_Shoot; }
   virtual float GetPowerUpTime() { return m_TimeToMax; }
   virtual int HasMouseParams() { return (m_MouseSpeed!=0||m_MouseZone!=0); }
   virtual int GetMouseSpeed() { return m_MouseSpeed; }
   virtual int GetMouseZone() { return m_MouseZone; }
   virtual BOOL HasSpeedScale() { return (m_SlewSpeedScale!=0||m_RotateSpeedScale!=0); }
   virtual float GetSlewSpeedScale() { return (m_SlewSpeedScale==0)?1.0:m_SlewSpeedScale; }
   virtual float GetRotateSpeedScale() { return (m_RotateSpeedScale==0)?1.0:m_RotateSpeedScale; }
   virtual BOOL IsIdleMotion() { return m_IsIdleMotion; }

private:
   sSkillConnectDesc m_Connect;
   char *m_ModelName;
   char *m_SchemaName;
   sMPlayerSkillData m_PlayerSkillData;
   float m_TimeToMax;
   BOOL m_Shoot;
   ePlayerTransition m_Transition;
   int m_MouseSpeed;
   int m_MouseZone;
   float m_SlewSpeedScale;
   float m_RotateSpeedScale;
   BOOL  m_IsIdleMotion;
};

sPlayerSkillFullDesc *g_pPlayerEditDesc=NULL;


class cMPlayerSkillFactory : public cMSkillFactory
{
public:
   virtual cMSkill *Create(const char *name, const int offset) \
      {   sPlayerSkillFullDesc *pDesc=&g_aPlayerSkillDesc[offset]; \
         if(g_aDefaultDesc[offset].controllerID==kControllerInvalid)
            return NULL;

          return new cMPlayerSkill((char *)name,pDesc);
      }

   virtual void BeginEdit(const char *name, const int offset) \
      { g_pPlayerEditDesc=&g_aPlayerSkillDesc[offset];}
   virtual void EndEdit(const char *name, const int offset) { g_pPlayerEditDesc=NULL; }

};

////////////////////////////////////////////////
////////////////////////////////////////////////
//
// DEBUGGING STUFF FOR PLAYER
//
////////////////////////////////////////////////
////////////////////////////////////////////////

// global editing routines for player skills

// ??   char *modelName; // XXX how to deal with this?

void PlayerEditSkillSetController(int controllerID)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->controllerID=controllerID;
   }
}

void PlayerEditSkillSetPowerupTime(float time)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->timeToMax=time;
      if(g_pPlayerEditDesc->skillData.flexDuration)
         g_pPlayerEditDesc->skillData.flexDuration=time;
   }
}

void PlayerEditSkillSetMaxFlex(int degrees)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.flexDegree=degrees;
   }
}

void PlayerEditSkillSetDuration(float time)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.duration=time;
   }
}

void PlayerEditSkillSetTimeScale(float time)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.timeScale=time;
   }
}

void PlayerEditSkillSetMouseSpeed(int speed)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->mouseSpeed=speed;
   }
}

void PlayerEditSkillSetMouseZone(int zone)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->mouseZone=zone;
   }
}

void PlayerEditSkillSetSlewSpeedScale(float scale)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->slewSpeedScale=scale;
   }
}

void PlayerEditSkillSetRotateSpeedScale(float scale)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->rotateSpeedScale=scale;
   }
}

void PlayerEditSkillSetStartPosOffset_X(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startPosOff.x=val;
   }
}

void PlayerEditSkillSetStartPosOffset_Y(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startPosOff.y=val;
   }
}

void PlayerEditSkillSetStartPosOffset_Z(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startPosOff.z=val;
   }
}

void PlayerEditSkillSetEndPosOffset_X(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endPosOff.x=val;
   }
}

void PlayerEditSkillSetEndPosOffset_Y(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endPosOff.y=val;
   }
}

void PlayerEditSkillSetEndPosOffset_Z(float val)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endPosOff.z=val;
   }
}

void PlayerEditSkillSetStartAngOffset_Vec(mxs_angvec *pVec)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startAngOff=*pVec;
   }
}

void PlayerEditSkillSetEndAngOffset_Vec(mxs_angvec *pVec)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endAngOff=*pVec;
   }
}

void PlayerEditSkillSetStartAngOffset_tX(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startAngOff.tx=degrees_to_fixang(angle);
   }
}

void PlayerEditSkillSetStartAngOffset_tY(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startAngOff.ty=degrees_to_fixang(angle);
   }
}

void PlayerEditSkillSetStartAngOffset_tZ(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.startAngOff.tz=degrees_to_fixang(angle);
   }
}

void PlayerEditSkillSetEndAngOffset_tX(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endAngOff.tx=degrees_to_fixang(angle);
   }
}

void PlayerEditSkillSetEndAngOffset_tY(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endAngOff.ty=degrees_to_fixang(angle);
   }
}

void PlayerEditSkillSetEndAngOffset_tZ(int angle)
{
   if(g_pPlayerEditDesc)
   {
      g_pPlayerEditDesc->skillData.endAngOff.tz=degrees_to_fixang(angle);
   }
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//
// TWEAKING STUFF FOR LOCOMOTIONS
//
////////////////////////////////////////////////
////////////////////////////////////////////////

void LocoWalkTimeWarp(float warp)
{
   PlayerSetEditSkill("Walk");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();
}

void LocoWalkStretch(float stretch)
{
   PlayerSetEditSkill("Walk");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.stretch=stretch;
   }
   PlayerFinishEdit();
}

void LocoRunTimeWarp(float warp)
{
   PlayerSetEditSkill("Run");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();
}

void LocoRunStretch(float stretch)
{
   PlayerSetEditSkill("Run");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.stretch=stretch;
   }
   PlayerFinishEdit();
}

void LocoSearchTimeWarp(float warp)
{
   PlayerSetEditSkill("WalkSearching");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();
}

void LocoSearchStretch(float stretch)
{
   PlayerSetEditSkill("WalkSearching");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.stretch=stretch;
   }
   PlayerFinishEdit();
}

void LocoCombatTimeWarp(float warp)
{
   PlayerSetEditSkill("CombatAdvance");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();

   PlayerSetEditSkill("CombatBackup");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();

   PlayerSetEditSkill("CombatSSLeft");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();

   PlayerSetEditSkill("CombatSSRight");
   if(g_pLocoEditDesc)
   {
      g_pLocoEditDesc->skillData.timeWarp=warp;
   }
   PlayerFinishEdit();
}

EXTERN float g_CombatTimeWarp;
EXTERN float g_CombatStretch;

void CombatSwingTimeWarp(float warp)
{
   g_CombatTimeWarp=warp;
}

void CombatSwingStretch(float stretch)
{
   g_CombatStretch=stretch;
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// SKILL CREATION METHODS
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

cMSkillFactory g_DefaultFactory;
cMLocoSkillFactory g_LocoFactory;
cMSinglePlaySkillFactory g_SinglePlayFactory;
cMCombatSkillFactory g_CombatFactory;
cMPlayerSkillFactory g_PlayerSkillFactory;

cMSkillFactory *g_MSkillFactory[] = { &g_DefaultFactory, &g_LocoFactory, &g_SinglePlayFactory, &g_PlayerSkillFactory, &g_CombatFactory };

class cSkill
{
public:
   cSkill(const char *name, const eMotionCategory category, const int offset) \
      { m_Name=(char *)name; m_Offset=offset;
        m_pMotionSkill=g_MSkillFactory[category]->Create(name,offset);
        m_Category=category; }

   ~cSkill() { if(m_pMotionSkill) delete m_pMotionSkill; }

   cMSkill *GetMotionSkill() { return m_pMotionSkill; }

   // skill modification stuff
   void BeginEdit() 
      { g_MSkillFactory[m_Category]->BeginEdit(m_Name,m_Offset); }

   void EndEdit()
      { g_MSkillFactory[m_Category]->EndEdit(m_Name,m_Offset);
        ResetMotionSkill();
      }

   char    *GetName() { return m_Name; }
private:
   cMSkill *ResetMotionSkill() { 
      if(m_pMotionSkill)
         delete m_pMotionSkill;
      m_pMotionSkill=g_MSkillFactory[m_Category]->Create(m_Name,m_Offset); 
      return m_pMotionSkill; }

   char *m_Name;
   int m_Offset;
   cMSkill *m_pMotionSkill;
   eMotionCategory m_Category;
};

cSkill g_aSkill[] = {
// locomotion skills
   cSkill("Walk", kMCat_Locomote,0),
   cSkill("StandNormal", kMCat_Locomote,1),
   cSkill("StandSearching", kMCat_Locomote,2),
   cSkill("StandCombat", kMCat_Locomote,3),
   cSkill("WalkSearching", kMCat_Locomote,4),
   cSkill("Run", kMCat_Locomote,5),
   cSkill("CombatAdvance", kMCat_Locomote,6),
   cSkill("CombatBackup", kMCat_Locomote,7),
   cSkill("CombatSSLeft", kMCat_Locomote,8),
   cSkill("CombatSSRight", kMCat_Locomote,9),
// single play skills
   cSkill("SinglePlay", kMCat_SinglePlay,0),
// combat skills
   cSkill("SwordSwing", kMCat_Combat,0),
// player skills
   // sword
   cSkill("PlayerSwordReady", kMCat_Player, 0),
   cSkill("PlayerSwordBlock", kMCat_Player, 1),
   cSkill("PlayerSwordAtBlock", kMCat_Player, 2),
   cSkill("PlayerSwordWindShort", kMCat_Player, 3),
   cSkill("PlayerSwordAtWoundShort", kMCat_Player, 4),
   cSkill("PlayerSwordSwingShort", kMCat_Player, 5),
   cSkill("PlayerSwordWindMedium", kMCat_Player, 6),
   cSkill("PlayerSwordAtWoundMedium", kMCat_Player, 7),
   cSkill("PlayerSwordSwingMedium", kMCat_Player, 8),
   cSkill("PlayerSwordWindLong", kMCat_Player, 9),
   cSkill("PlayerSwordAtWoundLong", kMCat_Player, 10),
   cSkill("PlayerSwordSwingLong", kMCat_Player, 11),
   cSkill("PlayerSwordBegin", kMCat_Player, 12),
   cSkill("PlayerSwordEnd", kMCat_Player, 13),
   // empty handed
   cSkill("PlayerEmptyIdle", kMCat_Player, 14),
   // lock pick
   cSkill("PlayerLockPickIdle", kMCat_Player, 15),
   cSkill("PlayerLockPickRaise", kMCat_Player, 16),
   cSkill("PlayerLockPickFrob", kMCat_Player, 17),
   cSkill("PlayerLockPickLower", kMCat_Player, 18),
   // flexibow
   cSkill("FlexBowIdle", kMCat_Player, 19),
   cSkill("FlexBowAim", kMCat_Player, 20),
   cSkill("FlexBowDraw", kMCat_Player, 21),
   cSkill("FlexBowFire", kMCat_Player, 22),
   cSkill("FlexBowRelax", kMCat_Player, 23),
   cSkill("FlexBowBegin", kMCat_Player, 24),
   cSkill("FlexBowEnd", kMCat_Player, 25),
};

cSkillTable g_SkillTable;
EXTERN cSkillTable *g_pSkillTable=&g_SkillTable;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////
//// SKILL TABLE CLASS IMPLEMENTATION
////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#ifndef SHIP

EXTERN void PlayerBowAlignArrow(); // from weapbow.cpp

Command SkillCommands[] =
{
   { "loco_walk_timewarp", FUNC_FLOAT, LocoWalkTimeWarp },
   { "loco_walk_stretch", FUNC_FLOAT, LocoWalkStretch },
   { "loco_run_timewarp", FUNC_FLOAT, LocoRunTimeWarp },
   { "loco_run_stretch", FUNC_FLOAT, LocoRunStretch },
   { "loco_search_timewarp", FUNC_FLOAT, LocoSearchTimeWarp },
   { "loco_search_stretch", FUNC_FLOAT, LocoSearchStretch },
   { "loco_combat_timewarp", FUNC_FLOAT, LocoCombatTimeWarp },
   { "combat_swing_timewarp", FUNC_FLOAT, CombatSwingTimeWarp },
   { "combat_swing_stretch", FUNC_FLOAT, CombatSwingStretch },
};

#define register_skill_commands() COMMANDS(SkillCommands,HK_GAME_MODE)

#else

#define register_skill_commands()

#endif

cSkillTable::cSkillTable()
{
   m_pSkills=g_aSkill;
   m_nSkills=sizeof(g_aSkill)/sizeof(g_aSkill[0]);
   m_pEditSkill=NULL;
}

cSkillTable::~cSkillTable()
{
}

BOOL cSkillTable::GetSelectDescBasics(eMotionCategory category, int *pNumValues, int *pMaxEntriesPerValue)
{
   switch(category)
   {
      case kMCat_Locomote:
         *pNumValues=kNumLocoValues;
         *pMaxEntriesPerValue=15;
         break;
      case kMCat_SinglePlay:
         *pNumValues=0; // no skill selection criteria, since only ever one skill
         *pMaxEntriesPerValue=15;
         break;
      case kMCat_Combat:
         *pNumValues=0; // no skill selection criteria, since only ever one skill
         *pMaxEntriesPerValue=15;
         break;
      case kMCat_Player: // this should never use keydesc
         *pNumValues=0;
         *pMaxEntriesPerValue=0;
         break;
      default:
         *pNumValues=0;
         *pMaxEntriesPerValue=15;
   }
   return TRUE;
}
   
// XXX TO DO: turn this into string hash table 
cMSkill *cSkillTable::GetMotionSkill(const tSkillID skill)
{
   int i;
   cSkill *pSkill;
 
   if(!skill)
      return NULL;
   pSkill=m_pSkills;
   for(i=0;i<m_nSkills;i++,pSkill++)
   {
      if(!strcmp(pSkill->GetName(),(char *)skill))
         return pSkill->GetMotionSkill();
   }
   return NULL;
}

// to allow designers to edit skills
void cSkillTable::SetEditSkill(const tSkillID skill)
{
   int i;
   cSkill *pSkill;
 
   if(!skill)
      return;
   pSkill=m_pSkills;
   for(i=0;i<m_nSkills;i++,pSkill++)
   {
      if(!strcmp(pSkill->GetName(),(char *)skill))
      {
         m_pEditSkill=pSkill;
         pSkill->BeginEdit();
      }
   }
}

void cSkillTable::FinishEdit()
{
   if(m_pEditSkill)
      m_pEditSkill->EndEdit();
   m_pEditSkill=NULL;
}

// XXX should really move these to cerebellum, but I'm being super-duper lame
// until I work out what the real skill table should be
void PlayerSetEditSkill(char *name)
{
   g_pSkillTable->SetEditSkill(name);
}

void PlayerFinishEdit()
{
   g_pSkillTable->FinishEdit();
}

void SkillTableInit()
{
   float val;

   register_skill_commands(); 
   // check for config variables
   if(config_get_float("loco_walk_timewarp",&val))
      LocoWalkTimeWarp(val);
   if(config_get_float("loco_walk_stretch",&val))
      LocoWalkStretch(val);
   if(config_get_float("loco_run_timewarp",&val))
      LocoRunTimeWarp(val);
   if(config_get_float("loco_run_stretch",&val))
      LocoRunStretch(val);
   if(config_get_float("loco_search_timewarp",&val))
      LocoSearchTimeWarp(val);
   if(config_get_float("loco_search_stretch",&val))
      LocoSearchStretch(val);
   if(config_get_float("loco_combat_timewarp",&val))
      LocoCombatTimeWarp(val);
   if(config_get_float("combat_swing_timewarp",&val))
      CombatSwingTimeWarp(val);
   if(config_get_float("combat_swing_stretch",&val))
      CombatSwingStretch(val);
}
