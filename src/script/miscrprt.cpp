/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/miscrprt.cpp,v 1.1 1998/10/21 23:41:26 dc Exp $
// Misc report functions

#ifdef REPORT

#include <mprintf.h>
#include <comtools.h>
#include <appagg.h>

#include <report.h>
#include <reprthlp.h>
#include <miscrprt.h>

#include <objedit.h>
#include <objpos.h>

#include <ambbase.h>
#include <ambprop.h>
#include <ambient_.h>

#include <dbmem.h>

////////////////////////
// woo

static void _ambobjs_report(int WarnLevel, void *data, char *buffer)
{
   BOOL loud=(WarnLevel>=kReportLevel_DumpAll);
   char *p=buffer;
   ObjID obj;
   int i, cnt=0, bad_cnt=0;

   ReportHelpersStartUse();
   rsprintf(&p,"\nAmbient Objects Report\n");
   for (i=0; i<ambMax(); i++)
   {
      obj=ambObjID(i);
      if (obj!=OBJ_NULL)
      {
         char room_name[64];
         AmbientSound *state=ambState(i);
         BOOL inroom=ReportHelperInRoom(obj,NULL,room_name);
         if (!inroom)
         {
            rsprintf(&p,"ERROR: %s not in room DB\n",ObjWarnName(obj));
            bad_cnt++;
         }
         if (loud)
         {
            if (state->flags&AMBFLG_S_ENVIRON)
               rsprintf(&p,"%s an Env with %s %s %s\n",ObjWarnName(obj),
                        state->schema_name.text,state->aux_schema_1.text,state->aux_schema_1.text);
            else
               rsprintf(&p,"%s an Obj rad %d with %s\n",ObjWarnName(obj),
                        state->rad,state->schema_name.text);
         }
         cnt++;
      }
   }
   rsprintf(&p,"Have %d ambient points of which %d may be bad\n",cnt,bad_cnt);
   ReportHelpersDoneWith();
}

////////////////////////
// init/term

void MiscReportFuncsInit(void)
{
#if 0   
   ReportRegisterObjCallback(_ambobjs_report,"AI Guys",NULL);
#endif
   ReportRegisterGenCallback(_ambobjs_report,kReportAllObj,"AmbObjs",NULL);
}

void MiscReportFuncsTerm(void)
{
#if 0   
   ReportUnRegisterObjCallback(_ambobjs_report,NULL);
#endif
   ReportUnRegisterGenCallback(_ambobjs_report,kReportAllObj,NULL);
}

#endif

