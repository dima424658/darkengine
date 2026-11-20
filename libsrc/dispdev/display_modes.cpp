#include "display_modes.h"

#include <SDL3/SDL.h>
#include <cstdlib>

int DisplayGetModes(eDisplayScope scope,
                            sDisplayMode *modes, int capacity,
                            int *native_width, int *native_height)
{
   if (!modes || capacity < 0)
      capacity = 0;
   if (native_width)
      *native_width = 0;
   if (native_height)
      *native_height = 0;

   const bool initialized =
      (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0;
   if (!initialized)
   {
#if defined(__linux__) || defined(__APPLE__)
      // Match the compositor before the first SDL video initialization.
      if (std::getenv("WAYLAND_DISPLAY") && !std::getenv("SDL_VIDEODRIVER"))
         setenv("SDL_VIDEODRIVER", "wayland", 0);
#endif
      if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
         return 0;
   }

   int displayCount = 0;
   SDL_DisplayID *displays = SDL_GetDisplays(&displayCount);
   if (!displays)
      displayCount = 0;
   int firstDisplay = 0;
   if (scope == kDisplayCurrentDisplay && displayCount > 0)
   {
      SDL_Window *window = SDL_GetKeyboardFocus();
      const SDL_DisplayID current = window
         ? SDL_GetDisplayForWindow(window) : displays[0];
      for (int i = 0; i < displayCount; ++i)
         if (displays[i] == current)
         {
            firstDisplay = i;
            break;
         }
      displayCount = 1;
   }

   int count = 0;
   for (int displayOffset = 0; displayOffset < displayCount; ++displayOffset)
   {
      const SDL_DisplayID display = displays[firstDisplay + displayOffset];
      const SDL_DisplayMode *current = SDL_GetCurrentDisplayMode(display);
      if (current && scope == kDisplayCurrentDisplay)
      {
         if (native_width)
            *native_width = current->w;
         if (native_height)
            *native_height = current->h;
      }

      int modeCount = 0;
      SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes(display,
         &modeCount);
      for (int i = 0; displayModes && i < modeCount && count < capacity; ++i)
      {
         const SDL_DisplayMode &mode = *displayModes[i];
         const int depth = SDL_BITSPERPIXEL(mode.format);
         int duplicate = 0;
         for (; duplicate < count; ++duplicate)
            if (modes[duplicate].width == mode.w &&
                modes[duplicate].height == mode.h &&
                modes[duplicate].bit_depth == depth)
               break;
         if (duplicate == count)
            modes[count++] = {mode.w, mode.h, depth};
      }
      SDL_free(displayModes);
   }

   SDL_free(displays);
   if (!initialized)
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
   return count;
}

