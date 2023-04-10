/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqrep.cpp,v 1.1 1998/08/16 22:53:29 dc Exp $
// tweq system report functions

#ifdef REPORT

#include <report.h>
#include <objedit.h>
#include <tweqexp.h>
#include <tweqrep.h>
#include <tweqbase.h>

#include <property.h>
#include <propbase.h>

#include <dbmem.h>

static BOOL _is_data_all_zero(void *data, int len)
{
   uchar *d=(uchar *)data;
   for (;len>0; len--)
      if (*d++)
         return FALSE;
   return TRUE;
}

static char *_do_analyze_tweq(ObjID obj, void *data, int len, BOOL warn, char *buf, char *name)
{
   if (_is_data_all_zero(data,len))
      sprintf(buf,"ERROR Likely: %s has a zero'ed %s config prop\n",ObjEditName(obj),name);
   else if (warn&&(!OBJ_IS_ABSTRACT(obj)))
      sprintf(buf,"Suspiciously, %s is concrete and has a %s config prop\n",ObjEditName(obj),name);
   else
      return buf;  // nothing fancy
   buf+=strlen(buf);
   return buf;
}

static void _tweq_report(int WarnLevel, void *data, char *buffer)
{  // we are only looking for errors, so we dont care what warn level this is
   sPropertyObjIter iter;
   BOOL warn=WarnLevel>kReportLevel_Errors;
   char *p=buffer;
   ObjID obj;
   
   strcpy(p,"\nTweq Properties:\n"); p+=strlen(p);

   sTweqVectorConfig *pVecCfg;
   tweq_scale_prop->IterStart(&iter);
   while (tweq_scale_prop->IterNextValue(&iter,&obj,&pVecCfg))
      p=_do_analyze_tweq(obj,pVecCfg,sizeof(sTweqVectorConfig),warn,p,"Scale");
   tweq_scale_prop->IterStart(&iter);
   tweq_rotate_prop->IterStart(&iter);
   while (tweq_rotate_prop->IterNextValue(&iter,&obj,&pVecCfg))
      p=_do_analyze_tweq(obj,pVecCfg,sizeof(sTweqVectorConfig),warn,p,"Rotate");
   tweq_rotate_prop->IterStart(&iter);

   sTweqJointsConfig *pJntCfg;
   tweq_joints_prop->IterStart(&iter);
   while (tweq_joints_prop->IterNextValue(&iter,&obj,&pJntCfg))
      p=_do_analyze_tweq(obj,pJntCfg,sizeof(sTweqJointsConfig),warn,p,"Joints");
   tweq_joints_prop->IterStart(&iter);

   sTweqModelsConfig *pMdlCfg;
   tweq_models_prop->IterStart(&iter);
   while (tweq_models_prop->IterNextValue(&iter,&obj,&pMdlCfg))
      p=_do_analyze_tweq(obj,pMdlCfg,sizeof(sTweqModelsConfig),warn,p,"Models");
   tweq_models_prop->IterStart(&iter);

   sTweqSimpleConfig *pSmpCfg;
   tweq_delete_prop->IterStart(&iter);
   while (tweq_delete_prop->IterNextValue(&iter,&obj,&pSmpCfg))
      p=_do_analyze_tweq(obj,pSmpCfg,sizeof(sTweqSimpleConfig),warn,p,"Delete");
   tweq_delete_prop->IterStart(&iter);
   tweq_flicker_prop->IterStart(&iter);
   while (tweq_flicker_prop->IterNextValue(&iter,&obj,&pSmpCfg))
      p=_do_analyze_tweq(obj,pSmpCfg,sizeof(sTweqSimpleConfig),warn,p,"Flicker");
   tweq_flicker_prop->IterStart(&iter);

   sTweqEmitterConfig *pEmtCfg;
   tweq_emitter_prop->IterStart(&iter);
   while (tweq_emitter_prop->IterNextValue(&iter,&obj,&pEmtCfg))
      p=_do_analyze_tweq(obj,pEmtCfg,sizeof(sTweqEmitterConfig),warn,p,"Emitter");
   tweq_emitter_prop->IterStart(&iter);         

   char *emitter_names[]={"ExtraEmitter1","ExtraEmitter2","ExtraEmitter3","ExtraEmitter4"};
   for (int i=0; i<NUM_EXTRA_EMITTERS; i++)
   {
      tweq_extra_emitter_prop[i]->IterStart(&iter);
      while (tweq_extra_emitter_prop[i]->IterNextValue(&iter,&obj,&pEmtCfg))
         p=_do_analyze_tweq(obj,pEmtCfg,sizeof(sTweqEmitterConfig),warn,p,emitter_names[i]);
      tweq_extra_emitter_prop[i]->IterStart(&iter);         
   }

   sTweqLockConfig *pLckCfg;
   tweq_lock_prop->IterStart(&iter);
   while (tweq_lock_prop->IterNextValue(&iter,&obj,&pLckCfg))
      p=_do_analyze_tweq(obj,pEmtCfg,sizeof(sTweqLockConfig),warn,p,"Lock");
   tweq_lock_prop->IterStart(&iter);         
}

// do the Init/Term for the Report stuff for Tweq
void TweqReportInit(void)
{
   ReportRegisterGenCallback(_tweq_report,kReportAllObj,"Tweq System",NULL);
}

void TweqReportTerm(void)
{
   ReportUnRegisterGenCallback(_tweq_report,kReportAllObj,NULL);
}

#endif  // REPORT


