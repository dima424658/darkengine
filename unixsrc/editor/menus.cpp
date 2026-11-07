#include <lg.h>
#include <menus.h>

#include "../lgd3d/runtime.h"

extern "C" void SetMainMenu(const char *name)
{
   DarkRenderSetTitle(name ? "DromEd - editor" : "DromEd");
}

extern "C" void SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos)
{
   (void)menu;
   (void)firstPos;
   (void)lastPos;
   (void)checkPos;
}
