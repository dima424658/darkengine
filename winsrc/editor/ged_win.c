/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/ged_win.c,v 1.3 2000/02/19 13:28:20 toml Exp $
// windows UI utility elements of Gedit

// quarantine the virus
#include <win32_platform.h>
#include <stdio.h>
// quarantine the virus


#include <ged_win.h>
#include <platform_services.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

// get y or n response
BOOL ged_winui_GetYorN(char *msg)
{
   return PlatformAskYesNo("DromEd confirmation",msg);
}

// draw a big menu
void ged_winui_Text(char *msg)
{
   if (PlatformBatchMode())
      fprintf(stderr,"DromEd: %s\n",msg ? msg : "");
   else
      MessageBox(NULL,msg,"Dromed Info",MB_OK);
}
