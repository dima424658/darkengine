/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/scrpted.cpp,v 1.2 2000/02/19 13:11:24 toml Exp $
#include <simpwrap.h>
#include <scrptpr_.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern "C" 
BOOL PickAndDropScript(void)
{
   BOOL retval = FALSE; 
   char **ppszMenu = (char **) Malloc(1);
   int iNumMenuSlots = 0;

   cScriptProp::cModuleSet::cIter iter;
   for (iter = cScriptProp::TheProp->Modules(); !iter.Done(); iter.Next())
   {
      ppszMenu = (char **) Realloc(ppszMenu, 
                                   (iNumMenuSlots + 1) * sizeof(char *));
      ppszMenu[iNumMenuSlots]
         = (char *) Malloc(sizeof (char) * (strlen(iter.Value().name) + 1));
      sprintf(ppszMenu[iNumMenuSlots], "%s", iter.Value().name);
      ++iNumMenuSlots;
   }

   // pop up our menu
   if (iNumMenuSlots) {
      int iChoice = PickFromStringList("Pick a module to drop",
                                       (const char **) ppszMenu, 
                                       iNumMenuSlots);
      if (iChoice != -1) {
         cScriptProp::TheProp->DropModule(ppszMenu[iChoice]);
         retval = TRUE;
      }
    }

   // get rid of all this stuff
   for (int i = 0; i < iNumMenuSlots; ++i)
      Free(ppszMenu[i]);
   Free(ppszMenu);
   return retval; 

}
