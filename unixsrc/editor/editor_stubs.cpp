#include <cstdio>
#include <filemenu.h>
#include <winui.h>

#include <render_backend.h>

extern "C" {
void EditorCreateGUI(void) {}
void EditorDestroyGUI(void) {}
void EditorStartWindowResizeTracking(void) {}
void EditorStopWindowResizeTracking(void) {}
void EditorRestoreWindowRect(void) {}
void uieditStyleSetup(void) {}
void uieditStyleCleanup(void) {}
void uieditRedrawAll(void) {}
void redraw_all_cmd(void) {}

int EditorGetPendingWindowSize(int *width, int *height)
{
   static int previousWidth;
   static int previousHeight;
   int currentWidth;
   int currentHeight;
   if (!width || !height ||
       !RenderBackendGetSettledClientSize(&currentWidth, &currentHeight) ||
       (currentWidth == previousWidth && currentHeight == previousHeight))
      return 0;
   previousWidth = currentWidth;
   previousHeight = currentHeight;
   *width = currentWidth;
   *height = currentHeight;
   return 1;
}

void ObjEditInit(void) {}
BOOL show_all_links = FALSE;
void FileMenu(void) {}
char *FileDialog(eFileMenuMode, sFileMenuFilter *, const char *, char *, int)
{
   return nullptr;
}
void SetMainMenu(const char *) {}
void SetRadioCheckmark(int, int, int, int) {}
BOOL winui_GetYorN(const char *)
{
   return FALSE;
}
void winui_Text(const char *message)
{
   if (message)
      std::fprintf(stderr, "%s\n", message);
}
}
