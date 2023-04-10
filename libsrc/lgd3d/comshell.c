// $Header: x:/prj/tech/libsrc/lgd3d/RCS/comshell.c 1.3 1997/10/29 15:51:02 KEVIN Exp $

#include <appagg.h>
#include <appapi.h>
#include <dispapi.h>
#include <gshelapi.h>
#include <inpinit.h>
#include <kb.h>
#include <kbcook.h>

#include <d3dtest.h>

tResult LGAPI AppCreateObjects (int argc, const char *argv[])
{
   // Parse commmandline arguments
   GUID *ddraw_guid = d3dtestParseArgs(argc, argv);

   // Create the application object
   GenericApplicationCreate(argc, argv, "d3dtest", NULL);

   // Create the display device
   DisplayDeviceCreate2(kDispFullScreen, kDispAttempt3D|kDispAttemptFlippable);

   if (ddraw_guid != NULL) {
      IDisplayDevice *pDD = AppGetObj(IDisplayDevice);
      pDD->lpVtbl->SetKind2(pDD, kDispFullScreen, kDispAttempt3D|kDispAttemptFlippable, ddraw_guid);
   }

   // Create the game shell
   GameShellCreate(kGameShellDefault & ~kLockFrame);

   // Init the input library
   InputCreate();

   return 0; // Success
}

tResult LGAPI AppInit ()
{
   d3dtestInit();

   return 0; // Success
}

tResult LGAPI AppExit ()
{
   d3dtestExit();

   return 0; // Success
}

#include <lgassert.h>
int LGAPI AppMain (int argc, const char *argv[])
{
   IGameShell * pGameShell = AppGetObj (IGameShell);

   do {
      IGameShell_PumpEvents(pGameShell, kPumpAll);
   } while (d3dtestLoop());

   return 0; // Success
}

// goofy abstraction needed because input lib doesn't work under dos
// and getch() doesn't work under windows, unless, of course, 
// we're a console app, but we're not.
bool AppGetKey(char *c)
{
   ushort key;

   if ((!kb_get_cooked(&key))||
       (!(key&KB_FLAG_DOWN)))
      return FALSE;

   *c = kb2ascii(key);
   return TRUE;
}
