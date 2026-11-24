#pragma once

inline constexpr int kVitaScreenWidth = 960;
inline constexpr int kVitaScreenHeight = 544;

void VitaCalculateViewport(int logicalWidth, int logicalHeight,
                           int *x, int *y, int *width, int *height);
