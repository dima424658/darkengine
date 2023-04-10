/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/report.h,v 1.8 1999/06/02 16:56:10 XEMU Exp $
// report system

#pragma once

#ifndef __REPORT_H
#define __REPORT_H

#include <reputil.h>
#include <objtype.h>

// types of report function
enum eReportType
{ 
  kReportHeader        = (1<<0),  // basic "info" about level
  kReportPerObj        = (1<<1),  // per object reporting    
  kReportAllObj        = (1<<2),  // report on all objs - for props, really
  kReportWorldDB       = (1<<3),  // world database reporting
  kReportRoom          = (1<<4),  // room database reporting
  kReportAIPath        = (1<<5),  // AI Pathfind database reporting
  kReportScript        = (1<<6),  // send script report gen messages
  kReportDebug         = (1<<7),  // debugging report gen, debugging memory/system issues
  kReportModels        = (1<<8),  // model usage
  kReportGame          = (1<<9),  // game specific info
  kReportPrivate       = (1<<16), // beginning of private internal flags  
  kReport_IntMax       = 0xffffffff,
};

#define kReportFirstType kReportHeader
#define kReportLastType  kReportGame

enum eReportFlags
{
  kReportFlg_HotRegion = (1<<0),  // hotregion as filter          \   if none of these
  kReportFlg_Select    = (1<<1),  // just use current selection    >   we assume all 
  kReportFlg_Hilight   = (1<<2),  // highlighting as filter       /    objs
  kReportFlg_AllObjs   = (1<<3),  // just go do all objects          
  kReportFlg_Concrete  = (1<<4),  // concrete only \  if neither of these 
  kReportFlg_Abstract  = (1<<5),  // abstract only /  then defaults to concrete only
  kReportFlg_ToFile    = (1<<6),  // send output to named file  \ xxx (avoid \ at end of line)
  kReportFlg_ToMono    = (1<<7),  // send output to mono         > output control
  kReportFlg_ToScr     = (1<<8),  // send output to screen      / xxx
};

enum eReportPhase
{
  kReportPhase_Init,
  kReportPhase_Loop,
  kReportPhase_Term
};

// suggested warn levels - should become an enum, i think
#define kReportLevel_Errors  0    // only tell me about things which are wrong
#define kReportLevel_Warning 1    // add in info about things which are suspicious
#define kReportLevel_Info    2    // also add "useful info" summaries
#define kReportLevel_DumpAll 3    // dump all possible everything you can think of

////////////////////////////////////
// callback management, types, setup

// callback types
typedef void (*ReportObjCallback)(ObjID obj, int WarnLevel, void *data, char *buffer, enum eReportPhase phase);
typedef void (*ReportGenCallback)(int WarnLevel, void *data, char *buffer);

#ifdef REPORT
// register a callback into the report system
EXTERN BOOL ReportRegisterObjCallback(ReportObjCallback cb, char *name, void *data);
EXTERN BOOL ReportRegisterGenCallback(ReportGenCallback cb, enum eReportType type, char *name, void *data);

// unregister a callback from the report system
//  (is it ok to just use name/cb for identity, or do we need data too?)
EXTERN BOOL ReportUnRegisterObjCallback(ReportObjCallback cb, void *data);
EXTERN BOOL ReportUnRegisterGenCallback(ReportGenCallback cb, enum eReportType type, void *data);

// return whether this obj is an active element of current report
EXTERN BOOL ReportCheckObjActive(ObjID obj);
#else
#define ReportRegisterObjCallback(cb,name,data)
#define ReportRegisterGenCallback(cb,type,name,data)
#define ReportUnRegisterObjCallback(cb,data)
#define ReportUnRegisterGenCallback(cb,type,data)
#define ReportCheckObjActive(o)
#endif

////////////////////////////////////
// actually get reports

// does a report with flags/obj specified
EXTERN void ReportDoSpecifiedReport(int types, int flags, int WarnLevel, ObjID obj);

// brings up the "report configuration" dialog
EXTERN void ReportConfigureReport(void);

// goes and does the current report
EXTERN void ReportDoCurrentReport(void);

////////////////////////////////////
// misc

// set output file name, erase clears it out, else append
EXTERN void ReportSetOutputFile(char *fname, BOOL erase);

////////////////////////////////////
// setup/closedown
#ifdef REPORT
EXTERN void ReportInit(void);
EXTERN void ReportTerm(void);
#else
#define ReportInit()
#define ReportTerm()
#endif // REPORT

#endif
