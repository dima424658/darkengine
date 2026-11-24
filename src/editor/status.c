/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/status.c,v 1.12 2000/02/19 13:11:27 toml Exp $

// Status line system

#include <stdio.h>
#include <string.h>
#include <lg.h>
#include <status.h>
#include <platform_services.h>
#include <config.h>
#include <mprintf.h>
#include <cmdterm.h>

static const int field_position[] = {0, 1, 20, 40, 42, 75, 77, 79, 100};
static char status_string[SF_NUM_FIELDS][128];

int StatusFieldPosition(int field)
{
   return field_position[field];
}

void StatusField(int field, const char *text)
{
   if (field < 0 || field >= SF_NUM_FIELDS)
      return;
   snprintf(status_string[field], sizeof(status_string[field]), "%s", text ? text : "");
}

const char *StatusGetField(int field)
{
   return field >= 0 && field < SF_NUM_FIELDS ? status_string[field] : "";
}

void Status(const char *text)
{
   StatusField(SF_STATUS, text);
   cmdterm_capture_status(text);
   if (!text || !*text)
      return;
   if (PlatformBatchMode())
   {
      fprintf(stderr, "DromEd: %s\n", text);
      fflush(stderr);
   }
#ifndef SHIP
   if (config_is_defined("status"))
      mprintf("STATUS: %s\n", text);
#endif
}
