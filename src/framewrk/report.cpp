/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/report.cpp,v 1.17 1999/06/02 16:56:08 XEMU Exp $
// report system

#ifdef REPORT

#include <string.h>
#include <time.h>

#include <lg.h>
#include <appagg.h>
#include <mprintf.h>

#include <objtype.h>
#include <report.h>
#include <status.h>

#include <dlistsim.h>
#include <dlisttem.h>
#include <dynarray.h>

#include <iobjsys.h>
#include <objquery.h>
#include <objedit.h>
#include <objdef.h>
#include <hilight.h>

#ifdef EDITOR
#include <editbr_.h>
#include <brinfo.h>
#include <vbrush.h>    // brush callback iterators
#include <brrend.h>
#include <brquery.h>
#endif

#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>
#include <repscrpt.h>
#include <dbfile.h>

#include <command.h>

#include <dbmem.h>

////////////////////////////////////
// callback management, types, setup

#define kReportFlg_IsObj (kReportPrivate<<0)

struct sReportCallback
{
   char *name;   // what system am i calling myself
   int   flags;  // what is my type
   void *cb;     // the callback itself
   void *data;   // a void * i want returned to me
};

typedef cSimpleDList<sReportCallback *> cCBChain;
static cCBChain callbackChain;

static BOOL _register_cb(void *cb, int flags, const char *name, void *data)
{
   sReportCallback *newCb=new sReportCallback;
   newCb->flags=flags;
   newCb->name=(char *)malloc(strlen(name)+1);
   strcpy(newCb->name,name);
   newCb->cb=cb;
   newCb->data=data;
   callbackChain.Append(newCb);
   return TRUE;
}

static BOOL _unregister_cb(void *cb, int flags, void *data)
{
   cCBChain::cIter cbiter;
   for (cbiter=callbackChain.Iter(); !cbiter.Done(); cbiter.Next())
   {
      sReportCallback *oldCBDat=cbiter.Value();
      if ((oldCBDat->cb==cb)&&(oldCBDat->flags==flags)&&(oldCBDat->data==data))
      {
         callbackChain.Delete(cbiter.Node());
         free(oldCBDat->name);
         free(oldCBDat);
         return TRUE;
      }
   }
   return FALSE;
}

// register a callback into the report system
BOOL ReportRegisterObjCallback(ReportObjCallback cb, const char *name, void *data)
{
   return _register_cb(cb,kReportPerObj|kReportFlg_IsObj,name,data);
}

BOOL ReportRegisterGenCallback(ReportGenCallback cb, eReportType type, const char *name, void *data)
{
   return _register_cb(cb,type,name,data);   
}

// unregister a callback from the report system
//  (is it ok to just use name/cb for identity, or do we need data too?)
BOOL ReportUnRegisterObjCallback(ReportObjCallback cb, void *data)
{
   return _unregister_cb(cb,kReportPerObj|kReportFlg_IsObj,data);
}

BOOL ReportUnRegisterGenCallback(ReportGenCallback cb, eReportType type, void *data)
{
   return _unregister_cb(cb,type,data);
}

////////////////////////////////////
// actually do reports - core functions

static char _ReportFname[255]="";
static BOOL _ReportClear=TRUE;

struct sReportConfig
{
   int   warn;
   int   types;
   int   flags;
   ObjID obj;
   Label fname;
};

static const char *report_warn_names[]=
 { "Errors only", "Warnings too", "Info", "Dump Everything possible" };

static const char *report_type_names[]=
 { "Header", "Per Obj", "All Obj", "WorldDB", "Rooms", "AIPath", "Script", "Debug", "Models", "Game" };

static const char *report_flag_names[]=
 { "HotRegion", "Selection", "Hilight", "AllObj", "Concrete", "Abstract", "ToFile", "ToMono", "ToScreen" };

static sFieldDesc report_config_fields[]=
{
 { "Warning Level", kFieldTypeEnum,   FieldLocation(sReportConfig,warn),  FullFieldNames(report_warn_names) },
 { "Report Types",  kFieldTypeBits,   FieldLocation(sReportConfig,types), FullFieldNames(report_type_names) },
 { "Control Flags", kFieldTypeBits,   FieldLocation(sReportConfig,flags), FullFieldNames(report_flag_names) },
 { "Focus Object",  kFieldTypeInt,    FieldLocation(sReportConfig,obj) },
 { "File Name",     kFieldTypeString, FieldLocation(sReportConfig,fname) },
};

static sStructDesc report_config_desc = StructDescBuild(sReportConfig,kStructFlagNone,report_config_fields);

// current "master" config
static sReportConfig _ReportConfig=
{
  2,
  kReportHeader|kReportAllObj|kReportPerObj|kReportWorldDB|kReportAIPath,
  kReportFlg_AllObjs|kReportFlg_Concrete|kReportFlg_ToFile,
  OBJ_NULL,
  "default.rep"
};

// track the objs that are part of this report
static cDynArray<ObjID> curObjs;

// for searching/sorting the object list
static int _report_obj_search(const void *pKey, const ObjID *item)
{
   if (*(ObjID *)item==*(ObjID *)pKey)
      return 0;
   else if (*(ObjID *)item<*(ObjID *)pKey)
      return -1;
   return 1;
}

static int _report_obj_sort(const ObjID *item1, const ObjID *item2)
{
   if (*(ObjID *)item1<*(ObjID *)item2)
      return -1;
   else if (*(ObjID *)item1>*(ObjID *)item2)
      return 1;
   return 0;
}

// return whether this obj is an active element of current report
BOOL ReportCheckObjActive(ObjID obj)
{
   return (curObjs.BSearch(&obj,_report_obj_search)!=BAD_INDEX);
}

static BOOL add_to_report(editBrush *br)
{
   if (brushGetType(br)==brType_OBJECT)
   {
      curObjs.Append(brObj_ID(br));
      return TRUE;
   }
   return FALSE;
}

// go get the "active" object list
static void _report_get_curObjs(sReportConfig *report)
{
   if (report->obj!=OBJ_NULL)  // just use the focused on object
      curObjs.Append(report->obj);
   else if ((report->flags&kReportFlg_AllObjs)||
            ((report->flags&(kReportFlg_Select|kReportFlg_HotRegion|kReportFlg_Hilight))==0))
   {
      AutoAppIPtr(ObjectSystem);
      eObjConcreteness concrete=kObjectConcrete;
      if (report->flags&kReportFlg_Abstract)
         if (report->flags&kReportFlg_Concrete)
            concrete|=kObjectAbstract;
         else
            concrete =kObjectAbstract;
      IObjectQuery *allObjQuery=pObjectSystem->Iter(concrete);
      while (!allObjQuery->Done())
      {
         curObjs.Append(allObjQuery->Object());
         allObjQuery->Next();
      }
      SafeRelease(allObjQuery);
   }
   else // go gather concrete objs from the world
   {
      if (report->flags&kReportFlg_Select)
      {
         editBrush *br=vBrush_editBrush_Get();
         if (vBrush_inCurGroup(br))
            vBrush_GroupOp(FALSE,(editbr_callback)add_to_report);
         else
            add_to_report(br);
      }
      if (report->flags&kReportFlg_HotRegion)
         brushRunOnActive((editbr_callback)add_to_report);
      if (report->flags&kReportFlg_Hilight)
         brFilter(highlight_check,add_to_report);
   }
   curObjs.Sort(_report_obj_sort);
}

static void _report_clear_curObjs(void)
{
   curObjs.SetSize(0);
}

////////////////////////////////////
// horrible output buffer management lameness

static char *_rpt_buffer=NULL;
static int   _rpt_buff_size=0;

static void _buffer_setup(void)
{
   if (((int)_rpt_buffer)|_rpt_buff_size)
      Warning(("Report buffer already exists in setup %x %d\n",_rpt_buffer,_rpt_buff_size));
   _rpt_buffer=NULL; _rpt_buff_size=0;
}

static void _buffer_clear(void)
{
   if (_rpt_buffer)
      free(_rpt_buffer);
   else
      Warning(("Report buffer non-existant in clear\n"));
   _rpt_buffer=NULL; _rpt_buff_size=0;
}

static void _buffer_add_to(const char *new_txt)
{
   int added_len=strlen(new_txt);
   if (added_len==0) return;
   if (_rpt_buff_size==0)
      _rpt_buffer=(char *)malloc(added_len+1);
   else
      _rpt_buffer=(char *)realloc(_rpt_buffer,_rpt_buff_size+added_len+1);
   strcpy(_rpt_buffer+_rpt_buff_size,new_txt);
   _rpt_buff_size+=added_len;
}

static void _buffer_output(sReportConfig *report)
{
   if (_rpt_buff_size<0)
   {
      Warning(("No Report Generated\n"));
      return;
   }
   if (report->flags&kReportFlg_ToFile)
   {   // if file name, output to file
      char *use_fname=NULL;
      if (_ReportFname[0]!='\0')
         use_fname=_ReportFname;
      else if (report->fname.text[0]!='\0')
         use_fname=report->fname.text;
      else
         Warning(("No filename to write to\n"));
      if (use_fname)
      {
         int fd;
         fd=open(use_fname,O_BINARY|O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
         if (fd==-1)
            Warning(("Couldnt Open %s to write\n",use_fname));
         else
         {
            write(fd,_rpt_buffer,_rpt_buff_size);
            close(fd);
         }
      }
   }
   if (report->flags&kReportFlg_ToScr)
   {  // if "screen display", output to screen
      // Tom should add zany display huge edit control thing here...
      Warning(("Not yet supported\n"));
   }
   if (report->flags&kReportFlg_ToMono)   
   {
      mono_clear();
      mprint(_rpt_buffer);
   }
}

////////////////////////////////////
// actually do the real report

static void _report_do_type(sReportConfig *report, eReportType type)
{
   cCBChain::cIter cbiter;
   char *temp_output=(char *)malloc(256*1024);  // thats a lot of text
   for (cbiter=callbackChain.Iter(); !cbiter.Done(); cbiter.Next())
   {
      sReportCallback *CBDat=cbiter.Value();
      if (CBDat->flags&type)
      {
         strcpy(temp_output,"");
         if (type==kReportPerObj)
         {  // iterate over obj dynarray, call once per
            ReportObjCallback realCB=(ReportObjCallback)CBDat->cb;
            realCB(OBJ_NULL, report->warn, CBDat->data, temp_output+strlen(temp_output), kReportPhase_Init);
            for (int i=0; i<curObjs.Size(); i++)
               realCB(curObjs[i], report->warn, CBDat->data, temp_output+strlen(temp_output), kReportPhase_Loop);
            realCB(OBJ_NULL, report->warn, CBDat->data, temp_output+strlen(temp_output), kReportPhase_Term);
         }
         else
         {  // just call in once
            ReportGenCallback realCB=(ReportGenCallback)CBDat->cb;
            realCB(report->warn, CBDat->data, temp_output);
         }
         _buffer_add_to(temp_output);
      }
   }
   free(temp_output);
}

static const char *get_level_name(void)
{
   static char buf[64];
   dbCurrentFile(buf,60);
   if (buf[0])
      return buf;
   else
      return "Anonymous";
}

// put the internal header on the report
static void _report_internal_header(sReportConfig *report)
{
   struct tm *time_of_day;
   time_t    ltime;
   char      buf[1024];

   time(&ltime);
   time_of_day=localtime(&ltime);
   sprintf(buf,"\nReport on %s generated on %s",get_level_name(),asctime(time_of_day));
   sprintf(buf+strlen(buf),"// using report parms %x - %d - %s\n",report->flags,report->warn,report->obj?ObjEditName(report->obj):"None");
   // want to print user name, level name, so on?
   _buffer_add_to(buf);
}

// put the internal trailer on the report
static void _report_internal_trailer(sReportConfig *report)
{
   struct tm *time_of_day;
   time_t    ltime;
   char      buf[256];
   
   time(&ltime);
   time_of_day=localtime(&ltime);
   sprintf(buf,"\nReport finished on %s\n",asctime(time_of_day));
   _buffer_add_to(buf);
}

// internal script message dispatch
static void _report_internal_script(sReportConfig *report)
{
   AutoAppIPtr_(ScriptMan,pScriptMan);
   char *temp_output=(char *)malloc(256*1024);  // thats a lot of text
   _buffer_add_to("Sending Report Messages to Scripts\n");
   for (int i=0; i<curObjs.Size(); i++)
   {
      temp_output[0]='\0';
      sReportMsg repmsg(curObjs[i],report->warn,report->flags,report->types,temp_output);
      pScriptMan->SendMessage(&repmsg);
      _buffer_add_to(temp_output);
   }
   free(temp_output);
}

// go do the report
static void _report_do(sReportConfig *report)
{
   Status("Generating Report...");
   _buffer_setup();
   _report_get_curObjs(report);
   _report_internal_header(report);
   for (int loop=kReportFirstType; loop<=kReportLastType; loop<<=1)
      if (report->types&loop)  // want to do a per loop header entry here
         _report_do_type(report,(eReportType)loop);
   if (report->types&kReportScript)
      _report_internal_script(report);
   _report_internal_trailer(report);
   _buffer_output(report);
   _report_clear_curObjs();
   _buffer_clear();
   Status("Report Finished");
}

////////////////////////////////////
// report setup code

// does a report with flags/obj specified
void ReportDoSpecifiedReport(int types, int flags, int WarnLevel, ObjID obj)
{
   sReportConfig tmp_report_config;
   tmp_report_config.flags=flags;
   tmp_report_config.types=types;
   tmp_report_config.warn=WarnLevel;
   tmp_report_config.obj=obj;
   _report_do(&tmp_report_config);
}

// brings up the "report configuration" dialog
void ReportConfigureReport(void)
{
   IStructEditor *sed;
   sStructEditorDesc editdesc;
   BOOL result;

   strncpy(editdesc.title, "Report Configuration", sizeof(editdesc.title));
   editdesc.flags=kStructEditNoApplyButton;
   sed=CreateStructEditor(&editdesc, &report_config_desc, &_ReportConfig);
   if (!sed)
   {
      Warning(("Cant create sdesc editor for report\n"));
      return;
   }
   result=IStructEditor_Go(sed,kStructEdModal);
   SafeRelease(sed);
   if (result)
      _report_do(&_ReportConfig);
}

// goes and does the current report
void ReportDoCurrentReport(void)
{
   _report_do(&_ReportConfig);
}

void ReportDoNamedReport(char *fname)
{
   if (fname)
      ReportSetOutputFile(fname,TRUE);
   ReportDoCurrentReport();
}

////////////////////////////////////
// misc

// set output file name, erase clears it out, else append
void ReportSetOutputFile(char *fname, BOOL erase)
{
   strcpy(_ReportFname,fname);
   _ReportClear=erase;
}

////////////////////////////////////
// setup/closedown
static void _clear_dlist(void)
{
   cCBChain::cIter cbiter;
   for (cbiter=callbackChain.Iter(); !cbiter.Done(); cbiter.Next())
   {
      sReportCallback *oldCBDat=cbiter.Value();
      callbackChain.Delete(cbiter.Node());
      free(oldCBDat->name);
      free(oldCBDat);
   }
}

// commands
static Command report_keys[] =
{
   { "do_report", FUNC_VOID, ReportConfigureReport },
   { "do_cur_reprt", FUNC_STRING, ReportDoNamedReport },
};

void ReportInit(void)
{
   COMMANDS(report_keys, HK_ALL);   
   //   _clear_dlist();  // for now, just do this on term, so startup sequence is ok
}

void ReportTerm(void)
{
   _clear_dlist();   
}

#endif  // REPORT
