#include <lg.h>
#include <menus.h>

#include "../render/runtime.h"

extern "C" void SetMainMenu(const char *name)
{
   DarkRenderSetTitle(name ? "DromEd - editor" : "DromEd");
}

extern "C" void MenuCommand(unsigned commandId)
{
   (void)commandId;
}

extern "C" void SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos)
{
   (void)menu;
   (void)firstPos;
   (void)lastPos;
   (void)checkPos;
}
