#include <SDL3/SDL.h>

#include <lg.h>
#include <editorplatform.h>

#include <appagg.h>
#include <cursors.h>
#include <gadget.h>
#include <gshelapi.h>
#include <runtime.h>
#include <dispwnd.h>
#include <vulkan_imgui.hpp>

namespace {
bool g_tracking;
bool g_cursorHidden;
}

extern "C" void EditorStartWindowResizeTracking(void)
{
   int width;
   int height;
   DarkRenderTakeResize(&width, &height);
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
   return g_tracking && DarkRenderTakeResize(width, height);
}

extern "C" void EditorPlatformInitializeControls(void)
{
}

extern "C" void EditorPlatformDestroyAuxiliaryWindows(void)
{
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
   const bool menu=VulkanImGuiMenuOwnsPointer();
   AutoAppIPtr_(DisplayWindow,window);
   const bool hide=menu || !window || SDL_GetMouseFocus()!=window->GetSDLWindow();
   if (hide!=g_cursorHidden)
   {
      if (hide)
         uiHideMouse(NULL);
      else
         uiShowMouse(NULL);
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
