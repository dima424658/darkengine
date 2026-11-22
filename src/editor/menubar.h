#ifndef __MENUBAR_H
#define __MENUBAR_H

EXTERN int EditorViewportMenu(const char *const *items, int count, int x, int y);

#ifdef __cplusplus
// The callback draws the menu items inside the bar. Pass NULL to hide it.
void SetMenuBar(void (*pfnDraw)());
bool MenuBarActive();
bool MenuBarOwnsPointer();
#endif

#endif
