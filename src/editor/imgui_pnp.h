#pragma once

#include <pnptools.h>

EXTERN void PnP_SetOrigin(Point origin);
EXTERN void PnP_CommandButtons(Rect *bounds, const char *prefix, int count, Point dimensions);
EXTERN void PnP_HeadingButton(Rect *bounds, const char *label, void (*activate)(int), int data);
EXTERN int PnP_ChoiceButton(Rect *bounds, const char *label, void (*activate)(int), int data);
EXTERN void PnP_SetChoice(int owner, BOOL selected);
EXTERN void PnP_FocusControl(int owner);
EXTERN void PnP_MatchControlHeight(int owner, const Rect *reference);
EXTERN int PnP_DragLabel(Rect *rect, const char *label, float min, float max, float step, float *value,
   void (*update)(PnP_SliderOp, Rect *, float, intptr_t), intptr_t data, ulong flags);

#ifdef __cplusplus
#include <functional>
struct ImTextureData;
void PaintEditorPreview(ImTextureData &texture, int x, int y, int width, int height,
                        const std::function<void()> &paint);
void DrawEditorPanels();
void ResetEditorPanelRenderer();
#endif
