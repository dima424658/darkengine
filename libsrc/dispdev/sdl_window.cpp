#include <SDL3/SDL.h>

#include <cstdio>

#include <appagg.h>
#include <vkrend.h>
#include <render_backend.h>

#include "sdl_window.h"

IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cSDLDisplayWindow);

cSDLDisplayWindow::cSDLDisplayWindow(IUnknown *pOuter)
{
   INIT_AGGREGATION_2(pOuter,
      IID_IDisplayWindow, static_cast<IDisplayWindow *>(this),
      IID_IVulkanWindow, static_cast<IVulkanWindow *>(this),
      kPriorityLibrary, nullptr);
}

cSDLDisplayWindow::~cSDLDisplayWindow()
{
   Close();
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::Open(int width, int height, const char *title)
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

   SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
      SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifndef __ANDROID__
   flags |= SDL_WINDOW_HIDDEN;
#endif
   m_window.reset(SDL_CreateWindow(title ? title : "Dark Engine", width, height, flags));
   if (!m_window)
   {
      std::fprintf(stderr, "SDL3 window creation failed: %s\n", SDL_GetError());
      SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
      m_sdlInitialized = false;
      return false;
   }

   m_showPending = (flags & SDL_WINDOW_HIDDEN) != 0;
   const Uint8 cursorBits[8]{};
   m_invisibleCursor.reset(SDL_CreateCursor(cursorBits, cursorBits, 8, 8, 0, 0));
   m_focused = (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_INPUT_FOCUS) != 0;
   m_showNativeCursor = false;
   UpdateRelativeMouse();
   return true;
}

STDMETHODIMP_(void) cSDLDisplayWindow::Close()
{
   if (m_invisibleCursor)
   {
      (void)SDL_SetCursor(SDL_GetDefaultCursor());
      m_invisibleCursor.reset();
   }
   m_window.reset();
   m_showPending = false;
   m_revealPending = false;
   m_relativeRequested = false;
   m_relativeActive = false;
   m_focused = false;
   if (m_sdlInitialized)
   {
      SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
      m_sdlInitialized = false;
   }
}

STDMETHODIMP_(SDL_Window *) cSDLDisplayWindow::GetSDLWindow()
{
   return m_window.get();
}

STDMETHODIMP_(void) cSDLDisplayWindow::GetMetrics(int *clientWidth, int *clientHeight,
   int *drawableWidth, int *drawableHeight)
{
   if (!m_window)
      return;
   if (clientWidth && clientHeight)
      (void)SDL_GetWindowSize(m_window.get(), clientWidth, clientHeight);
   if (drawableWidth && drawableHeight)
      (void)SDL_GetWindowSizeInPixels(m_window.get(), drawableWidth, drawableHeight);
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::SetFullscreen(BOOL enabled)
{
   if (!m_window)
      return false;
   if (!SDL_SetWindowFullscreen(m_window.get(), enabled))
   {
      std::fprintf(stderr, "SDL fullscreen change failed: %s\n", SDL_GetError());
      return false;
   }
   return true;
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::StartupWindowPending()
{
   return m_showPending;
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::ShowForPresentation()
{
   if (!m_showPending)
      return true;

   // SDL applies a hidden window's pending fullscreen mode on show.
   // Keep the native window transparent while the platform maps and
   // resizes it, where window opacity is supported.
   const bool fullscreen = (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_FULLSCREEN) != 0;
   if (fullscreen && !m_revealPending)
      m_revealPending = SDL_SetWindowOpacity(m_window.get(), 0.0f);
   if (!SDL_ShowWindow(m_window.get()))
   {
      std::fprintf(stderr, "SDL window could not be shown: %s\n", SDL_GetError());
      return false;
   }
   if (fullscreen)
      SDL_SyncWindow(m_window.get());
   m_showPending = false;
   return true;
}

STDMETHODIMP_(void) cSDLDisplayWindow::RevealAfterPresent()
{
   if (m_revealPending)
   {
      if (!SDL_SetWindowOpacity(m_window.get(), 1.0f))
         std::fprintf(stderr, "SDL window could not restore opacity: %s\n", SDL_GetError());
      m_revealPending = false;
   }
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetTitle(const char *title)
{
   if (m_window)
      SDL_SetWindowTitle(m_window.get(), title ? title : "Dark Engine");
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetNativeCursorVisible(BOOL visible)
{
   const bool showNativeCursor = visible != FALSE;
   if (m_showNativeCursor == showNativeCursor)
      return;
   m_showNativeCursor = showNativeCursor;
   UpdateRelativeMouse();
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetMousePositionLogical(int x, int y)
{
   if (m_window && !m_relativeActive && RenderBackendLogicalToClientPoint(&x, &y))
      SDL_WarpMouseInWindow(m_window.get(), static_cast<float>(x), static_cast<float>(y));
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetRelativeMouse(BOOL enabled)
{
   m_relativeRequested = enabled;
   UpdateRelativeMouse();
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetFocused(BOOL focused)
{
   m_focused = focused;
   UpdateRelativeMouse();
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::IsRelativeMouseActive()
{
   return m_relativeActive;
}

STDMETHODIMP_(void) cSDLDisplayWindow::SetWindowedClientSizeHint(int width, int height)
{
   if (!m_window || width <= 0 || height <= 0 ||
       (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_FULLSCREEN))
      return;
   SDL_SetWindowSize(m_window.get(), width, height);
   AutoAppIPtr_(VulkanRenderer,renderer);
   if (renderer)
      renderer->WindowResized();
}

STDMETHODIMP_(BOOL) cSDLDisplayWindow::IsMinimized()
{
   return m_window && (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_MINIMIZED) != 0;
}

void cSDLDisplayWindow::UpdateRelativeMouse()
{
   if (!m_window)
      return;
   const bool shouldBeActive = m_relativeRequested && m_focused;
   if (SDL_GetWindowRelativeMouseMode(m_window.get()) != shouldBeActive)
      (void)SDL_SetWindowRelativeMouseMode(m_window.get(), shouldBeActive);
   m_relativeActive = SDL_GetWindowRelativeMouseMode(m_window.get());
   if (m_relativeActive || !m_showNativeCursor)
   {
      if (m_invisibleCursor)
         (void)SDL_SetCursor(m_invisibleCursor.get());
      (void)SDL_HideCursor();
   }
   else
   {
      if (m_invisibleCursor && SDL_GetCursor() == m_invisibleCursor.get())
         (void)SDL_SetCursor(SDL_GetDefaultCursor());
      (void)SDL_ShowCursor();
   }
}

tResult LGAPI DisplayWindowCreate(IUnknown *pOuter)
{
   return pOuter && new cSDLDisplayWindow(pOuter) ? S_OK : E_FAIL;
}
