#ifndef DARK_DISPLAY_WINDOW_H
#define DARK_DISPLAY_WINDOW_H

struct SDL_Window;

bool SDLDisplayOpenWindow(int width, int height, const char *title);
void SDLDisplayCloseWindow(void);
SDL_Window *SDLDisplayGetWindow(void);
void SDLDisplayGetWindowMetrics(int *clientWidth, int *clientHeight,
   int *drawableWidth, int *drawableHeight);
bool SDLDisplayWindowIsMinimized(void);
bool SDLDisplaySetFullscreen(bool enabled);
void SDLDisplaySetTitle(const char *title);
void SDLDisplaySetNativeCursorVisible(bool visible);
void SDLDisplaySetMousePositionLogical(int x, int y);
void SDLDisplaySetRelativeMouse(bool enabled);
void SDLDisplaySetFocused(bool focused);
bool SDLDisplayRelativeMouseActive(void);
void SDLDisplaySetWindowedClientSizeHint(int width, int height);

#endif
