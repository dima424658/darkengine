#include <lg.h>
#include <editorplatform.h>

#include "../render/runtime.h"

namespace {
bool g_tracking;
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
