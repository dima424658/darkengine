/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/skilltab.h,v 1.11 2000/01/31 09:50:01 adurant Exp $
#pragma once

#ifndef __SKILLTAB_H
#define __SKILLTAB_H

#include <skilltyp.h>

typedef struct sMLocoSelectDesc
{
   union {
      uchar pValues[5];
      struct {   
         uchar media;
         uchar facing;
         uchar speed;
         uchar kind;
         uchar phys;
      };   
   };
} sMLocoSelectDesc;

// this is used instead of skill selection desc for player, since 
// want player to always execute a pre-defined set of maneuvers when
// he start/ends/aborts using an item.  (Aborting using an item is what
// happens when player wants to switch items.)
// We don't need to match skills to best fit arbitrary control inputs.
typedef struct sSkillConnectDesc
{
   tSkillID nextDefault; // skill to perform next if no inputs received
   tSkillID nextIfStart; // skill to perform next if start action input received
   tSkillID nextIfFinish;// skill to perform next if finish action input received
   tSkillID nextIfAbort; // skill to perform next if abort action input received
} sSkillConnectDesc;

typedef enum ePlayerTransition
{
   kTrans_Immediate,
   kTrans_AtEnd,
   kTrans_Invalid=0xff,
} ePlayerTransition;

typedef struct sMPlayerSkillData
{
   char *motionName; // use name explicitly, since I don't think we want
                     // multi-mo-caps/descriptor flag stuff for player
   mxs_vector startPosOff;
   mxs_angvec startAngOff;
   mxs_vector endPosOff;
   mxs_angvec endAngOff;
   float      duration;   // for specifying fixed duration
   float      timeScale;  // for stretching motion captures proportionally
   float      flexDuration; // time to max flex
   int        flexDegree; // maximum flex, in degrees
} sMPlayerSkillData;



#ifdef __cplusplus ///////////////


#include <mskltype.h>
#include <mmanuver.h>
#include <mskltree.h>



typedef class cMSkill cMSkill;

EXTERN cManeuverFactory *g_aManeuverFactory[];

class cMSkill
{
public:
//   cMSkill(char *name, int controllerID);

   virtual ~cMSkill() {}

   virtual const sMSkillKeyDesc *GetKeyDesc() const { return &m_KeyDesc; }

   eMotionCategory GetCategory() const { return m_Category; }
//   int GetControllerID() const { return m_ControllerID; }
   cManeuverFactory *GetManeuverFactory() const { return g_aManeuverFactory[m_ControllerID]; }

   virtual char *GetName() { return m_Name; }

   virtual sMGaitSkillData *GetGaitSkillData() { return NULL; }

   // data that player maneuvers use to execute
   virtual sMPlayerSkillData *GetPlayerSkillData() { return NULL; }


   // XXX these really do not belong in cMSkill!
   // relevent for player skills only currently
   virtual sSkillConnectDesc *GetSkillConnectData() { return NULL; }
   virtual char *GetModelName() { return NULL; }
   virtual char *GetSchemaName() { return NULL; }
   virtual BOOL Shoot() { return FALSE; }
   virtual float GetPowerUpTime() { return 0; }
   virtual ePlayerTransition GetTransitionType() { return kTrans_Invalid; }
   virtual BOOL HasMouseParams() { return FALSE; }
   virtual int GetMouseSpeed() { return -1; }
   virtual int GetMouseZone() { return -1; }
   virtual BOOL HasSpeedScale() { return FALSE; }
   virtual float GetSlewSpeedScale() { return 1.0; }
   virtual float GetRotateSpeedScale() { return 1.0; }
   virtual BOOL IsIdleMotion() { return FALSE; }

protected:
   char *m_Name;
   eMotionCategory m_Category;
   int m_ControllerID;
   sMSkillKeyDesc m_KeyDesc;
};

class cSkillTable
{
public:
//
// Constructor/Destructor
//
   cSkillTable();
   ~cSkillTable();

   cMSkill *GetMotionSkill(const tSkillID skill);
   void SetEditSkill(const tSkillID skill);
   void FinishEdit();

   BOOL GetSelectDescBasics(eMotionCategory category, int *pNumValues, int *pMaxEntriesPerValue);

private:
   cSkill *m_pSkills;
   int m_nSkills;
   cSkill *m_pEditSkill;

// identifier -> handle hash table
// arrays of added skills
};

EXTERN cSkillTable *g_pSkillTable;

#endif /* for ifdef __cplusplus */

EXTERN void SkillTableInit();


#ifndef SHIP
/* editing funcs */

EXTERN void PlayerSetEditSkill(char *name);
EXTERN void PlayerFinishEdit();
EXTERN void PlayerEditSkillSetController(int controllerID);
EXTERN void PlayerEditSkillSetPowerupTime(float time);
EXTERN void PlayerEditSkillSetMaxFlex(int degrees);
EXTERN void PlayerEditSkillSetDuration(float time);
EXTERN void PlayerEditSkillSetTimeScale(float time);
EXTERN void PlayerEditSkillSetMouseSpeed(int speed);
EXTERN void PlayerEditSkillSetMouseZone(int zone);
EXTERN void PlayerEditSkillSetSlewSpeedScale(float scale);
EXTERN void PlayerEditSkillSetRotateSpeedScale(float scale);

EXTERN void PlayerEditSkillSetStartAngOffset_Vec(mxs_angvec *pVec);
EXTERN void PlayerEditSkillSetEndAngOffset_Vec(mxs_angvec *pVec);
EXTERN void PlayerEditSkillSetStartPosOffset_X(float val);
EXTERN void PlayerEditSkillSetStartPosOffset_Y(float val);
EXTERN void PlayerEditSkillSetStartPosOffset_Z(float val);
EXTERN void PlayerEditSkillSetEndPosOffset_X(float val);
EXTERN void PlayerEditSkillSetEndPosOffset_Y(float val);
EXTERN void PlayerEditSkillSetEndPosOffset_Z(float val);
EXTERN void PlayerEditSkillSetStartAngOffset_tX(int angle);
EXTERN void PlayerEditSkillSetStartAngOffset_tY(int angle);
EXTERN void PlayerEditSkillSetStartAngOffset_tZ(int angle);
EXTERN void PlayerEditSkillSetEndAngOffset_tX(int angle);
EXTERN void PlayerEditSkillSetEndAngOffset_tY(int angle);
EXTERN void PlayerEditSkillSetEndAngOffset_tZ(int angle);
#endif

#endif
