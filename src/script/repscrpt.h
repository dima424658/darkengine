/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/repscrpt.h,v 1.1 1998/06/19 00:14:33 dc Exp $
// report system script implementation

#pragma once

#ifndef __REPSCRPT_H
#define __REPSCRPT_H

#include <objtype.h>

#include <scrptmsg.h>
#include <report.h>

struct sReportMsg: public sScrMsg
{
   int   WarnLevel;  // WarnLevel set by the user
   int   Flags;      // made up from ReportFlags in report.h
   int   Types;      // made up from ReportTypes in report.h
   char *TextBuffer; // where to write your output
   
   sReportMsg(ObjID rep_obj, int warn, int flg, int typ, char *buffer)
      : sScrMsg(rep_obj, "ReportMessage"),
        WarnLevel(warn), Flags(flg), Types(typ), TextBuffer(buffer)
   {
   }
};

#ifdef SCRIPT
#define OnReportMessage()             SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ReportMessage,sReportMsg)
#define DefaultOnReportMessage()      SCRIPT_CALL_BASE(ReportMessage)
#endif  // SCRIPT

#define REPORT_MESSAGE_DEFAULTS()     SCRIPT_DEFINE_MESSAGE_DEFAULT_(ReportMessage,sReportMsg)
#define REPORT_MESSAGE_MAP_ENTRIES()  SCRIPT_MSG_MAP_ENTRY(ReportMessage)

#define ReportPrint  sprintf
#define ReportString message.TextBuffer+strlen(message.TextBuffer)

#endif  // __REPSCRPT_H
