#ifndef DARK_SDL_WINDOW_H
#define DARK_SDL_WINDOW_H

#include <memory>

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>

#include <dispwnd.h>
#include <aggmemb.h>

#include <vkwnd.h>

class cSDLDisplayWindow final : public IDisplayWindow, public IVulkanWindow
{
   DECLARE_SIMPLE_AGGREGATION(cSDLDisplayWindow);

public:
   explicit cSDLDisplayWindow(IUnknown *pOuter);
   ~cSDLDisplayWindow();

   STDMETHOD_(BOOL, Open)(int width, int height, const char *title) override;
   STDMETHOD_(void, Close)() override;
   STDMETHOD_(SDL_Window *, GetSDLWindow)() override;
   STDMETHOD_(void, GetMetrics)(int *clientWidth, int *clientHeight,
      int *drawableWidth, int *drawableHeight) override;
   STDMETHOD_(BOOL, IsMinimized)() override;
   STDMETHOD_(BOOL, StartupWindowPending)() override;
   STDMETHOD_(BOOL, ShowForPresentation)() override;
   STDMETHOD_(void, RevealAfterPresent)() override;
   STDMETHOD_(BOOL, SetFullscreen)(BOOL enabled) override;
   STDMETHOD_(void, SetTitle)(const char *title) override;
   STDMETHOD_(void, SetNativeCursorVisible)(BOOL visible) override;
   STDMETHOD_(void, SetMousePositionLogical)(int x, int y) override;
   STDMETHOD_(void, SetRelativeMouse)(BOOL enabled) override;
   STDMETHOD_(void, SetFocused)(BOOL focused) override;
   STDMETHOD_(BOOL, IsRelativeMouseActive)() override;
   STDMETHOD_(void, SetWindowedClientSizeHint)(int width, int height) override;

   STDMETHOD_(BOOL, GetInstanceExtensions)(std::vector<const char *> &extensions,
      uint32_t &drawableWidth, uint32_t &drawableHeight, char *error,
      unsigned errorCapacity) override;
   STDMETHOD_(BOOL, GetDrawableSize)(uint32_t &drawableWidth,
      uint32_t &drawableHeight, char *error, unsigned errorCapacity) override;
   STDMETHOD_(BOOL, CreateSurface)(vk::Instance instance, vk::SurfaceKHR &surface,
      char *error, unsigned errorCapacity) override;

private:
   void UpdateRelativeMouse();

   struct WindowDeleter
   {
      void operator()(SDL_Window *window) const noexcept
      {
         SDL_DestroyWindow(window);
      }
   };

   struct CursorDeleter
   {
      void operator()(SDL_Cursor *cursor) const noexcept
      {
         SDL_DestroyCursor(cursor);
      }
   };

   std::unique_ptr<SDL_Window, WindowDeleter> m_window;
   std::unique_ptr<SDL_Cursor, CursorDeleter> m_invisibleCursor;
   bool m_relativeRequested{};
   bool m_relativeActive{};
   bool m_focused{};
   bool m_showNativeCursor{};
   bool m_sdlInitialized{};
   bool m_showPending{};
   bool m_revealPending{};
};

#endif
