#include "vita_layout.hpp"

#include <algorithm>

void VitaCalculateViewport(int logicalWidth, int logicalHeight,
                           int *x, int *y, int *width, int *height)
{
   if (!x || !y || !width || !height)
      return;
   if (logicalWidth <= 0 || logicalHeight <= 0)
   {
      *x = 0;
      *y = 0;
      *width = kVitaScreenWidth;
      *height = kVitaScreenHeight;
      return;
   }

   const int fitWidth =
      std::min(kVitaScreenWidth,
               kVitaScreenHeight * logicalWidth / logicalHeight);
   const int fitHeight =
      std::min(kVitaScreenHeight,
               kVitaScreenWidth * logicalHeight / logicalWidth);
   *width = fitWidth;
   *height = fitHeight;
   *x = (kVitaScreenWidth - fitWidth) / 2;
   *y = (kVitaScreenHeight - fitHeight) / 2;
}
