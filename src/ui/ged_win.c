/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/ged_win.c,v 1.3 2000/02/19 13:28:20 toml Exp $
// windows UI utility elements of Gedit

// quarantine the virus
#include <windows.h>
// quarantine the virus


#include <ged_win.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// get y or n response
BOOL ged_winui_GetYorN(char *msg)
{
   int rv=MessageBox(NULL,msg,"Dromed Yes or No Question",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION);
   return rv==IDYES;
}

// draw a big menu
void ged_winui_Text(char *msg)
{
   MessageBox(NULL,msg,"Dromed Info",MB_OK);
}
