#include <SDL3/SDL.h>

#include <lg.h>
#include <editorplatform.h>

#include <appagg.h>
#include <cursors.h>
#include <darkdlgs/darkdlgs.h>
#include <gadget.h>
#include <gshelapi.h>
#include <vkrend.h>
#include <dispwnd.h>
#include <menubar.h>
#include <mouse.h>
#include <render_backend.h>

namespace {
bool g_tracking;
bool g_cursorHidden;
}

extern "C" void EditorStartWindowResizeTracking(void)
{
   int width;
   int height;
   AutoAppIPtr_(VulkanRenderer,renderer);
   if (renderer)
      renderer->TakeResize(&width, &height);
   g_tracking = true;
}

extern "C" void EditorStopWindowResizeTracking(void)
{
   g_tracking = false;
}

extern "C" void EditorRestoreWindowRect(void)
{
}

extern "C" BOOL EditorGetPendingWindowSize(int *width, int *height)
{
   if (!g_tracking)
      return FALSE;
   AutoAppIPtr_(VulkanRenderer,renderer);
   return renderer && renderer->TakeResize(width, height);
}

extern "C" void EditorPlatformInitializeControls(void)
{
}

extern "C" void EditorPlatformDestroyAuxiliaryWindows(void)
{
}

extern "C" void EditorCloseDialogs(void)
{
   CloseDarkDlgs();
}

void EditorShowNativeCursor(BOOL show)
{
   AutoAppIPtr_(GameShell,pShell);
   int flags=0;
   pShell->GetFlags(&flags);
   if (show)
      flags|=kShowNativeCursor;
   else
      flags&=~kShowNativeCursor;
   pShell->SetFlags(flags);
}

void EditorUpdateCursor(void)
{
   const bool menu=MenuBarOwnsPointer();
   AutoAppIPtr_(DisplayWindow,window);
   const bool hide=menu || !window || SDL_GetMouseFocus()!=window->GetSDLWindow();
   if (hide!=g_cursorHidden)
   {
      if (hide)
         uiHideMouse(NULL);
      else
      {
         // The menu can consume motion while the software cursor is hidden.
         float clientX, clientY;
         SDL_GetMouseState(&clientX, &clientY);
         int x = static_cast<int>(clientX);
         int y = static_cast<int>(clientY);
         if (RenderBackendClientToLogicalPoint(&x, &y))
            mouse_sync_xy(static_cast<short>(x), static_cast<short>(y));
         uiShowMouse(NULL);
      }
      g_cursorHidden=hide;
   }
   const LGadRoot *root=LGadCurrentRoot();
   EditorShowNativeCursor(menu || !root || !root->curs);
}

void EditorResetCursor(void)
{
   if (g_cursorHidden)
   {
      uiShowMouse(NULL);
      g_cursorHidden=false;
   }
   EditorShowNativeCursor(FALSE);
}
