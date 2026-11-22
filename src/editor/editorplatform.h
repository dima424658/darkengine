#pragma once

#ifndef __EDITORPLATFORM_H
#define __EDITORPLATFORM_H

// Shared SDL editor-window services; layout and providers live in src/editor.
EXTERN void EditorStartWindowResizeTracking(void);
EXTERN void EditorStopWindowResizeTracking(void);
EXTERN void EditorRestoreWindowRect(void);
EXTERN BOOL EditorGetPendingWindowSize(int *width, int *height);
EXTERN void EditorPlatformInitializeControls(void);
EXTERN void EditorPlatformDestroyAuxiliaryWindows(void);
EXTERN void EditorShowNativeCursor(BOOL show);
EXTERN void EditorUpdateCursor(void);
EXTERN void EditorResetCursor(void);

#endif
