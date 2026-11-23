#pragma once

EXTERN void InitializeEditorWorkspace(void);
EXTERN void ShutdownEditorWorkspace(void);
struct grs_bitmap;
EXTERN void ShowEditorImage(const char *name, const struct grs_bitmap *bitmap);
EXTERN void ShowEditorColorPalette(void);

#ifdef __cplusplus
#include <functional>
#include <rect.h>
bool BeginEditorWorkspaceWindow(const char *name, const Rect &rect, unsigned flags);
void QueueEditorWorkspaceAction(std::function<void()> action);
void ResetEditorImageRenderer();
void PrepareEditorWorkspace();
void DrawEditorWorkspace();
void ExecuteEditorWorkspaceActions();
void ResetEditorWorkspaceInput();
bool EditorWorkspaceOwnsPointer(float x, float y, int button = 0);
bool EditorWorkspaceOwnsKeyboard();
bool CancelEditorWorkspaceResize();
bool EditorWorkspaceUsesCameraCursor(float x, float y);
#endif
