/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyablty.cpp,v 1.10 1998/09/29 18:14:49 CCAROLLO Exp $

#include <plyablty.h>
#include <plyablt_.h>
#include <playrobj.h>
#include <plyrmode.h>
#include <creature.h> // @TODO: move invalid to crettype.h
#include <headmove.h>
#include <prjctile.h>
#include <schema.h>
#include <schbase.h>
#include <esnd.h>
#include <weapon.h>

// must be last header
#include <dbmem.h>


// @NOTE: although pDesc gets copied, its skills pointer must remain valid,
// since space for skills isn't copied.  This is done since skilldescs are going
// to be defined statically in code anyway, so why store them twice
cPlayerAbility::cPlayerAbility(const sPlayerAbilityDesc *pDesc)
{
   DefaultInit(pDesc);
}

void cPlayerAbility::DefaultInit(const sPlayerAbilityDesc *pDesc)
{
   m_CurSkillID=kSkillInvalid;
   m_sndSchHandle=SCH_HANDLE_NULL;
   m_TimeToMax=0;
   if(pDesc)
   {
      int i;

      m_Desc=*pDesc;
      // build tag sets from desc
      m_TagSetList.SetSize(m_Desc.nSkills);
      for(i=0;i<m_Desc.nSkills;i++)
      {
         if(m_Desc.pSkills[i].tagNames)
         {
            cTagSet tags(m_Desc.pSkills[i].tagNames);
            m_TagSetList.SetItem(&tags,i);
         } else
         {
            cTagSet tags;
            m_TagSetList.SetItem(&tags,i);
         }
      }
      Assert_(m_TagSetList.Size()==m_Desc.nSkills);
   } else
   {
      m_Desc.modelName.text[0]='\0';
      m_Desc.creatureType=kCreatureTypeInvalid;
      m_Desc.startSkillID=kSkillInvalid;
      m_Desc.nSkills=0;
      m_Desc.pSkills=NULL;
   }
}

cPlayerAbility::~cPlayerAbility()
{
}

void cPlayerAbility::Begin()
{
}

void cPlayerAbility::End()
{
   CleanupSkill(m_CurSkillID);
}

// dippy funcs

static int HasMouseParams(sPlayerSkillFullDesc *pD) 
{ 
   return (pD->mouseSpeed!=0||pD->mouseZone!=0); 
}

static BOOL HasSpeedScale(sPlayerSkillFullDesc *pD) 
{ 
   return (pD->slewSpeedScale!=0||pD->rotateSpeedScale!=0); 
}

static float GetSlewSpeedScale(sPlayerSkillFullDesc *pD) 
{ 
   return (pD->slewSpeedScale==0)?1.0:pD->slewSpeedScale; 
}

static float GetRotateSpeedScale(sPlayerSkillFullDesc *pD) 
{ 
   return (pD->rotateSpeedScale==0)?1.0:pD->rotateSpeedScale; 
}

static void SndSchemaEndCallback(int hSchema, ObjID schemaID, void *pData)
{
   if(hSchema==((cPlayerAbility *)pData)->GetSoundSchemaHandle())
   {
      ((cPlayerAbility *)pData)->SetSoundSchemaHandle(SCH_HANDLE_NULL);
   }
}

BOOL cPlayerAbility::GetSoundSchemaName(int skillID,Label *pName)
{
   if(skillID>=m_Desc.nSkills || skillID<0)
      return FALSE;

   sPlayerSkillFullDesc *pSkill=&m_Desc.pSkills[skillID];

   const char *pSName;
   if(NULL==(pSName=pSkill->sndSchemaName))
      return FALSE;

   strncpy(pName->text,pSName,16);   
   return TRUE;
}

void cPlayerAbility::DefaultSetupSkill(int skillID)
{
   Assert_(skillID<m_Desc.nSkills && skillID>=0);

   m_CurSkillID=skillID;

   if(skillID>=m_Desc.nSkills || skillID<0)
      return;

   sPlayerSkillFullDesc *pSkill=&m_Desc.pSkills[skillID];

   // setup player skill data
   m_SkillData=m_Desc.pSkills[m_CurSkillID].skillData;

   // set mouse and speed controls
   if(HasMouseParams(pSkill))
   {
      headmoveSetMouseParams(pSkill->mouseSpeed,pSkill->mouseZone);
   }
   if(HasSpeedScale(pSkill))
   {
      AddSpeedScale("PlayerAbility", pSkill->slewSpeedScale, pSkill->rotateSpeedScale);
   }

   cTagSet tags;
   GetMotionTags(skillID, &tags);
   tags.Add(cTag("Event", "Motion"));
   sSchemaCallParams callParams;
   
   callParams.flags=SCH_SET_CALLBACK;
   callParams.callback=SndSchemaEndCallback;
   callParams.pData=(void *)this;
   if ((m_sndSchHandle = ESndPlay(&tags, PlayerObject(), GetWeaponObjID(PlayerObject()), &callParams)) == SCH_HANDLE_NULL)
   {
      // play schema
      Label name;
      if(GetSoundSchemaName(skillID,&name))
      {
         m_sndSchHandle=SchemaPlay((Label *)&name, &callParams);
      }
      else
         m_sndSchHandle=SCH_HANDLE_NULL;
   }

   m_TimeToMax=pSkill->timeToMax;
}

void cPlayerAbility::DefaultCleanupSkill(int skillID)
{
   m_CurSkillID=kSkillInvalid;

   if(skillID>=m_Desc.nSkills || skillID<0)
      return;

   sPlayerSkillFullDesc *pSkill=&m_Desc.pSkills[skillID];

   if(HasMouseParams(pSkill))
      headmoveSetMouseParams(-1,-1); // restore default mouse movement
   if(HasSpeedScale(pSkill))
      RemoveSpeedScale("PlayerAbility");

   // stop any previously playing schemas
   if(m_sndSchHandle!=SCH_HANDLE_NULL)
   {
      SchemaPlayHalt(m_sndSchHandle);
      m_sndSchHandle=SCH_HANDLE_NULL;
   }
}

BOOL cPlayerAbility::GetModelName(Label **pName)
{
   if(!strlen(m_Desc.modelName.text))
      return FALSE;
   *pName=&m_Desc.modelName;
   return TRUE;
}

BOOL cPlayerAbility::SetModelName(Label *pName)
{
   if(!pName)
      return FALSE;
   strcpy(m_Desc.modelName.text,pName->text);
   return TRUE;
}

BOOL cPlayerAbility::GetCreatureType(int *pType)
{
   if(m_Desc.creatureType==kCreatureTypeInvalid)
      return FALSE;
    *pType=m_Desc.creatureType;
    return TRUE;
}

int cPlayerAbility::GetStartSkillID()
{
   return m_Desc.startSkillID;
}

// actType gets ignored by default
int cPlayerAbility::DefaultGetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return kSkillInvalid;

   int nextSkill;
   sSkillConnectDesc *pConnect=&m_Desc.pSkills[skillID].connect;

   // @TODO: should just make this a lookup
   switch(input)
   {
      case kPlyrInput_None:
         nextSkill=pConnect->nextDefault;
         break;
      case kPlyrInput_Start:
         nextSkill=pConnect->nextIfStart;
#if 0 // @TODO: get this working again
         if(nextSkill&&actType!=kPlayerActionTypeInvalid)
         {
            // get special start action for type
            // XXX this may need to be more sophisticated - check if you would
            // otherwise be idling or something.  Only really made to work for
            // sword swings and blocks.  
            // only call if startaction would otherwise at least have you
            // doing _something_        HACK HACK HACK  KJ 1/98

            // note this leaves nextSkill unchanged unless there is a start action
            PlayerSkillGetStartActionSkillForMode(g_CurrentPlayerMode,actType,&nextSkill);
         }
#endif
         break;
      case kPlyrInput_Finish:
         nextSkill=pConnect->nextIfFinish;
         break;
      case kPlyrInput_Abort:
         nextSkill=pConnect->nextIfAbort;
         break;
      default:
         nextSkill=kSkillInvalid;
         break;
   };          

   return nextSkill;
}

BOOL cPlayerAbility::IsIdle(int skillID)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return FALSE;
   return m_Desc.pSkills[skillID].IsIdleMotion;
}

ePlayerTransition cPlayerAbility::GetTransitionType(int skillID)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return kTrans_Invalid;
   return m_Desc.pSkills[skillID].trans;
}

BOOL cPlayerAbility::GetMotionTags(int skillID, cTagSet *pTags)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return FALSE;
   *pTags=m_TagSetList[skillID];
   return TRUE;
}

BOOL cPlayerAbility::GetCurControllerID(int *pID)
{
   if(m_CurSkillID==kSkillInvalid)
      return FALSE;
   *pID=m_Desc.pSkills[m_CurSkillID].controllerID;
   return TRUE;
}

sMPlayerSkillData *cPlayerAbility::GetCurSkillData()
{
   if(m_CurSkillID==kSkillInvalid)
      return NULL;
   return &m_SkillData;
}
