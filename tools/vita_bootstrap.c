#include <psp2/gxm.h>
#include <psp2/kernel/processmgr.h>
#include <SDL2/SDL.h>
#include <vitaGL.h>

#include <stdio.h>

#ifndef DARK_VITA_TITLE
#define DARK_VITA_TITLE "Thief Trilogy Vita Test"
#endif

#ifndef DARK_VITA_TITLE_ID
#define DARK_VITA_TITLE_ID "THFG00000"
#endif

#ifndef DARK_VITA_ASSET_ROOT
#define DARK_VITA_ASSET_ROOT "ux0:data/thiefgold"
#endif

int main(void)
{
   if (!vglInitExtended(0, 960, 544, 0x800000,
                       SCE_GXM_MULTISAMPLE_NONE))
      sceKernelExitProcess(1);

   if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
   {
      fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      sceKernelExitProcess(2);
   }

   SDL_Log("Vita bootstrap: %s (%s), assets: %s",
           DARK_VITA_TITLE, DARK_VITA_TITLE_ID, DARK_VITA_ASSET_ROOT);
   glViewport(0, 0, 960, 544);
   glClearColor(0.035f, 0.055f, 0.08f, 1.0f);

   int running = 1;
   while (running)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_QUIT ||
             (event.type == SDL_CONTROLLERBUTTONDOWN &&
              event.cbutton.button == SDL_CONTROLLER_BUTTON_START))
            running = 0;
      }

      glClear(GL_COLOR_BUFFER_BIT);
      vglSwapBuffers(GL_FALSE);
      SDL_Delay(16);
   }

   SDL_Quit();
   sceKernelExitProcess(0);
   return 0;
}
