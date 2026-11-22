#ifndef __MENUBAR_H
#define __MENUBAR_H

// The callback draws the menu items inside the bar. Pass NULL to hide it.
void SetMenuBar(void (*pfnDraw)());
bool MenuBarActive();
bool MenuBarOwnsPointer();

#endif
