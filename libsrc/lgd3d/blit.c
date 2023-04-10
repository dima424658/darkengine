// $Header: x:/prj/tech/libsrc/lgd3d/RCS/blit.c 1.1 1997/06/02 16:41:01 KEVIN Exp $

#include <windows.h>
#include <ddraw.h>

#include <lgassert.h>
#include <dev2d.h>

#include <setup.h>

static void lock_surface(grs_bitmap *bm, LPDIRECTDRAWSURFACE ddsurf)
{
   int type;

   DDSURFACEDESC sd;
   sd.dwSize = sizeof(sd);
   IDirectDrawSurface_Lock(ddsurf, NULL, &sd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

   switch (sd.ddpfPixelFormat.dwRGBBitCount) {
   case 8:
      type = BMT_FLAT8;
      break;
   case 15:
   case 16:
      type = BMT_FLAT16;
      break;
   default:
      CriticalMsg("Unsupported pixel format");
   }

   gr_init_bitmap(bm, (uchar *)sd.lpSurface, type, 0, sd.dwWidth, sd.dwHeight);
}

static void unlock_surface(grs_bitmap *bm, LPDIRECTDRAWSURFACE ddsurf)
{
   IDirectDrawSurface_Unlock(ddsurf, (LPVOID) bm->bits);
}

void lgd3d_blit(void)
{
   grs_bitmap bm;

   lock_surface(&bm, lpddDevice);
   gr_bitmap(&bm, 0, 0);
   unlock_surface(&bm, lpddDevice);
}
