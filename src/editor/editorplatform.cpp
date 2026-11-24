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
#include <imgui_workspace.h>
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
   const bool overlay = MenuBarOwnsPointer();
   AutoAppIPtr_(DisplayWindow,window);
   const bool inEditor = window && SDL_GetMouseFocus() == window->GetSDLWindow();
   float clientX, clientY;
   SDL_GetMouseState(&clientX, &clientY);
   const LGadRoot *root = LGadCurrentRoot();
   const bool custom = inEditor && !overlay && root && root->curs &&
      EditorWorkspaceUsesCameraCursor(clientX, clientY);

   UpdateEditorImGuiCursor(overlay);
   const bool hide = !custom;
   if (hide != g_cursorHidden)
   {
      if (custom)
      {
         // ImGui may have consumed motion while the software cursor was hidden.
         int x = static_cast<int>(clientX);
         int y = static_cast<int>(clientY);
         if (RenderBackendClientToLogicalPoint(&x, &y))
            mouse_sync_xy(static_cast<short>(x), static_cast<short>(y));
         uiShowMouse(NULL);
      }
      else
         uiHideMouse(NULL);
      g_cursorHidden = hide;
   }
   EditorShowNativeCursor(!custom);
}

void EditorResetCursor(void)
{
   UpdateEditorImGuiCursor(false);
   if (g_cursorHidden)
   {
      uiShowMouse(NULL);
      g_cursorHidden=false;
   }
   EditorShowNativeCursor(FALSE);
}
