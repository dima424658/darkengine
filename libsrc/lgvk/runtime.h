#pragma once

#include <stdint.h>

bool DarkRenderOpen(int width, int height, const char *title);
void DarkRenderClose(void);
bool DarkRenderSetFullscreen(bool enabled);
void DarkRenderPresent(const void *pixels, int width, int height, int rowBytes,
                       int bitDepth, const uint8_t *palette);
bool DarkRenderPumpEvents(void);
bool DarkRenderPumpMovieEvents(void);
bool DarkRenderTakeResize(int *width, int *height);
void DarkRenderBeginApplicationFrame(void);
void DarkRenderEndApplicationFrame(void);
bool DarkRenderPresentMovieFrame(const void *rgba, int width, int height,
                                 int rowBytes);
void DarkRenderSetTitle(const char *title);
void DarkRenderShowCursor(bool show);
void DarkRenderSetMousePosition(int x, int y);
void DarkRenderSetRelativeMouse(bool enabled);
