#include "vita_layout.hpp"

#include <cstdio>
#include <cstdlib>

static void require(bool condition)
{
   if (!condition)
   {
      std::fputs("vita layout check failed\n", stderr);
      std::exit(1);
   }
}

int main()
{
   int x = -1, y = -1, width = -1, height = -1;
   VitaCalculateViewport(640, 480, &x, &y, &width, &height);
   require(x == 117 && y == 0);
   require(width == 725 && height == 544);

   VitaCalculateViewport(16, 9, &x, &y, &width, &height);
   require(x == 0 && y == 2);
   require(width == 960 && height == 540);

   VitaCalculateViewport(0, 0, &x, &y, &width, &height);
   require(x == 0 && y == 0);
   require(width == 960 && height == 544);

   std::puts("vita 960x544 layout check passed");
   return 0;
}
