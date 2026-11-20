#pragma once

#include <stdint.h>

bool DarkRenderOpen(int width, int height, const char * title);
void DarkRenderClose(void);
void DarkRenderPresent(const void * pixels, int width, int height, int rowBytes,
                       int bitDepth, const uint8_t * palette);
bool DarkRenderTakeResize(int * width, int * height);
void DarkRenderBeginApplicationFrame(void);
void DarkRenderEndApplicationFrame(void);
void DarkRenderPresentProgress(void);
bool DarkRenderHasDirectCanvas();
void DarkRenderInvalidateCanvas();
void DarkRenderPresentRect(const void * pixels, int width, int height,
                           int rowBytes, int bitDepth, const uint8_t * palette,
                           int x0, int y0, int x1, int y1);
bool DarkRenderPresentMovieFrame(const void * rgba, int width, int height,
                                 int rowBytes);
