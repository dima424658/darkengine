/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aireport.cpp,v 1.6 1998/10/30 00:13:18 dc Exp $
// AI Related report functions

#ifdef REPORT

#include <mprintf.h>
#include <comtools.h>
#include <appagg.h>

#include <report.h>
#include <reprthlp.h>

#include <objedit.h>
#include <objpos.h>
#include <aireport.h>
#include <relation.h>
#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <traitman.h>
#include <traitbas.h>
#include <iobjsys.h>
#include <objquery.h>

#include <osysbase.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <aiapi.h>
#include <aipathdb.h>
#include <aipthloc.h>
#include <port.h>
#include <aiprrngd.h>  // vantage and cover points

#include <dbmem.h>

////////////////////////
// misc helper
static BOOL _is_inworld(ObjID obj)
{
   mxs_vector *pos=ObjPosGetLocVector(obj);
   Location ourloc;
   
   MakeLocationFromVector(&ourloc,pos);
   ComputeCellForLocation(&ourloc);
   if (ourloc.cell==CELL_INVALID)
      if (AIFindClosestCell(*pos,0)==0)
         return FALSE;
   return TRUE;
}

////////////////////////
// report on patrol points

static BOOL _is_trolpt(ObjID obj, IRelation *pPatrolRel)
{
   return (pPatrolRel->AnyLinks(obj,LINKOBJ_WILDCARD)||pPatrolRel->AnyLinks(LINKOBJ_WILDCARD,obj));
}

static void _trolpt_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   static int bad_cnt, total_cnt;
   static IRelation *pPatrolRel;
   char *p=buffer;

   if (WarnLevel<kReportLevel_Info) return;  // nothing to say

   switch (phase)
   {
      case kReportPhase_Init:
         {
            AutoAppIPtr_(LinkManager,pLinkMan);
            pPatrolRel=pLinkMan->GetRelationNamed("AIPatrol");
            bad_cnt=total_cnt=0;
            rsprintf(&p,"\nPatrol Point Report\n");
            ReportHelpersStartUse();            
         }
         break;
      case kReportPhase_Loop:
         if (_is_trolpt(obj,pPatrolRel))
         {
            BOOL bad=!_is_inworld(obj);
            total_cnt++;
            if (bad)
               bad_cnt++;
            if (loud||bad)
               rsprintf(&p,"%s %s%s%s\n",bad?"ERROR:":"",ObjWarnName(obj),RHLocS(obj),bad?" is out of world":"");
         }
         break;
      case kReportPhase_Term:
         rsprintf(&p,"There are %d TrolPt's of which %d may be bad\n",total_cnt,bad_cnt);
         SafeRelease(pPatrolRel);
         ReportHelpersDoneWith();
         break;
   }
}

////////////////////////
// report on all the guys

static void _aiguys_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   static int ai_cnt;
   static ITraitManager *pTraitMan;
   static IObjectSystem *pObjSys;
   char *p=buffer;

   if (WarnLevel<kReportLevel_Info) return;  // nothing to say

   switch (phase)
   {
      case kReportPhase_Init:
         ai_cnt=0;
         rsprintf(&p,"\nAI Guys Report\n");
         ReportHelpersStartUse();
         pTraitMan=AppGetObj(ITraitManager);
         pObjSys=AppGetObj(IObjectSystem);
         break;
      case kReportPhase_Loop:
         if (ObjIsAI(obj))
         {
            ai_cnt++;
            if (loud)
            {
               rsprintf(&p,"AI %s%s\n",ObjEditName(obj),RHLocS(obj));
               cAutoIPtr<IObjectQuery> query = pTraitMan->Query(obj,kTraitQueryDonors); 
               for (; !query->Done(); query->Next())
               {
                  ObjID donor = query->Object(); 
                  if (pTraitMan->IsMetaProperty(donor))
                  {
                     char *s=(char *)ObjEditName(donor);
                     int mp_cnt=0;
                     if (strnicmp(s,"M-",2)==0)
                     {
                        char *r=strchr(s,'(');
                        if (r) *r='\0';
                        rsprintf(&p,"%s%s",mp_cnt==0?"  has metaprops ":", ",s+2);
                        mp_cnt++;
                     }
                     if (mp_cnt)
                        rsprintf(&p,"\n");
                  }
               }
            }
         }
         break;
      case kReportPhase_Term:
         if (ai_cnt)
            rsprintf(&p,"Total of %d AIs\n",ai_cnt);
         else
            rsprintf(&p,"No AIs\n");
         ReportHelpersDoneWith();
         SafeRelease(pTraitMan);
         SafeRelease(pObjSys);
         break;
   }
}

////////////////////////
// report on patrols/links/so on

#if 0
static void _patrols_report(int WarnLevel, void *data, char *buffer)
{

}
#endif

////////////////////////
// report on vantage and other points

// Count vantage, flee, etc points
//  it is a property

static void _combatpts_report(int WarnLevel, void *data, char *buffer)
{
   BOOL loud=(WarnLevel>=kReportLevel_DumpAll);
   sPropertyObjIter iter;
   int pcnt, badcnt;
   char *p=buffer;
   ObjID obj;

   // really want to check for any ranged AIs first
   rsprintf(&p,"\nCombat Control (Vantage, Flee, Cover) Points\n");
   sVantagePtProp *vpVal;  // value, decay_speed
   pcnt=badcnt=0;
   ReportHelpersStartUse();
   if (loud) rsprintf(&p,"Vantage Points\n");
   g_pVantagePtProp->IterStart(&iter);
   while (g_pVantagePtProp->IterNextValue(&iter,&obj,&vpVal))
      if (OBJ_IS_CONCRETE(obj))
      {
         BOOL bad=!_is_inworld(obj);
         pcnt++;
         if (bad)
            badcnt++;
         if (loud||bad)
         {
            rsprintf(&p,"%s %s%s%s",bad?"ERROR:":"",ObjWarnName(obj),RHLocS(obj),bad?" out of world":"");
            if (loud)
               rsprintf(&p,"val %d decay %f",vpVal->value,vpVal->decay_speed);
            rsprintf(&p,"\n");
         }
      }
   g_pVantagePtProp->IterStop(&iter);
   rsprintf(&p,"There are %d vantage points",pcnt);
   if (badcnt)
      rsprintf(&p," of which %d may be bad",badcnt);
   rsprintf(&p,"\n");

   // ideally, would like to check for 2 very close to eachother someday
   pcnt=badcnt=0;
   if (loud) rsprintf(&p,"Flee Points\n");
   IProperty *_pFleeProp=GetPropertyNamed("AI_FleePoint");
   _pFleeProp->IterStart(&iter);
   while (_pFleeProp->IterNext(&iter,&obj))
      if (OBJ_IS_CONCRETE(obj))
      {
         BOOL bad=!_is_inworld(obj);
         pcnt++;
         if (bad)
            badcnt++;
         if (loud||bad)
            rsprintf(&p,"%s %s%s%s\n",bad?"ERROR:":"",ObjWarnName(obj),RHLocS(obj),bad?" out of world":"");
      }
   _pFleeProp->IterStop(&iter);
   rsprintf(&p,"There are %d flee points",pcnt);
   if (badcnt)
      rsprintf(&p," of which %d may be bad",badcnt);
   rsprintf(&p,"\n");
   
   // cover points
   sCoverPtProp *cpVal;  // value, decay_speed, can_duck
   pcnt=badcnt=0;
   if (loud) rsprintf(&p,"Cover Points\n");
   g_pCoverPtProp->IterStart(&iter);
   while (g_pCoverPtProp->IterNextValue(&iter,&obj,&cpVal))
      if (OBJ_IS_CONCRETE(obj))
      {
         BOOL bad=!_is_inworld(obj);
         pcnt++;
         if (bad)
            badcnt++;
         if (loud||bad)
         {
            rsprintf(&p,"%s %s%s%s",bad?"ERROR:":"",ObjWarnName(obj),RHLocS(obj),bad?" out of world":"");
            if (loud)
               rsprintf(&p,"val %d decay %f duck %d",cpVal->value,cpVal->decay_speed,cpVal->can_duck);
            rsprintf(&p,"\n");
         }
      }
   g_pCoverPtProp->IterStop(&iter);
   ReportHelpersDoneWith();   
   rsprintf(&p,"There are %d cover points",pcnt);
   if (badcnt)
      rsprintf(&p," of which %d may be bad",badcnt);
   rsprintf(&p,"\n");
}

////////////////////////
// report on the path DB itself

extern unsigned AIGetPathDBSize(void);

static void _dbsize_report(int WarnLevel, void *data, char *buffer)
{
   int size=AIGetPathDBSize();
   char *p=buffer;
   rsprintf(&p,"\nAI Path DB Size %dk\n",(size+1023)/1024);
}

////////////////////////
// init/term

void AIReportInit(void)
{
#if 0   
   ReportRegisterGenCallback(_patrols_report,kReportAllObj,"AI Patrols",NULL);
   ReportRegisterGenCallback(_pathcells_report,kReportAIPath,"AI PathCells",NULL);
#endif
   ReportRegisterObjCallback(_aiguys_report,"AI Guys",NULL);
   ReportRegisterObjCallback(_trolpt_report,"Patrol points",NULL);
   ReportRegisterGenCallback(_dbsize_report,kReportAIPath,"AI DB Size",NULL);
   ReportRegisterGenCallback(_combatpts_report,kReportAIPath,"Ranged Points",NULL);
   
}

void AIReportTerm(void)
{
#if 0   
   ReportUnRegisterGenCallback(_patrols_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_pathcells_report,kReportAIPath,NULL);
#endif
   ReportUnRegisterObjCallback(_aiguys_report,NULL);
   ReportUnRegisterObjCallback(_trolpt_report,NULL);
   ReportUnRegisterGenCallback(_dbsize_report,kReportAIPath,NULL);
   ReportUnRegisterGenCallback(_combatpts_report,kReportAIPath,NULL);   
}

#endif
