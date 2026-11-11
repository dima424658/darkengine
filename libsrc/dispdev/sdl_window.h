#ifndef DARK_SDL_WINDOW_H
#define DARK_SDL_WINDOW_H

#include <SDL3/SDL.h>
#include "display_window.h"

SDL_Window *SDLDisplayGetWindow(void);
SDL_WindowID SDLDisplayGetWindowId(void);
bool SDLDisplayWindowHasFlag(SDL_WindowFlags flag);

#endif
