#include <lg.h>

#define WIN32_LEAN_AND_MEAN
#include <win32_platform.h>
#include <commctrl.h>

#include <comtools.h>
#include <wappapi.h>
#include <appagg.h>
#include <editorplatform.h>

#include <memall.h>
#include <dbmem.h>

static HWND editorMainWindow;
static WNDPROC editorPreviousWindowProc;
static BOOL editorInSizeMove;
static BOOL editorResizePending;
static int editorPendingWidth;
static int editorPendingHeight;
static RECT editorPendingWindowRect;
static BOOL editorWindowRectPending;
static BOOL editorPendingMaximized;

static void remember_editor_window_size(HWND window)
{
   RECT client;

   if (GetClientRect(window, &client))
   {
      int width = client.right - client.left;
      int height = client.bottom - client.top;

      if (width >= 400 && height >= 300)
      {
         editorPendingWidth = width;
         editorPendingHeight = height;
         editorResizePending = TRUE;
         if (GetWindowRect(window, &editorPendingWindowRect))
         {
            editorWindowRectPending = TRUE;
            editorPendingMaximized = IsZoomed(window);
         }
      }
   }
}

static LRESULT CALLBACK editor_window_proc(HWND window, UINT message,
                                           WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      case WM_ENTERSIZEMOVE:
         editorInSizeMove = TRUE;
         break;
      case WM_SIZE:
         if (wParam != SIZE_MINIMIZED && !editorInSizeMove)
            remember_editor_window_size(window);
         break;
      case WM_EXITSIZEMOVE:
         editorInSizeMove = FALSE;
         remember_editor_window_size(window);
         break;
   }

   return CallWindowProc(editorPreviousWindowProc, window, message, wParam, lParam);
}

void EditorStartWindowResizeTracking(void)
{
   IWinApp *app = AppGetObj(IWinApp);
   HWND window = IWinApp_GetMainWnd(app);
   SafeRelease(app);

   if (window && !editorPreviousWindowProc)
   {
      editorMainWindow = window;
      editorPreviousWindowProc = (WNDPROC)SetWindowLongPtr(
         window, GWLP_WNDPROC, (LONG_PTR)editor_window_proc);
   }
}

void EditorStopWindowResizeTracking(void)
{
   if (editorMainWindow && editorPreviousWindowProc &&
       (WNDPROC)GetWindowLongPtr(editorMainWindow, GWLP_WNDPROC) == editor_window_proc)
      SetWindowLongPtr(editorMainWindow, GWLP_WNDPROC,
                       (LONG_PTR)editorPreviousWindowProc);

   editorMainWindow = NULL;
   editorPreviousWindowProc = NULL;
   editorInSizeMove = FALSE;
   editorResizePending = FALSE;
}

void EditorRestoreWindowRect(void)
{
   IWinApp *app = AppGetObj(IWinApp);
   HWND window = IWinApp_GetMainWnd(app);
   SafeRelease(app);

   if (window && editorWindowRectPending)
   {
      if (editorPendingMaximized)
      {
         if (!IsZoomed(window))
            ShowWindow(window, SW_MAXIMIZE);
      }
      else
         SetWindowPos(window, NULL,
                      editorPendingWindowRect.left,
                      editorPendingWindowRect.top,
                      editorPendingWindowRect.right - editorPendingWindowRect.left,
                      editorPendingWindowRect.bottom - editorPendingWindowRect.top,
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
   }
   editorWindowRectPending = FALSE;
}

BOOL EditorGetPendingWindowSize(int *width, int *height)
{
   if (!editorResizePending)
      return FALSE;

   *width = editorPendingWidth;
   *height = editorPendingHeight;
   editorResizePending = FALSE;
   return TRUE;
}

void EditorPlatformInitializeControls(void)
{
   InitCommonControls();
}

void EditorPlatformDestroyAuxiliaryWindows(void)
{
   IWinApp *app = AppGetObj(IWinApp);
   HWND window;
   HWND previous;
   HWND mainWindow = IWinApp_GetMainWnd(app);
   SafeRelease(app);

   window = previous = mainWindow;
   while ((window = GetNextWindow(window, GW_HWNDPREV)) != NULL)
   {
      if (GetWindow(window, GW_OWNER) == mainWindow)
      {
         DestroyWindow(window);
         window = previous;
      }
      else
         previous = window;
   }

   window = previous = mainWindow;
   while ((window = GetNextWindow(window, GW_HWNDNEXT)) != NULL)
   {
      if (GetWindow(window, GW_OWNER) == mainWindow)
      {
         DestroyWindow(window);
         window = previous;
      }
      else
         previous = window;
   }
}
