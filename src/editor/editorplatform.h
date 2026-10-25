#pragma once

#ifndef __EDITORPLATFORM_H
#define __EDITORPLATFORM_H

// Native editor-window services. Implementations live in winsrc/editor and
// unixsrc/editor; the editor layout itself remains platform-neutral.
EXTERN void EditorStartWindowResizeTracking(void);
EXTERN void EditorStopWindowResizeTracking(void);
EXTERN void EditorRestoreWindowRect(void);
EXTERN BOOL EditorGetPendingWindowSize(int *width, int *height);
EXTERN void EditorPlatformInitializeControls(void);
EXTERN void EditorPlatformDestroyAuxiliaryWindows(void);

#endif
