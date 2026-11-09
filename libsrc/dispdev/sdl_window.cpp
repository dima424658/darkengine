#pragma pack(push, 8)
#include <SDL3/SDL.h>
#pragma pack(pop)

#include <cstdio>

#include <render_backend.h>

#include "sdl_window.h"
#include "display_window.h"
#include "../lgd3d/event_bridge.h"

namespace
{
#pragma pack(push, 8)
class cSDLDisplayWindow
{
public:
   ~cSDLDisplayWindow()
   {
      Close();
   }

   bool Open(int width, int height, const char *title)
   {
      if (m_window)
      {
         SetTitle(title);
         return true;
      }
      if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
      {
         std::fprintf(stderr, "SDL3 initialization failed: %s\n", SDL_GetError());
         return false;
      }
      m_sdlInitialized = true;

      m_window = SDL_CreateWindow(title ? title : "Dark Engine", width, height,
         SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
         SDL_WINDOW_HIGH_PIXEL_DENSITY);
      if (!m_window)
      {
         std::fprintf(stderr, "SDL3 window creation failed: %s\n", SDL_GetError());
         SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
         m_sdlInitialized = false;
         return false;
      }

      const Uint8 cursorBits[8]{};
      m_invisibleCursor = SDL_CreateCursor(cursorBits, cursorBits, 8, 8, 0, 0);
      m_focused = (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
      m_showNativeCursor = false;
      UpdateRelativeMouse();
      return true;
   }

   void Close()
   {
      if (m_invisibleCursor)
      {
         (void)SDL_SetCursor(SDL_GetDefaultCursor());
         SDL_DestroyCursor(m_invisibleCursor);
         m_invisibleCursor = nullptr;
      }
      if (m_window)
      {
         SDL_DestroyWindow(m_window);
         m_window = nullptr;
      }
      m_relativeRequested = false;
      m_relativeActive = false;
      m_focused = false;
      if (m_sdlInitialized)
      {
         SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
         m_sdlInitialized = false;
      }
   }

   SDL_Window *Get() const { return m_window; }
   SDL_WindowID GetId() const
   {
      return m_window ? SDL_GetWindowID(m_window) : 0;
   }

   void GetMetrics(int *clientWidth, int *clientHeight, int *drawableWidth,
      int *drawableHeight) const
   {
      if (!m_window)
         return;
      if (clientWidth && clientHeight)
         (void)SDL_GetWindowSize(m_window, clientWidth, clientHeight);
      if (drawableWidth && drawableHeight)
         (void)SDL_GetWindowSizeInPixels(m_window, drawableWidth, drawableHeight);
   }

   bool SetFullscreen(bool enabled)
   {
      if (!m_window)
         return false;
      if (!SDL_SetWindowFullscreen(m_window, enabled))
      {
         std::fprintf(stderr, "SDL fullscreen change failed: %s\n", SDL_GetError());
         return false;
      }
      return true;
   }

   void SetTitle(const char *title)
   {
      if (m_window)
         SDL_SetWindowTitle(m_window, title ? title : "Dark Engine");
   }

   void SetNativeCursorVisible(bool visible)
   {
      m_showNativeCursor = visible;
      UpdateRelativeMouse();
   }

   void SetMousePosition(float x, float y)
   {
      if (m_window && !m_relativeActive)
         SDL_WarpMouseInWindow(m_window, x, y);
   }

   void SetRelativeMouse(bool enabled)
   {
      m_relativeRequested = enabled;
      UpdateRelativeMouse();
   }

   void SetFocused(bool focused)
   {
      m_focused = focused;
      UpdateRelativeMouse();
   }

   bool RelativeMouseActive() const { return m_relativeActive; }

   void SetWindowedClientSizeHint(int width, int height)
   {
      if (!m_window || width <= 0 || height <= 0 ||
          HasFlag(SDL_WINDOW_FULLSCREEN))
         return;
      SDL_SetWindowSize(m_window, width, height);
      DarkRenderWindowResized();
   }

   bool HasFlag(SDL_WindowFlags flag) const
   {
      return m_window && (SDL_GetWindowFlags(m_window) & flag) != 0;
   }

private:
   void UpdateRelativeMouse()
   {
      if (!m_window)
         return;
      const bool shouldBeActive = m_relativeRequested && m_focused;
      if (SDL_GetWindowRelativeMouseMode(m_window) != shouldBeActive)
         (void)SDL_SetWindowRelativeMouseMode(m_window, shouldBeActive);
      m_relativeActive = SDL_GetWindowRelativeMouseMode(m_window);
      if (m_relativeActive || !m_showNativeCursor)
      {
         if (m_invisibleCursor)
            (void)SDL_SetCursor(m_invisibleCursor);
         (void)SDL_HideCursor();
      }
      else
      {
         if (m_invisibleCursor && SDL_GetCursor() == m_invisibleCursor)
            (void)SDL_SetCursor(SDL_GetDefaultCursor());
         (void)SDL_ShowCursor();
      }
   }

   SDL_Window *m_window{};
   SDL_Cursor *m_invisibleCursor{};
   bool m_relativeRequested{};
   bool m_relativeActive{};
   bool m_focused{};
   bool m_showNativeCursor{};
   bool m_sdlInitialized{};
};
#pragma pack(pop)

cSDLDisplayWindow &DisplayWindow()
{
   static cSDLDisplayWindow window;
   return window;
}
}

bool SDLDisplayOpenWindow(int width, int height, const char *title)
{ return DisplayWindow().Open(width, height, title); }

void SDLDisplayCloseWindow(void) { DisplayWindow().Close(); }
SDL_Window *SDLDisplayGetWindow(void) { return DisplayWindow().Get(); }
SDL_WindowID SDLDisplayGetWindowId(void) { return DisplayWindow().GetId(); }

void SDLDisplayGetWindowMetrics(int *clientWidth, int *clientHeight,
   int *drawableWidth, int *drawableHeight)
{
   DisplayWindow().GetMetrics(clientWidth, clientHeight, drawableWidth,
      drawableHeight);
}

bool SDLDisplaySetFullscreen(bool enabled)
{ return DisplayWindow().SetFullscreen(enabled); }
void SDLDisplaySetTitle(const char *title) { DisplayWindow().SetTitle(title); }
void SDLDisplaySetNativeCursorVisible(bool visible)
{ DisplayWindow().SetNativeCursorVisible(visible); }
void SDLDisplaySetMousePositionLogical(int x, int y)
{
   if (!RenderBackendLogicalToClientPoint(&x, &y))
      return;
   DisplayWindow().SetMousePosition(static_cast<float>(x),
      static_cast<float>(y));
}
void SDLDisplaySetRelativeMouse(bool enabled)
{ DisplayWindow().SetRelativeMouse(enabled); }
void SDLDisplaySetFocused(bool focused) { DisplayWindow().SetFocused(focused); }
bool SDLDisplayRelativeMouseActive(void)
{ return DisplayWindow().RelativeMouseActive(); }
void SDLDisplaySetWindowedClientSizeHint(int width, int height)
{ DisplayWindow().SetWindowedClientSizeHint(width, height); }
bool SDLDisplayWindowHasFlag(SDL_WindowFlags flag)
{ return DisplayWindow().HasFlag(flag); }
bool SDLDisplayWindowIsMinimized(void)
{ return DisplayWindow().HasFlag(SDL_WINDOW_MINIMIZED); }
