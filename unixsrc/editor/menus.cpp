#include <lg.h>
#include <menus.h>

#include "../../libsrc/dispdev/display_window.h"

extern "C" void SetMainMenu(const char *name)
{
   SDLDisplaySetTitle(name ? "DromEd - editor" : "DromEd");
}

extern "C" void SetRadioCheckmark(int menu, int firstPos, int lastPos, int checkPos)
{
   (void)menu;
   (void)firstPos;
   (void)lastPos;
   (void)checkPos;
}

#if defined(_WIN32)
// SDL3 has no native menu bar API. The SDL Windows editor currently has no
// platform menu items to dispatch, but keep the legacy editor command bridge
// linkable for callers shared with the native editor implementation.
void MenuCommand(unsigned command)
{
   (void)command;
}
#endif
