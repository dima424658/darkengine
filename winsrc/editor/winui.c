/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/winui.c,v 1.2 2000/02/19 13:28:24 toml Exp $
// windows UI utility elements of Gedit

// quarantine the virus
#include <win32_platform.h>
#include <stdio.h>
// quarantine the virus


#include <winui.h>
#include <platform_services.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

// get y or n response
BOOL winui_GetYorN(const char *msg)
{
   return PlatformAskYesNo("DromEd confirmation",msg);
}

// draw a big menu
void winui_Text(const char *msg)
{
   if (PlatformBatchMode())
      fprintf(stderr,"DromEd: %s\n",msg ? msg : "");
   else
      MessageBox(NULL,msg,"Dromed Info",MB_OK);
}
